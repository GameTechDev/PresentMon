// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"
#include "../IntelPresentMon/CommonUtilities/Math.h"
#include "../IntelPresentMon/CommonUtilities/str/String.h"
#include "../IntelPresentMon/CommonUtilities/Mc/MetricsTypes.h"
#include "../IntelPresentMon/CommonUtilities/Mc/MetricsCalculator.h"

#include <algorithm>
#include <shlwapi.h>
#include <thread>

using namespace pmon;

static std::thread gThread;
static bool gQuit = false;

#include <iostream>
#include <set>
#include <deque>

// When we collect realtime ETW events, we don't receive the events in real
// time but rather sometime after they occur.  Since the user might be toggling
// recording based on realtime cues (e.g., watching the target application) we
// maintain a history of realtime record toggle events from the user.  When we
// consider recording an event, we can look back and see what the recording
// state was at the time the event actually occurred.
//
// gRecordingToggleHistory is a vector of QueryPerformanceCounter() values at
// times when the recording state changed, and gIsRecording is the recording
// state at the current time.
//
// CRITICAL_SECTION used as this is expected to have low contention (e.g., *no*
// contention when capturing from ETL).

static CRITICAL_SECTION gRecordingToggleCS;
static std::vector<uint64_t> gRecordingToggleHistory;
static bool gIsRecording = false;

void SetOutputRecordingState(bool record)
{
    auto const& args = GetCommandLineArgs();

    EnterCriticalSection(&gRecordingToggleCS);

    if (gIsRecording != record) {
        gIsRecording = record;

        // When capturing from an ETL file, just use the current recording state.
        // It's not clear how best to map realtime to ETL QPC time, and there
        // aren't any realtime cues in this case.
        if (args.mEtlFileName == nullptr) {
            uint64_t qpc = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &qpc);
            gRecordingToggleHistory.emplace_back(qpc);
        }
    }

    LeaveCriticalSection(&gRecordingToggleCS);
}

static bool CopyRecordingToggleHistory(std::vector<uint64_t>* recordingToggleHistory)
{
    std::vector<uint64_t> newToggles;
    bool currentRecordingState;
    {
        EnterCriticalSection(&gRecordingToggleCS);
        newToggles.swap(gRecordingToggleHistory);
        currentRecordingState = gIsRecording;
        LeaveCriticalSection(&gRecordingToggleCS);
    }

    recordingToggleHistory->insert(recordingToggleHistory->end(), newToggles.begin(), newToggles.end());
    return currentRecordingState;
}

// Processes are handled differently when running in realtime collection vs.
// ETL collection.  When reading an ETL, we receive NT_PROCESS events whenever
// a process is created or exits which we use to update the active processes.
//
// When collecting events in realtime and with elevated privilege, we should
// get similar ProcessStart/ProcessStop events, but only if PresentMon is
// running when the process started/stopped.  If we don't have elevated
// privilege or we missed a process start/stop we update the active processes
// whenever we notice an event with a new process id.  If it's a target
// process, we obtain a handle to the process, and periodically check it to see
// if it has exited.

static std::unordered_map<uint32_t, ProcessInfo> gProcesses;
static uint32_t gTargetProcessCount = 0;

// Removes any directory and extension, and converts the remaining name to
// lower case.
void CanonicalizeProcessName(std::wstring* name)
{
    size_t i = name->find_last_of(L"./\\");
    if (i != std::wstring::npos && (*name)[i] == L'.') {
        name->resize(i);
        i = name->find_last_of(L"/\\");
    }

    *name = name->substr(i + 1);

    std::transform(name->begin(), name->end(), name->begin(),
                   [](wchar_t c) { return (wchar_t) ::towlower(c); });
}

static bool IsTargetProcess(uint32_t processId, std::wstring const& processName)
{
    auto const& args = GetCommandLineArgs();

    std::wstring compareName;
    if (args.mExcludeProcessNames.size() + args.mTargetProcessNames.size() > 0) {
        compareName = processName;
        CanonicalizeProcessName(&compareName);
    }

    // --exclude
    for (auto excludeProcessName : args.mExcludeProcessNames) {
        if (excludeProcessName == compareName) {
            return false;
        }
    }

    // --capture_all
    if (args.mTargetPid == 0 && args.mTargetProcessNames.empty()) {
        return true;
    }

    // --process_id
    if (args.mTargetPid != 0 && args.mTargetPid == processId) {
        return true;
    }

    // --process_name
    for (auto targetProcessName : args.mTargetProcessNames) {
        if (targetProcessName == compareName) {
            return true;
        }
    }

    return false;
}

static void HandleTerminatedProcess(
    ProcessInfo* processInfo)
{
    auto const& args = GetCommandLineArgs();

    if (processInfo->mIsTargetProcess) {
        // Close this process' CSV.
        CloseMultiCsv(processInfo);

        // Quit if this is the last process tracked for --terminate_on_proc_exit.
        gTargetProcessCount -= 1;
        if (args.mTerminateOnProcExit && gTargetProcessCount == 0) {
            ExitMainThread();
        }
    }
}

static void ProcessProcessEvent(
    ProcessEvent const& processEvent)
{
    if (processEvent.IsStartEvent) {
        auto pr = gProcesses.emplace(processEvent.ProcessId, ProcessInfo{});
        auto info = &pr.first->second;

        if (!pr.second) {
            HandleTerminatedProcess(info);
        }

        info->mHandle          = NULL;
        info->mModuleName      = processEvent.ImageFileName;
        info->mOutputCsv       = nullptr;
        info->mIsTargetProcess = IsTargetProcess(processEvent.ProcessId, processEvent.ImageFileName);

        if (info->mIsTargetProcess) {
            gTargetProcessCount += 1;
        }
    } else {
        auto ii = gProcesses.find(processEvent.ProcessId);
        if (ii != gProcesses.end()) {
            HandleTerminatedProcess(&ii->second);
            gProcesses.erase(std::move(ii));
        }
    }
}

static void UpdateProcessEvents(
    PMTraceConsumer* pmConsumer,
    std::vector<ProcessEvent>* processEvents)
{
    std::vector<ProcessEvent> newProcessEvents;
    pmConsumer->DequeueProcessEvents(newProcessEvents);

    if (!newProcessEvents.empty()) {
        processEvents->insert(processEvents->end(), newProcessEvents.begin(), newProcessEvents.end());
        newProcessEvents.clear();
        newProcessEvents.shrink_to_fit();

        std::sort(processEvents->begin(), processEvents->end(), [](ProcessEvent const& a, ProcessEvent const& b) { return a.QpcTime < b.QpcTime; });
    }

    // Check if any realtime processes terminated and create process events for them.
    //
    // We assume that the process terminated now, which is wrong but conservative and functionally
    // ok because no other process should start with the same PID as long as we're still holding a
    // handle to it.
    for (auto& pair : gProcesses) {
        auto processId = pair.first;
        auto processInfo = &pair.second;

        DWORD exitCode = 0;
        if (processInfo->mHandle != NULL && GetExitCodeProcess(processInfo->mHandle, &exitCode) && exitCode != STILL_ACTIVE) {
            uint64_t qpc = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &qpc);

            ProcessEvent e;
            e.ImageFileName = processInfo->mModuleName;
            e.QpcTime       = qpc;
            e.ProcessId     = processId;
            e.IsStartEvent  = false;
            processEvents->push_back(e);

            CloseHandle(processInfo->mHandle);
            processInfo->mHandle = NULL;
        }
    }
}

static void UpdateAverage(float* avg, double value)
{
    float constexpr expAvgScale = 0.0165f; // similar result to 120-present moving average

    if (value == 0.0) {
        *avg = 0.f;
    } else if (*avg == 0.f) {
        *avg = float(value);
    } else {
        *avg = (1.f - expAvgScale) * *avg + expAvgScale * float(value);
    }
}

static void PruneOldSwapChainData(
    PMTraceSession const& pmSession,
    uint64_t latestTimestamp)
{
    // sometimes we arrive here after skipping all frame events in the processing loop,
    // in which case we don't have a valid timestamp for the latest frame and should not
    // attempt to do any pruning during this pass
    if (latestTimestamp == 0) {
        return;
    }

    auto minTimestamp = latestTimestamp - pmSession.MilliSecondsDeltaToTimestamp(4000.0);

    for (auto& pair : gProcesses) {
        auto processInfo = &pair.second;

        // Check if this is DWM process
        const auto processNameLower = util::str::ToLower(processInfo->mModuleName);
        const bool isDwmProcess = (processNameLower.find(L"dwm.exe") != std::wstring::npos);

        for (auto ii = processInfo->mSwapChain.begin(), ie = processInfo->mSwapChain.end(); ii != ie; ) {
            auto swapChainAddress = ii->first;
            auto chain = &ii->second;

            // Don't prune DWM swap chains with address 0x0
            bool shouldSkipPruning = isDwmProcess && swapChainAddress == 0x0;
            bool shouldPrune = false;
            if (!shouldSkipPruning) {
                shouldPrune = chain->mUnifiedSwapChain.IsPrunableBefore(minTimestamp);
            }

            if (shouldPrune) {
                ii = processInfo->mSwapChain.erase(ii);
            }
            else {
                ++ii;
            }
        }
    }
}

static void QueryProcessName(uint32_t processId, ProcessInfo* info)
{
    auto const& args = GetCommandLineArgs();

    wchar_t path[MAX_PATH];
    const wchar_t* processName = L"<unknown>";
    HANDLE handle = NULL;

    if (args.mEtlFileName == nullptr) {
        handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (handle != NULL) {
            DWORD numChars = _countof(path);
            if (QueryFullProcessImageNameW(handle, 0, path, &numChars)) {
                for (;; --numChars) {
                    if (numChars == 0 || path[numChars - 1] == L'\\' || path[numChars - 1] == L'/') {
                        processName = &path[numChars];
                        break;
                    }
                }
            }
        }
    }

    info->mModuleName = processName;
    info->mHandle     = handle;
}

static bool GetPresentProcessInfo(
    std::shared_ptr<PresentEvent> const& presentEvent,
    bool create,
    ProcessInfo** outProcessInfo,
    SwapChainData** outChain,
    uint64_t* outPresentTime)
{
    ProcessInfo* processInfo;
    auto ii = gProcesses.find(presentEvent->ProcessId);
    if (ii != gProcesses.end()) {
        processInfo = &ii->second;
    } else {
        if (!create) {
            *outProcessInfo = nullptr;
            *outChain       = nullptr;
            *outPresentTime = presentEvent->PresentStartTime;
            return false;
        }

        ProcessInfo info;
        QueryProcessName(presentEvent->ProcessId, &info);
        info.mOutputCsv       = nullptr;
        info.mIsTargetProcess = IsTargetProcess(presentEvent->ProcessId, info.mModuleName);
        if (info.mIsTargetProcess) {
            gTargetProcessCount += 1;
        }

        processInfo = &gProcesses.emplace(presentEvent->ProcessId, info).first->second;
    }

    if (!processInfo->mIsTargetProcess) {
        return true;
    }

    auto chain = &processInfo->mSwapChain[presentEvent->SwapChainAddress];
    if (!chain->mUnifiedSwapChain.swapChain.lastPresent.has_value()) {
        using namespace pmon::util::metrics;
        chain->mUnifiedSwapChain.SeedFromFirstPresent(FrameData::CopyFrameData(presentEvent));
        return true;
    }

    *outProcessInfo = processInfo;
    *outChain       = chain;
    *outPresentTime = chain->mUnifiedSwapChain.GetLastPresentQpc();
    return false;
}

static void ProcessRecordingToggle(
    bool* isRecording)
{
    auto const& args = GetCommandLineArgs();

    if (*isRecording) {
        *isRecording = false;

        IncrementRecordingCount();

        if (args.mMultiCsv) {
            for (auto& pair : gProcesses) {
                CloseMultiCsv(&pair.second);
            }
        } else {
            CloseGlobalCsv();
        }
    } else {
        *isRecording = true;
    }
}

static FrameMetrics1 ToFrameMetrics1(pmon::util::metrics::FrameMetrics const& m)
{
    FrameMetrics1 out{};
    out.msBetweenPresents = m.msBetweenPresents;
    out.msInPresentApi = m.msInPresentApi;
    out.msUntilRenderComplete = m.msUntilRenderComplete;
    out.msUntilDisplayed = m.msUntilDisplayed;
    out.msBetweenDisplayChange = m.msBetweenDisplayChange;
    out.msUntilRenderStart = m.msUntilRenderStart;
    out.msGPUDuration = m.msGpuDuration;
    out.msVideoDuration = m.msVideoDuration;
    out.msSinceInput = m.msSinceInput;
    out.qpcScreenTime = m.screenTimeQpc;
    out.msFlipDelay = m.msFlipDelay.has_value() ? m.msFlipDelay.value() : 0.0;
    return out;
}

static void ProcessEvents(
    PMTraceSession const& pmSession,
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    std::vector<ProcessEvent>* processEvents,
    std::vector<uint64_t>* recordingToggleHistory,
    bool currentRecordingState)
{
    auto const& args = GetCommandLineArgs();
    auto computeAvg = args.mConsoleOutput == ConsoleOutput::Statistics;

    // Determine the recording state and when the next toggle is.
    size_t recordingToggleIndex = 0;
    size_t recordingToggleCount = recordingToggleHistory->size();
    bool checkRecordingToggle   = recordingToggleCount > 0;
    bool isRecording            = recordingToggleCount & 1 ? !currentRecordingState : currentRecordingState;

    // Determine if there are process events to check.
    size_t processEventIndex = 0;
    size_t processEventCount = processEvents->size();
    bool   checkProcessTime  = processEventCount > 0;

    const uint64_t qpcFrequency = static_cast<uint64_t>(pmSession.mTimestampFrequency.QuadPart);
    const uint64_t qpcStart = static_cast<uint64_t>(pmSession.mStartTimestamp.QuadPart);
    pmon::util::QpcConverter qpc(qpcFrequency, qpcStart);

    // Iterate through the processEvents, handling process events and recording toggles along the
    // way.
    uint64_t presentTime = 0;
    for (auto const& presentEvent : presentEvents) {

        // Ignore failed and lost presents.
        if (presentEvent->IsLost || presentEvent->PresentFailed) {
            continue;
        }

        // Look up the process this present belongs to.  If the process info doesn't exist yet,
        // handle process events first and then check again.  
        ProcessInfo* processInfo = nullptr;
        SwapChainData* chain = nullptr;
        if (GetPresentProcessInfo(presentEvent, false, &processInfo, &chain, &presentTime)) {
            continue;
        }

        // Handle any process events that occurred before this present
        if (checkProcessTime) {
            while ((*processEvents)[processEventIndex].QpcTime < presentTime) {
                auto& processEvent = (*processEvents)[processEventIndex];

                // If this is a termination event for the same process as the current present,
                // skip processing this termination to avoid removing the process info we need
                if (!processEvent.IsStartEvent && processEvent.ProcessId == presentEvent->ProcessId) {
                    // Don't process this termination yet - we have a present from this process to handle first
                    break;
                }

                ProcessProcessEvent((*processEvents)[processEventIndex]);
                processEventIndex += 1;
                if (processEventIndex == processEventCount) {
                    checkProcessTime = false;
                    break;
                }
            }
        }

        // Handle any recording toggles that occurred before this present
        if (checkRecordingToggle) {
            while ((*recordingToggleHistory)[recordingToggleIndex] < presentTime) {
                ProcessRecordingToggle(&isRecording);
                recordingToggleIndex += 1;
                if (recordingToggleIndex == recordingToggleCount) {
                    checkRecordingToggle = false;
                    break;
                }
            }
        }

        // If we didn't get process info, try again (this time querying realtime data if needed).
        if (processInfo == nullptr && GetPresentProcessInfo(presentEvent, true, &processInfo, &chain, &presentTime)) {
            continue;
        }

        auto ready = chain->mUnifiedSwapChain.Enqueue(pmon::util::metrics::FrameData::CopyFrameData(presentEvent),
            args.mUseV1Metrics ? pmon::util::metrics::MetricsVersion::V1 : pmon::util::metrics::MetricsVersion::V2);

        // Do we need to emit metrics for this present?
        const bool emit = (isRecording || computeAvg);

        for (auto& it : ready) {
            // Build FrameData copies for the unified calculator state-advance (and V2 metrics).
            using namespace pmon::util::metrics;

            FrameData frame = std::move(it.present);

            FrameData nextFrame{};
            FrameData* nextPtr = nullptr;
            if (it.nextDisplayed.has_value()) {
                nextFrame = std::move(*it.nextDisplayed);
                nextPtr = &nextFrame;
            }

            if (args.mUseV1Metrics) {
                // V1: compute immediately (no look-ahead) and emit legacy V1 CSV.
                auto computed = ComputeMetricsForPresent(qpc, frame, nullptr, chain->mUnifiedSwapChain.swapChain, MetricsVersion::V1);

                if (emit) {
                    for (auto const& cm : computed) {
                        auto const m1 = ToFrameMetrics1(cm.metrics);

                        if (isRecording) {
                            UpdateCsv(pmSession, processInfo, frame, m1);
                        }

                        if (computeAvg) {
                            UpdateAverage(&chain->mUnifiedSwapChain.avgCPUDuration, m1.msBetweenPresents);
                            UpdateAverage(&chain->mUnifiedSwapChain.avgGPUDuration, m1.msGPUDuration);

                            if (m1.msUntilDisplayed > 0) {
                                UpdateAverage(&chain->mUnifiedSwapChain.avgDisplayLatency, m1.msUntilDisplayed);
                                if (m1.msBetweenDisplayChange > 0) {
                                    UpdateAverage(&chain->mUnifiedSwapChain.avgDisplayedTime, m1.msBetweenDisplayChange);
                                }
                            }
                        }
                    }
                }
            }
            else {
                // V2 unified metrics: compute + advance together
                auto computed = ComputeMetricsForPresent(qpc, frame, nextPtr, chain->mUnifiedSwapChain.swapChain, MetricsVersion::V2);

                if (emit) {
                    for (auto const& cm : computed) {
                        auto const& m = cm.metrics;

                        if (isRecording) {
                            UpdateCsv(pmSession, processInfo, frame, m);
                        }

                        if (computeAvg) {
                            UpdateAverage(&chain->mUnifiedSwapChain.avgCPUDuration, m.msCPUBusy + m.msCPUWait);
                            if (m.msUntilDisplayed > 0) {
                                UpdateAverage(&chain->mUnifiedSwapChain.avgDisplayLatency, m.msDisplayLatency);
                                UpdateAverage(&chain->mUnifiedSwapChain.avgDisplayedTime, m.msDisplayedTime);
                                UpdateAverage(&chain->mUnifiedSwapChain.avgMsUntilDisplayed, m.msUntilDisplayed);
                                UpdateAverage(&chain->mUnifiedSwapChain.avgMsBetweenDisplayChange, m.msBetweenDisplayChange);
                            }
                        }
                    }
                }
            }
        }
    }

    // Prune any SwapChainData that hasn't seen an update for over 4 seconds.
    PruneOldSwapChainData(pmSession, presentTime);

    // Erase any recording toggles and process events that were processed.
    if (recordingToggleIndex > 0) {
        recordingToggleHistory->erase(recordingToggleHistory->begin(), recordingToggleHistory->begin() + recordingToggleIndex);
    }
    if (processEventIndex > 0) {
        processEvents->erase(processEvents->begin(), processEvents->begin() + processEventIndex);
    }
}

void Output(PMTraceSession const* pmSession)
{
    SetThreadDescription(GetCurrentThread(), L"PresentMon Output Thread");

    auto const& args = GetCommandLineArgs();

    // Structures to track processes and statistics from recorded events.
    std::vector<uint64_t> recordingToggleHistory;
    std::vector<ProcessEvent> processEvents;
    std::vector<std::shared_ptr<PresentEvent>> presentEvents;
    processEvents.reserve(128);
    presentEvents.reserve(1024);

    for (;;) {
        // Read gQuit here, but then check it after processing queued events.
        // This ensures that we call Dequeue*() at least once after
        // events have stopped being collected so that all events are included.
        auto quit = gQuit;

        // Copy recording toggle history from MainThread
        bool currentRecordingState = CopyRecordingToggleHistory(&recordingToggleHistory);

        // Copy process events, present events, and lost present events from ConsumerThread.
        UpdateProcessEvents(pmSession->mPMConsumer, &processEvents);
        pmSession->mPMConsumer->DequeuePresentEvents(presentEvents);

        // Process all the collected events, and update the various tracking
        // and statistics data structures.
        if (!presentEvents.empty()) {
            ProcessEvents(*pmSession, presentEvents, &processEvents, &recordingToggleHistory, currentRecordingState);
            presentEvents.clear();
        }

        // Display information to console if requested.  If debug build and
        // simple console, print a heartbeat if recording.
        //
        // gIsRecording is the real timeline recording state.  Because we're
        // just reading it without correlation to gRecordingToggleHistory, we
        // don't need the critical section.
        switch (args.mConsoleOutput) {
        #if _DEBUG
        case ConsoleOutput::Simple:
            if (currentRecordingState && args.mCSVOutput != CSVOutput::None) {
                wprintf(L".");
            }
            break;
        #endif
        case ConsoleOutput::Statistics:
            if (BeginConsoleUpdate()) {
                for (auto const& pair : gProcesses) {
                    UpdateConsole(pair.first, pair.second);
                }

                if (currentRecordingState && args.mCSVOutput != CSVOutput::None) {
                    ConsolePrintLn(L"** RECORDING **");
                }

                EndConsoleUpdate();
            }
            break;
        }

        // Everything is processed and output out at this point, so if we're
        // quiting we don't need to update the rest.
        if (quit) {
            break;
        }

        // Sleep to reduce overhead.
        Sleep(100);
    }

    // Close all CSV and process handles
    for (auto& pair : gProcesses) {
        auto processInfo = &pair.second;
        if (processInfo->mHandle != NULL) {
            CloseHandle(processInfo->mHandle);
        }
        CloseMultiCsv(processInfo);
    }
    CloseGlobalCsv();

    gProcesses.clear();

    gRecordingToggleHistory.clear();
    gRecordingToggleHistory.shrink_to_fit();
}


void StartOutputThread(PMTraceSession const& pmSession)
{
    InitializeCriticalSection(&gRecordingToggleCS);
    gQuit = false;
    gThread = std::thread(Output, &pmSession); // Doesn't work to pass a reference, it makes a copy
}

void StopOutputThread()
{
    if (gThread.joinable()) {
        gQuit = true;
        gThread.join();

        DeleteCriticalSection(&gRecordingToggleCS);
    }
}