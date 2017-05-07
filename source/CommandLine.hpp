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

#pragma once
#include <windows.h>

enum class Verbosity {
    Simple,
    Normal,
    Verbose
};

// Target:           mTargetProcessName mTargetPid mEtlFileName
//  All processes    nullptr            0          nullptr
//  Process by name  process name       0          nullptr
//  Process by ID    nullptr            pid        nullptr
//  ETL file         nullptr            0          path
struct CommandLineArgs {
    const char *mOutputFileName = nullptr;
    const char *mTargetProcessName = nullptr;
    const char *mEtlFileName = nullptr;
    UINT mTargetPid = 0;
    UINT mDelay = 0;
    UINT mTimer = 0;
    UINT mRecordingCount = 0;
    UINT mHotkeyModifiers = MOD_NOREPEAT;
    UINT mHotkeyVirtualKeyCode = VK_F11;
    bool mOutputFile = true;
    bool mScrollLockToggle = false;
    bool mScrollLockIndicator = false;
    bool mExcludeDropped = false;
    Verbosity mVerbosity = Verbosity::Normal;
    bool mSimpleConsole = false;
    bool mTerminateOnProcExit = false;
    bool mTerminateAfterTimer = false;
    bool mHotkeySupport = false;
    bool mTryToElevate = true;
};

bool ParseCommandLine(int argc, char** argv, CommandLineArgs* out);
bool RestartAsAdministrator(int argc, char** argv);
void SetConsoleTitle(int argc, char** argv);
bool EnableScrollLock(bool enable);
