// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#define WINVER _WIN32_WINNT_WIN10 // To make TdhLoadManifestFromBinary available
#include "PresentMon.hpp"

enum {
    HOTKEY_ID = 0x80,

    // Timer ID's must be non-zero
    DELAY_TIMER_ID = 1,
    TIMED_TIMER_ID = 2,
};

static HWND gWnd = NULL;
static bool gIsRecording = false;
static uint32_t gHotkeyIgnoreCount = 0;

static bool LoadNVDDManifest()
{
    WCHAR CurrModuleDirW[MAX_PATH];
    if (!GetModuleFileNameW(NULL, CurrModuleDirW, MAX_PATH)) {
        PrintError(L"error: failed to get exe fullpath\n");
        return false;
    }

    std::wstring mypath = CurrModuleDirW;
    auto status = TdhLoadManifestFromBinary(&mypath[0]);
    if (ERROR_SUCCESS != status)
    {
        PrintError(L"error: failed to load manifest embedded in %s\n", mypath.c_str());
        return false;
    }
    return true;
}

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
            PrintWarning(L"warning: could not toggle scroll lock.\n");
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
    if (args.mConsoleOutput == ConsoleOutput::Simple && args.mCSVOutput != CSVOutput::None) {
        wprintf(L"Started recording.\n");
    }
    if (args.mScrollLockIndicator) {
        EnableScrollLock(true);
    }

    // Tell OutputThread to record
    SetOutputRecordingState(true);

    // Start --timed timer
    if (args.mStartTimer) {
        SetTimer(gWnd, TIMED_TIMER_ID, args.mTimer * 1000, (TIMERPROC) nullptr);
    }
}

static void StopRecording()
{
    auto const& args = GetCommandLineArgs();

    assert(IsRecording() == true);
    gIsRecording = false;

    // Stop time --timed timer if there is one
    if (args.mStartTimer) {
        KillTimer(gWnd, TIMED_TIMER_ID);
    }

    // Tell OutputThread to stop recording
    SetOutputRecordingState(false);

    // Notify the user we're no longer recording
    if (args.mScrollLockIndicator) {
        EnableScrollLock(false);
    }
    if (args.mConsoleOutput == ConsoleOutput::Simple && args.mCSVOutput != CSVOutput::None) {
        wprintf(L"Stopped recording.\n");
    }
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

int wmain(int argc, wchar_t** argv)
{
    // Load system DLLs
    LoadLibraryExA("advapi32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    LoadLibraryExA("shell32.dll",  NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    LoadLibraryExA("tdh.dll",      NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    LoadLibraryExA("user32.dll",   NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

    // Load NVIDIA DisplayDriver event manifest embedded in the PresentMon binary
    if (!LoadNVDDManifest()) {
        return 1;
    }

    // Initialize console
    SetThreadDescription(GetCurrentThread(), L"PresentMon Consumer Thread");
    InitializeConsole();

    // Parse command line arguments.
    if (!ParseCommandLine(argc, argv)) {
        return 1;
    }

    auto const& args = GetCommandLineArgs();

    // Special case handling for --terminate_existing_session
    if (args.mTerminateExistingSession) {
        auto status = StopNamedTraceSession(args.mSessionName);
        switch (status) {
        case ERROR_SUCCESS:                return 0;
        case ERROR_WMI_INSTANCE_NOT_FOUND: PrintError(L"error: no existing sessions found: %s\n", args.mSessionName); break;
        default:                           PrintError(L"error: failed to terminate existing session (%s): %lu\n", args.mSessionName, status); break;
        }
        return 7;
    }

    // Attempt to elevate process privilege if necessary.
    //
    // If we are processing an ETL file we don't need elevated privilege, but
    // for realtime analysis we need SeDebugPrivilege in order to open handles
    // to processes started by other accounts (see OutputThread.cpp).
    // 
    // If we can't enable SeDebugPrivilege, try to restart PresentMon as
    // administrator unless the user requested not to.
    // 
    // RestartAsAdministrator() waits for the elevated process to complete in
    // order to report stderr and obtain it's exit code.
    if (args.mEtlFileName == nullptr && // realtime analysis
        !EnableDebugPrivilege()) {      // failed to enable SeDebugPrivilege
        if (args.mTryToElevate) {
            return RestartAsAdministrator(argc, argv);
        }

        PrintWarning(
            L"warning: PresentMon requires elevated privilege in order to query processes that are\n"
            L"         short-running or started on another account.  Without it, those processes will\n"
            L"         be listed as '<unknown>' and they can't be targeted by --process_name nor trigger\n"
            L"         --terminate_on_proc_exit.\n");
    }

    // Create a message queue to handle the input messages.
    WNDCLASSEXW wndClass = { sizeof(wndClass) };
    wndClass.lpfnWndProc = HandleWindowMessage;
    wndClass.lpszClassName = L"PresentMon";
    if (!RegisterClassExW(&wndClass)) {
        PrintError(L"error: failed to register hotkey class.\n");
        return 3;
    }

    gWnd = CreateWindowExW(0, wndClass.lpszClassName, L"PresentMonWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, nullptr);
    if (!gWnd) {
        PrintError(L"error: failed to create hotkey window.\n");
        UnregisterClassW(wndClass.lpszClassName, NULL);
        return 4;
    }

    // Register the hotkey.
    if (args.mHotkeySupport && !RegisterHotKey(gWnd, HOTKEY_ID, args.mHotkeyModifiers, args.mHotkeyVirtualKeyCode)) {
        PrintHotkeyError();
        DestroyWindow(gWnd);
        UnregisterClassW(wndClass.lpszClassName, NULL);
        return 5;
    }

    // Set CTRL handler (note: must set gWnd before setting the handler).
    SetConsoleCtrlHandler(HandleCtrlEvent, TRUE);

    // Create event consumers
    PMTraceConsumer pmConsumer(
        args.mPresentEventCircularBufferSize != 0
        ? args.mPresentEventCircularBufferSize
        : PMTraceConsumer::PRESENTEVENT_CIRCULAR_BUFFER_SIZE);

    pmConsumer.mTrackDisplay               = args.mTrackDisplay;
    pmConsumer.mTrackGPU                   = args.mTrackGPU;
    pmConsumer.mTrackGPUVideo              = args.mTrackGPUVideo;
    pmConsumer.mTrackInput                 = args.mTrackInput;
    pmConsumer.mTrackFrameType             = args.mTrackFrameType;
    pmConsumer.mTrackPMMeasurements        = args.mTrackPMMeasurements;
    pmConsumer.mTrackAppTiming             = args.mTrackAppTiming;
    pmConsumer.mTrackHybridPresent         = args.mTrackHybridPresent;
    pmConsumer.mDisableOfflineBackpressure = args.mDisableOfflineBackpressure;
    pmConsumer.mTrackPcLatency             = args.mTrackPcLatency;
    if (args.mTargetPid != 0) {
        pmConsumer.mFilteredProcessIds = true;
        pmConsumer.AddTrackedProcessForFiltering(args.mTargetPid);
    }

    // Start the ETW trace session.
    PMTraceSession pmSession;
    pmSession.mPMConsumer = &pmConsumer;
    auto status = pmSession.Start(args.mEtlFileName, args.mSessionName);

    // If a session with this same name is already running, we either exit or
    // stop it and start a new session.  This is useful if a previous process
    // failed to properly shut down the session for some reason.
    if (status == ERROR_ALREADY_EXISTS) {
        if (args.mStopExistingSession) {
            PrintWarning(
                L"warning: a trace session named \"%s\" is already running and it will be stopped.\n"
                L"         Use --session_name with a different name to start a new session.\n",
                args.mSessionName);
        } else {
            PrintError(
                L"error: a trace session named \"%s\" is already running. Use --stop_existing_session\n"
                L"       to stop the existing session, or use --session_name with a different name to\n"
                L"       start a new session.\n",
                args.mSessionName);

            SetConsoleCtrlHandler(HandleCtrlEvent, FALSE);
            DestroyWindow(gWnd);
            UnregisterClass(wndClass.lpszClassName, NULL);
            return 6;
        }

        status = StopNamedTraceSession(args.mSessionName);
        if (status == ERROR_SUCCESS) {
            status = pmSession.Start(args.mEtlFileName, args.mSessionName);
        }
    }

    // Exit with an error if we failed to start a new session
    if (status != ERROR_SUCCESS) {
        PrintError(L"error: failed to start trace session: ");
        switch (status) {
        case ERROR_FILE_NOT_FOUND: PrintError(L"file not found.\n"); break;
        case ERROR_PATH_NOT_FOUND: PrintError(L"path not found.\n"); break;
        case ERROR_BAD_PATHNAME:   PrintError(L"invalid --session_name.\n"); break;
        case ERROR_ACCESS_DENIED:  PrintError(L"access denied.\n"); break;
        case ERROR_FILE_CORRUPT:   PrintError(L"invalid --etl_file.\n"); break;
        default:                   PrintError(L"error code %lu.\n", status); break;
        }

        if (status == ERROR_ACCESS_DENIED && !InPerfLogUsersGroup()) {
            PrintError(
                L"       PresentMon requires either administrative privileges or to be run by a user in the\n"
                L"       \"Performance Log Users\" user group.  View the readme for more details.\n");
        }

        SetConsoleCtrlHandler(HandleCtrlEvent, FALSE);
        DestroyWindow(gWnd);
        UnregisterClassW(wndClass.lpszClassName, NULL);
        return 6;
    }

    // Set deferral time limit to 2 seconds
    if (pmConsumer.mDeferralTimeLimit == 0) {
        pmConsumer.mDeferralTimeLimit = pmSession.mTimestampFrequency.QuadPart * 2;
    }

    // Start the consumer and output threads
    StartConsumerThread(pmSession.mTraceHandle);
    StartOutputThread(pmSession);

    // If the user wants to use the scroll lock key as an indicator of when
    // PresentMon is recording events, save the original state and set scroll
    // lock to the recording state.
    auto originalScrollLockEnabled = args.mScrollLockIndicator
        ? EnableScrollLock(IsRecording())
        : false;

    // If the user didn't specify --hotkey, simulate a hotkey press to start the
    // recording right away.
    if (!args.mHotkeySupport) {
        PostMessageW(gWnd, WM_HOTKEY, HOTKEY_ID, args.mHotkeyModifiers & ~MOD_NOREPEAT);
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

    // Stop the trace session.
    if (args.mScrollLockIndicator) {
        EnableScrollLock(originalScrollLockEnabled);
    }

    pmSession.Stop();

    // Wait for the consumer and output threads to end (which are using the
    // consumers).
    WaitForConsumerThreadToExit();
    StopOutputThread();

    // Output warning if events were lost.
    if (pmSession.mNumBuffersLost > 0) {
        PrintWarning(L"warning: %lu ETW buffers were lost.\n", pmSession.mNumBuffersLost);
    }
    if (pmSession.mNumEventsLost > 0) {
        PrintWarning(L"warning: %lu ETW events were lost.\n", pmSession.mNumEventsLost);
    }
    if (pmConsumer.mNumOverflowedPresents > 0) {
        PrintWarning(L"warning: %lu overflowed present events detected. This could be due to a high-fps application.\n",
                     pmConsumer.mNumOverflowedPresents);
        PrintWarning(L"         Consider increasing the present event circular buffer size to a value larger\n");
        PrintWarning(L"         than the default of 2048, e.g., --set_circular_buffer_size 4096.\n");

                  
    }

    /* We cannot remove the Ctrl handler because it is in an infinite sleep so
     * this call will never return, either hanging the application or having
     * the threshold timer trigger and force terminate (depending on what Ctrl
     * code was used).  Instead, we just let the process tear down take care of
     * it.
    SetConsoleCtrlHandler(HandleCtrlEvent, FALSE);
    */
    DestroyWindow(gWnd);
    UnregisterClassW(wndClass.lpszClassName, NULL);
    FinalizeConsole();
    return 0;
}
