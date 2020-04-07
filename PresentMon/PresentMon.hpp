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

#pragma once

/*
ETW Architecture:

    Controller -----> Trace Session <----- Providers (e.g., DXGI, D3D9, DXGK, DWM, Win32K)
                           |
                           \-------------> Consumers (e.g., ../PresentData/PresentMonTraceConsumer)

PresentMon Architecture:

    MainThread: starts and stops the trace session and coordinates user
    interaction.

    ConsumerThread: is controlled by the trace session, and collects and
    analyzes ETW events.

    OutputThread: is controlled by the trace session, and outputs analyzed
    events to the CSV and/or console.

The trace session and ETW analysis is always running, but whether or not
collected data is written to the CSV file(s) is controlled by a recording state
which is controlled from MainThread based on user input or timer.
*/

#include "../PresentData/MixedRealityTraceConsumer.hpp"
#include "../PresentData/PresentMonTraceConsumer.hpp"

#include <unordered_map>

enum class Verbosity {
    Simple,
    Normal,
    Verbose
};

enum class ConsoleOutput {
    None,
    Simple,
    Full
};

struct CommandLineArgs {
    std::vector<const char*> mTargetProcessNames;
    std::vector<const char*> mExcludeProcessNames;
    const char *mOutputCsvFileName;
    const char *mEtlFileName;
    const char *mSessionName;
    UINT mTargetPid;
    UINT mDelay;
    UINT mTimer;
    UINT mHotkeyModifiers;
    UINT mHotkeyVirtualKeyCode;
    ConsoleOutput mConsoleOutputType;
    Verbosity mVerbosity;
    bool mOutputCsvToFile;
    bool mOutputCsvToStdout;
    bool mOutputQpcTime;
    bool mScrollLockIndicator;
    bool mExcludeDropped;
    bool mTerminateOnProcExit;
    bool mTerminateAfterTimer;
    bool mHotkeySupport;
    bool mTryToElevate;
    bool mIncludeWindowsMixedReality;
    bool mMultiCsv;
    bool mStopExistingSession;
};

// CSV output only requires last presented/displayed event to compute frame
// information, but if outputing to the console we maintain a longer history of
// presents to compute averages, limited to 120 events (2 seconds @ 60Hz) to
// reduce memory/compute overhead.
struct SwapChainData {
    enum { PRESENT_HISTORY_MAX_COUNT = 120 };
    std::shared_ptr<PresentEvent> mPresentHistory[PRESENT_HISTORY_MAX_COUNT];
    uint32_t mPresentHistoryCount;
    uint32_t mNextPresentIndex;
    uint32_t mLastDisplayedPresentIndex;
};

struct OutputCsv {
    FILE* mFile;
    FILE* mWmrFile;
};

struct ProcessInfo {
    std::string mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle;
    OutputCsv mOutputCsv;
    bool mTargetProcess;
};

#include "LateStageReprojectionData.hpp"

// CommandLine.cpp:
bool ParseCommandLine(int argc, char** argv);
CommandLineArgs const& GetCommandLineArgs();

// Console.cpp:
bool InitializeConsole();
void ConsolePrint(char const* format, ...);
void ConsolePrintLn(char const* format, ...);
void CommitConsole();
void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo);

// ConsumerThread.cpp:
void StartConsumerThread(TRACEHANDLE traceHandle);
void WaitForConsumerThreadToExit();

// CsvOutput.cpp:
void IncrementRecordingCount();
OutputCsv GetOutputCsv(ProcessInfo* processInfo);
void CloseOutputCsv(ProcessInfo* processInfo);
void UpdateCsv(ProcessInfo* processInfo, SwapChainData const& chain, PresentEvent const& p);
const char* FinalStateToDroppedString(PresentResult res);
const char* PresentModeToString(PresentMode mode);
const char* RuntimeToString(Runtime rt);

// MainThread.cpp:
void ExitMainThread();

// OutputThread.cpp:
void StartOutputThread();
void StopOutputThread();
void SetOutputRecordingState(bool record);

// Privilege.cpp:
void ElevatePrivilege(int argc, char** argv);

// TraceSession.cpp:
bool StartTraceSession();
void StopTraceSession();
void CheckLostReports(ULONG* eventsLost, ULONG* buffersLost);
void DequeueAnalyzedInfo(
    std::vector<NTProcessEvent>* ntProcessEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presents,
    std::vector<std::shared_ptr<LateStageReprojectionEvent>>* lsrs);
double QpcDeltaToSeconds(uint64_t qpcDelta);
uint64_t SecondsDeltaToQpc(double secondsDelta);
double QpcToSeconds(uint64_t qpc);

