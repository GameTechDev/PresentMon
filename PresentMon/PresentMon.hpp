// Copyright (C) 2017,2019-2023 Intel Corporation
// SPDX-License-Identifier: MIT

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

// Verbosity of console output for normal operation:
//     None = none
//     Simple = recording changes, etc.
//     Full = statistics about captured presents
enum class ConsoleOutput {
    None,
    Simple,
    Full
};

struct CommandLineArgs {
    std::vector<std::wstring> mTargetProcessNames;
    std::vector<std::wstring> mExcludeProcessNames;
    const wchar_t *mOutputCsvFileName;
    const wchar_t *mEtlFileName;
    const wchar_t *mSessionName;
    UINT mTargetPid;
    UINT mDelay;
    UINT mTimer;
    UINT mHotkeyModifiers;
    UINT mHotkeyVirtualKeyCode;
    ConsoleOutput mConsoleOutputType;
    bool mTrackDisplay;
    bool mTrackDebug;
    bool mTrackInput;
    bool mTrackGPU;
    bool mTrackGPUVideo;
    bool mTrackWMR;
    bool mOutputCsvToFile;
    bool mOutputCsvToStdout;
    bool mOutputQpcTime;
    bool mOutputQpcTimeInSeconds;
    bool mOutputDateTime;
    bool mScrollLockIndicator;
    bool mExcludeDropped;
    bool mTerminateExisting;
    bool mTerminateOnProcExit;
    bool mStartTimer;
    bool mTerminateAfterTimer;
    bool mHotkeySupport;
    bool mTryToElevate;
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
    uint32_t mLastDisplayedPresentIndex; // UINT32_MAX if none displayed
};

struct OutputCsv {
    FILE* mFile;
    FILE* mWmrFile;
};

struct ProcessInfo {
    std::wstring mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle;
    OutputCsv mOutputCsv;
    bool mIsTargetProcess;
};

#include "LateStageReprojectionData.hpp"

// CommandLine.cpp:
bool ParseCommandLine(int argc, wchar_t** argv);
CommandLineArgs const& GetCommandLineArgs();

// Console.cpp:
bool InitializeConsole();
bool IsConsoleInitialized();
int PrintWarning(char const* format, ...);
int PrintError(char const* format, ...);
int PrintWarningNoNewLine(char const* format, ...);
int PrintErrorNoNewLine(char const* format, ...);
void ConsolePrint(char const* format, ...);
void ConsolePrintLn(char const* format, ...);
void CommitConsole();
void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo);

// ConsumerThread.cpp:
void StartConsumerThread(TRACEHANDLE traceHandle);
void WaitForConsumerThreadToExit();

// CsvOutput.cpp:
void IncrementRecordingCount();
OutputCsv GetOutputCsv(ProcessInfo* processInfo, uint32_t processId);
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
void CanonicalizeProcessName(std::wstring* path);

// Privilege.cpp:
bool InPerfLogUsersGroup();
bool EnableDebugPrivilege();
int RestartAsAdministrator(int argc, wchar_t** argv);

// TraceSession.cpp:
bool StartTraceSession();
void StopTraceSession();
void CheckLostReports(ULONG* eventsLost, ULONG* buffersLost);
void DequeueAnalyzedInfo(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents,
    std::vector<std::shared_ptr<LateStageReprojectionEvent>>* lsrs);
double QpcDeltaToSeconds(uint64_t qpcDelta);
double QpcDeltaToSeconds(uint64_t qpcFrom, uint64_t qpcTo);
double PositiveQpcDeltaToSeconds(uint64_t qpcFrom, uint64_t qpcTo);
uint64_t SecondsDeltaToQpc(double secondsDelta);
double QpcToSeconds(uint64_t qpc);
void QpcToLocalSystemTime(uint64_t qpc, SYSTEMTIME* st, uint64_t* ns);
