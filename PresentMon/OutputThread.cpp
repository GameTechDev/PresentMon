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

using ProcessIterator = std::map<ProcessKey, ProcessInfo>::iterator;
static std::set<uint64_t> gActiveTargets;
static unsigned gTransitionDepth = 0;
static bool gTransitionRetiredTarget = false;

static ProcessIterator FindProcessAtOrBefore(uint32_t processId, uint64_t timestamp)
{
    auto ii = gProcesses.upper_bound({processId, timestamp});
    if (ii != gProcesses.begin() && (--ii)->first.first == processId) return ii;
    return gProcesses.end();
}

static ProcessIterator FindProcess(uint32_t processId, uint64_t timestamp)
{
    auto ii = FindProcessAtOrBefore(processId, timestamp);
    if (ii != gProcesses.end() && ii->second.mTerminationTime.has_value() &&
        timestamp > *ii->second.mTerminationTime) return gProcesses.end();
    return ii;
}

// Nested lookups are part of the same event. Exit is checked after every
// membership change in that event, including installation of a replacement.
class ProcessTransition {
public:
    ProcessTransition()
    {
        if (gTransitionDepth++ == 0) gTransitionRetiredTarget = false;
    }
    ~ProcessTransition()
    {
        if (--gTransitionDepth == 0 && gTransitionRetiredTarget && gActiveTargets.empty() &&
            GetCommandLineArgs().mTerminateOnProcExit) ExitMainThread();
    }
};

static void RefreshTargetMembership(ProcessInfo const& info)
{
    if (info.mIsTargetProcess && !info.mIsHistorical) gActiveTargets.insert(info.mLifetimeId);
    else gActiveTargets.erase(info.mLifetimeId);
    gTargetProcessCount = (uint32_t)gActiveTargets.size();
}

static void ClearProcessTracking()
{
    for (auto const& entry : gProcesses) {
        if (entry.second.mHandle != NULL) CloseHandle(entry.second.mHandle);
    }
    gProcesses.clear();
    gActiveTargets.clear();
    gTargetProcessCount = 0;
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

static bool IsKnownProcessName(std::wstring const& name)
{
    return !name.empty() && name != L"<unknown>";
}

static bool CompatibleProcessNames(std::wstring const& a, std::wstring const& b)
{
    return !IsKnownProcessName(a) || !IsKnownProcessName(b) || _wcsicmp(a.c_str(), b.c_str()) == 0;
}

static void SetProcessName(ProcessInfo* info, uint32_t processId, std::wstring name)
{
    if (!IsKnownProcessName(name) && IsKnownProcessName(info->mModuleName)) name = info->mModuleName;
    info->mModuleName = std::move(name);
    info->mIsTargetProcess = IsTargetProcess(processId, info->mModuleName);
    // Only lifetimes that can write a per-process CSV join the catalog.
    if (GetCommandLineArgs().mMultiCsv && info->mIsTargetProcess) BindProcessCsv(info, processId);
    RefreshTargetMembership(*info);
}

static bool IsConfirmedTimeSource(ProcessTimeSource value)
{
    return value == ProcessTimeSource::Etw || value == ProcessTimeSource::Inferred;
}

static void BoundHistoricalProcess(ProcessInfo* info, uint64_t timestamp, ProcessTimeSource source)
{
    if (!info->mTerminationTime.has_value() ||
        (IsConfirmedTimeSource(source) && !IsConfirmedTimeSource(info->mEndSource)) ||
        (IsConfirmedTimeSource(source) == IsConfirmedTimeSource(info->mEndSource) && timestamp < *info->mTerminationTime)) {
        info->mTerminationTime = timestamp;
        info->mEndSource = source;
    }
    info->mIsHistorical = true;
    RefreshTargetMembership(*info);
}

static void HandleTerminatedProcess(ProcessInfo* info, uint64_t timestamp,
    ProcessTimeSource source = ProcessTimeSource::Etw)
{
    if (gActiveTargets.contains(info->mLifetimeId)) gTransitionRetiredTarget = true;
    BoundHistoricalProcess(info, timestamp, source);
    if (info->mHandle != NULL) {
        CloseHandle(info->mHandle);
        info->mHandle = NULL;
    }
}

struct LifetimeInterval {
    std::optional<uint64_t> mEnd;
    bool mHistorical = false;
};

// This is the common boundary view for frames, inventories, and start
// reconciliation. Queued boundaries constrain ownership without retiring it.
static LifetimeInterval GetLifetimeInterval(uint32_t pid, uint64_t start,
    std::optional<uint64_t> queuedEnd = {})
{
    LifetimeInterval interval{queuedEnd};
    for (auto ii = gProcesses.upper_bound({pid, start}); ii != gProcesses.end() && ii->first.first == pid; ++ii) {
        if (ii->second.mHasStartEvent) {
            if (!interval.mEnd.has_value() || ii->first.second < *interval.mEnd) interval.mEnd = ii->first.second;
            interval.mHistorical = true;
            break;
        }
    }
    return interval;
}

static ProcessIterator RekeyProvisional(ProcessIterator current, uint64_t lowerBound)
{
    auto first = current->second.mFirstReceivedPresentTime;
    if (first == 0) return current;
    ProcessKey key{current->first.first, std::min(lowerBound, first)};
    if (key <= current->first) return current;
    if (gProcesses.contains(key)) key.second = first;
    if (key <= current->first || gProcesses.contains(key)) return current;
    auto node = gProcesses.extract(current);
    node.key() = key;
    return gProcesses.insert(std::move(node)).position;
}

static void RetirePrecedingLifetimes(ProcessIterator current, uint64_t timestamp)
{
    for (auto ii = gProcesses.lower_bound({current->first.first, 0}); ii != current; ++ii) {
        auto& info = ii->second;
        if (!info.mIsHistorical && (info.mHasStartEvent || info.mFirstReceivedPresentTime <= timestamp)) {
            HandleTerminatedProcess(&info, timestamp, current->second.mStartSource);
        }
    }
}

static ProcessIterator FindRealtimeProcess(uint32_t processId, uint64_t timestamp);
static void QueryProcessName(uint32_t processId, ProcessInfo* info);

static void ProcessRundownEvent(ProcessEvent const& event, std::optional<uint64_t> queuedEnd)
{
    auto previous = GetCommandLineArgs().mEtlFileName == nullptr
        ? FindRealtimeProcess(event.ProcessId, event.QpcTime)
        : FindProcessAtOrBefore(event.ProcessId, event.QpcTime);
    auto inventoryInterval = GetLifetimeInterval(event.ProcessId, event.QpcTime, queuedEnd);
    if (previous != gProcesses.end() && !previous->second.mHasStartEvent &&
        previous->second.mFirstReceivedPresentTime != 0 && inventoryInterval.mEnd.has_value() &&
        previous->second.mFirstReceivedPresentTime >= *inventoryInterval.mEnd) {
        auto lower = *inventoryInterval.mEnd;
        if (lower < previous->second.mFirstReceivedPresentTime) ++lower;
        RekeyProvisional(previous, lower);
        previous = FindProcess(event.ProcessId, event.QpcTime);
    }
    if (previous == gProcesses.end() ||
        (previous->second.mTerminationTime.has_value() && *previous->second.mTerminationTime < event.QpcTime)) {
        auto earlier = FindProcessAtOrBefore(event.ProcessId, event.QpcTime);
        uint64_t lower = earlier == gProcesses.end() ? 0 :
            earlier->second.mTerminationTime.value_or(earlier->first.second) + 1;
        previous = gProcesses.emplace(ProcessKey{event.ProcessId, lower}, ProcessInfo{}).first;
        auto interval = GetLifetimeInterval(event.ProcessId, event.QpcTime);
        if (interval.mEnd.has_value()) BoundHistoricalProcess(&previous->second, *interval.mEnd, ProcessTimeSource::Inferred);
        if (GetCommandLineArgs().mEtlFileName == nullptr) {
            ProcessInfo observed;
            QueryProcessName(event.ProcessId, &observed);
            std::unique_ptr<void, decltype(&CloseHandle)> handle(observed.mHandle, &CloseHandle);
            if (IsProcessCreationAfter(observed.mCreationTimestamp, event.QpcTime)) {
                BoundHistoricalProcess(&previous->second, observed.mCreationTimestamp, ProcessTimeSource::OsCorrelation);
            }
        }
    }
    SetProcessName(&previous->second, event.ProcessId, event.ImageFileName);
    if (event.CreationTime != 0 && previous->second.mCreationTime == 0) {
        previous->second.mCreationTime = event.CreationTime;
        previous->second.mCreationTimeFromEvent = true;
    }
}

static void ProcessStopEvent(ProcessEvent const& event)
{
    auto previous = FindProcessAtOrBefore(event.ProcessId, event.QpcTime);
    if (event.TrackingId != 0) {
        previous = gProcesses.lower_bound({event.ProcessId, 0});
        while (previous != gProcesses.end() && previous->first.first == event.ProcessId &&
            previous->second.mLifetimeId != event.TrackingId) ++previous;
        if (previous == gProcesses.end() || previous->first.first != event.ProcessId) return;
    } else if (previous != gProcesses.end() && !previous->second.mHasStartEvent &&
        previous->second.mFirstReceivedPresentTime > event.QpcTime) {
        RekeyProvisional(previous, event.QpcTime + 1);
        previous = FindProcessAtOrBefore(event.ProcessId, event.QpcTime);
    }
    if (previous != gProcesses.end()) HandleTerminatedProcess(&previous->second, event.QpcTime, event.TimeSource);
}

static bool CanReconcileProvisional(
    ProcessInfo const& info,
    ProcessEvent const& event,
    LifetimeInterval const& interval)
{
    if (info.mFirstReceivedPresentTime < event.QpcTime) return false;
    if (interval.mEnd.has_value() && info.mLastReceivedPresentTime >= *interval.mEnd) return false;
    if (info.mTerminationTime.has_value() && event.QpcTime >= *info.mTerminationTime) return false;
    if (event.CreationTime != 0 && info.mCreationTime != 0 && event.CreationTime != info.mCreationTime) return false;
    if (IsProcessCreationAfter(info.mCreationTimestamp, event.QpcTime)) return false;
    return CompatibleProcessNames(info.mModuleName, event.ImageFileName);
}

static void ProcessStartEvent(ProcessEvent const& event, std::optional<uint64_t> queuedEnd)
{
    ProcessKey key{event.ProcessId, event.QpcTime};
    if (event.CreationTime != 0) {
        for (auto ii = gProcesses.lower_bound({event.ProcessId, 0}); ii != gProcesses.end() && ii->first.first == event.ProcessId; ++ii) {
            auto& info = ii->second;
            if (!info.mHasStartEvent || !info.mCreationTimeFromEvent || info.mCreationTime != event.CreationTime ||
                !CompatibleProcessNames(info.mModuleName, event.ImageFileName) ||
                (info.mTerminationTime.has_value() && event.QpcTime > *info.mTerminationTime)) continue;
            auto lower = std::min(key.second, ii->first.second);
            auto upper = std::max(key.second, ii->first.second);
            auto between = gProcesses.upper_bound({event.ProcessId, lower});
            if (between != gProcesses.end() && between->first.first == event.ProcessId && between != ii &&
                between->first.second < upper) continue;
            SetProcessName(&info, event.ProcessId, event.ImageFileName);
            if (key < ii->first && !gProcesses.contains(key)) {
                auto node = gProcesses.extract(ii);
                node.key() = key;
                gProcesses.insert(std::move(node));
            }
            return;
        }
    }
    auto existing = gProcesses.find(key);
    if (existing != gProcesses.end() && existing->second.mHasStartEvent) {
        if (!IsKnownProcessName(existing->second.mModuleName)) SetProcessName(&existing->second, event.ProcessId, event.ImageFileName);
        return;
    }
    auto interval = GetLifetimeInterval(event.ProcessId, event.QpcTime, queuedEnd);
    for (auto ii = gProcesses.lower_bound({event.ProcessId, 0}); ii != gProcesses.end() && ii->first.first == event.ProcessId;) {
        auto current = ii++;
        auto& info = current->second;
        if (info.mHasStartEvent || info.mFirstReceivedPresentTime == 0) continue;
        if (interval.mEnd.has_value() && info.mFirstReceivedPresentTime >= *interval.mEnd) {
            auto lower = *interval.mEnd;
            if (lower < info.mFirstReceivedPresentTime) ++lower;
            if (existing == current) existing = gProcesses.end();
            RekeyProvisional(current, lower);
            continue;
        }
        if ((existing != gProcesses.end() && existing != current) ||
            !CanReconcileProvisional(info, event, interval)) continue;
        SetProcessName(&info, event.ProcessId, event.ImageFileName);
        info.mHasStartEvent = true;
        info.mStartSource = event.TimeSource;
        if (event.CreationTime != 0) {
            info.mCreationTime = event.CreationTime;
            info.mCreationTimeFromEvent = true;
        }
        if (interval.mHistorical && interval.mEnd.has_value()) BoundHistoricalProcess(&info, *interval.mEnd, ProcessTimeSource::Inferred);
        if (current->first != key) {
            auto node = gProcesses.extract(current);
            node.key() = key;
            current = gProcesses.insert(std::move(node)).position;
        }
        RetirePrecedingLifetimes(current, event.QpcTime);
        return;
    }
    auto previous = FindProcessAtOrBefore(event.ProcessId, event.QpcTime);
    if (previous != gProcesses.end() && !previous->second.mHasStartEvent &&
        (!previous->second.mTerminationTime.has_value() || *previous->second.mTerminationTime > event.QpcTime) &&
        previous->second.mFirstReceivedPresentTime > event.QpcTime) {
        RekeyProvisional(previous, previous->second.mFirstReceivedPresentTime);
    }
    auto info = gProcesses.emplace(key, ProcessInfo{}).first;
    info->second.mHasStartEvent = true;
    info->second.mStartSource = event.TimeSource;
    info->second.mCreationTime = event.CreationTime;
    info->second.mCreationTimeFromEvent = event.CreationTime != 0;
    if (interval.mHistorical && interval.mEnd.has_value()) BoundHistoricalProcess(&info->second, *interval.mEnd, ProcessTimeSource::Inferred);
    SetProcessName(&info->second, event.ProcessId, event.ImageFileName);
    RetirePrecedingLifetimes(info, event.QpcTime);
}

static void ProcessProcessEvent(ProcessEvent const& event, std::optional<uint64_t> queuedEnd = {})
{
    ProcessTransition transition;
    if (event.IsRundown) {
        ProcessRundownEvent(event, queuedEnd);
    } else if (!event.IsStartEvent) {
        ProcessStopEvent(event);
    } else {
        ProcessStartEvent(event, queuedEnd);
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
            e.TimeSource = ProcessTimeSource::PollObservation;
            e.TrackingId = processInfo->mLifetimeId;
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (GetProcessTimes(processInfo->mHandle, &creation, &exit, &kernel, &user)) {
                auto exitQpc = FileTimeToQpc(FileTimeValue(exit));
                if (exitQpc != 0) {
                    e.QpcTime = exitQpc;
                    e.TimeSource = ProcessTimeSource::OsCorrelation;
                }
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
            RefreshTargetMembership(*processInfo);
            if (processInfo->mHandle != NULL) CloseHandle(processInfo->mHandle);
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
            DWORD exitCode = STILL_ACTIVE;
            if (GetExitCodeProcess(handle, &exitCode) && exitCode != STILL_ACTIVE) {
                info->mIsHistorical = true;
                info->mTerminationTime = FileTimeToQpc(FileTimeValue(exit));
                if (*info->mTerminationTime == 0) info->mTerminationTime = (uint64_t)util::GetCurrentTimestamp();
                info->mEndSource = ProcessTimeSource::OsCorrelation;
                CloseHandle(handle);
                handle = NULL;
            }
        }
    }

    info->mModuleName = processName;
    info->mHandle     = handle;
}

static ProcessIterator FindRealtimeProcess(uint32_t processId, uint64_t timestamp)
{
    ProcessTransition transition;
    auto ii = FindProcess(processId, timestamp);
    auto earlier = FindProcessAtOrBefore(processId, timestamp);
    auto interval = GetLifetimeInterval(processId, timestamp);
    // A confirmed exit or a later applied start wins over creation-time tolerance.
    if (ii != gProcesses.end() && ii->second.mTerminationTime.has_value() &&
        IsConfirmedTimeSource(ii->second.mEndSource)) return ii;
    if (interval.mHistorical) return ii;
    DWORD exitCode = 0;
    if (ii != gProcesses.end() && ii->second.mHandle != NULL &&
        GetExitCodeProcess(ii->second.mHandle, &exitCode) && exitCode == STILL_ACTIVE) return ii;

    ProcessInfo observed;
    QueryProcessName(processId, &observed);
    std::unique_ptr<void, decltype(&CloseHandle)> observedHandle(observed.mHandle, &CloseHandle);
    if (observed.mCreationTimestamp == 0 || IsProcessCreationAfter(observed.mCreationTimestamp, timestamp)) return ii;

    auto next = gProcesses.upper_bound({processId, timestamp});
    if (next != gProcesses.end() && next->first.first == processId && !next->second.mHasStartEvent &&
        !next->second.mIsHistorical && next->second.mCreationTime == observed.mCreationTime) return next;

    auto sameIdentity = ii != gProcesses.end() && ii->second.mCreationTime != 0 &&
        ii->second.mCreationTime == observed.mCreationTime;
    auto replacement = ii == gProcesses.end() || (!sameIdentity &&
        (ii->second.mIsHistorical || ii->second.mCreationTime != 0 ||
         !CompatibleProcessNames(ii->second.mModuleName, observed.mModuleName) ||
         (ii->second.mHasStartEvent && IsProcessCreationAfter(observed.mCreationTimestamp, ii->first.second))));
    if (!replacement) {
        auto& info = ii->second;
        if (!info.mIsHistorical) {
            if (info.mHandle != NULL) CloseHandle(info.mHandle);
            info.mHandle = observedHandle.release();
            info.mCreationTime = observed.mCreationTime;
            info.mCreationTimestamp = observed.mCreationTimestamp;
            SetProcessName(&info, processId, observed.mModuleName);
            if (observed.mIsHistorical) {
                HandleTerminatedProcess(&info, *observed.mTerminationTime, observed.mEndSource);
            }
        }
        return ii;
    }

    auto lower = std::min(observed.mCreationTimestamp, timestamp);
    if (earlier != gProcesses.end() && earlier->second.mTerminationTime.has_value()) {
        lower = std::max(lower, *earlier->second.mTerminationTime + 1);
        if (lower > timestamp) return ii;
    }
    ProcessKey key{processId, lower};
    auto inserted = gProcesses.try_emplace(key, std::move(observed));
    if (!inserted.second) return inserted.first;
    auto& info = inserted.first->second;
    info.mHandle = observedHandle.release();
    info.mStartSource = ProcessTimeSource::OsCorrelation;
    SetProcessName(&info, processId, info.mModuleName);
    if (earlier != gProcesses.end() && &earlier->second != &info && !earlier->second.mIsHistorical) {
        HandleTerminatedProcess(&earlier->second, lower, ProcessTimeSource::OsCorrelation);
    }
    return inserted.first;
}

static bool GetPresentProcessInfo(
    std::shared_ptr<PresentEvent> const& presentEvent,
    ProcessInfo** outProcessInfo,
    SwapChainData** outChain,
    uint64_t* outPresentTime)
{
    ProcessTransition transition;
    auto pid = presentEvent->ProcessId;
    auto timestamp = presentEvent->PresentStartTime;
    auto ii = GetCommandLineArgs().mEtlFileName == nullptr
        ? FindRealtimeProcess(pid, timestamp) : FindProcess(pid, timestamp);
    if (ii == gProcesses.end()) {
        ProcessInfo info;
        QueryProcessName(pid, &info);
        auto interval = GetLifetimeInterval(pid, timestamp);
        if (interval.mEnd.has_value()) BoundHistoricalProcess(&info, *interval.mEnd, ProcessTimeSource::Inferred);
        if (IsProcessCreationAfter(info.mCreationTimestamp, timestamp)) {
            BoundHistoricalProcess(&info, info.mCreationTimestamp, ProcessTimeSource::OsCorrelation);
        }
        if (info.mIsHistorical) {
            if (info.mHandle != NULL) CloseHandle(info.mHandle);
            info.mHandle = NULL;
            info.mModuleName = L"<unknown>";
            info.mCreationTime = 0;
            info.mCreationTimestamp = 0;
        }
        auto earlier = FindProcessAtOrBefore(pid, timestamp);
        auto lower = earlier == gProcesses.end() ? 0 :
            earlier->second.mTerminationTime.value_or(earlier->first.second) + 1;
        ii = gProcesses.emplace(ProcessKey{pid, lower}, std::move(info)).first;
        SetProcessName(&ii->second, pid, ii->second.mModuleName);
    }
    auto& info = ii->second;
    if (info.mFirstReceivedPresentTime == 0 || timestamp < info.mFirstReceivedPresentTime) info.mFirstReceivedPresentTime = timestamp;
    info.mLastReceivedPresentTime = std::max(info.mLastReceivedPresentTime, timestamp);
    if (!info.mIsTargetProcess) return true;
    *outProcessInfo = &info;
    auto chain = &info.mSwapChain[presentEvent->SwapChainAddress];
    chain->mLastReceivedPresentTime = std::max(chain->mLastReceivedPresentTime, timestamp);
    *outChain = chain;
    *outPresentTime = chain->mUnifiedSwapChain.GetLastPresentQpcOr(timestamp);
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

// Realtime ETW flushes quiet per-CPU buffers about once a second, so events with earlier
// timestamps can still be in flight. Idle lifecycle progress stays behind that window.
static std::optional<uint64_t> RealtimeIdleWatermark(PMTraceSession const& pmSession, uint64_t now)
{
    auto lag = pmSession.MilliSecondsDeltaToTimestamp(2000.0);
    if (now <= lag) return {};
    return now - lag;
}

struct ProcessEventPlan {
    std::vector<std::optional<uint64_t>> mNextBoundary;
    std::vector<std::vector<size_t>> mRundownsAfter;
    std::vector<bool> mProcessed;
    size_t mBoundaryVisits = 0;
};

static ProcessEventPlan PrepareProcessEvents(std::vector<ProcessEvent> const& events)
{
    ProcessEventPlan plan;
    plan.mNextBoundary.resize(events.size());
    plan.mRundownsAfter.resize(events.size() + 1);
    plan.mProcessed.resize(events.size());
    std::unordered_map<uint32_t, uint64_t> next;
    for (size_t i = events.size(); i-- > 0;) {
        ++plan.mBoundaryVisits;
        auto const& event = events[i];
        auto boundary = next.find(event.ProcessId);
        if (boundary != next.end()) plan.mNextBoundary[i] = boundary->second;
        if (!event.IsRundown) next[event.ProcessId] = event.QpcTime;
    }
    std::unordered_map<uint32_t, size_t> preceding;
    for (size_t i = 0; i < events.size(); ++i) {
        ++plan.mBoundaryVisits;
        auto const& event = events[i];
        if (event.IsRundown) {
            auto boundary = preceding.find(event.ProcessId);
            plan.mRundownsAfter[boundary == preceding.end() ? events.size() : boundary->second].push_back(i);
        } else preceding[event.ProcessId] = i;
    }
    return plan;
}

static void ApplyReadyRundowns(std::vector<ProcessEvent> const& events, ProcessEventPlan* plan, size_t preceding)
{
    for (auto index : plan->mRundownsAfter[preceding]) {
        ProcessProcessEvent(events[index], plan->mNextBoundary[index]);
        plan->mProcessed[index] = true;
    }
}

static void ProcessEvents(
    PMTraceSession const& pmSession,
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    std::vector<ProcessEvent>* processEvents,
    std::vector<uint64_t>* recordingToggleHistory,
    bool currentRecordingState,
    std::optional<uint64_t> idleProgress = {})
{
    auto const& args = GetCommandLineArgs();
    auto computeAvg = args.mConsoleOutput == ConsoleOutput::Statistics;

    // Determine the recording state and when the next toggle is.
    size_t recordingToggleIndex = 0;
    size_t recordingToggleCount = recordingToggleHistory->size();
    bool checkRecordingToggle   = recordingToggleCount > 0;
    bool isRecording            = recordingToggleCount & 1 ? !currentRecordingState : currentRecordingState;

    auto processPlan = PrepareProcessEvents(*processEvents);
    size_t processEventIndex = 0;
    ApplyReadyRundowns(*processEvents, &processPlan, processEvents->size());
    auto advanceLifecycle = [&](uint64_t timestamp) {
        while (processEventIndex < processEvents->size()) {
            auto index = processEventIndex;
            if (processPlan.mProcessed[index]) {
                ++processEventIndex;
                continue;
            }
            auto const& event = (*processEvents)[index];
            if (event.QpcTime > timestamp) break;
            ProcessProcessEvent(event, processPlan.mNextBoundary[index]);
            processPlan.mProcessed[index] = true;
            ++processEventIndex;
            ApplyReadyRundowns(*processEvents, &processPlan, index);
        }
    };

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
        advanceLifecycle(presentEvent->PresentStartTime);

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

    if (idleProgress.has_value()) advanceLifecycle(*idleProgress);

    // Prune any SwapChainData that hasn't seen an update for over 4 seconds.
    PruneOldSwapChainData(pmSession, newestPresentStartTime);

    // Erase any recording toggles and process events that were processed.
    if (recordingToggleIndex > 0) {
        recordingToggleHistory->erase(recordingToggleHistory->begin(), recordingToggleHistory->begin() + recordingToggleIndex);
    }
    size_t pending = 0;
    for (size_t index = 0; index < processEvents->size(); ++index) {
        if (!processPlan.mProcessed[index]) {
            if (pending != index) (*processEvents)[pending] = std::move((*processEvents)[index]);
            ++pending;
        }
    }
    processEvents->resize(pending);
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
        } else if (args.mEtlFileName == nullptr && !processEvents.empty()) {
            // Realtime process exits must advance even when no process presents again.
            ProcessEvents(*pmSession, presentEvents, &processEvents, &recordingToggleHistory,
                currentRecordingState, RealtimeIdleWatermark(*pmSession, (uint64_t)util::GetCurrentTimestamp()));
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
                    if (gActiveTargets.contains(pair.second.mLifetimeId)) {
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

    CloseGlobalCsv();
    ClearProcessTracking();

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
