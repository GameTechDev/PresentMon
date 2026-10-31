// Copyright (C) 2017-2024 Intel Corporation
// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMon.hpp"
#include "../IntelPresentMon/CommonUtilities/Math.h"
#include "../IntelPresentMon/CommonUtilities/str/String.h"
#include "../IntelPresentMon/CommonUtilities/Mc/MetricsTypes.h"
#include "../IntelPresentMon/CommonUtilities/Mc/MetricsCalculator.h"

#include <algorithm>
#include <map>
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

// Keep exited lifetimes available for delayed presents. A new start for a reused PID gets its
// own history, and presents select a lifetime by their start timestamp, not their dequeue time.
// Start time zero represents a process discovered before its start event was available.
using ProcessKey = std::pair<uint32_t, uint64_t>;
static std::map<ProcessKey, ProcessInfo> gProcesses;
static uint32_t gTargetProcessCount = 0;

static auto FindProcess(uint32_t processId, uint64_t timestamp)
{
    auto ii = gProcesses.upper_bound({processId, timestamp});
    if (ii != gProcesses.begin()) {
        --ii;
        if (ii->first.first == processId) {
            return ii;
        }
    }
    return gProcesses.end();
}

static bool IsProcessCreationAfter(uint64_t creationTimestamp, uint64_t timestamp)
{
    // The Win7 wall-clock fallback can be coarser than the QPC event timestamp.
    return creationTimestamp > timestamp &&
           creationTimestamp - timestamp > util::GetTimestampFrequencyUint64() / 50;
}

static uint64_t FileTimeValue(FILETIME const& time)
{
    return ((uint64_t)time.dwHighDateTime << 32) | time.dwLowDateTime;
}

static uint64_t FileTimeToQpc(uint64_t timestamp)
{
    if (timestamp == 0) return 0;
    using PreciseTime = void (WINAPI*)(LPFILETIME);
    static auto preciseTime = reinterpret_cast<PreciseTime>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetSystemTimePreciseAsFileTime"));
    FILETIME wallTime{};
    LARGE_INTEGER before{}, after{}, frequency{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&before);
    if (preciseTime != nullptr) {
        preciseTime(&wallTime);
    } else {
        GetSystemTimeAsFileTime(&wallTime);
    }
    QueryPerformanceCounter(&after);
    auto wall = FileTimeValue(wallTime);
    auto now = (uint64_t)(before.QuadPart + (after.QuadPart - before.QuadPart) / 2);
    if (timestamp > wall) return 0;
    auto age = (double)(wall - timestamp) * (double)frequency.QuadPart / 10000000.0;
    return age < (double)now ? now - (uint64_t)age : 0;
}

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

static void ShareProcessCsv(uint32_t processId, ProcessInfo* info)
{
    for (auto ii = gProcesses.lower_bound({processId, 0});
         ii != gProcesses.end() && ii->first.first == processId; ++ii) {
        if (&ii->second != info && ii->second.mModuleName == info->mModuleName) {
            info->mOutputCsv = ii->second.mOutputCsv;
            break;
        }
    }
}

static void SetProcessName(ProcessInfo* info, uint32_t processId, std::wstring name)
{
    if ((name.empty() || name == L"<unknown>") &&
        !info->mModuleName.empty() && info->mModuleName != L"<unknown>") {
        name = info->mModuleName;
    }
    auto target = IsTargetProcess(processId, name);
    if (!info->mTerminationTime.has_value() && target != info->mIsTargetProcess) {
        if (target) {
            ++gTargetProcessCount;
        } else {
            --gTargetProcessCount;
        }
    }
    info->mModuleName = std::move(name);
    info->mIsTargetProcess = target;
    // Resolve the shared path before the first open, including when the name arrives late.
    if (info->mOutputCsv->mPath.empty() && !info->mOutputCsv->mFile) {
        ShareProcessCsv(processId, info);
    }
}

static void HandleTerminatedProcess(
    ProcessInfo* processInfo,
    uint64_t timestamp)
{
    if (processInfo->mTerminationTime.has_value()) {
        return;
    }
    processInfo->mTerminationTime = timestamp;
    if (processInfo->mHandle != NULL) {
        CloseHandle(processInfo->mHandle);
        processInfo->mHandle = NULL;
    }

    if (processInfo->mIsTargetProcess) {
        gTargetProcessCount -= 1;
    }
    // Keep the CSV open and the swap-chain history intact until delayed presents have drained.
}

static std::map<ProcessKey, ProcessInfo>::iterator FindRealtimeProcess(uint32_t processId, uint64_t timestamp);
static void QueryProcessName(uint32_t processId, ProcessInfo* info);

static void ProcessProcessEvent(
    ProcessEvent const& processEvent,
    std::optional<uint64_t> nextLifetimeBoundary = {})
{
    bool targetTerminated = false;
    auto previous = FindProcess(processEvent.ProcessId, processEvent.QpcTime);
    if (processEvent.IsRundown) {
        auto realtime = GetCommandLineArgs().mEtlFileName == nullptr;
        if (realtime) previous = FindRealtimeProcess(processEvent.ProcessId, processEvent.QpcTime);
        // An inventory timestamp describes an existing process, rather than a birth or exit.
        if (previous == gProcesses.end() ||
            (previous->second.mTerminationTime.has_value() &&
             *previous->second.mTerminationTime < processEvent.QpcTime)) {
            ProcessKey key{processEvent.ProcessId, previous == gProcesses.end() ? 0 :
                std::max(*previous->second.mTerminationTime, previous->first.second) + 1};
            previous = gProcesses.emplace(key, ProcessInfo{}).first;
            auto next = gProcesses.upper_bound(key);
            if (next != gProcesses.end() && next->first.first == processEvent.ProcessId) {
                previous->second.mTerminationTime = next->first.second;
            }
            if (realtime) {
                ProcessInfo observed;
                QueryProcessName(processEvent.ProcessId, &observed);
                std::unique_ptr<void, decltype(&CloseHandle)> handle(observed.mHandle, &CloseHandle);
                if (IsProcessCreationAfter(observed.mCreationTimestamp, processEvent.QpcTime)) {
                    // A newer OS owner proves this inventory belongs to an exited lifetime.
                    previous->second.mTerminationTime = std::min(
                        previous->second.mTerminationTime.value_or(observed.mCreationTimestamp),
                        observed.mCreationTimestamp);
                }
            }
        }
        SetProcessName(&previous->second, processEvent.ProcessId, processEvent.ImageFileName);
        return;
    }
    if (processEvent.IsStartEvent) {
        ProcessKey key{processEvent.ProcessId, processEvent.QpcTime};
        auto existing = gProcesses.find(key);
        if (existing != gProcesses.end() && existing->second.mHasStartEvent) return;

        // A start can be dequeued after presents from the same lifetime. Keep that provisional
        // entry's buffered rows and stream, and replace its estimated/unknown start with the event.
        for (auto ii = gProcesses.lower_bound({processEvent.ProcessId, 0});
             ii != gProcesses.end() && ii->first.first == processEvent.ProcessId; ) {
            auto current = ii++;
            auto& info = current->second;
            if (existing != gProcesses.end() && existing != current) continue;
            if (!info.mHasStartEvent && info.mFirstReceivedPresentTime != 0 &&
                processEvent.QpcTime <= info.mFirstReceivedPresentTime &&
                (!info.mTerminationTime.has_value() || processEvent.QpcTime < *info.mTerminationTime) &&
                !IsProcessCreationAfter(info.mCreationTimestamp, processEvent.QpcTime)) {
                if (nextLifetimeBoundary.has_value() && info.mFirstReceivedPresentTime >= *nextLifetimeBoundary) {
                    // This history belongs after the older queued lifetime. Keep it
                    // selectable without applying its eventual start event early.
                    auto lowerBound = *nextLifetimeBoundary;
                    if (lowerBound < info.mFirstReceivedPresentTime) ++lowerBound;
                    ProcessKey provisionalKey{processEvent.ProcessId, lowerBound};
                    if (gProcesses.find(provisionalKey) != gProcesses.end()) {
                        provisionalKey.second = info.mFirstReceivedPresentTime;
                    }
                    if (provisionalKey > current->first && gProcesses.find(provisionalKey) == gProcesses.end()) {
                        if (existing == current) existing = gProcesses.end();
                        if (previous == current) previous = gProcesses.end();
                        auto node = gProcesses.extract(current);
                        node.key() = provisionalKey;
                        gProcesses.insert(std::move(node));
                    }
                    continue;
                }
                SetProcessName(&info, processEvent.ProcessId, processEvent.ImageFileName);
                info.mHasStartEvent = true;
                if (previous != gProcesses.end() && previous != current) {
                    targetTerminated = previous->second.mIsTargetProcess && !previous->second.mTerminationTime.has_value();
                    HandleTerminatedProcess(&previous->second, processEvent.QpcTime);
                }
                if (current->first != key) {
                    auto node = gProcesses.extract(current);
                    node.key() = key;
                    gProcesses.insert(std::move(node));
                }
                if (targetTerminated && GetCommandLineArgs().mTerminateOnProcExit && gTargetProcessCount == 0) {
                    ExitMainThread();
                }
                return;
            }
        }
        previous = FindProcess(processEvent.ProcessId, processEvent.QpcTime);
        auto inserted = gProcesses.emplace(key, ProcessInfo{});
        if (!inserted.second) {
            return;
        }
        auto info = &inserted.first->second;
        info->mHasStartEvent = true;
        auto next = gProcesses.upper_bound(key);
        if (next != gProcesses.end() && next->first.first == processEvent.ProcessId &&
            (!nextLifetimeBoundary.has_value() || next->second.mHasStartEvent)) {
            // A delayed historical start must not add another active target.
            info->mTerminationTime = next->first.second;
        }
        SetProcessName(info, processEvent.ProcessId, processEvent.ImageFileName);

        if (previous != gProcesses.end()) {
            targetTerminated = previous->second.mIsTargetProcess && !previous->second.mTerminationTime.has_value();
            HandleTerminatedProcess(&previous->second, processEvent.QpcTime);
        }
    } else {
        if (previous != gProcesses.end()) {
            targetTerminated = previous->second.mIsTargetProcess && !previous->second.mTerminationTime.has_value();
            HandleTerminatedProcess(&previous->second, processEvent.QpcTime);
        }
    }
    if (targetTerminated && GetCommandLineArgs().mTerminateOnProcExit && gTargetProcessCount == 0) {
        ExitMainThread();
    }
}

static void UpdateProcessEvents(
    PMTraceConsumer* pmConsumer,
    std::vector<ProcessEvent>* processEvents)
{
    std::vector<ProcessEvent> newProcessEvents;
    pmConsumer->DequeueProcessEvents(newProcessEvents);
    bool addedEvents = !newProcessEvents.empty();

    if (!newProcessEvents.empty()) {
        processEvents->insert(processEvents->end(), newProcessEvents.begin(), newProcessEvents.end());
        newProcessEvents.clear();
        newProcessEvents.shrink_to_fit();
    }

    // Check if any realtime processes terminated and create process events for them.
    //
    // Prefer its actual exit timestamp so the stop stays with the old lifetime across PID reuse.
    // If that timestamp is unavailable, fall back to the time at which polling observed the exit.
    for (auto& pair : gProcesses) {
        auto processId = pair.first.first;
        auto processInfo = &pair.second;

        DWORD exitCode = 0;
        if (processInfo->mHandle != NULL && GetExitCodeProcess(processInfo->mHandle, &exitCode) && exitCode != STILL_ACTIVE) {
            uint64_t qpc = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &qpc);

            ProcessEvent e;
            e.ImageFileName = processInfo->mModuleName;
            e.QpcTime       = qpc;
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (GetProcessTimes(processInfo->mHandle, &creation, &exit, &kernel, &user)) {
                auto exitQpc = FileTimeToQpc(FileTimeValue(exit));
                if (exitQpc != 0) e.QpcTime = exitQpc;
            }
            e.ProcessId     = processId;
            e.IsStartEvent  = false;
            processEvents->push_back(e);
            addedEvents = true;

            CloseHandle(processInfo->mHandle);
            processInfo->mHandle = NULL;
        }
    }
    if (addedEvents) {
        // A polled exit uses its actual exit timestamp and can precede events already queued.
        std::sort(processEvents->begin(), processEvents->end(),
            [](ProcessEvent const& a, ProcessEvent const& b) { return a.QpcTime < b.QpcTime; });
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
    // ETL replay can process large present batches with a wide QPC span; end-of-batch
    // pruning is unreliable and breaks steady-presenter debugging. Realtime keeps pruning.
    if (GetCommandLineArgs().mEtlFileName != nullptr) {
        return;
    }

    // sometimes we arrive here after skipping all frame events in the processing loop,
    // in which case we don't have a valid timestamp for the latest frame and should not
    // attempt to do any pruning during this pass
    if (latestTimestamp == 0) {
        return;
    }

    auto inactivityLimit = pmSession.MilliSecondsDeltaToTimestamp(4000.0);
    if (latestTimestamp <= inactivityLimit) {
        return;
    }
    auto minTimestamp = latestTimestamp - inactivityLimit;

    for (auto processIter = gProcesses.begin(); processIter != gProcesses.end(); ) {
        auto processInfo = &processIter->second;

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
                shouldPrune = chain->mLastReceivedPresentTime != 0 &&
                              chain->mLastReceivedPresentTime < minTimestamp;
            }

            if (shouldPrune) {
                ii = processInfo->mSwapChain.erase(ii);
            }
            else {
                ++ii;
            }
        }
        if (processInfo->mTerminationTime.has_value() &&
            *processInfo->mTerminationTime < minTimestamp &&
            processInfo->mLastReceivedPresentTime < minTimestamp) {
            // The shared CSV stream stays open if a newer lifetime still uses the same path.
            processIter = gProcesses.erase(processIter);
        } else {
            ++processIter;
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
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (GetProcessTimes(handle, &creation, &exit, &kernel, &user)) {
                info->mCreationTime = FileTimeValue(creation);
                info->mCreationTimestamp = FileTimeToQpc(info->mCreationTime);
            }
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

static std::map<ProcessKey, ProcessInfo>::iterator FindRealtimeProcess(uint32_t processId, uint64_t timestamp)
{
    auto ii = FindProcess(processId, timestamp);
    auto next = gProcesses.upper_bound({processId, timestamp});
    auto hasNext = next != gProcesses.end() && next->first.first == processId;
    // An actual start event is an exact boundary, rather than a correlated estimate.
    if (hasNext && next->second.mHasStartEvent) return ii;
    DWORD exitCode = 0;
    if (ii != gProcesses.end() && ii->second.mHandle != NULL &&
        GetExitCodeProcess(ii->second.mHandle, &exitCode) && exitCode == STILL_ACTIVE) return ii;

    ProcessInfo observed;
    QueryProcessName(processId, &observed);
    auto closeObserved = [&] {
        if (observed.mHandle != NULL) CloseHandle(observed.mHandle);
    };
    if (observed.mCreationTimestamp == 0 || IsProcessCreationAfter(observed.mCreationTimestamp, timestamp)) {
        closeObserved();
        return ii;
    }
    // An earlier frame from another swap chain can precede the first frame used to key
    // this estimated lifetime. Reuse its identity and buffered history within tolerance.
    if (hasNext && next->second.mCreationTime == observed.mCreationTime) {
        closeObserved();
        return next;
    }

    auto sameIdentity = ii != gProcesses.end() && ii->second.mCreationTime != 0 &&
                        ii->second.mCreationTime == observed.mCreationTime;
    auto replacement = !sameIdentity && (ii == gProcesses.end() ||
        (ii->second.mTerminationTime.has_value() &&
         IsProcessCreationAfter(observed.mCreationTimestamp, *ii->second.mTerminationTime)) ||
        (ii->second.mCreationTime != 0 && ii->second.mCreationTime != observed.mCreationTime) ||
        (ii->second.mHasStartEvent && IsProcessCreationAfter(observed.mCreationTimestamp, ii->first.second)) ||
        (ii->second.mFirstReceivedPresentTime != 0 &&
         IsProcessCreationAfter(observed.mCreationTimestamp, ii->second.mFirstReceivedPresentTime)));
    if (!replacement) {
        if (!ii->second.mTerminationTime.has_value()) {
            if (ii->second.mHandle != NULL) CloseHandle(ii->second.mHandle);
            ii->second.mHandle = observed.mHandle;
            ii->second.mCreationTime = observed.mCreationTime;
            ii->second.mCreationTimestamp = observed.mCreationTimestamp;
            if (!ii->second.mHasStartEvent || ii->second.mModuleName == L"<unknown>") {
                SetProcessName(&ii->second, processId, std::move(observed.mModuleName));
            }
        } else {
            closeObserved();
        }
        return ii;
    }

    ProcessKey key{processId, std::min(observed.mCreationTimestamp, timestamp)};
    auto inserted = gProcesses.emplace(key, std::move(observed));
    if (!inserted.second) {
        closeObserved();
        return inserted.first;
    }
    auto& info = inserted.first->second;
    info.mIsTargetProcess = IsTargetProcess(processId, info.mModuleName);
    if (info.mIsTargetProcess) ++gTargetProcessCount;
    ShareProcessCsv(processId, &info);
    auto targetTerminated = ii != gProcesses.end() && ii->second.mIsTargetProcess &&
                            !ii->second.mTerminationTime.has_value();
    if (ii != gProcesses.end()) HandleTerminatedProcess(&ii->second, key.second);
    if (targetTerminated && GetCommandLineArgs().mTerminateOnProcExit && gTargetProcessCount == 0) {
        ExitMainThread();
    }
    return inserted.first;
}

static bool GetPresentProcessInfo(
    std::shared_ptr<PresentEvent> const& presentEvent,
    ProcessInfo** outProcessInfo,
    SwapChainData** outChain,
    uint64_t* outPresentTime)
{
    ProcessInfo* processInfo;
    auto ii = GetCommandLineArgs().mEtlFileName == nullptr
        ? FindRealtimeProcess(presentEvent->ProcessId, presentEvent->PresentStartTime)
        : FindProcess(presentEvent->ProcessId, presentEvent->PresentStartTime);
    if (ii != gProcesses.end()) {
        processInfo = &ii->second;
    } else {
        ProcessInfo info;
        QueryProcessName(presentEvent->ProcessId, &info);
        auto next = gProcesses.upper_bound({presentEvent->ProcessId, presentEvent->PresentStartTime});
        if (next != gProcesses.end() && next->first.first == presentEvent->ProcessId) {
            info.mTerminationTime = next->first.second;
        }
        if (IsProcessCreationAfter(info.mCreationTimestamp, presentEvent->PresentStartTime)) {
            info.mTerminationTime = std::min(info.mTerminationTime.value_or(info.mCreationTimestamp),
                                             info.mCreationTimestamp);
        }
        if (info.mTerminationTime.has_value()) {
            // A later start bounds this historical lifetime. Preserve its rows without
            // assigning the current process's name or counting it as an active target.
            if (info.mHandle != NULL) CloseHandle(info.mHandle);
            info.mHandle = NULL;
            info.mModuleName = L"<unknown>";
            info.mCreationTime = 0;
            info.mCreationTimestamp = 0;
        }
        info.mIsTargetProcess = IsTargetProcess(presentEvent->ProcessId, info.mModuleName);
        if (info.mIsTargetProcess && !info.mTerminationTime.has_value()) {
            gTargetProcessCount += 1;
        }

        processInfo = &gProcesses.emplace(ProcessKey{presentEvent->ProcessId, 0}, std::move(info)).first->second;
    }

    if (processInfo->mFirstReceivedPresentTime == 0 || presentEvent->PresentStartTime < processInfo->mFirstReceivedPresentTime) {
        processInfo->mFirstReceivedPresentTime = presentEvent->PresentStartTime;
    }
    processInfo->mLastReceivedPresentTime = std::max(processInfo->mLastReceivedPresentTime, presentEvent->PresentStartTime);
    if (!processInfo->mIsTargetProcess) {
        return true;
    }

    *outProcessInfo = processInfo;
    auto chain = &processInfo->mSwapChain[presentEvent->SwapChainAddress];
    chain->mLastReceivedPresentTime = std::max(chain->mLastReceivedPresentTime, presentEvent->PresentStartTime);
    *outChain       = chain;
    *outPresentTime = chain->mUnifiedSwapChain.GetLastPresentQpcOr(
        presentEvent->PresentStartTime);
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
    out.msFlipDelay = pmon::util::metrics::IsMissingFrameMetricValue(m.msFlipDelay) ? 0.0 : m.msFlipDelay;
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
    uint64_t newestPresentStartTime = 0;
    for (auto const& presentEvent : presentEvents) {

        // Ignore failed and lost presents.
        if (presentEvent->IsLost || presentEvent->PresentFailed) {
            continue;
        }

        newestPresentStartTime = std::max(newestPresentStartTime, presentEvent->PresentStartTime);

        // Advance lifecycle events even for filtered processes. Histories are selected by present
        // start time below, so a late present can still use an exited or superseded lifetime.
        if (checkProcessTime) {
            while ((*processEvents)[processEventIndex].QpcTime <= presentEvent->PresentStartTime) {
                auto& processEvent = (*processEvents)[processEventIndex];
                std::optional<uint64_t> nextLifetimeBoundary;
                if (processEvent.IsStartEvent && !processEvent.IsRundown) {
                    auto next = std::find_if(processEvents->begin() + processEventIndex + 1, processEvents->end(),
                        [&](ProcessEvent const& event) {
                            return event.ProcessId == processEvent.ProcessId && !event.IsRundown;
                        });
                    if (next != processEvents->end()) nextLifetimeBoundary = next->QpcTime;
                }
                ProcessProcessEvent(processEvent, nextLifetimeBoundary);
                processEventIndex += 1;
                if (processEventIndex == processEventCount) {
                    checkProcessTime = false;
                    break;
                }
            }
        }

        ProcessInfo* processInfo = nullptr;
        SwapChainData* chain = nullptr;
        if (GetPresentProcessInfo(presentEvent, &processInfo, &chain, &presentTime)) {
            continue;
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

        // Do we need to emit metrics for this present?
        const bool emit = (isRecording || computeAvg);

        using namespace pmon::util::metrics;

        const bool seedPresentOnly = !chain->mUnifiedSwapChain.swapChain.lastPresent.has_value();

        if (args.mUseV1Metrics) {
            FrameData frame = FrameData::CopyFrameData(presentEvent);
            UnifiedSwapChain::SanitizeDisplayedRepeatedPresents(frame);
            auto computed = ComputeMetricsForPresent(qpc, frame, chain->mUnifiedSwapChain.swapChain);

            if (emit && !seedPresentOnly) {
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
            auto processed = chain->mUnifiedSwapChain.ProcessPresent(
                qpc,
                FrameData::CopyFrameData(presentEvent));

            for (auto const& row : processed) {
                auto const& frame = row.present;
                auto const& m = row.computed.metrics;

                if (emit) {
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

    // Prune any SwapChainData that hasn't seen an update for over 4 seconds.
    PruneOldSwapChainData(pmSession, newestPresentStartTime);

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
                    if (!pair.second.mTerminationTime.has_value()) {
                        UpdateConsole(pair.first.first, pair.second);
                    }
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
    gTargetProcessCount = 0;

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
