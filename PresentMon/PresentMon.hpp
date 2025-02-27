// Copyright (C) 2017-2024 Intel Corporation
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

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../PresentData/PresentMonTraceSession.hpp"

#include <unordered_map>
#include <queue>

// Verbosity of console output for normal operation:
enum class ConsoleOutput {
    None,      // no output
    Simple,    // commands such as recording start/stop
    Statistics // statistics about captured presents
};

// Optional units to use for time-based metrics metric
enum class TimeUnit {
    MilliSeconds,       // Milliseconds since recording began
    QPC,                // QueryPerformanceCounter value
    QPCMilliSeconds,    // QueryPerformanceCounter value converted into milliseconds
    DateTime,           // Date and time
};

// How to ouput per-frame metrics
enum class CSVOutput {
    None,   // Don't
    File,   // To a CSV file
    Stdout  // To STDOUT in CSV format
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
    TimeUnit mTimeUnit;
    CSVOutput mCSVOutput;
    ConsoleOutput mConsoleOutput;
    bool mTrackDisplay;
    bool mTrackInput;
    bool mTrackGPU;
    bool mTrackGPUVideo;
    bool mTrackFrameType;
    bool mTrackPMMeasurements;
    bool mTrackAppTiming;
    bool mTrackHybridPresent;
    bool mScrollLockIndicator;
    bool mExcludeDropped;
    bool mTerminateExistingSession;
    bool mTerminateOnProcExit;
    bool mStartTimer;
    bool mTerminateAfterTimer;
    bool mHotkeySupport;
    bool mTryToElevate;
    bool mMultiCsv;
    bool mUseV1Metrics;
    bool mStopExistingSession;
    bool mWriteFrameId;
    bool mWriteDisplayTime;
    bool mDisableOfflineBackpressure;
};

// Metrics computed per-frame.  Duration and Latency metrics are in milliseconds.
struct FrameMetrics {
    uint64_t mCPUStart;
    double mCPUBusy;
    double mCPUWait;
    double mGPULatency;
    double mGPUBusy;
    double mVideoBusy;
    double mGPUWait;
    double mDisplayLatency;
    double mDisplayedTime;
    double mAnimationError;
    double mAnimationTime;
    double mClickToPhotonLatency;
    double mAllInputPhotonLatency;
    uint64_t mScreenTime;
    FrameType mFrameType;
    double mInstrumentedLatency;

    // Internal Intel Metrics
    double mInstrumentedRenderLatency;
    double mInstrumentedSleep;
    double mInstrumentedGpuLatency;
    double mReadyTimeToDisplayLatency;
    double mInstrumentedInputTime;
};

struct FrameMetrics1 {
    double msBetweenPresents;
    double msInPresentApi;
    double msUntilRenderComplete;
    double msUntilDisplayed;
    double msBetweenDisplayChange;
    double msUntilRenderStart;
    double msGPUDuration;
    double msVideoDuration;
    double msSinceInput;
    uint64_t qpcScreenTime;
};

// We store SwapChainData per process and per swapchain, where we maintain:
// - information on previous presents needed for console output or to compute metrics for upcoming
//   presents,
// - pending presents whose metrics cannot be computed until future presents are received,
// - exponential averages of key metrics displayed in console output.
struct SwapChainData {
    // Pending presents waiting for the next displayed present.
    std::vector<std::shared_ptr<PresentEvent>> mPendingPresents;

    // The most recent present that has been processed (e.g., output into CSV and/or used for frame
    // statistics).
    std::shared_ptr<PresentEvent> mLastPresent;

    // The CPU start and screen time for the most recent frame that was displayed
    uint64_t mLastDisplayedSimStartTime = 0;
    uint64_t mLastDisplayedScreenTime = 0;
    // QPC of first received simulation start time from the application provider
    uint64_t mFirstAppSimStartTime = 0;

    // QPC of last received input data that did not make it to the screen due 
    // to the Present() being dropped
    uint64_t mLastReceivedNotDisplayedAllInputTime;
    uint64_t mLastReceivedNotDisplayedMouseClickTime;
    uint64_t mLastReceivedNotDisplayedAppProviderInputTime;

    // Frame statistics
    float mAvgCPUDuration = 0.f;
    float mAvgGPUDuration = 0.f;
    float mAvgDisplayLatency = 0.f;
    float mAvgDisplayedTime = 0.f;
};

struct ProcessInfo {
    std::wstring mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle;
    FILE* mOutputCsv;
    bool mIsTargetProcess;
};

// CommandLine.cpp:
bool ParseCommandLine(int argc, wchar_t** argv);
CommandLineArgs const& GetCommandLineArgs();
void PrintHotkeyError();

// Console.cpp:
void InitializeConsole();
void FinalizeConsole();
bool StdOutIsConsole();
bool BeginConsoleUpdate();
void EndConsoleUpdate();
void ConsolePrint(wchar_t const* format, ...);
void ConsolePrintLn(wchar_t const* format, ...);
void UpdateConsole(uint32_t processId, ProcessInfo const& processInfo);
int PrintWarning(wchar_t const* format, ...);
int PrintError(wchar_t const* format, ...);

// ConsumerThread.cpp:
void StartConsumerThread(TRACEHANDLE traceHandle);
void WaitForConsumerThreadToExit();

// CsvOutput.cpp:
void IncrementRecordingCount();
void CloseMultiCsv(ProcessInfo* processInfo);
void CloseGlobalCsv();
const char* PresentModeToString(PresentMode mode);
const char* RuntimeToString(Runtime rt);
void UpdateCsv(PMTraceSession const& pmSession, ProcessInfo* processInfo, PresentEvent const& p, FrameMetrics const& metrics);
void UpdateCsv(PMTraceSession const& pmSession, ProcessInfo* processInfo, PresentEvent const& p, FrameMetrics1 const& metrics);

// MainThread.cpp:
void ExitMainThread();

// OutputThread.cpp:
void StartOutputThread(PMTraceSession const& pmSession);
void StopOutputThread();
void SetOutputRecordingState(bool record);
void CanonicalizeProcessName(std::wstring* path);

// Privilege.cpp:
bool InPerfLogUsersGroup();
bool EnableDebugPrivilege();
int RestartAsAdministrator(int argc, wchar_t** argv);

