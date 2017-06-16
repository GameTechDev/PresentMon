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

#include <generated/version.h>
#include <stdio.h>

#include "CommandLine.hpp"

namespace {

bool CombineArguments(
    int argc,
    char** argv,
    char* out,
    size_t outSize)
{
    size_t idx = 0;
    for (int i = 1; i < argc && idx < outSize; ++i) {
        if (idx >= outSize) {
            return false; // was truncated
        }

        if (argv[i][0] != '\"' && strchr(argv[i], ' ')) {
            idx += snprintf(out + idx, outSize - idx, " \"%s\"", argv[i]);
        } else {
            idx += snprintf(out + idx, outSize - idx, " %s", argv[i]);
        }
    }

    return true;
}

bool ParseModifier(char* arg, UINT* inoutModifier)
{
    struct {
        char const* arg;
        UINT modifier;
    } options[] = {
        { "alt",     MOD_ALT     },
        { "control", MOD_CONTROL },
        { "ctrl",    MOD_CONTROL },
        { "shift",   MOD_SHIFT   },
        { "sh",      MOD_SHIFT   },
        { "windows", MOD_WIN     },
        { "win",     MOD_WIN     },
    };

    for (size_t i = 0, N = _countof(options); i < N; ++i) {
        if (_stricmp(arg, options[i].arg) == 0) {
            *inoutModifier |= options[i].modifier;
            return true;
        }
    }

    return false;
}

bool ParseKeyCode(char* arg, UINT* outKeyCode)
{
#define CHECK_KEY(_Arg, _Code) \
    if (_stricmp(arg, _Arg) == 0) { *outKeyCode = _Code; return true; }
    CHECK_KEY("BACKSPACE", VK_BACK)
    CHECK_KEY("TAB", VK_TAB)
    CHECK_KEY("CLEAR", VK_CLEAR)
    CHECK_KEY("ENTER", VK_RETURN)
    CHECK_KEY("PAUSE", VK_PAUSE)
    CHECK_KEY("CAPSLOCK", VK_CAPITAL)
    CHECK_KEY("ESC", VK_ESCAPE)
    CHECK_KEY("SPACE", VK_SPACE)
    CHECK_KEY("PAGEUP", VK_PRIOR)
    CHECK_KEY("PAGEDOWN", VK_NEXT)
    CHECK_KEY("END", VK_END)
    CHECK_KEY("HOME", VK_HOME)
    CHECK_KEY("LEFT", VK_LEFT)
    CHECK_KEY("UP", VK_UP)
    CHECK_KEY("RIGHT", VK_RIGHT)
    CHECK_KEY("DOWN", VK_DOWN)
    CHECK_KEY("PRINTSCREEN", VK_SNAPSHOT)
    CHECK_KEY("INS", VK_INSERT)
    CHECK_KEY("DEL", VK_DELETE)
    CHECK_KEY("HELP", VK_HELP)
    CHECK_KEY("0", 0x30)
    CHECK_KEY("1", 0x31)
    CHECK_KEY("2", 0x32)
    CHECK_KEY("3", 0x33)
    CHECK_KEY("4", 0x34)
    CHECK_KEY("5", 0x35)
    CHECK_KEY("6", 0x36)
    CHECK_KEY("7", 0x37)
    CHECK_KEY("8", 0x38)
    CHECK_KEY("9", 0x39)
    CHECK_KEY("A", 0x42)
    CHECK_KEY("B", 0x43)
    CHECK_KEY("C", 0x44)
    CHECK_KEY("D", 0x45)
    CHECK_KEY("E", 0x46)
    CHECK_KEY("F", 0x47)
    CHECK_KEY("G", 0x48)
    CHECK_KEY("H", 0x49)
    CHECK_KEY("I", 0x4A)
    CHECK_KEY("J", 0x4B)
    CHECK_KEY("K", 0x4C)
    CHECK_KEY("L", 0x4D)
    CHECK_KEY("M", 0x4E)
    CHECK_KEY("N", 0x4F)
    CHECK_KEY("O", 0x50)
    CHECK_KEY("P", 0x51)
    CHECK_KEY("Q", 0x52)
    CHECK_KEY("R", 0x53)
    CHECK_KEY("S", 0x54)
    CHECK_KEY("T", 0x55)
    CHECK_KEY("U", 0x56)
    CHECK_KEY("V", 0x57)
    CHECK_KEY("W", 0x58)
    CHECK_KEY("X", 0x59)
    CHECK_KEY("Y", 0x5A)
    CHECK_KEY("Num0", VK_NUMPAD0)
    CHECK_KEY("Num1", VK_NUMPAD1)
    CHECK_KEY("Num2", VK_NUMPAD2)
    CHECK_KEY("Num3", VK_NUMPAD3)
    CHECK_KEY("Num4", VK_NUMPAD4)
    CHECK_KEY("Num5", VK_NUMPAD5)
    CHECK_KEY("Num6", VK_NUMPAD6)
    CHECK_KEY("Num7", VK_NUMPAD7)
    CHECK_KEY("Num8", VK_NUMPAD8)
    CHECK_KEY("Num9", VK_NUMPAD9)
    CHECK_KEY("Multiply", VK_MULTIPLY)
    CHECK_KEY("Add", VK_ADD)
    CHECK_KEY("Separator", VK_SEPARATOR)
    CHECK_KEY("Subtract", VK_SUBTRACT)
    CHECK_KEY("Decimal", VK_DECIMAL)
    CHECK_KEY("Divide", VK_DIVIDE)
    CHECK_KEY("F1", VK_F1)
    CHECK_KEY("F2", VK_F2)
    CHECK_KEY("F3", VK_F3)
    CHECK_KEY("F4", VK_F4)
    CHECK_KEY("F5", VK_F5)
    CHECK_KEY("F6", VK_F6)
    CHECK_KEY("F7", VK_F7)
    CHECK_KEY("F8", VK_F8)
    CHECK_KEY("F9", VK_F9)
    CHECK_KEY("F10", VK_F10)
    CHECK_KEY("F11", VK_F11)
    CHECK_KEY("F12", VK_F12)
    CHECK_KEY("F13", VK_F13)
    CHECK_KEY("F14", VK_F14)
    CHECK_KEY("F15", VK_F15)
    CHECK_KEY("F16", VK_F16)
    CHECK_KEY("F17", VK_F17)
    CHECK_KEY("F18", VK_F18)
    CHECK_KEY("F19", VK_F19)
    CHECK_KEY("F20", VK_F20)
    CHECK_KEY("F21", VK_F21)
    CHECK_KEY("F22", VK_F22)
    CHECK_KEY("F23", VK_F23)
    CHECK_KEY("F24", VK_F24)
    CHECK_KEY("NUMLOCK", VK_NUMLOCK)
    CHECK_KEY("SCROLLLOCK", VK_SCROLL)

    return false;
}

void AssignHotkey(int* inoutArgIdx, int argc, char** argv, CommandLineArgs* args)
{
    auto argIdx = *inoutArgIdx;

    args->mHotkeySupport = true;
    if (++argIdx == argc) {
        return;
    }

#pragma warning(suppress: 4996)
    auto token = strtok(argv[argIdx], "+");
    for (;;) {
        auto prev = token;
#pragma warning(suppress: 4996)
        token = strtok(nullptr, "+");
        if (token == nullptr) {
            if (!ParseKeyCode(prev, &args->mHotkeyVirtualKeyCode)) {
                return;
            }
            break;
        }

        if (!ParseModifier(prev, &args->mHotkeyModifiers)) {
            return;
        }
    }

    *inoutArgIdx = argIdx;
}

UINT atou(char const* a)
{
    int i = atoi(a);
    return i <= 0 ? 0 : (UINT) i;
}

void PrintHelp()
{
    // NOTE: remember to update README.md when modifying usage
    fprintf(stderr,
        "PresentMon %s\n"
        "\n"
        "Capture target options:\n"
        "    -captureall                Record all processes (default).\n"
        "    -process_name [exe name]   Record specific process specified by name.\n"
        "    -process_id [integer]      Record specific process specified by ID.\n"
        "\n"
        "Output options:\n"
        "    -no_csv                    Do not create any output file.\n"
        "    -output_file [path]        Write CSV output to specified path. Otherwise, the default is\n"
        "                               PresentMon-PROCESSNAME-TIME.csv.\n"
        "\n"
        "Control and filtering options:\n"
        "    -etl_file [path]           Consume events from an ETL file instead of a running process.\n"
        "    -scroll_toggle             Only record events while scroll lock is enabled.\n"
        "    -scroll_indicator          Set scroll lock while recording events.\n"
        "    -hotkey [key]              Use specified key to start and stop recording, writing to a\n"
        "                               unique file each time (default is F11).\n"
        "    -delay [seconds]           Wait for specified time before starting to record. When using\n"
        "                               -hotkey, delay occurs each time recording is started.\n"
        "    -timed [seconds]           Stop recording after the specified amount of time.  PresentMon will exit\n"
        "                               timer expires.\n"
        "    -exclude_dropped           Exclude dropped presents from the csv output.\n"
        "    -terminate_on_proc_exit    Terminate PresentMon when all instances of the specified process exit.\n"
        "    -terminate_after_timed     Terminate PresentMon after the timed trace, specified using -timed, completes.\n"
        "    -simple                    Disable advanced tracking (try this if you encounter crashes).\n"
        "    -verbose                   Adds additional data to output not relevant to normal usage.\n"
        "    -dont_restart_as_admin     Don't try to elevate privilege.\n"
        "    -no_top                    Don't display active swap chains in the console window.\n",
        PRESENT_MON_VERSION
        );
}

}

bool ParseCommandLine(int argc, char** argv, CommandLineArgs* args)
{
    bool simple = false;
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
#define ARG1(Arg, Assign) \
        if (strcmp(argv[i], Arg) == 0) { \
            Assign; \
            continue; \
        }

#define ARG2(Arg, Assign) \
        if (strcmp(argv[i], Arg) == 0) { \
            if (++i < argc) { \
                Assign; \
                continue; \
            } \
            fprintf(stderr, "error: %s expecting argument.\n", Arg); \
        }

        // Capture target options
             ARG1("-captureall",             args->mTargetProcessName   = nullptr)
        else ARG2("-process_name",           args->mTargetProcessName   = argv[i])
        else ARG2("-process_id",             args->mTargetPid           = atou(argv[i]))
        else ARG2("-etl_file",               args->mEtlFileName         = argv[i])

        // Output options
        else ARG1("-no_csv",                 args->mOutputFile          = false)
        else ARG2("-output_file",            args->mOutputFileName      = argv[i])

        // Control and filtering options
        else ARG1("-hotkey",                 AssignHotkey(&i, argc, argv, args))
        else ARG1("-scroll_toggle",          args->mScrollLockToggle    = true)
        else ARG1("-scroll_indicator",       args->mScrollLockIndicator = true)
        else ARG2("-delay",                  args->mDelay               = atou(argv[i]))
        else ARG2("-timed",                  args->mTimer               = atou(argv[i]))
        else ARG1("-exclude_dropped",        args->mExcludeDropped      = true)
        else ARG1("-terminate_on_proc_exit", args->mTerminateOnProcExit = true)
        else ARG1("-terminate_after_timed",  args->mTerminateAfterTimer = true)
        else ARG1("-simple",                 simple                     = true)
        else ARG1("-verbose",                verbose                    = true)
        else ARG1("-dont_restart_as_admin",  args->mTryToElevate        = false)
        else ARG1("-no_top",                 args->mSimpleConsole       = true)

        // Provided argument wasn't recognized
        else fprintf(stderr, "error: %s '%s'.\n",
            i > 0 && strcmp(argv[i - 1], "-hotkey") == 0 ? "failed to parse hotkey" : "unrecognized argument",
            argv[i]);

        PrintHelp();
        return false;
    }

    // Validate command line arguments
    if (((args->mTargetProcessName == nullptr) ? 0 : 1) +
        ((args->mTargetPid         <= 0      ) ? 0 : 1) > 1) {
        fprintf(stderr, "error: only specify one of -captureall, -process_name, or -process_id.\n");
        PrintHelp();
        return false;
    }

    if (args->mEtlFileName && args->mHotkeySupport) {
        fprintf(stderr, "error: -etl_file and -hotkey arguments are not compatible.\n");
        PrintHelp();
        return false;
    }

    if (args->mHotkeySupport) {
        if ((args->mHotkeyModifiers & MOD_CONTROL) != 0 && args->mHotkeyVirtualKeyCode == 0x44 /*C*/) {
            fprintf(stderr, "error: 'CTRL+C' cannot be used as a -hotkey, it is reserved for terminating the trace.\n");
            PrintHelp();
            return false;
        }

        if (args->mHotkeyModifiers == MOD_NOREPEAT && args->mHotkeyVirtualKeyCode == VK_F12) {
            fprintf(stderr, "error: 'F12' cannot be used as a -hotkey, it is reserved for the debugger.\n");
            PrintHelp();
            return false;
        }
    }

    if (simple && verbose) {
        fprintf(stderr, "error: -simple and -verbose arguments are not compatible.\n");
        PrintHelp();
        return false;
    }
    else if (simple) {
        args->mVerbosity = Verbosity::Simple;
    }
    else if (verbose) {
        args->mVerbosity = Verbosity::Verbose;
    }

    return true;
}

bool RestartAsAdministrator(
    int argc,
    char** argv)
{
    char exe_path[MAX_PATH] = {};
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));

    char args[1024] = {};
    if (!CombineArguments(argc, argv, args, sizeof(args))) {
        fprintf(stderr, "internal error: command line arguments too long.\n");
        return false;
    }

#pragma warning(suppress: 4302 4311) // truncate HINSTANCE to int
    auto ret = (int) ShellExecuteA(NULL, "runas", exe_path, args, NULL, SW_SHOW);
    if (ret > 32) {
        return true;
    }
    fprintf(stderr, "error: failed to elevate privilege");
    switch (ret) {
    case 0:                      fprintf(stderr, " (out of memory)"); break;
    case ERROR_FILE_NOT_FOUND:   fprintf(stderr, " (file not found)"); break;
    case ERROR_PATH_NOT_FOUND:   fprintf(stderr, " (path was not found)"); break;
    case ERROR_BAD_FORMAT:       fprintf(stderr, " (image is invalid)"); break;
    case SE_ERR_ACCESSDENIED:    fprintf(stderr, " (access denied)"); break;
    case SE_ERR_ASSOCINCOMPLETE: fprintf(stderr, " (association is incomplete)"); break;
    case SE_ERR_DDEBUSY:         fprintf(stderr, " (DDE busy)"); break;
    case SE_ERR_DDEFAIL:         fprintf(stderr, " (DDE transaction failed)"); break;
    case SE_ERR_DDETIMEOUT:      fprintf(stderr, " (DDE transaction timed out)"); break;
    case SE_ERR_DLLNOTFOUND:     fprintf(stderr, " (DLL not found)"); break;
    case SE_ERR_NOASSOC:         fprintf(stderr, " (no association)"); break;
    case SE_ERR_OOM:             fprintf(stderr, " (out of memory)"); break;
    case SE_ERR_SHARE:           fprintf(stderr, " (sharing violation)"); break;
    }
    fprintf(stderr, ".\n");

    return false;
}

void SetConsoleTitle(
    int argc,
    char** argv)
{
    char args[MAX_PATH] = {};
    size_t idx = snprintf(args, MAX_PATH, "PresentMon");
    if (!CombineArguments(argc, argv, args + idx, MAX_PATH - idx)) {
        args[MAX_PATH - 4] = '.';
        args[MAX_PATH - 3] = '.';
        args[MAX_PATH - 2] = '.';
    }

    SetConsoleTitleA(args);
}

bool EnableScrollLock(bool enable)
{
    auto enabled = (GetKeyState(VK_SCROLL) & 1) == 1;
    if (enabled != enable) {
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

