// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
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
#include <optional>

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

// What the animation error calculation is based on
enum class AnimationErrorSource {
    CpuStart,
    AppProvider,
    PCLatency,
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
    UINT mPresentEventCircularBufferSize;
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
    bool mTrackPcLatency;
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
    bool mUseV2Metrics;
    bool mStopExistingSession;
    bool mWriteFrameId;
    bool mWriteDisplayTime;
    bool mDisableOfflineBackpressure;
};

// Metrics computed per-frame.  Duration and Latency metrics are in milliseconds.
struct FrameMetrics {
    uint64_t mTimeInSeconds = 0;
    double mMsDisplayLatency = 0;
    double mMsDisplayedTime = 0;
    double mMsBetweenPresents = 0;
    double mMsInPresentApi = 0;
    double mMsUntilRenderComplete = 0;
    double mMsUntilDisplayed = 0;
    double mMsBetweenDisplayChange = 0;
    uint64_t mCPUStart = 0;
    double mMsCPUBusy = 0;
    double mMsCPUWait = 0;
    double mMsGPULatency = 0;
    double mMsGPUBusy = 0;
    double mMsVideoBusy = 0;
    double mMsGPUWait = 0;
    std::optional<double> mMsAnimationError = {};
    std::optional<double> mAnimationTime = {};
    double mMsClickToPhotonLatency = 0;
    double mMsAllInputPhotonLatency = 0;
    uint64_t mScreenTime = 0;
    FrameType mFrameType = FrameType::NotSet;
    double mMsInstrumentedLatency = 0;
    double mMsPcLatency = 0;
    double mMsBetweenSimStarts = 0;

    // Internal Intel Metrics
    double mMsInstrumentedRenderLatency = 0;
    double mMsInstrumentedSleep = 0;
    double mMsInstrumentedGpuLatency = 0;
    double mMsReadyTimeToDisplayLatency = 0;
    double mMsInstrumentedInputTime = 0;

    // Internal NVIDIA Metrics
    double mMsFlipDelay;
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

    // Internal NVIDIA Metrics
    double msFlipDelay;
};

struct FrameMetrics2 {
    uint64_t mCPUStart = 0;
    double mCPUBusy = 0;
    double mCPUWait = 0;
    double mGPULatency = 0;
    double mGPUBusy = 0;
    double mVideoBusy = 0;
    double mGPUWait = 0;
    double mDisplayLatency = 0;
    double mDisplayedTime = 0;
    std::optional<double> mAnimationError = {};
    std::optional<double> mAnimationTime = {};
    double mClickToPhotonLatency = 0;
    double mAllInputPhotonLatency = 0;
    uint64_t mScreenTime = 0;
    FrameType mFrameType = FrameType::NotSet;
    double mInstrumentedLatency = 0;
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
    // The most recent application present that has been processed (e.g., output into CSV and/or used
    // for frame statistics).
    std::shared_ptr<PresentEvent> mLastAppPresent;

    // QPC of the last simulation start time iregardless of whether it was displayed or not
    uint64_t mLastSimStartTime = 0;

    // The CPU start and screen time for the most recent frame that was displayed
    uint64_t mLastDisplayedSimStartTime = 0;
    uint64_t mLastDisplayedAppScreenTime = 0;
    uint64_t mLastDisplayedScreenTime = 0;
    // QPC of first received simulation start time from the application provider
    uint64_t mFirstAppSimStartTime = 0;

    // QPC of last received input data that did not make it to the screen due 
    // to the Present() being dropped
    uint64_t mLastReceivedNotDisplayedAllInputTime = 0;
    uint64_t mLastReceivedNotDisplayedMouseClickTime = 0;
    uint64_t mLastReceivedNotDisplayedAppProviderInputTime = 0;
    // QPC of the last PC Latency simulation start
    uint64_t mLastReceivedNotDisplayedPclSimStart = 0;

    // Animation error source. Start with CPU start QPC and switch if
    // we receive a valid PCL or App Provider simulation start time.
    AnimationErrorSource mAnimationErrorSource = AnimationErrorSource::CpuStart;

    // Frame statistics
    float mAvgCPUDuration = 0.f;
    float mAvgGPUDuration = 0.f;
    float mAvgDisplayLatency = 0.f;
    float mAvgDisplayedTime = 0.f;
    float mAvgMsUntilDisplayed = 0.f;
    float mAvgMsBetweenDisplayChange = 0.f;
    double mEmaInput2FrameStartTime = 0.f;
    double mAccumulatedInput2FrameStartTime = 0.f;

    // Internal NVIDIA Metrics
    uint64_t mLastDisplayedFlipDelay = 0;
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

