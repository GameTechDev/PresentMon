/*
Copyright 2017 Intel Corporation

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

#include <assert.h>
#include <thread>
#include <windows.h>

#include "CommandLine.hpp"
#include "PresentMon.hpp"

namespace {

enum {
    HOTKEY_ID = 0x80,

    WM_STOP_ETW_THREADS = WM_USER + 0,
};

HWND g_hWnd = 0;
bool g_originalScrollLockEnabled = false;

std::thread g_EtwConsumingThread;
bool g_StopEtwThreads = true;

bool EtwThreadsRunning()
{
    return g_EtwConsumingThread.joinable();
}

void StartEtwThreads(CommandLineArgs const& args)
{
    assert(!EtwThreadsRunning());
    assert(EtwThreadsShouldQuit());
    g_StopEtwThreads = false;
    g_EtwConsumingThread = std::thread(EtwConsumingThread, args);
}

void StopEtwThreads(CommandLineArgs* args)
{
    assert(EtwThreadsRunning());
    assert(g_StopEtwThreads == false);
    g_StopEtwThreads = true;
    g_EtwConsumingThread.join();
    args->mRecordingCount++;
}

BOOL WINAPI ConsoleCtrlHandler(
    _In_ DWORD dwCtrlType
    )
{
    (void) dwCtrlType;
    PostStopRecording();
    PostQuitProcess();
    return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto args = (CommandLineArgs*) GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID) {
            if (EtwThreadsRunning()) {
                StopEtwThreads(args);
            } else {
                StartEtwThreads(*args);
            }
        }
        break;

    case WM_STOP_ETW_THREADS:
        if (EtwThreadsRunning()) {
            StopEtwThreads(args);
        }
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateMessageQueue(CommandLineArgs& args)
{
    WNDCLASSEXW Class = { sizeof(Class) };
    Class.lpfnWndProc = WindowProc;
    Class.lpszClassName = L"PresentMon";
    if (!RegisterClassExW(&Class)) {
        fprintf(stderr, "error: failed to register hotkey class.\n");
        return 0;
    }

    HWND hWnd = CreateWindowExW(0, Class.lpszClassName, L"PresentMonWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, nullptr);
    if (!hWnd) {
        fprintf(stderr, "error: failed to create hotkey window.\n");
        return 0;
    }

    if (args.mHotkeySupport) {
        if (!RegisterHotKey(hWnd, HOTKEY_ID, args.mHotkeyModifiers, args.mHotkeyVirtualKeyCode)) {
            fprintf(stderr, "error: failed to register hotkey.\n");
            DestroyWindow(hWnd);
            return 0;
        }
    }

    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&args));

    return hWnd;
}

bool HaveAdministratorPrivileges()
{
    enum {
        PRIVILEGE_UNKNOWN,
        PRIVILEGE_ELEVATED,
        PRIVILEGE_NOT_ELEVATED,
    } static privilege = PRIVILEGE_UNKNOWN;

    if (privilege == PRIVILEGE_UNKNOWN) {
        typedef BOOL(WINAPI *OpenProcessTokenProc)(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle);
        typedef BOOL(WINAPI *GetTokenInformationProc)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, DWORD *ReturnLength);
        HMODULE advapi = LoadLibraryA("advapi32");
        if (advapi) {
            OpenProcessTokenProc OpenProcessToken = (OpenProcessTokenProc)GetProcAddress(advapi, "OpenProcessToken");
            GetTokenInformationProc GetTokenInformation = (GetTokenInformationProc)GetProcAddress(advapi, "GetTokenInformation");
            if (OpenProcessToken && GetTokenInformation) {
                HANDLE hToken = NULL;
                if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
                    /** BEGIN WORKAROUND: struct TOKEN_ELEVATION and enum value
                     * TokenElevation are not defined in the vs2003 headers, so
                     * we reproduce them here. **/
                    enum { WA_TokenElevation = 20 };
                    struct {
                        DWORD TokenIsElevated;
                    } token = {};
                    /** END WA **/

                    DWORD dwSize = 0;
                    if (GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS) WA_TokenElevation, &token, sizeof(token), &dwSize)) {
                        privilege = token.TokenIsElevated ? PRIVILEGE_ELEVATED : PRIVILEGE_NOT_ELEVATED;
                    }

                    CloseHandle(hToken);
                }
            }
            FreeLibrary(advapi);
        }
    }

    return privilege == PRIVILEGE_ELEVATED;
}

}

bool EtwThreadsShouldQuit()
{
    return g_StopEtwThreads;
}

void PostToggleRecording(CommandLineArgs const& args)
{
    PostMessage(g_hWnd, WM_HOTKEY, HOTKEY_ID, args.mHotkeyModifiers & ~MOD_NOREPEAT);
}

void PostStopRecording()
{
    PostMessage(g_hWnd, WM_STOP_ETW_THREADS, 0, 0);
}

void PostQuitProcess()
{
    PostMessage(g_hWnd, WM_QUIT, 0, 0);
}

int main(int argc, char** argv)
{
    // Parse command line arguments
    CommandLineArgs args;
    if (!ParseCommandLine(argc, argv, &args)) {
        return 1;
    }

    // Check required privilege
    if (!args.mEtlFileName && !HaveAdministratorPrivileges()) {
        if (args.mTryToElevate) {
            fprintf(stderr, "warning: process requires administrator privilege; attempting to elevate.\n");
            if (!RestartAsAdministrator(argc, argv)) {
                return 1;
            }
        } else {
            fprintf(stderr, "error: process requires administrator privilege.\n");
        }
        return 2;
    }

    int ret = 0;

    // Set console title to command line arguments
    SetConsoleTitle(argc, argv);

    // If the user wants to use the scroll lock key as an indicator of when
    // present mon is recording events, make sure it is disabled to start.
    if (args.mScrollLockIndicator) {
        g_originalScrollLockEnabled = EnableScrollLock(false);
    }

    // Create a message queue to handle WM_HOTKEY, WM_STOP_ETW_THREADS, and
    // WM_QUIT messages.
    HWND hWnd = CreateMessageQueue(args);
    if (hWnd == 0) {
        ret = 3;
        goto clean_up;
    }

    // Set CTRL handler to capture when the user tries to close the process by
    // closing the console window or CTRL-C or similar.  The handler will
    // ignore this and instead post WM_QUIT to our message queue.
    //
    // We must set g_hWnd before setting the handler.
    g_hWnd = hWnd;
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    // If the user didn't specify -hotkey, simulate a hotkey press to start the
    // recording right away.
    if (!args.mHotkeySupport) {
        PostToggleRecording(args);
    }

    // Enter the main thread message loop.  This thread will block waiting for
    // any messages, which will control the hotkey-toggling and process
    // shutdown.
    for (MSG message = {}; GetMessageW(&message, hWnd, 0, 0); ) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    // Everything should be shutdown by now.
    assert(!EtwThreadsRunning());

clean_up:
    // Restore original scroll lock state
    if (args.mScrollLockIndicator) {
        EnableScrollLock(g_originalScrollLockEnabled);
    }

    return ret;
}
