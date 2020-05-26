/*
Copyright 2017-2019 Intel Corporation

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

#include "PresentMon.hpp"

struct KeyNameCode
{
    char const* mName;
    UINT mCode;
};

static KeyNameCode const HOTKEY_MODS[] = {
    { "ALT",     MOD_ALT     },
    { "CONTROL", MOD_CONTROL },
    { "CTRL",    MOD_CONTROL },
    { "SHIFT",   MOD_SHIFT   },
    { "WINDOWS", MOD_WIN     },
    { "WIN",     MOD_WIN     },
};

static KeyNameCode const HOTKEY_KEYS[] = {
    { "BACKSPACE", VK_BACK },
    { "TAB", VK_TAB },
    { "CLEAR", VK_CLEAR },
    { "ENTER", VK_RETURN },
    { "PAUSE", VK_PAUSE },
    { "CAPSLOCK", VK_CAPITAL },
    { "ESC", VK_ESCAPE },
    { "SPACE", VK_SPACE },
    { "PAGEUP", VK_PRIOR },
    { "PAGEDOWN", VK_NEXT },
    { "END", VK_END },
    { "HOME", VK_HOME },
    { "LEFT", VK_LEFT },
    { "UP", VK_UP },
    { "RIGHT", VK_RIGHT },
    { "DOWN", VK_DOWN },
    { "PRINTSCREEN", VK_SNAPSHOT },
    { "INS", VK_INSERT },
    { "DEL", VK_DELETE },
    { "HELP", VK_HELP },
    { "NUMLOCK", VK_NUMLOCK },
    { "SCROLLLOCK", VK_SCROLL },
    { "NUM0", VK_NUMPAD0 },
    { "NUM1", VK_NUMPAD1 },
    { "NUM2", VK_NUMPAD2 },
    { "NUM3", VK_NUMPAD3 },
    { "NUM4", VK_NUMPAD4 },
    { "NUM5", VK_NUMPAD5 },
    { "NUM6", VK_NUMPAD6 },
    { "NUM7", VK_NUMPAD7 },
    { "NUM8", VK_NUMPAD8 },
    { "NUM9", VK_NUMPAD9 },
    { "MULTIPLY", VK_MULTIPLY },
    { "ADD", VK_ADD },
    { "SEPARATOR", VK_SEPARATOR },
    { "SUBTRACT", VK_SUBTRACT },
    { "DECIMAL", VK_DECIMAL },
    { "DIVIDE", VK_DIVIDE },
    { "0", 0x30 },
    { "1", 0x31 },
    { "2", 0x32 },
    { "3", 0x33 },
    { "4", 0x34 },
    { "5", 0x35 },
    { "6", 0x36 },
    { "7", 0x37 },
    { "8", 0x38 },
    { "9", 0x39 },
    { "A", 0x42 },
    { "B", 0x43 },
    { "C", 0x44 },
    { "D", 0x45 },
    { "E", 0x46 },
    { "F", 0x47 },
    { "G", 0x48 },
    { "H", 0x49 },
    { "I", 0x4A },
    { "J", 0x4B },
    { "K", 0x4C },
    { "L", 0x4D },
    { "M", 0x4E },
    { "N", 0x4F },
    { "O", 0x50 },
    { "P", 0x51 },
    { "Q", 0x52 },
    { "R", 0x53 },
    { "S", 0x54 },
    { "T", 0x55 },
    { "U", 0x56 },
    { "V", 0x57 },
    { "W", 0x58 },
    { "X", 0x59 },
    { "Y", 0x5A },
    { "F1", VK_F1 },
    { "F2", VK_F2 },
    { "F3", VK_F3 },
    { "F4", VK_F4 },
    { "F5", VK_F5 },
    { "F6", VK_F6 },
    { "F7", VK_F7 },
    { "F8", VK_F8 },
    { "F9", VK_F9 },
    { "F10", VK_F10 },
    { "F11", VK_F11 },
    { "F12", VK_F12 },
    { "F13", VK_F13 },
    { "F14", VK_F14 },
    { "F15", VK_F15 },
    { "F16", VK_F16 },
    { "F17", VK_F17 },
    { "F18", VK_F18 },
    { "F19", VK_F19 },
    { "F20", VK_F20 },
    { "F21", VK_F21 },
    { "F22", VK_F22 },
    { "F23", VK_F23 },
    { "F24", VK_F24 },
};

static CommandLineArgs gCommandLineArgs;

static bool ParseKeyName(KeyNameCode const* valid, size_t validCount, char* name, char const* errorMessage, UINT* outKeyCode)
{
    for (size_t i = 0; i < validCount; ++i) {
        if (_stricmp(name, valid[i].mName) == 0) {
            *outKeyCode = valid[i].mCode;
            return true;
        }
    }

    int col = fprintf(stderr, "error: %s '%s'. Valid options (case insensitive):", errorMessage, name);

    for (size_t i = 0; i < validCount; ++i) {
        auto len = strlen(valid[i].mName);
        if (col + len + 1 > 80) {
            col = fprintf(stderr, "\n   ") - 1;
        }
        col += fprintf(stderr, " %s", valid[i].mName);
    }
    fprintf(stderr, "\n");

    return false;
}

static bool AssignHotkey(int i, int argc, char** argv, CommandLineArgs* args)
{
    if (i == argc) {
        fprintf(stderr, "error: -hotkey missing key argument.\n");
        return false;
    }

#pragma warning(suppress: 4996)
    auto token = strtok(argv[i], "+");
    for (;;) {
        auto prev = token;
#pragma warning(suppress: 4996)
        token = strtok(nullptr, "+");
        if (token == nullptr) {
            if (!ParseKeyName(HOTKEY_KEYS, _countof(HOTKEY_KEYS), prev, "invalid -hotkey key", &args->mHotkeyVirtualKeyCode)) {
                return false;
            }
            break;
        }

        if (!ParseKeyName(HOTKEY_MODS, _countof(HOTKEY_MODS), prev, "invalid -hotkey modifier", &args->mHotkeyModifiers)) {
            return false;
        }
    }

    args->mHotkeySupport = true;
    return true;
}

static UINT atou(char const* a)
{
    int i = atoi(a);
    return i <= 0 ? 0 : (UINT) i;
}

static void PrintHelp()
{
    // NOTE: remember to update README.md when modifying usage
    char* s[] = {
        "Capture target options", nullptr,
        "-captureall",              "Record all processes (default).",
        "-process_name [exe name]", "Record only processes with the provided name."
                                    " This argument can be repeated to capture multiple processes.",
        "-exclude [exe name]",      "Don't record specific process specified by name."
                                    " This argument can be repeated to exclude multiple processes.",
        "-process_id [integer]",    "Record only the process specified by ID.",
        "-etl_file [path]",         "Consume events from an ETL file instead of running processes.",

        "Output options (see README for file naming defaults)", nullptr,
        "-output_file [path]",      "Write CSV output to specified path.",
        "-output_stdout",           "Write CSV output to STDOUT.",
        "-multi_csv",               "Create a separate CSV file for each captured process.",
        "-no_csv",                  "Do not create any output file.",
        "-no_top",                  "Don't display active swap chains in the console window.",
        "-qpc_time",                "Output present time as performance counter value (see"
                                    " QueryPerformanceCounter()).",

        "Recording options", nullptr,
        "-hotkey [key]",            "Use specified key to start and stop recording, writing to a"
                                    " unique CSV file each time. 'key' is of the form MODIFIER+KEY,"
                                    " e.g., alt+shift+f11. (See README for subsequent file naming).",
        "-delay [seconds]",         "Wait for specified time before starting to record."
                                    " If using -hotkey, delay occurs each time recording is started.",
        "-timed [seconds]",         "Stop recording after the specified amount of time.",
        "-exclude_dropped",         "Exclude dropped presents from the csv output.",
        "-scroll_indicator",        "Enable scroll lock while recording.",
        "-simple",                  "Disable GPU/display tracking.",
        "-verbose",                 "Adds additional data to output not relevant to normal usage.",

        "Execution options", nullptr,
        "-session_name [name]",     "Use the specified name to start a new realtime ETW session, instead"
                                    " of the default \"PresentMon\". This can be used to start multiple"
                                    " realtime capture process at the same time (using distinct names)."
                                    " A realtime PresentMon capture cannot start if there are any"
                                    " existing sessions with the same name.",
        "-stop_existing_session",   "If a trace session with the same name is already running, stop"
                                    " the existing session (to allow this one to proceed).",
        "-dont_restart_as_admin",   "Don't try to elevate privilege.  Elevated privilege isn't required"
                                    " to trace a process you started, but PresentMon requires elevated"
                                    " privilege in order to query processes started on another account."
                                    " Without it, these processes cannot be targetted by name and will be"
                                    " listed as '<error>'.",
        "-terminate_on_proc_exit",  "Terminate PresentMon when all the target processes have exited.",
        "-terminate_after_timed",   "When using -timed, terminate PresentMon after the timed capture completes.",
        "-shutdown",                "Terminate already running PresentMon process",

        "Beta options", nullptr,
        "-include_mixed_reality",   "Capture Windows Mixed Reality data to a CSV file with \"_WMR\" suffix.",
    };

    // NOTE: remember to update README.md when modifying usage
    fprintf(stderr, "PresentMon %s\n", PRESENT_MON_VERSION);

    size_t argWidth = 0;
    for (size_t i = 0; i < _countof(s); i += 2) {
        auto arg = s[i];
        auto desc = s[i + 1];
        if (desc != nullptr) {
            argWidth = max(argWidth, strlen(arg));
        }
    }

    size_t descWidth = 80 - 4 - min(40, argWidth);

    for (size_t i = 0; i < _countof(s); i += 2) {
        auto arg = s[i];
        auto desc = s[i + 1];
        if (desc == nullptr) {
            fprintf(stderr, "\n%s:\n", arg);
        } else {
            fprintf(stderr, "  %-*s  ", (int) argWidth, arg);
            for (auto len = strlen(desc); len > 0; ) {
                if (len <= descWidth) {
                    fprintf(stderr, "%s\n", desc);
                    break;
                }

                auto w = descWidth;
                while (desc[w] != ' ') {
                    --w;
                }
                fprintf(stderr, "%.*s\n%-*s", (int) w, desc, (int) (argWidth + 4), "");
                desc += w + 1;
                len -= w + 1;
            }
        }
    }
}

CommandLineArgs const& GetCommandLineArgs()
{
    return gCommandLineArgs;
}

bool ParseCommandLine(int argc, char** argv)
{
    auto args = &gCommandLineArgs;

    args->mTargetProcessNames.clear();
    args->mExcludeProcessNames.clear();
    args->mOutputCsvFileName = nullptr;
    args->mEtlFileName = nullptr;
    args->mSessionName = "PresentMon";
    args->mTargetPid = 0;
    args->mDelay = 0;
    args->mTimer = 0;
    args->mHotkeyModifiers = MOD_NOREPEAT;
    args->mHotkeyVirtualKeyCode = 0;
    args->mOutputCsvToFile = true;
    args->mOutputCsvToStdout = false;
    args->mOutputQpcTime = false;
    args->mScrollLockIndicator = false;
    args->mExcludeDropped = false;
    args->mVerbosity = Verbosity::Normal;
    args->mConsoleOutputType = ConsoleOutput::Full;
    args->mTerminateOnProcExit = false;
    args->mTerminateAfterTimer = false;
    args->mHotkeySupport = false;
    args->mTryToElevate = true;
    args->mIncludeWindowsMixedReality = false;
    args->mMultiCsv = false;
    args->mStopExistingSession = false;
    args->mShutdown = false;

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

        // Capture target options:
        if (strcmp(argv[i], "-captureall") == 0) {
            if (!args->mTargetProcessNames.empty()) {
                fprintf(stderr, "warning: -captureall elides all previous -process_name command line arguments.\n");
                args->mTargetProcessNames.clear();
            }
            continue;
        }

        else ARG2("-process_name",           args->mTargetProcessNames.emplace_back(argv[i]))
        else ARG2("-exclude",                args->mExcludeProcessNames.emplace_back(argv[i]))
        else ARG2("-process_id",             args->mTargetPid                  = atou(argv[i]))
        else ARG2("-etl_file",               args->mEtlFileName                = argv[i])

        // Output options:
        else ARG2("-output_file",            args->mOutputCsvFileName          = argv[i])
        else ARG1("-output_stdout",          args->mOutputCsvToStdout          = true)
        else ARG1("-multi_csv",              args->mMultiCsv                   = true)
        else ARG1("-no_csv",                 args->mOutputCsvToFile            = false)
        else ARG1("-no_top",                 args->mConsoleOutputType          = ConsoleOutput::Simple)
        else ARG1("-qpc_time",               args->mOutputQpcTime              = true)

        // Recording options:
        else if (strcmp(argv[i], "-hotkey") == 0) { if (AssignHotkey(++i, argc, argv, args)) continue; }
        else ARG2("-delay",                  args->mDelay                      = atou(argv[i]))
        else ARG2("-timed",                  args->mTimer                      = atou(argv[i]))
        else ARG1("-exclude_dropped",        args->mExcludeDropped             = true)
        else ARG1("-scroll_indicator",       args->mScrollLockIndicator        = true)
        else ARG1("-simple",                 simple                            = true)
        else ARG1("-verbose",                verbose                           = true)

        // Execution options:
        else ARG2("-session_name",           args->mSessionName                = argv[i])
        else ARG1("-stop_existing_session",  args->mStopExistingSession        = true)
        else ARG1("-dont_restart_as_admin",  args->mTryToElevate               = false)
        else ARG1("-terminate_on_proc_exit", args->mTerminateOnProcExit        = true)
        else ARG1("-terminate_after_timed",  args->mTerminateAfterTimer        = true)

        // Beta options:
        else ARG1("-include_mixed_reality",  args->mIncludeWindowsMixedReality = true)
        else ARG1("-shutdown",               args->mShutdown                   = true)

        // Provided argument wasn't recognized
        else fprintf(stderr, "error: unrecognized argument '%s'.\n", argv[i]);

        PrintHelp();
        return false;
    }

    // Set mVerbosity enum based on simple/verbose collection specified.  If
    // both -simple and -verbose arguments are used, ignore -simple.
    if (verbose) {
        if (simple) {
            fprintf(stderr, "warning: -simple and -verbose arguments are not compatible; ignoring -simple.\n");
        }
        args->mVerbosity = Verbosity::Verbose;
    }
    else if (simple) {
        args->mVerbosity = Verbosity::Simple;
    }

    // Disallow hotkey of CTRL+C, CTRL+SCROLL, and F12
    if (args->mHotkeySupport) {
        if ((args->mHotkeyModifiers & MOD_CONTROL) != 0 && (
            args->mHotkeyVirtualKeyCode == 0x44 /*C*/ ||
            args->mHotkeyVirtualKeyCode == VK_SCROLL)) {
            fprintf(stderr, "error: CTRL+C or CTRL+SCROLL cannot be used as a -hotkey, they are reserved for terminating the trace.\n");
            PrintHelp();
            return false;
        }

        if (args->mHotkeyModifiers == MOD_NOREPEAT && args->mHotkeyVirtualKeyCode == VK_F12) {
            fprintf(stderr, "error: 'F12' cannot be used as a -hotkey, it is reserved for the debugger.\n");
            PrintHelp();
            return false;
        }
    }

    // If -no_csv is used, ignore -qpc_time, -multi_csv, -output_file, or -output_stdout
    // if they are also used.
    if (!args->mOutputCsvToFile) {
        if (args->mOutputQpcTime) {
            fprintf(stderr, "warning: -qpc_time and -no_csv arguments are not compatible; ignoring -qpc_time.\n");
            args->mOutputQpcTime = false;
        }
        if (args->mMultiCsv) {
            fprintf(stderr, "warning: -multi_csv and -no_csv arguments are not compatible; ignoring -multi_csv.\n");
            args->mMultiCsv = false;
        }
        if (args->mOutputCsvFileName != nullptr) {
            fprintf(stderr, "warning: -output_file and -no_csv arguments are not compatible; ignoring -output_file.\n");
            args->mOutputCsvFileName = nullptr;
        }
        if (args->mOutputCsvToStdout) {
            fprintf(stderr, "warning: -output_stdout and -no_csv arguments are not compatible; ignoring -output_stdout.\n");
            args->mOutputCsvToStdout = false;
        }
    }

    // If we're outputing CSV to stdout, we can't use it for console output.
    //
    // Further, we're currently limited to outputing CSV to either file(s) or
    // stdout, so disallow use of both -output_file and -output_stdout.  Also,
    // since -output_stdout redirects all CSV output to stdout ignore
    // -multi_csv or -include_mixed_reality in this case.
    if (args->mOutputCsvToStdout) {
        args->mConsoleOutputType = ConsoleOutput::None; // No warning needed if user used -no_top, just swap out Simple for None

        if (args->mOutputCsvFileName != nullptr) {
            fprintf(stderr, "error: only one of -output_file or -output_stdout arguments can be used.\n");
            PrintHelp();
            return false;
        }

        if (args->mMultiCsv) {
            fprintf(stderr, "warning: -multi_csv and -output_stdout are not compatible; ignoring -multi_csv.\n");
            args->mMultiCsv = false;
        }

        if (args->mIncludeWindowsMixedReality) {
            fprintf(stderr, "warning: -include_mixed_reality and -output_stdout are not compatible; ignoring -include_mixed_reality.\n");
            args->mIncludeWindowsMixedReality = false;
        }
    }

    // Try to initialize the console, and warn if we're not going to be able to
    // do the advanced display as requested.
    if (args->mConsoleOutputType == ConsoleOutput::Full && !args->mOutputCsvToStdout && !InitializeConsole()) {
        if (args->mOutputCsvToFile) {
            fprintf(stderr, "warning: could not initialize console display; continuing with -no_top.\n");
            args->mConsoleOutputType = ConsoleOutput::Simple;
        } else {
            fprintf(stderr, "error: could not initialize console display; use -no_top or -output_stdout in this environment.\n");
            PrintHelp();
            return false;
        }
    }

    return true;
}

