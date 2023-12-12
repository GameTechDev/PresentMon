// Copyright (C) 2017,2019-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include <generated/version.h>

#include "PresentMon.hpp"
#include "../PresentData/Debug.hpp"
#include <algorithm>

namespace {

enum {
    DEFAULT_CONSOLE_WIDTH   = 80,
    MAX_ARG_COLUMN_WIDTH    = 40,
    MIN_DESC_COLUMN_WIDTH   = 20,
    ARG_DESC_COLUMN_PADDING = 4,
};

struct KeyNameCode
{
    wchar_t const* mName;
    UINT mCode;
};

KeyNameCode const HOTKEY_MODS[] = {
    { L"ALT",     MOD_ALT     },
    { L"CONTROL", MOD_CONTROL },
    { L"CTRL",    MOD_CONTROL },
    { L"SHIFT",   MOD_SHIFT   },
    { L"WINDOWS", MOD_WIN     },
    { L"WIN",     MOD_WIN     },
};

KeyNameCode const HOTKEY_KEYS[] = {
    { L"BACKSPACE", VK_BACK },
    { L"TAB", VK_TAB },
    { L"CLEAR", VK_CLEAR },
    { L"ENTER", VK_RETURN },
    { L"PAUSE", VK_PAUSE },
    { L"CAPSLOCK", VK_CAPITAL },
    { L"ESC", VK_ESCAPE },
    { L"SPACE", VK_SPACE },
    { L"PAGEUP", VK_PRIOR },
    { L"PAGEDOWN", VK_NEXT },
    { L"END", VK_END },
    { L"HOME", VK_HOME },
    { L"LEFT", VK_LEFT },
    { L"UP", VK_UP },
    { L"RIGHT", VK_RIGHT },
    { L"DOWN", VK_DOWN },
    { L"PRINTSCREEN", VK_SNAPSHOT },
    { L"INS", VK_INSERT },
    { L"DEL", VK_DELETE },
    { L"HELP", VK_HELP },
    { L"NUMLOCK", VK_NUMLOCK },
    { L"SCROLLLOCK", VK_SCROLL },
    { L"NUM0", VK_NUMPAD0 },
    { L"NUM1", VK_NUMPAD1 },
    { L"NUM2", VK_NUMPAD2 },
    { L"NUM3", VK_NUMPAD3 },
    { L"NUM4", VK_NUMPAD4 },
    { L"NUM5", VK_NUMPAD5 },
    { L"NUM6", VK_NUMPAD6 },
    { L"NUM7", VK_NUMPAD7 },
    { L"NUM8", VK_NUMPAD8 },
    { L"NUM9", VK_NUMPAD9 },
    { L"MULTIPLY", VK_MULTIPLY },
    { L"ADD", VK_ADD },
    { L"SEPARATOR", VK_SEPARATOR },
    { L"SUBTRACT", VK_SUBTRACT },
    { L"DECIMAL", VK_DECIMAL },
    { L"DIVIDE", VK_DIVIDE },
    { L"0", 0x30 },
    { L"1", 0x31 },
    { L"2", 0x32 },
    { L"3", 0x33 },
    { L"4", 0x34 },
    { L"5", 0x35 },
    { L"6", 0x36 },
    { L"7", 0x37 },
    { L"8", 0x38 },
    { L"9", 0x39 },
    { L"A", 0x41 },
    { L"B", 0x42 },
    { L"C", 0x43 },
    { L"D", 0x44 },
    { L"E", 0x45 },
    { L"F", 0x46 },
    { L"G", 0x47 },
    { L"H", 0x48 },
    { L"I", 0x49 },
    { L"J", 0x4A },
    { L"K", 0x4B },
    { L"L", 0x4C },
    { L"M", 0x4D },
    { L"N", 0x4E },
    { L"O", 0x4F },
    { L"P", 0x50 },
    { L"Q", 0x51 },
    { L"R", 0x52 },
    { L"S", 0x53 },
    { L"T", 0x54 },
    { L"U", 0x55 },
    { L"V", 0x56 },
    { L"W", 0x57 },
    { L"X", 0x58 },
    { L"Y", 0x59 },
    { L"Z", 0x5A },
    { L"F1", VK_F1 },
    { L"F2", VK_F2 },
    { L"F3", VK_F3 },
    { L"F4", VK_F4 },
    { L"F5", VK_F5 },
    { L"F6", VK_F6 },
    { L"F7", VK_F7 },
    { L"F8", VK_F8 },
    { L"F9", VK_F9 },
    { L"F10", VK_F10 },
    { L"F11", VK_F11 },
    { L"F12", VK_F12 },
    { L"F13", VK_F13 },
    { L"F14", VK_F14 },
    { L"F15", VK_F15 },
    { L"F16", VK_F16 },
    { L"F17", VK_F17 },
    { L"F18", VK_F18 },
    { L"F19", VK_F19 },
    { L"F20", VK_F20 },
    { L"F21", VK_F21 },
    { L"F22", VK_F22 },
    { L"F23", VK_F23 },
    { L"F24", VK_F24 },
};

CommandLineArgs gCommandLineArgs;

size_t GetConsoleWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO info = {};
    return GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) == 0
        ? DEFAULT_CONSOLE_WIDTH
        : std::max<size_t>(DEFAULT_CONSOLE_WIDTH, info.srWindow.Right - info.srWindow.Left + 1);
}

bool ParseKeyName(KeyNameCode const* valid, size_t validCount, wchar_t* name, char const* errorMessage, UINT* outKeyCode)
{
    for (size_t i = 0; i < validCount; ++i) {
        if (_wcsicmp(name, valid[i].mName) == 0) {
            *outKeyCode = valid[i].mCode;
            return true;
        }
    }

    int col = PrintErrorNoNewLine("error: %s '%ws'.\nValid options (case insensitive):", errorMessage, name);

    size_t consoleWidth = GetConsoleWidth();
    for (size_t i = 0; i < validCount; ++i) {
        auto len = wcslen(valid[i].mName);
        if (col + len + 1 > consoleWidth) {
            col = PrintErrorNoNewLine("\n   ") - 1;
        }
        col += PrintErrorNoNewLine(" %ws", valid[i].mName);
    }
    fprintf(stderr, "\n");

    return false;
}

bool AssignHotkey(wchar_t* key, CommandLineArgs* args)
{
#pragma warning(suppress: 4996)
    auto token = wcstok(key, L"+");
    for (;;) {
        auto prev = token;
#pragma warning(suppress: 4996)
        token = wcstok(nullptr, L"+");
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

void SetCaptureAll(CommandLineArgs* args)
{
    if (!args->mTargetProcessNames.empty()) {
        PrintWarning("warning: -captureall elides all previous -process_name arguments.");
        args->mTargetProcessNames.clear();
    }
    if (args->mTargetPid != 0) {
        PrintWarning("warning: -captureall elides all previous -process_id arguments.");
        args->mTargetPid = 0;
    }
}

// Allow /ARG, -ARG, or --ARG
bool ParseArgPrefix(wchar_t** arg)
{
    if (**arg == '/') {
        *arg += 1;
        return true;
    }

    if (**arg == '-') {
        *arg += 1;
        if (**arg == '-') {
            *arg += 1;
        }
        return true;
    }

    return false;
}

bool ParseArg(wchar_t* arg, wchar_t const* option)
{
    return
        ParseArgPrefix(&arg) &&
        _wcsicmp(arg, option) == 0;
}

bool ParseValue(wchar_t** argv, int argc, int* i)
{
    if (*i + 1 < argc) {
        *i += 1;
        return true;
    }
    PrintError("error: %ws expecting argument.", argv[*i]);
    return false;
}

bool ParseValue(wchar_t** argv, int argc, int* i, wchar_t const** value)
{
    if (!ParseValue(argv, argc, i)) return false;
    *value = argv[*i];
    return true;
}

bool ParseValue(wchar_t** argv, int argc, int* i, std::vector<std::wstring>* value)
{
    wchar_t const* v = nullptr;
    if (!ParseValue(argv, argc, i, &v)) return false;
    value->emplace_back(v);
    return true;
}

bool ParseValue(wchar_t** argv, int argc, int* i, UINT* value)
{
    wchar_t const* v = nullptr;
    if (!ParseValue(argv, argc, i, &v)) return false;
    *value = wcstoul(v, nullptr, 10);
    return true;
}

void PrintHelp()
{
    fprintf(stderr, "PresentMon %s\n", PRESENT_MON_VERSION);

    // Layout usage 
    char* s[] = {
        #include <generated/command_line_options.inl>
    };
    size_t argWidth = 0;
    for (size_t i = 0; i < _countof(s); i += 2) {
        auto arg = s[i];
        auto desc = s[i + 1];
        if (desc != nullptr) {
            argWidth = std::max(argWidth, strlen(arg));
        }
    }

    argWidth = std::min<size_t>(argWidth, MAX_ARG_COLUMN_WIDTH);

    size_t descWidth = std::max<size_t>(MIN_DESC_COLUMN_WIDTH, GetConsoleWidth() - ARG_DESC_COLUMN_PADDING - argWidth);

    // Print usage
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

}

CommandLineArgs const& GetCommandLineArgs()
{
    return gCommandLineArgs;
}

bool ParseCommandLine(int argc, wchar_t** argv)
{
    auto args = &gCommandLineArgs;

    args->mTargetProcessNames.clear();
    args->mExcludeProcessNames.clear();
    args->mOutputCsvFileName = nullptr;
    args->mEtlFileName = nullptr;
    args->mSessionName = L"PresentMon";
    args->mTargetPid = 0;
    args->mDelay = 0;
    args->mTimer = 0;
    args->mHotkeyModifiers = MOD_NOREPEAT;
    args->mHotkeyVirtualKeyCode = 0;
    args->mTrackDisplay = true;
    args->mTrackDebug = false;
    args->mTrackInput = false;
    args->mTrackGPU = false;
    args->mTrackGPUVideo = false;
    args->mTrackWMR = false;
    args->mOutputCsvToFile = true;
    args->mOutputCsvToStdout = false;
    args->mOutputQpcTime = false;
    args->mOutputQpcTimeInSeconds = false;
    args->mOutputDateTime = false;
    args->mScrollLockIndicator = false;
    args->mExcludeDropped = false;
    args->mConsoleOutputType = ConsoleOutput::Full;
    args->mTerminateExisting = false;
    args->mTerminateOnProcExit = false;
    args->mStartTimer = false;
    args->mTerminateAfterTimer = false;
    args->mHotkeySupport = false;
    args->mTryToElevate = false;
    args->mMultiCsv = false;
    args->mStopExistingSession = false;

    bool DEPRECATED_dontRestart = false;
    bool DEPRECATED_simple = false;
    bool DEPRECATED_verbose = false;
    bool DEPRECATED_wmr = false;

    #if PRESENTMON_ENABLE_DEBUG_TRACE
    bool verboseTrace = false;
    #endif

    for (int i = 1; i < argc; ++i) {
        // Capture target options:
             if (ParseArg(argv[i], L"captureall"))   { SetCaptureAll(args);                                         continue; }
        else if (ParseArg(argv[i], L"process_name")) { if (ParseValue(argv, argc, &i, &args->mTargetProcessNames))  continue; }
        else if (ParseArg(argv[i], L"exclude"))      { if (ParseValue(argv, argc, &i, &args->mExcludeProcessNames)) continue; }
        else if (ParseArg(argv[i], L"process_id"))   { if (ParseValue(argv, argc, &i, &args->mTargetPid))           continue; }
        else if (ParseArg(argv[i], L"etl_file"))     { if (ParseValue(argv, argc, &i, &args->mEtlFileName))         continue; }

        // Output options:
        else if (ParseArg(argv[i], L"output_file"))   { if (ParseValue(argv, argc, &i, &args->mOutputCsvFileName)) continue; }
        else if (ParseArg(argv[i], L"output_stdout")) { args->mOutputCsvToStdout      = true;                  continue; }
        else if (ParseArg(argv[i], L"multi_csv"))     { args->mMultiCsv               = true;                  continue; }
        else if (ParseArg(argv[i], L"no_csv"))        { args->mOutputCsvToFile        = false;                 continue; }
        else if (ParseArg(argv[i], L"no_top"))        { args->mConsoleOutputType      = ConsoleOutput::Simple; continue; }
        else if (ParseArg(argv[i], L"qpc_time"))      { args->mOutputQpcTime          = true;                  continue; }
        else if (ParseArg(argv[i], L"qpc_time_s"))    { args->mOutputQpcTimeInSeconds = true;                  continue; }

        // Recording options:
        else if (ParseArg(argv[i], L"hotkey"))           { if (ParseValue(argv, argc, &i) && AssignHotkey(argv[i], args)) continue; }
        else if (ParseArg(argv[i], L"delay"))            { if (ParseValue(argv, argc, &i, &args->mDelay)) continue; }
        else if (ParseArg(argv[i], L"timed"))            { if (ParseValue(argv, argc, &i, &args->mTimer)) { args->mStartTimer = true; continue; } }
        else if (ParseArg(argv[i], L"exclude_dropped"))  { args->mExcludeDropped      = true; continue; }
        else if (ParseArg(argv[i], L"scroll_indicator")) { args->mScrollLockIndicator = true; continue; }
        else if (ParseArg(argv[i], L"no_track_display")) { args->mTrackDisplay        = false; continue; }
        else if (ParseArg(argv[i], L"track_debug"))      { args->mTrackDebug          = true; continue; }
        else if (ParseArg(argv[i], L"simple"))           { DEPRECATED_simple          = true; continue; }
        else if (ParseArg(argv[i], L"verbose"))          { DEPRECATED_verbose         = true; continue; }

        // Execution options:
        else if (ParseArg(argv[i], L"session_name"))           { if (ParseValue(argv, argc, &i, &args->mSessionName)) continue; }
        else if (ParseArg(argv[i], L"stop_existing_session"))  { args->mStopExistingSession = true; continue; }
        else if (ParseArg(argv[i], L"terminate_existing"))     { args->mTerminateExisting   = true; continue; }
        else if (ParseArg(argv[i], L"dont_restart_as_admin"))  { DEPRECATED_dontRestart     = true; continue; }
        else if (ParseArg(argv[i], L"restart_as_admin"))       { args->mTryToElevate        = true; continue; }
        else if (ParseArg(argv[i], L"terminate_on_proc_exit")) { args->mTerminateOnProcExit = true; continue; }
        else if (ParseArg(argv[i], L"terminate_after_timed"))  { args->mTerminateAfterTimer = true; continue; }

        // Beta options:
        else if (ParseArg(argv[i], L"date_time"))              { args->mOutputDateTime       = true; continue; }
        else if (ParseArg(argv[i], L"track_gpu"))              { args->mTrackGPU             = true; continue; }
        else if (ParseArg(argv[i], L"track_gpu_video"))        { args->mTrackGPUVideo        = true; continue; }
        else if (ParseArg(argv[i], L"track_input"))            { args->mTrackInput           = true; continue; }
        else if (ParseArg(argv[i], L"track_mixed_reality"))    { args->mTrackWMR             = true; continue; }
        else if (ParseArg(argv[i], L"include_mixed_reality"))  { DEPRECATED_wmr              = true; continue; }

        // Hidden options:
        #if PRESENTMON_ENABLE_DEBUG_TRACE
        else if (ParseArg(argv[i], L"debug_verbose_trace")) { verboseTrace = true; continue; }
        #endif

        // Provided argument wasn't recognized
        else if (!(ParseArg(argv[i], L"?") || ParseArg(argv[i], L"h") || ParseArg(argv[i], L"help"))) {
            PrintError("error: unrecognized argument '%ws'.", argv[i]);
        }

        PrintHelp();
        return false;
    }

    // Handle deprecated command line arguments
    if (DEPRECATED_simple) {
        PrintWarning("warning: -simple command line argument has been deprecated; using -no_track_display instead.");
        args->mTrackDisplay = false;
    }
    if (DEPRECATED_verbose) {
        PrintWarning("warning: -verbose command line argument has been deprecated; using -track_debug instead.");
        args->mTrackDebug = true;
    }
    if (DEPRECATED_wmr) {
        PrintWarning("warning: -include_mixed_reality command line argument has been deprecated; using -track_mixed_reality instead.");
        args->mTrackWMR = true;
    }
    if (DEPRECATED_dontRestart) {
        PrintWarning("warning: -dont_restart_as_admin command line argument has been deprecated; it is now the default behaviour.");
    }

    // Ignore -no_track_display if required for other requested tracking
    if (args->mTrackDebug && !args->mTrackDisplay) {
        PrintWarning("warning: -track_debug requires display tracking; ignoring -no_track_display.");
        args->mTrackDisplay = true;
    }
    if (args->mTrackGPU && !args->mTrackDisplay) {
        PrintWarning("warning: -track_gpu requires display tracking; ignoring -no_track_display.");
        args->mTrackDisplay = true;
    }
    if (args->mTrackGPUVideo && !args->mTrackDisplay) {
        PrintWarning("warning: -track_gpu_video requires display tracking; ignoring -no_track_display.");
        args->mTrackDisplay = true;
    }

    // Enable -qpc_time if only -qpc_time_s was provided, since we use that to
    // add the column.
    if (args->mOutputQpcTimeInSeconds) {
        args->mOutputQpcTime = true;
    }

    // -date_time is mutually exclusive to -qpc_time and -qpc_time_s
    if (args->mOutputDateTime && (args->mOutputQpcTime || args->mOutputQpcTimeInSeconds)) {
        PrintError("error: -date_time and -qpc_time or -qpc_time_s cannot be used at the same time.");
        PrintHelp();
        return false;
    }

    // Disallow hotkey of CTRL+C, CTRL+SCROLL, and F12
    if (args->mHotkeySupport) {
        if ((args->mHotkeyModifiers & MOD_CONTROL) != 0 && (
            args->mHotkeyVirtualKeyCode == 0x44 /*C*/ ||
            args->mHotkeyVirtualKeyCode == VK_SCROLL)) {
            PrintError("error: CTRL+C or CTRL+SCROLL cannot be used as a -hotkey, they are reserved for terminating the trace.");
            PrintHelp();
            return false;
        }

        if (args->mHotkeyModifiers == MOD_NOREPEAT && args->mHotkeyVirtualKeyCode == VK_F12) {
            PrintError("error: 'F12' cannot be used as a -hotkey, it is reserved for the debugger.");
            PrintHelp();
            return false;
        }
    }

    // If -no_csv is used, ignore -date_time, -qpc_time, -qpc_time_s, -multi_csv,
    // -output_file, or -output_stdout if they are also used.
    if (!args->mOutputCsvToFile) {
        if (args->mOutputQpcTime) {
            PrintWarning("warning: -qpc_time and -qpc_time_s are only relevant for CSV output; ignoring due to -no_csv.");
            args->mOutputQpcTime = false;
            args->mOutputQpcTimeInSeconds = false;
        }
        if (args->mOutputDateTime) {
            PrintWarning("warning: -date_time is only relevant for CSV output; ignoring due to -no_csv.");
            args->mOutputDateTime = false;
        }
        if (args->mMultiCsv) {
            PrintWarning("warning: -multi_csv and -no_csv arguments are not compatible; ignoring -multi_csv.");
            args->mMultiCsv = false;
        }
        if (args->mOutputCsvFileName != nullptr) {
            PrintWarning("warning: -output_file and -no_csv arguments are not compatible; ignoring -output_file.");
            args->mOutputCsvFileName = nullptr;
        }
        if (args->mOutputCsvToStdout) {
            PrintWarning("warning: -output_stdout and -no_csv arguments are not compatible; ignoring -output_stdout.");
            args->mOutputCsvToStdout = false;
        }
    }

    // If we're outputing CSV to stdout, we can't use it for console output.
    //
    // Further, we're currently limited to outputing CSV to either file(s) or
    // stdout, so disallow use of both -output_file and -output_stdout.  Also,
    // since -output_stdout redirects all CSV output to stdout ignore
    // -multi_csv or -track_mixed_reality in this case.
    if (args->mOutputCsvToStdout) {
        args->mConsoleOutputType = ConsoleOutput::None;

        if (args->mOutputCsvFileName != nullptr) {
            PrintError("error: only one of -output_file or -output_stdout arguments can be used.");
            PrintHelp();
            return false;
        }

        if (args->mMultiCsv) {
            PrintWarning("warning: -multi_csv and -output_stdout are not compatible; ignoring -multi_csv.");
            args->mMultiCsv = false;
        }

        if (args->mTrackWMR) {
            PrintWarning("warning: -track_mixed_reality and -output_stdout are not compatible; ignoring -track_mixed_reality.");
            args->mTrackWMR = false;
        }
    }

    // Enable verbose trace if requested, and disable Full or Simple console output
    #if PRESENTMON_ENABLE_DEBUG_TRACE
    if (verboseTrace) {
        EnableVerboseTrace(true);
        args->mConsoleOutputType = ConsoleOutput::None;
    }
    #endif

    // Try to initialize the console, and warn if we're not going to be able to
    // do the advanced display as requested.
    if (args->mConsoleOutputType == ConsoleOutput::Full && !IsConsoleInitialized()) {
        PrintWarning("warning: could not initialize console display; continuing with -no_top.");
        args->mConsoleOutputType = ConsoleOutput::Simple;
    }

    // If -terminate_existing, warn about any normal arguments since we'll just
    // be stopping an existing session and then exiting.
    if (args->mTerminateExisting && argc != 2) {
        PrintWarning("warning: -terminate_existing exits without capturing anything; ignoring all capture,\n"
                     "         output, and recording arguments.");
    }

    // Convert the provided process names into a canonical form used for comparison.
    // The comparison is not case-sensitive, and does not include any directory nor
    // extension.
    //
    // This is because the different paths for obtaining process information return
    // different image name strings.  e.g., the ProcessStart event typically has a
    // full path including "\\Device\\..." and ProcessStop event sometimes is
    // missing part of the extension.
    for (auto& name : args->mTargetProcessNames) {
        CanonicalizeProcessName(&name);
    }
    for (auto& name : args->mExcludeProcessNames) {
        CanonicalizeProcessName(&name);
    }

    return true;
}
