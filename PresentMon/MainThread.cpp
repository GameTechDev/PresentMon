/*
Copyright 2017-2020 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "PresentMon.hpp"
#include "../PresentData/TraceSession.hpp"

enum {
    HOTKEY_ID = 0x80,

    // Timer ID's must be non-zero
    DELAY_TIMER_ID = 1,
    TIMED_TIMER_ID = 2,
};

static HWND gWnd = NULL;
static bool gIsRecording = false;
static uint32_t gHotkeyIgnoreCount = 0;

static bool EnableScrollLock(bool enable)
{
    auto const& args = GetCommandLineArgs();

    auto enabled = (GetKeyState(VK_SCROLL) & 1) == 1;
    if (enabled != enable) {
        // If the hotkey is SCROLLLOCK, SendInput() will cause the hotkey to
        // trigger (entering an infinite recording toggle loop) so note that
        // the message handler should ignore one of them.
        if (args.mHotkeySupport &&
            args.mHotkeyVirtualKeyCode == VK_SCROLL &&
            args.mHotkeyModifiers == MOD_NOREPEAT) {
            gHotkeyIgnoreCount += 1;
        }

        // Send SCROLLLOCK press message.
        auto extraInfo = GetMessageExtraInfo();
        INPUT input[2] = {};

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = VK_SCROLL;
        input[0].ki.dwExtraInfo = extraInfo;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = VK_SCROLL;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        input[1].ki.dwExtraInfo = extraInfo;

        auto sendCount = SendInput(2, input, sizeof(INPUT));
        if (sendCount != 2) {
            fprintf(stderr, "warning: could not toggle scroll lock.\n");
        }
    }

    return enabled;
}

static bool IsRecording()
{
    return gIsRecording;
}

static void StartRecording()
{
    auto const& args = GetCommandLineArgs();

    assert(IsRecording() == false);
    gIsRecording = true;

    // Notify user we're recording
#if !DEBUG_VERBOSE
    if (args.mConsoleOutputType == ConsoleOutput::Simple) {
        printf("Started recording.\n");
    }
#endif
    if (args.mScrollLockIndicator) {
        EnableScrollLock(true);
    }

    // Tell OutputThread to record
    SetOutputRecordingState(true);

    // Start -timed timer
    if (args.mStartTimer) {
        SetTimer(gWnd, TIMED_TIMER_ID, args.mTimer * 1000, (TIMERPROC) nullptr);
    }
}

static void StopRecording()
{
    auto const& args = GetCommandLineArgs();

    assert(IsRecording() == true);
    gIsRecording = false;

    // Stop time -timed timer if there is one
    if (args.mStartTimer) {
        KillTimer(gWnd, TIMED_TIMER_ID);
    }

    // Tell OutputThread to stop recording
    SetOutputRecordingState(false);

    // Notify the user we're no longer recording
    if (args.mScrollLockIndicator) {
        EnableScrollLock(false);
    }
#if !DEBUG_VERBOSE
    if (args.mConsoleOutputType == ConsoleOutput::Simple) {
        printf("Stopped recording.\n");
    }
#endif
}

// Handle Ctrl events (CTRL_C_EVENT, CTRL_BREAK_EVENT, CTRL_CLOSE_EVENT,
// CTRL_LOGOFF_EVENT, CTRL_SHUTDOWN_EVENT) by redirecting the termination into
// a WM_QUIT message so that the shutdown code is still executed.
static BOOL CALLBACK HandleCtrlEvent(DWORD ctrlType)
{
    (void) ctrlType;
    if (IsRecording()) {
        StopRecording();
    }
    ExitMainThread();

    // The other threads are now shutting down but if we return the system may
    // terminate the process before they complete, which may leave the trace
    // session open.  We could wait for shutdown confirmation, but this
    // function is run in a separate thread so we just put it to sleep
    // indefinately and let the application shut itself down.
    Sleep(INFINITE);
    return TRUE; // The signal was handled, don't call any other handlers
}

// Handle window messages to toggle recording on/off
static LRESULT CALLBACK HandleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto const& args = GetCommandLineArgs();

    switch (uMsg) {
    case WM_TIMER:
        switch (wParam) {
        case DELAY_TIMER_ID:
            StartRecording();
            KillTimer(hWnd, DELAY_TIMER_ID);
            return 0;

        case TIMED_TIMER_ID:
            StopRecording();
            if (args.mTerminateAfterTimer) {
                ExitMainThread();
            }
            return 0;
        }
        break;

    case WM_HOTKEY:
        if (gHotkeyIgnoreCount > 0) {
            gHotkeyIgnoreCount -= 1;
            break;
        }

        if (IsRecording()) {
            StopRecording();
        } else if (args.mDelay == 0) {
            StartRecording();
        } else {
            SetTimer(hWnd, DELAY_TIMER_ID, args.mDelay * 1000, (TIMERPROC) nullptr);
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ExitMainThread()
{
    PostMessage(gWnd, WM_QUIT, 0, 0);
}

int main(int argc, char** argv)
{
    // Parse command line arguments.
    if (!ParseCommandLine(argc, argv)) {
        return 1;
    }

    auto const& args = GetCommandLineArgs();

    // Special case handling for -terminate_existing
    if (args.mTerminateExisting) {
        auto status = TraceSession::StopNamedSession(args.mSessionName);
        switch (status) {
        case ERROR_SUCCESS: return 0;
        case ERROR_WMI_INSTANCE_NOT_FOUND: fprintf(stderr, "error: no existing sessions found: %s\n", args.mSessionName); break;
        default: fprintf(stderr, "error: failed to terminate existing session (%s): %u\n", args.mSessionName, status); break;
        }
        return 7;
    }

    // Attempt to elevate process privilege if necessary.
    //
    // If a new process needs to be started, this will wait for the elevated
    // process to complete in order to report stderr and exit code, and then
    // abort from within ElevatePrivilege() (i.e., the rest of this function
    // won't run in this process).
    ElevatePrivilege(argc, argv);

    // Create a message queue to handle the input messages.
    WNDCLASSEXW wndClass = { sizeof(wndClass) };
    wndClass.lpfnWndProc = HandleWindowMessage;
    wndClass.lpszClassName = L"PresentMon";
    if (!RegisterClassExW(&wndClass)) {
        fprintf(stderr, "error: failed to register hotkey class.\n");
        return 3;
    }

    gWnd = CreateWindowExW(0, wndClass.lpszClassName, L"PresentMonWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, nullptr);
    if (!gWnd) {
        fprintf(stderr, "error: failed to create hotkey window.\n");
        UnregisterClass(wndClass.lpszClassName, NULL);
        return 4;
    }

    // Register the hotkey.
    if (args.mHotkeySupport && !RegisterHotKey(gWnd, HOTKEY_ID, args.mHotkeyModifiers, args.mHotkeyVirtualKeyCode)) {
        fprintf(stderr, "error: failed to register hotkey.\n");
        DestroyWindow(gWnd);
        UnregisterClass(wndClass.lpszClassName, NULL);
        return 5;
    }

    // Set CTRL handler (note: must set gWnd before setting the handler).
    SetConsoleCtrlHandler(HandleCtrlEvent, TRUE);

    // Start the ETW trace session (including consumer and output threads).
    if (!StartTraceSession()) {
        SetConsoleCtrlHandler(HandleCtrlEvent, FALSE);
        DestroyWindow(gWnd);
        UnregisterClass(wndClass.lpszClassName, NULL);
        return 6;
    }

    // If the user wants to use the scroll lock key as an indicator of when
    // PresentMon is recording events, save the original state and set scroll
    // lock to the recording state.
    auto originalScrollLockEnabled = args.mScrollLockIndicator
        ? EnableScrollLock(IsRecording())
        : false;

    // If the user didn't specify -hotkey, simulate a hotkey press to start the
    // recording right away.
    if (!args.mHotkeySupport) {
        PostMessage(gWnd, WM_HOTKEY, HOTKEY_ID, args.mHotkeyModifiers & ~MOD_NOREPEAT);
    }

    // Enter the MainThread message loop.  This thread will block waiting for
    // any window messages, dispatching the appropriate function to
    // HandleWindowMessage(), and then blocking again until the WM_QUIT message
    // arrives or the window is destroyed.
    for (MSG message = {};;) {
        BOOL r = GetMessageW(&message, gWnd, 0, 0);
        if (r == 0) { // Received WM_QUIT message.
            break;
        }
        if (r == -1) { // Indicates error in message loop, e.g. gWnd is no
                       // longer valid. This can happen if PresentMon is killed.
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    // Shut everything down.
    if (args.mScrollLockIndicator) {
        EnableScrollLock(originalScrollLockEnabled);
    }
    StopTraceSession();
    /* We cannot remove the Ctrl handler because it is in an infinite sleep so
     * this call will never return, either hanging the application or having
     * the threshold timer trigger and force terminate (depending on what Ctrl
     * code was used).  Instead, we just let the process tear down take care of
     * it.
    SetConsoleCtrlHandler(HandleCtrlEvent, FALSE);
    */
    DestroyWindow(gWnd);
    UnregisterClass(wndClass.lpszClassName, NULL);
    return 0;
}
