// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"
#include "../IntelPresentMon/CommonUtilities/Math.h"

#include <algorithm>
#include <shlwapi.h>
#include <thread>

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

static void UpdateChain(
    SwapChainData* chain,
    std::shared_ptr<PresentEvent> const& p)
{
    if (p->FinalState == PresentResult::Presented) {
        if (p->Displayed.empty() == false) {
            if (p->Displayed.back().first == FrameType::NotSet ||
                p->Displayed.back().first == FrameType::Application) {
                
                // If the chain animation error source has been set to either
                // app provider or PCL latency then set the last displayed simulation start time and the
                // first app simulation start time based on the animation error source type.
                if (chain->mAnimationErrorSource == AnimationErrorSource::AppProvider) {
                    chain->mLastDisplayedSimStartTime = p->AppSimStartTime;
                    if (chain->mFirstAppSimStartTime == 0) {
                        // Received the first app sim start time.
                        chain->mFirstAppSimStartTime = p->AppSimStartTime;
                    }
                    chain->mLastDisplayedAppScreenTime = p->Displayed.back().second;
                } else if (chain->mAnimationErrorSource == AnimationErrorSource::PCLatency) {
                    // In the case of PCLatency only set values if pcl sim start time is not zero.
                    if (p->PclSimStartTime != 0) {
                        chain->mLastDisplayedSimStartTime = p->PclSimStartTime;
                        if (chain->mFirstAppSimStartTime == 0) {
                            // Received the first app sim start time.
                            chain->mFirstAppSimStartTime = p->PclSimStartTime;
                        }
                        chain->mLastDisplayedAppScreenTime = p->Displayed.back().second;
                    }
                } else {
                    // Currently sourcing animation error from CPU start time, however check
                    // to see if we have a valid app provider or PCL sim start time and set the
                    // new animation source and set the first app sim start time
                    if (p->AppSimStartTime != 0) {
                        chain->mAnimationErrorSource = AnimationErrorSource::AppProvider;
                        chain->mLastDisplayedSimStartTime = p->AppSimStartTime;
                        if (chain->mFirstAppSimStartTime == 0) {
                            // Received the first app sim start time.
                            chain->mFirstAppSimStartTime = p->AppSimStartTime;
                        }
                        chain->mLastDisplayedAppScreenTime = p->Displayed.back().second;
                    } else if (p->PclSimStartTime != 0) {
                        chain->mAnimationErrorSource = AnimationErrorSource::PCLatency;
                        chain->mLastDisplayedSimStartTime = p->PclSimStartTime;
                        if (chain->mFirstAppSimStartTime == 0) {
                            // Received the first app sim start time.
                            chain->mFirstAppSimStartTime = p->PclSimStartTime;
                        }
                        chain->mLastDisplayedAppScreenTime = p->Displayed.back().second;
                    } else {
                        if (chain->mLastAppPresent != nullptr) {
                            chain->mLastDisplayedSimStartTime = chain->mLastAppPresent->PresentStartTime +
                                chain->mLastAppPresent->TimeInPresent;
                        }
                        chain->mLastDisplayedAppScreenTime = p->Displayed.back().second;
                    }
                }
            }
        }
        // Want this to always be updated with the last displayed screen time regardless if the
        // frame was generated or not
        chain->mLastDisplayedScreenTime = p->Displayed.empty() ? 0 : p->Displayed.back().second;
        // Update last flipDelay. For NV GPU, size of p->Displayed can only be empty or 1
        chain->mLastDisplayedFlipDelay = p->Displayed.empty() ? 0 : p->FlipDelay;
    }

    if (p->Displayed.empty() == false) {
        if (p->Displayed.back().first == FrameType::NotSet ||
            p->Displayed.back().first == FrameType::Application) {
            chain->mLastAppPresent = p;
        }
    } else {
        // If the last present was not displayed, we need to set the last app present to the last
        // present.
        chain->mLastAppPresent = p;
    }

    // Set chain->mLastSimStartTime to either p->PclSimStartTime or p->AppSimStartTime depending on
    // if either are not zero. If both are zero, do not set.
    if (p->PclSimStartTime != 0) {
        chain->mLastSimStartTime = p->PclSimStartTime;
    } else if (p->AppSimStartTime != 0) {
        chain->mLastSimStartTime = p->AppSimStartTime;
    }

    // Want this to always be updated with the last present regardless if the
    // frame was generated or not
    chain->mLastPresent = p;
}

static void AdjustScreenTimeForCollapsedPresentNV1(
    SwapChainData* chain,
    std::shared_ptr<PresentEvent> const& p,
    uint64_t& screenTime)
{
    if (chain->mLastDisplayedFlipDelay > 0 && (chain->mLastDisplayedScreenTime > screenTime)) {
        // If chain->mLastDisplayedScreenTime that is adjusted by flipDelay is larger than screenTime,
        // it implies the last displayed present is a collapsed present, or a runt frame.
        // So we adjust the screenTime and flipDelay of screenTime,
        // effectively making screenTime equals to chain->mLastDisplayedScreenTime.

        // Cast away constness of p to adjust the screenTime and flipDelay.
        PresentEvent* pp = const_cast<PresentEvent*>(p.get());
        if (!pp->Displayed.empty()) {
            pp->FlipDelay += chain->mLastDisplayedScreenTime - screenTime;
            pp->Displayed[0].second = chain->mLastDisplayedScreenTime;
            screenTime = pp->Displayed[0].second;
        }
    }
}

static void ReportMetrics1(
    PMTraceSession const& pmSession,
    ProcessInfo* processInfo,
    SwapChainData* chain,
    std::shared_ptr<PresentEvent> const& p,
    bool isRecording,
    bool computeAvg)
{
    bool displayed = p->FinalState == PresentResult::Presented;

    uint64_t screenTime = p->Displayed.empty() ? 0 : p->Displayed[0].second;

    // Special handling for NV flipDelay
    AdjustScreenTimeForCollapsedPresentNV1(chain, p, screenTime);

    FrameMetrics1 metrics;
    metrics.msBetweenPresents      = chain->mLastPresent == nullptr ? 0 : pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastPresent->PresentStartTime, p->PresentStartTime);
    metrics.msInPresentApi         = pmSession.TimestampDeltaToMilliSeconds(p->TimeInPresent);
    metrics.msUntilRenderComplete  = pmSession.TimestampDeltaToMilliSeconds(p->PresentStartTime, p->ReadyTime);
    metrics.msUntilDisplayed       = !displayed ? 0 : pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PresentStartTime, screenTime);
    metrics.msBetweenDisplayChange = !displayed || chain->mLastDisplayedScreenTime == 0 ? 0 : pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastDisplayedScreenTime, screenTime);
    metrics.msUntilRenderStart     = pmSession.TimestampDeltaToMilliSeconds(p->PresentStartTime, p->GPUStartTime);
    metrics.msGPUDuration          = pmSession.TimestampDeltaToMilliSeconds(p->GPUDuration);
    metrics.msVideoDuration        = pmSession.TimestampDeltaToMilliSeconds(p->GPUVideoDuration);
    metrics.msSinceInput           = p->InputTime == 0 ? 0 : pmSession.TimestampDeltaToMilliSeconds(p->PresentStartTime - p->InputTime);
    metrics.qpcScreenTime          = screenTime;
    metrics.msFlipDelay            = p->FlipDelay ? pmSession.TimestampDeltaToMilliSeconds(p->FlipDelay) : 0;

    if (isRecording) {
        UpdateCsv(pmSession, processInfo, *p, metrics);
    }

    if (computeAvg) {
        UpdateAverage(&chain->mAvgCPUDuration, metrics.msBetweenPresents);
        UpdateAverage(&chain->mAvgGPUDuration, metrics.msGPUDuration);
        if (metrics.msUntilDisplayed > 0) {
            UpdateAverage(&chain->mAvgDisplayLatency, metrics.msUntilDisplayed);
            if (metrics.msBetweenDisplayChange > 0) {
                UpdateAverage(&chain->mAvgDisplayedTime, metrics.msBetweenDisplayChange);
            }
        }
    }

    UpdateChain(chain, p);
}

static void CalculateAnimationTime(
    PMTraceSession const& pmSession,
    const uint64_t& firstAppSimStartTime,
    const uint64_t& currentSimTime,
    double& animationTime) {

    auto firstSimStartTime = firstAppSimStartTime != 0 ? firstAppSimStartTime : pmSession.mStartTimestamp.QuadPart;
    if (currentSimTime > firstSimStartTime) {
        animationTime = pmSession.TimestampDeltaToMilliSeconds(firstSimStartTime, currentSimTime);
    } else {
        animationTime = 0.;
    }
}


static void AdjustScreenTimeForCollapsedPresentNV(
    std::shared_ptr<PresentEvent> const& p,
    PresentEvent const* nextDisplayedPresent,
    uint64_t& screenTime,
    uint64_t& nextScreenTime)
{
    // nextDisplayedPresent should always be non-null for NV GPU.
    if (p->FlipDelay && screenTime > nextScreenTime && nextDisplayedPresent) {
        // If screenTime that is adjusted by flipDelay is larger than nextScreenTime,
        // it implies this present is a collapsed present, or a runt frame.
        // So we adjust the screenTime and flipDelay of nextDisplayedPresent,
        // effectively making nextScreenTime equals to screenTime.

        // Cast away constness of nextDisplayedPresent to adjust the screenTime and flipDelay.
        PresentEvent* nextDispPresent = const_cast<PresentEvent*>(nextDisplayedPresent);
        nextDispPresent->FlipDelay += (screenTime - nextScreenTime);
        nextScreenTime = screenTime;
        nextDispPresent->Displayed[0].second = nextScreenTime;
    }
}

static void ReportMetricsHelper(
    PMTraceSession const& pmSession,
    ProcessInfo* processInfo,
    SwapChainData* chain,
    std::shared_ptr<PresentEvent> const& p,
    PresentEvent const* nextDisplayedPresent,
    bool isRecording,
    bool computeAvg)
{
    // Figure out what display index to start processing.
    //
    // The following cases are expected:
    // p.Displayed empty and nextDisplayedPresent == nullptr:       process p as not displayed
    // p.Displayed with size N and nextDisplayedPresent == nullptr: process p.Displayed[0..N-2] as displayed, postponing N-1
    // p.Displayed with size N and nextDisplayedPresent != nullptr: process p.Displayed[N-1]    as displayed
    auto displayCount = p->Displayed.size();
    bool displayed = p->FinalState == PresentResult::Presented && displayCount > 0;
    size_t displayIndex = displayed && nextDisplayedPresent != nullptr ? displayCount - 1 : 0;

    // Figure out what display index to attribute cpu work, gpu work, animation error, and input
    // latency to. Start looking from the current display index.
    size_t appIndex = std::numeric_limits<size_t>::max();
    if (displayCount > 0) {
        for (size_t i = displayIndex; i < displayCount; ++i) {
            if (p->Displayed[i].first == FrameType::NotSet ||
                p->Displayed[i].first == FrameType::Application) {
                appIndex = i;
                break;
            }
        }
    } else {
        // If there are no displayed frames
        appIndex = 0;
    }

    do {
        // PB = PresentStartTime
        // PE = PresentEndTime
        // D  = ScreenTime
        //
        // chain->mLastPresent:    PB--PE----D
        // p:                          |        PB--PE----D
        // ...                         |        |   |     |     PB--PE
        // nextDisplayedPresent:       |        |   |     |             PB--PE----D
        //                             |        |   |     |                       |
        // mCPUStart/mCPUBusy:         |------->|   |     |                       |
        // mCPUWait:                            |-->|     |                       |
        // mDisplayLatency:            |----------------->|                       |
        // mDisplayedTime:                                |---------------------->|

        // Lookup the ScreenTime and next ScreenTime
        uint64_t screenTime = 0;
        uint64_t nextScreenTime = 0;
        if (displayed) {
            screenTime = p->Displayed[displayIndex].second;

            if (displayIndex + 1 < displayCount) {
                nextScreenTime = p->Displayed[displayIndex + 1].second;
            } else if (nextDisplayedPresent != nullptr) {
                nextScreenTime = nextDisplayedPresent->Displayed[0].second;
            } else {
                return;
            }
        }

        double msGPUDuration = 0.0;

        FrameMetrics metrics{};
        metrics.mCPUStart = 0;

        // Calculate these metrics for every present
        metrics.mTimeInSeconds = p->PresentStartTime;
        metrics.mMsBetweenPresents = chain->mLastPresent == nullptr ? 0 : 
            pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastPresent->PresentStartTime, p->PresentStartTime);
        metrics.mMsInPresentApi = pmSession.TimestampDeltaToMilliSeconds(p->TimeInPresent);
        metrics.mMsUntilRenderComplete = pmSession.TimestampDeltaToMilliSeconds(p->PresentStartTime, p->ReadyTime);

        if (chain->mLastAppPresent) {
            if (chain->mLastAppPresent->AppPropagatedPresentStartTime != 0) {
                metrics.mCPUStart = chain->mLastAppPresent->AppPropagatedPresentStartTime + chain->mLastAppPresent->AppPropagatedTimeInPresent;
            }
            else {
                metrics.mCPUStart = chain->mLastAppPresent->PresentStartTime + chain->mLastAppPresent->TimeInPresent;
            }
        } else {
            metrics.mCPUStart = chain->mLastPresent->PresentStartTime + chain->mLastPresent->TimeInPresent;
        }

        if (displayIndex == appIndex) {
            uint64_t gpuStartTime = 0;
            if (p->AppPropagatedPresentStartTime != 0) {
                msGPUDuration = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppPropagatedGPUStartTime, p->AppPropagatedReadyTime);
                gpuStartTime = p->AppPropagatedGPUStartTime;
                metrics.mMsCPUBusy = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->AppPropagatedPresentStartTime);
                metrics.mMsCPUWait = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedTimeInPresent);
                metrics.mMsGPULatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, gpuStartTime);
                metrics.mMsGPUBusy = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedGPUDuration);
                metrics.mMsVideoBusy = pmSession.TimestampDeltaToMilliSeconds(p->AppPropagatedGPUVideoDuration);
                metrics.mMsGPUWait = std::max(0.0, msGPUDuration - metrics.mMsGPUBusy);
            } else {
                msGPUDuration = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->GPUStartTime, p->ReadyTime);
                gpuStartTime = p->GPUStartTime;
                metrics.mMsCPUBusy = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, p->PresentStartTime);
                metrics.mMsCPUWait = pmSession.TimestampDeltaToMilliSeconds(p->TimeInPresent);
                metrics.mMsGPULatency = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, gpuStartTime);
                metrics.mMsGPUBusy = pmSession.TimestampDeltaToMilliSeconds(p->GPUDuration);
                metrics.mMsVideoBusy = pmSession.TimestampDeltaToMilliSeconds(p->GPUVideoDuration);
                metrics.mMsGPUWait = std::max(0.0, msGPUDuration - metrics.mMsGPUBusy);
            }

            // Need both AppSleepStart and AppSleepEnd to calculate XellSleep
            metrics.mMsInstrumentedSleep      = (p->AppSleepEndTime == 0 || p->AppSleepStartTime == 0) ? 0 :
                                                pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppSleepStartTime, p->AppSleepEndTime);
            // If there isn't a valid sleep end time use the sim start time
            auto InstrumentedStartTime      = p->AppSleepEndTime != 0 ? p->AppSleepEndTime : p->AppSimStartTime;
            // If neither the sleep end time or sim start time is valid, there is no
            // way to calculate the Xell Gpu latency
            metrics.mMsInstrumentedGpuLatency = InstrumentedStartTime == 0 ? 0 :
                                                pmSession.TimestampDeltaToUnsignedMilliSeconds(InstrumentedStartTime, gpuStartTime);

            // If we have both a valid pcl sim start time and a valid app sim start time, we use the pcl sim start time.
            if (p->PclSimStartTime != 0) {
                metrics.mMsBetweenSimStarts = pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastSimStartTime, p->PclSimStartTime);
            } else if (p->AppSimStartTime != 0) {
                metrics.mMsBetweenSimStarts = pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastSimStartTime, p->AppSimStartTime);
            }
        } else {
            metrics.mMsCPUBusy                = 0;
            metrics.mMsCPUWait                = 0;
            metrics.mMsGPULatency             = 0;
            metrics.mMsGPUBusy                = 0;
            metrics.mMsVideoBusy              = 0;
            metrics.mMsGPUWait                = 0;
            metrics.mMsInstrumentedSleep      = 0;
            metrics.mMsInstrumentedGpuLatency = 0;
            metrics.mMsBetweenSimStarts       = 0;
        }

        // If the frame was displayed regardless of how it was produced, calculate the following
        // metrics
        if (displayed) {
            // Special handling for NV flipDelay
            AdjustScreenTimeForCollapsedPresentNV(p, nextDisplayedPresent, screenTime, nextScreenTime);

            // Calculate the various display metrics
            metrics.mMsFlipDelay            = p->FlipDelay ? pmSession.TimestampDeltaToMilliSeconds(p->FlipDelay) : 0;
            metrics.mMsDisplayLatency       = pmSession.TimestampDeltaToUnsignedMilliSeconds(metrics.mCPUStart, screenTime);
            metrics.mMsDisplayedTime        = pmSession.TimestampDeltaToUnsignedMilliSeconds(screenTime, nextScreenTime);
            metrics.mMsUntilDisplayed       = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PresentStartTime, screenTime);
            metrics.mMsBetweenDisplayChange = chain->mLastDisplayedScreenTime == 0 ? 0 :
                                              pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastDisplayedScreenTime, screenTime);
            metrics.mScreenTime             = screenTime;

            // If we have AppRenderSubmitStart calculate the render latency
            metrics.mMsInstrumentedRenderLatency  = p->AppRenderSubmitStartTime == 0 ? 0 : 
                                                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppRenderSubmitStartTime, screenTime);
            metrics.mMsReadyTimeToDisplayLatency  = pmSession.TimestampDeltaToUnsignedMilliSeconds(p->ReadyTime, screenTime);
            // If there isn't a valid sleep end time use the sim start time
            auto InstrumentedStartTime = p->AppSleepEndTime != 0 ? p->AppSleepEndTime : p->AppSimStartTime;
            // If neither the sleep end time or sim start time is valid, there is no
            // way to calculate the Xell Gpu latency
            metrics.mMsInstrumentedLatency = InstrumentedStartTime == 0 ? 0 : 
                pmSession.TimestampDeltaToUnsignedMilliSeconds(InstrumentedStartTime, screenTime);

            metrics.mMsPcLatency = 0.f;
            // Check to see if we have a valid pc latency sim start time.
            if (p->PclSimStartTime != 0) {
                if (p->PclInputPingTime == 0) {
                    if (chain->mAccumulatedInput2FrameStartTime != 0) {
                        // This frame was displayed but we don't have a pc latency input time. However, there is accumulated time
                        // so there is a pending input that will now hit the screen. Add in the time from the last not
                        // displayed pc simulation start to this frame's pc simulation start.
                        chain->mAccumulatedInput2FrameStartTime +=
                            pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedPclSimStart, p->PclSimStartTime);
                        // Add all of the accumlated time to the average input to frame start time.
                        chain->mEmaInput2FrameStartTime = pmon::util::CalculateEma(
                            chain->mEmaInput2FrameStartTime,
                            chain->mAccumulatedInput2FrameStartTime,
                            0.1);
                        // Reset the tracking variables for when we have a dropped frame with a pc latency input
                        chain->mAccumulatedInput2FrameStartTime = 0.f;
                        chain->mLastReceivedNotDisplayedPclSimStart = 0;
                    }
                } else {
                    chain->mEmaInput2FrameStartTime = pmon::util::CalculateEma(
                        chain->mEmaInput2FrameStartTime,
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PclInputPingTime, p->PclSimStartTime),
                        0.1);
                }
            }
            // If we have a non-zero average input to frame start time and a PC Latency simulation
            // start time calculate the PC Latency
            auto simStartTime = p->PclSimStartTime != 0 ? p->PclSimStartTime : chain->mLastSimStartTime;
            if (chain->mEmaInput2FrameStartTime != 0.f && simStartTime != 0) {
                metrics.mMsPcLatency = chain->mEmaInput2FrameStartTime +
                    pmSession.TimestampDeltaToMilliSeconds(simStartTime, screenTime);
            }
        } else {
            metrics.mMsDisplayLatency               = 0;
            metrics.mMsDisplayedTime                = 0;
            metrics.mMsUntilDisplayed               = 0;
            metrics.mMsBetweenDisplayChange         = 0;
            metrics.mScreenTime                     = 0;
            metrics.mMsInstrumentedLatency          = 0;
            metrics.mMsInstrumentedRenderLatency    = 0;
            metrics.mMsReadyTimeToDisplayLatency    = 0;
            metrics.mMsPcLatency                    = 0;
            if (p->PclSimStartTime != 0) {
                if (p->PclInputPingTime != 0) {
                    // This frame was dropped but we have valid pc latency input and simulation start
                    // times. Calculate the initial input to sim start time.
                    chain->mAccumulatedInput2FrameStartTime =
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(p->PclInputPingTime, p->PclSimStartTime);
                } else if (chain->mAccumulatedInput2FrameStartTime != 0.f) {
                    // This frame was also dropped and there is no pc latency input time. However, since we have
                    // accumulated time this means we have a pending input that has had multiple dropped frames
                    // and has not yet hit the screen. Calculate the time between the last not displayed sim start and
                    // this sim start and add it to our accumulated total
                    chain->mAccumulatedInput2FrameStartTime +=
                        pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedPclSimStart, p->PclSimStartTime);
                }
                chain->mLastReceivedNotDisplayedPclSimStart = p->PclSimStartTime;
            }
        }

        // The following metrics use both the frame's displayed and origin information.
        metrics.mMsClickToPhotonLatency   = 0;
        metrics.mMsAllInputPhotonLatency  = 0;
        metrics.mMsInstrumentedInputTime  = 0;
        metrics.mMsAnimationError         = std::nullopt;
        metrics.mAnimationTime            = std::nullopt;

        if (displayIndex == appIndex) {
            if (displayed) {
                // For all input device metrics check to see if there were any previous device input times 
                // that were attached to a dropped frame and if so use the last received times for the
                // metric calculations
                auto updatedInputTime = chain->mLastReceivedNotDisplayedAllInputTime == 0 ? 0 :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedAllInputTime, screenTime);
                metrics.mMsAllInputPhotonLatency = p->InputTime == 0 ? updatedInputTime : 
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->InputTime, screenTime);

                updatedInputTime = chain->mLastReceivedNotDisplayedMouseClickTime == 0 ? 0 :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedMouseClickTime, screenTime);
                metrics.mMsClickToPhotonLatency = p->MouseClickTime == 0 ? updatedInputTime :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->MouseClickTime, screenTime);

                updatedInputTime = chain->mLastReceivedNotDisplayedAppProviderInputTime == 0 ? 0 :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(chain->mLastReceivedNotDisplayedAppProviderInputTime, screenTime);
                metrics.mMsInstrumentedInputTime = p->AppInputSample.first == 0 ? updatedInputTime :
                    pmSession.TimestampDeltaToUnsignedMilliSeconds(p->AppInputSample.first, screenTime);

                // Reset all last received device times
                chain->mLastReceivedNotDisplayedAllInputTime = 0;
                chain->mLastReceivedNotDisplayedMouseClickTime = 0;
                chain->mLastReceivedNotDisplayedAppProviderInputTime = 0;

                // Next calculate the animation error and animation time. First calculate the simulation
                // start time. Simulation start can be either an app provided sim start time via the provider or
                // PCL stats or, if not present,the cpu start.
                uint64_t simStartTime = 0;
                if (chain->mAnimationErrorSource == AnimationErrorSource::PCLatency) {
                    // If the pcl latency is the source of the animation error then use the pcl sim start time.
                    simStartTime = p->PclSimStartTime;
                } else if (chain->mAnimationErrorSource == AnimationErrorSource::AppProvider) {
                    // If the app provider is the source of the animation error then use the app sim start time.
                    simStartTime = p->AppSimStartTime;
                } else if (chain->mAnimationErrorSource == AnimationErrorSource::CpuStart) {
                    // If the cpu start time is the source of the animation error then use the cpu start time.
                    simStartTime = metrics.mCPUStart;
                }

                if (chain->mLastDisplayedSimStartTime != 0) {
                    // If the simulation start time is less than the last displayed simulation start time it means
                    // we are transitioning to app provider events.
                    if (simStartTime > chain->mLastDisplayedSimStartTime) {
                        metrics.mMsAnimationError = pmSession.TimestampDeltaToMilliSeconds(screenTime - chain->mLastDisplayedAppScreenTime,
                            simStartTime - chain->mLastDisplayedSimStartTime);
                    }
                }
                // If we have a value in app sim start or pcl sim start and we haven't set the first
                // sim start time then we are transitioning from using cpu start to
                // an application provided timestamp. Set the animation time to zero
                // for the first frame.
                if ((p->AppSimStartTime != 0 || p->PclSimStartTime != 0) && chain->mFirstAppSimStartTime == 0) {
                    metrics.mAnimationTime = 0.;
                }
                else {
                    double animationTime = 0.;
                    CalculateAnimationTime(pmSession, chain->mFirstAppSimStartTime, simStartTime, animationTime);
                    metrics.mAnimationTime = animationTime;
                }
            } else {
                if (p->InputTime != 0) {
                    chain->mLastReceivedNotDisplayedAllInputTime = p->InputTime;
                }
                if (p->MouseClickTime != 0) {
                    chain->mLastReceivedNotDisplayedMouseClickTime = p->MouseClickTime;
                }
                if (p->AppInputSample.first != 0) {
                    chain->mLastReceivedNotDisplayedAppProviderInputTime = p->AppInputSample.first;
                }
            }
        }


        if (p->Displayed.empty()) {
            metrics.mFrameType = FrameType::NotSet;
        } else {
            metrics.mFrameType = p->Displayed[displayIndex].first;
        }

        if (isRecording) {
            UpdateCsv(pmSession, processInfo, *p, metrics);
        }

        if (computeAvg) {
            if (displayIndex == appIndex) {
                UpdateAverage(&chain->mAvgCPUDuration, metrics.mMsCPUBusy + metrics.mMsCPUWait);
                UpdateAverage(&chain->mAvgGPUDuration, msGPUDuration);
            }
            if (displayed) {
                UpdateAverage(&chain->mAvgDisplayLatency, metrics.mMsDisplayLatency);
                UpdateAverage(&chain->mAvgDisplayedTime, metrics.mMsDisplayedTime);
                UpdateAverage(&chain->mAvgMsUntilDisplayed, metrics.mMsUntilDisplayed);
                UpdateAverage(&chain->mAvgMsBetweenDisplayChange, metrics.mMsBetweenDisplayChange);
            }
        }

        displayIndex += 1;
    } while (displayIndex < displayCount);

    UpdateChain(chain, p);
}

static void ReportMetrics(
    PMTraceSession const& pmSession,
    ProcessInfo* processInfo,
    SwapChainData* chain,
    std::shared_ptr<PresentEvent> const& p,
    bool isRecording,
    bool computeAvg)
{
    // Remove Repeated flips if they are in Application->Repeated or Repeated->Application sequences.
    for (size_t i = 0, n = p->Displayed.size(); i + 1 < n; ) {
        if (p->Displayed[i].first == FrameType::Application && p->Displayed[i + 1].first == FrameType::Repeated) {
            p->Displayed.erase(p->Displayed.begin() + i + 1);
            n -= 1;
        } else if (p->Displayed[i].first == FrameType::Repeated && p->Displayed[i + 1].first == FrameType::Application) {
            p->Displayed.erase(p->Displayed.begin() + i);
            n -= 1;
        } else {
            i += 1;
        }
    }

    // For the chain's first present, we just initialize mLastPresent to give a baseline for the
    // first frame.
    if (chain->mLastPresent == nullptr) {
        UpdateChain(chain, p);
        return;
    }

    // If chain->mPendingPresents is non-empty, then it contains a displayed present followed by
    // some number of discarded presents.  If the displayed present has multiple Displayed entries,
    // all but the last have already been handled.
    //
    // If p is displayed, then we can complete all pending presents, and complete any flips in p
    // except for the last one, but then we have to add p to the pending list to wait for the next
    // displayed frame.
    //
    // If p is not displayed, we can process it now unless it is blocked behind an earlier present
    // waiting for the next displayed one, in which case we need to add it to the pending list as
    // well.
    if (p->FinalState == PresentResult::Presented) {
        for (auto const& p2 : chain->mPendingPresents) {
            ReportMetricsHelper(pmSession, processInfo, chain, p2, p.get(), isRecording, computeAvg);
        }
        ReportMetricsHelper(pmSession, processInfo, chain, p, nullptr, isRecording, computeAvg);
        chain->mPendingPresents.clear();
        chain->mPendingPresents.push_back(p);
    } else {
        if (chain->mPendingPresents.empty()) {
            ReportMetricsHelper(pmSession, processInfo, chain, p, nullptr, isRecording, computeAvg);
        } else {
            chain->mPendingPresents.push_back(p);
        }
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
        for (auto ii = processInfo->mSwapChain.begin(), ie = processInfo->mSwapChain.end(); ii != ie; ) {
            auto chain = &ii->second;
            if (chain->mLastPresent->PresentStartTime < minTimestamp) {
                ii = processInfo->mSwapChain.erase(ii);
            } else {
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
    if (chain->mLastPresent == nullptr) {
        UpdateChain(chain, presentEvent);
        return true;
    }

    *outProcessInfo = processInfo;
    *outChain       = chain;
    *outPresentTime = chain->mLastPresent->PresentStartTime;
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

        // If we are recording or presenting metrics to console then update the metrics and pending
        // presents.  Otherwise, just update the latest present details in the chain.
        //
        // If there are more than one pending PresentEvents, then the first one is displayed and the
        // rest aren't.  Otherwise, there will only be one (or zero) pending presents.
        if (isRecording || computeAvg) {
            if (args.mUseV1Metrics) {
                ReportMetrics1(pmSession, processInfo, chain, presentEvent, isRecording, computeAvg);
            } else {
                ReportMetrics(pmSession, processInfo, chain, presentEvent, isRecording, computeAvg);
            }
        } else {
            UpdateChain(chain, presentEvent);
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

