// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Logging.h"
#include "RealtimePresentMonSession.h"
#include "CliOptions.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/Event.h"
#include "../CommonUtilities/Qpc.h"
#include "../CommonUtilities/Exception.h"
#include <shlwapi.h>

using namespace pmon;
using namespace svc;
using namespace std::literals;

static const std::wstring kRealTimeSessionName = L"PMService";

RealtimePresentMonSession::RealtimePresentMonSession()
    : target_process_count_(0),
    quit_output_thread_(false)
{
    pm_session_name_.clear();
    processes_.clear();
    pm_consumer_.reset();
}

bool RealtimePresentMonSession::IsTraceSessionActive() {
    return (pm_consumer_ != nullptr);
}

PM_STATUS RealtimePresentMonSession::StartStreaming(uint32_t client_process_id,
    uint32_t target_process_id,
    std::string& nsmFileName) {

    // Check to see if the target process is valid
    HANDLE target_process_handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION, FALSE, target_process_id);
    if (target_process_handle == nullptr) {
        return PM_STATUS::PM_STATUS_INVALID_PID;
    }
    CloseHandle(target_process_handle);

    PM_STATUS status = streamer_.StartStreaming(client_process_id,
        target_process_id, nsmFileName, false, false, false, false, false);
    if (status != PM_STATUS::PM_STATUS_SUCCESS) {
        return status;
    }
    else {
        // Add the client process id to be monitored
        GetProcessInfo(client_process_id);
        auto status = StartTraceSession();
        if (status == PM_STATUS_FAILURE) {
            // Unable to start a trace session. Destroy the NSM and
            // return status
            streamer_.StopStreaming(target_process_id);
            return status;
        }
        // Only signal the streaming started event when we have
        // exactly one stream after returning from StartStreaming.
        if ((streamer_.NumActiveStreams() == 1) && evtStreamingStarted_) {
            evtStreamingStarted_.Set();
        }
        // Also monitor the target process id
        GetProcessInfo(target_process_id);

        return PM_STATUS::PM_STATUS_SUCCESS;
    }
}

void RealtimePresentMonSession::StopStreaming(uint32_t client_process_id,
    uint32_t target_process_id) {
    streamer_.StopStreaming(client_process_id, target_process_id);
    if ((streamer_.NumActiveStreams() == 0) && evtStreamingStarted_) {
        evtStreamingStarted_.Reset();
        StopTraceSession();
    }
}

bool RealtimePresentMonSession::CheckTraceSessions(bool forceTerminate) {
    if (((GetActiveStreams() == 0) && (IsTraceSessionActive() == true)) ||
        forceTerminate) {
        StopTraceSession();
        return true;
    }
    return false;
}

HANDLE RealtimePresentMonSession::GetStreamingStartHandle() {
    return evtStreamingStarted_;
}

void RealtimePresentMonSession::FlushEvents()
{
    struct TraceProperties : public EVENT_TRACE_PROPERTIES {
        wchar_t mSessionName[MAX_PATH];
    } props{};
    props.Wnode.BufferSize = (ULONG)sizeof(TraceProperties);
    props.LoggerNameOffset = offsetof(TraceProperties, mSessionName);
    if (ControlTraceW(trace_session_.mSessionHandle, nullptr, &props, EVENT_TRACE_CONTROL_FLUSH)) {
        pmlog_warn("Failed manual flush of ETW event buffer").hr();
    }
}

PM_STATUS RealtimePresentMonSession::StartTraceSession() {
    std::lock_guard<std::mutex> lock(session_mutex_);

    if (pm_consumer_) {
        return PM_STATUS::PM_STATUS_SERVICE_ERROR;
    }

    auto expectFilteredEvents = IsWindows8Point1OrGreater();
    auto filterProcessIds =
        false;  // Does not support process names at this point

    // Create consumers
    try {
        pm_consumer_ = std::make_unique<PMTraceConsumer>();
    }
    catch (...) {
        return PM_STATUS::PM_STATUS_FAILURE;
    }

    pm_consumer_->mFilteredEvents = expectFilteredEvents;
    pm_consumer_->mFilteredProcessIds = filterProcessIds;
    pm_consumer_->mTrackDisplay = true;
    pm_consumer_->mTrackGPU = true;
    pm_consumer_->mTrackGPUVideo = false;
    pm_consumer_->mTrackInput = true;
    pm_consumer_->mTrackFrameType = true;
    pm_consumer_->mTrackAppTiming = true;
    pm_consumer_->mTrackPcLatency = true;

    auto& opt = clio::Options::Get();
    if (opt.etwSessionName.AsOptional().has_value()) {
        pm_session_name_ =
            util::str::ToWide(opt.etwSessionName.AsOptional().value());
    }
    else {
        pm_session_name_ = kRealTimeSessionName;
    }

    const wchar_t* etl_file_name = nullptr;
    // Start the session. If a session with this name is already running, we stop
    // it and start a new session. This is useful if a previous process failed to
    // properly shut down the session for some reason.
    trace_session_.mPMConsumer = pm_consumer_.get();
    auto status = trace_session_.Start(etl_file_name, pm_session_name_.c_str());

    if (status == ERROR_ALREADY_EXISTS) {
        status = StopNamedTraceSession(pm_session_name_.c_str());
        if (status == ERROR_SUCCESS) {
            status = trace_session_.Start(etl_file_name, pm_session_name_.c_str());
        }
    }

    // Report error if we failed to start a new session
    if (status != ERROR_SUCCESS) {
        pm_consumer_.reset();
        switch (status) {
        case ERROR_ALREADY_EXISTS:
            return PM_STATUS::PM_STATUS_SERVICE_ERROR;
        case ERROR_FILE_NOT_FOUND:
            // We should NEVER receive this return value in
            // the realtime session
            assert(status != ERROR_FILE_NOT_FOUND);
            return PM_STATUS::PM_STATUS_INVALID_ETL_FILE;
        default:
            return PM_STATUS::PM_STATUS_FAILURE;
        }
    }

    // Set deferral time limit to 2 seconds
    if (trace_session_.mPMConsumer->mDeferralTimeLimit == 0) {
        trace_session_.mPMConsumer->mDeferralTimeLimit = trace_session_.mTimestampFrequency.QuadPart * 2;
    }

    // Start the consumer and output threads
    StartConsumerThread(trace_session_.mTraceHandle);
    StartOutputThread();
    return PM_STATUS::PM_STATUS_SUCCESS;
}

void RealtimePresentMonSession::StopTraceSession() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    // Stop the trace session.
    trace_session_.Stop();

    // Wait for the consumer and output threads to end (which are using the
    // consumers).
    WaitForConsumerThreadToExit();
    StopOutputThread();

    // Stop all streams
    streamer_.StopAllStreams();
    if (evtStreamingStarted_) {
        evtStreamingStarted_.Reset();
    }

    if (pm_consumer_) {
        pm_consumer_.reset();
    }
}

void RealtimePresentMonSession::StartConsumerThread(TRACEHANDLE traceHandle) {
    consumer_thread_ = std::thread(&RealtimePresentMonSession::Consume, this, traceHandle);
}

void RealtimePresentMonSession::WaitForConsumerThreadToExit() {
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

void RealtimePresentMonSession::DequeueAnalyzedInfo(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents) {
    pm_consumer_->DequeueProcessEvents(*processEvents);
    pm_consumer_->DequeuePresentEvents(*presentEvents);
}

void RealtimePresentMonSession::AddPresents(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex, bool recording, bool checkStopQpc,
    uint64_t stopQpc, bool* hitStopQpc) {
    auto i = *presentEventIndex;

    if (trace_session_.mStartTimestamp.QuadPart != 0) {
        streamer_.SetStartQpc(trace_session_.mStartTimestamp.QuadPart);
    }

    // logging of ETW latency
    if constexpr (svc::v::etwq) {
        pmlog_verb(svc::v::etwq)(std::format("Processing [{}] frames", presentEvents.size()));
        for (auto& p : presentEvents) {
            if (p->FinalState == PresentResult::Presented) {
                const auto per = util::GetTimestampPeriodSeconds();
                const auto now = util::GetCurrentTimestamp();
                // TODO: Presents can now have multiple displayed frames if we are tracking
                // frame types. For now take the first displayed frame for logging stats
                const auto lag = util::TimestampDeltaToSeconds(p->Displayed[0].second, now, per);
                pmlog_verb(svc::v::etwq)(std::format("Frame [{}] lag: {} ms", p->FrameId, lag * 1000.));
            }
        }
    }

    for (auto n = presentEvents.size(); i < n; ++i) {
        auto& presentEvent = presentEvents[i];
        assert(presentEvent->IsCompleted);

        // Ignore failed and lost presents.
        if (presentEvent->IsLost || presentEvent->PresentFailed) {
            continue;
        }

        // Stop processing events if we hit the next stop time.
        if (checkStopQpc && presentEvent->PresentStartTime >= stopQpc) {
            *hitStopQpc = true;
            break;
        }

        // Look up the swapchain this present belongs to.
        auto processInfo = GetProcessInfo(presentEvent->ProcessId);
        if (!processInfo->mTargetProcess) {
            continue;
        }

        PresentMonPowerTelemetryInfo power_telemetry = {};
        std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
            gpu_telemetry_cap_bits = {};
        if (telemetry_container_) {
            auto current_adapters = telemetry_container_->GetPowerTelemetryAdapters();
            if (current_adapters.size() != 0 &&
                current_telemetry_adapter_id_ < current_adapters.size()) {
                auto current_telemetry_adapter =
                    current_adapters.at(current_telemetry_adapter_id_).get();
                if (auto data = current_telemetry_adapter->GetClosest(
                    presentEvent->PresentStartTime)) {
                    power_telemetry = *data;
                }
                gpu_telemetry_cap_bits = current_telemetry_adapter
                    ->GetPowerTelemetryCapBits();
            }
        }

        CpuTelemetryInfo cpu_telemetry = {};
        std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
            cpu_telemetry_cap_bits = {};
        if (cpu_) {
            if (auto data = cpu_->GetClosest(presentEvent->PresentStartTime)) {
                cpu_telemetry = *data;
            }
            cpu_telemetry_cap_bits = cpu_->GetCpuTelemetryCapBits();
        }

        auto result = processInfo->mSwapChain.emplace(
            presentEvent->SwapChainAddress, SwapChainData());
        auto chain = &result.first->second;
        if (result.second) {
            chain->mPresentHistoryCount = 0;
            chain->mLastPresentQPC = 0;
            chain->mLastDisplayedPresentQPC = 0;
        }

        // Remove Repeated flips if they are in Application->Repeated or Repeated->Application sequences.
        for (size_t i = 0, n = presentEvent->Displayed.size(); i + 1 < n; ) {
            if (presentEvent->Displayed[i].first == FrameType::Application &&
                presentEvent->Displayed[i + 1].first == FrameType::Repeated) {
                presentEvent->Displayed.erase(presentEvent->Displayed.begin() + i + 1);
                n -= 1;
            }
            else if (presentEvent->Displayed[i].first == FrameType::Repeated &&
                presentEvent->Displayed[i + 1].first == FrameType::Application) {
                presentEvent->Displayed.erase(presentEvent->Displayed.begin() + i);
                n -= 1;
            }
            else {
                i += 1;
            }
        }

        streamer_.ProcessPresentEvent(
            presentEvent.get(), &power_telemetry, &cpu_telemetry,
            chain->mLastPresentQPC, chain->mLastDisplayedPresentQPC,
            processInfo->mModuleName, gpu_telemetry_cap_bits,
            cpu_telemetry_cap_bits);

        chain->mLastPresentQPC = presentEvent->PresentStartTime;
        if (presentEvent->FinalState == PresentResult::Presented) {
            chain->mLastDisplayedPresentQPC = presentEvent->Displayed.empty() ? 0 : presentEvent->Displayed[0].second;
        }
        else if (chain->mLastDisplayedPresentQPC == chain->mLastPresentQPC) {
            chain->mLastDisplayedPresentQPC = 0;
        }

        chain->mPresentHistoryCount += 1;
    }

    *presentEventIndex = i;
}

void RealtimePresentMonSession::ProcessEvents(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    bool eventProcessingDone = false;

    // Copy any analyzed information from ConsumerThread and early-out if there
    // isn't any.
    DequeueAnalyzedInfo(processEvents, presentEvents);
    if (processEvents->empty() && presentEvents->empty()) {
        return;
    }

    // Handle Process events; created processes are added to gProcesses and
    // terminated processes are added to terminatedProcesses.
    //
    // Handling of terminated processes need to be deferred until we observe a
    // present event that started after the termination time.  This is because
    // while a present must start before termination, it can complete after
    // termination.
    UpdateProcesses(*processEvents, terminatedProcesses);

    size_t presentEventIndex = 0;
    size_t terminatedProcessIndex = 0;

    // Iterate through the terminated process history. If we hit a present that
    // started after the termination, we can handle the process termination and
    // continue. Otherwise, we're done handling all the presents and any
    // outstanding terminations will have to wait for the next batch of events.
    for (; terminatedProcessIndex < terminatedProcesses->size();
        ++terminatedProcessIndex) {
        auto const& pair = (*terminatedProcesses)[terminatedProcessIndex];
        auto terminatedProcessId = pair.first;
        auto terminatedProcessQpc = pair.second;

        auto hitTerminatedProcess = false;
        AddPresents(*presentEvents, &presentEventIndex, true, true,
            terminatedProcessQpc, &hitTerminatedProcess);
        if (!hitTerminatedProcess) {
            eventProcessingDone = true;
            break;
        }
        HandleTerminatedProcess(terminatedProcessId);
    }

    if (!eventProcessingDone) {
        // Process all present events. The PresentMon service is always recording
        // and in this instance we are not concerned with checking for a stop QPC.
        auto hitStopQPC = false;
        AddPresents(*presentEvents, &presentEventIndex, true, false, 0,
            &hitStopQPC);
    }

    // Clear events processed.
    processEvents->clear();
    presentEvents->clear();

    // Finished processing all events.  Erase the terminated processes that we
    // handled now.
    if (terminatedProcessIndex > 0) {
        terminatedProcesses->erase(
            terminatedProcesses->begin(),
            terminatedProcesses->begin() + terminatedProcessIndex);
    }

    return;
}

void RealtimePresentMonSession::Consume(TRACEHANDLE traceHandle) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // You must call OpenTrace() prior to calling this function
    //
    // ProcessTrace() blocks the calling thread until it
    //     1) delivers all events in a trace log file, or
    //     2) the BufferCallback function returns FALSE, or
    //     3) you call CloseTrace(), or
    //     4) the controller stops the trace session.
    //
    // There may be a several second delay before the function returns.
    //
    // ProcessTrace() is supposed to return ERROR_CANCELLED if BufferCallback
    // (EtwThreadShouldQuit) returns FALSE; and ERROR_SUCCESS if the trace
    // completes (parse the entire ETL, fills the maximum file size, or is
    // explicitly closed).
    //
    // However, it seems to always return ERROR_SUCCESS.

    ProcessTrace(&traceHandle, 1, NULL, NULL);
}

void RealtimePresentMonSession::Output() {
    try {
        // Structures to track processes and statistics from recorded events.
        std::vector<ProcessEvent> processEvents;
        std::vector<std::shared_ptr<PresentEvent>> presentEvents;
        std::vector<std::pair<uint32_t, uint64_t>> terminatedProcesses;
        processEvents.reserve(128);
        presentEvents.reserve(4096);
        terminatedProcesses.reserve(16);

        // create a periodic timer used to check for terminated processes / quit while also waiting for events
        auto hTimer = util::win::Handle(CreateWaitableTimerW(
            nullptr, FALSE, nullptr
        ));
        if (!hTimer) {
            pmlog_error("Failed creating timer").hr();
        }
        // set timer period to 100ms
        {
            const LARGE_INTEGER dueTime{ .QuadPart = 0 };
            if (!SetWaitableTimer(hTimer, &dueTime, 100, nullptr, nullptr, FALSE)) {
                pmlog_error("Failed setting timer").hr();
            }
        }

        util::QpcTimer timer;

        while (true) {
            // Read quit_output_thread_ here, but then check it after processing
            // queued events. This ensures that we call DequeueAnalyzedInfo() at
            // least once after events have stopped being collected so that all
            // events are included.
            //        
            // TODO: consider replacing this flag with a waitable event
            const auto quit = quit_output_thread_.load();

            // Copy and process all the collected events, and update the various
            // tracking and statistics data structures.
            ProcessEvents(&processEvents, &presentEvents, &terminatedProcesses);

            // Everything is processed and output out at this point, so if we're
            // quiting we don't need to update the rest.
            if (quit) {
                pmlog_dbg("Finishing Output loop due to quit signal");
                break;
            }

            // wait for either events to process or periodic polling timer
            while (auto idx = util::win::WaitAnyEvent(pm_consumer_->hEventsReadyEvent, hTimer)) {
                // events are ready so we should process them
                if (*idx == 0) {
                    pmlog_verb(v::etwq)("Event(s) ready");
                    break;
                }
                pmlog_verb(v::etwq)("Doing periodic Output processing");
                // Timer has elapsed so we should do periodic polling operations
                // Update tracking information.
                CheckForTerminatedRealtimeProcesses(&terminatedProcesses);
                // check for quit signal
                if (quit_output_thread_.load()) {
                    pmlog_dbg("Detected quit signal");
                    break;
                }
            }
        }

        // Process handles
        std::lock_guard<std::mutex> lock(process_mutex_);
        for (auto& pair : processes_) {
            auto processInfo = &pair.second;
            if (processInfo->mHandle != NULL) {
                CloseHandle(processInfo->mHandle);
            }
        }
        processes_.clear();
    }
    catch (...) {
        pmlog_error(util::ReportException());
    }
}

void RealtimePresentMonSession::StartOutputThread() {
    quit_output_thread_ = false;
    output_thread_ = std::thread(&RealtimePresentMonSession::Output, this);
}

void RealtimePresentMonSession::StopOutputThread() {
    if (output_thread_.joinable()) {
        quit_output_thread_ = true;
        output_thread_.join();
    }
}

ProcessInfo* RealtimePresentMonSession::GetProcessInfo(uint32_t processId) {
    std::lock_guard<std::mutex> lock(process_mutex_);
    auto result = processes_.emplace(processId, ProcessInfo());
    auto processInfo = &result.first->second;
    auto newProcess = result.second;

    if (newProcess) {
        // Try to open a limited handle into the process in order to query its
        // name and also periodically check if it has terminated.  This will
        // fail (with GetLastError() == ERROR_ACCESS_DENIED) if the process was
        // run on another account, unless we're running with SeDebugPrivilege.
        auto pProcessName = L"<error>";
        wchar_t path[MAX_PATH];
        const auto handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (handle) {
            auto numChars = (DWORD)std::size(path);
            if (QueryFullProcessImageNameW(handle, 0, path, &numChars)) {
                pProcessName = PathFindFileNameW(path);
            }
        }

        InitProcessInfo(processInfo, processId, handle, pProcessName);
    }

    return processInfo;
}

void RealtimePresentMonSession::InitProcessInfo(ProcessInfo* processInfo, uint32_t processId,
    HANDLE handle,
    std::wstring const& processName) {
    processInfo->mHandle = handle;
    processInfo->mModuleName = processName;
    processInfo->mTargetProcess = true;

    target_process_count_ += 1;
}

void RealtimePresentMonSession::UpdateProcesses(
    std::vector<ProcessEvent> const& processEvents,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    for (auto const& processEvent : processEvents) {
        if (processEvent.IsStartEvent) {
            // This event is a new process starting, the pid should not already be
            // in processes_.
            std::lock_guard<std::mutex> lock(process_mutex_);
            auto result = processes_.emplace(processEvent.ProcessId, ProcessInfo());
            auto processInfo = &result.first->second;
            auto newProcess = result.second;
            if (newProcess) {
                InitProcessInfo(processInfo, processEvent.ProcessId, NULL,
                    processEvent.ImageFileName);
            }
        }
        else {
            // Note any process termination in terminatedProcess, to be handled
            // once the present event stream catches up to the termination time.
            terminatedProcesses->emplace_back(processEvent.ProcessId,
                processEvent.QpcTime);
        }
    }
}

void RealtimePresentMonSession::HandleTerminatedProcess(uint32_t processId) {
    std::lock_guard<std::mutex> lock(process_mutex_);
    auto iter = processes_.find(processId);
    if (iter == processes_.end()) {
        return;  // shouldn't happen.
    }

    auto processInfo = &iter->second;
    if (processInfo->mTargetProcess) {
        // TODO(megalvan): Need to figure this out
        // Close this process' CSV.
        // CloseOutputCsv(processInfo);

        target_process_count_ -= 1;
    }
    processes_.erase(std::move(iter));
    streamer_.StopStreaming(processId);
}

// Check if any realtime processes terminated and add them to the terminated
// list.
//
// We assume that the process terminated now, which is wrong but conservative
// and functionally ok because no other process should start with the same PID
// as long as we're still holding a handle to it.
void RealtimePresentMonSession::CheckForTerminatedRealtimeProcesses(
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    std::lock_guard<std::mutex> lock(process_mutex_);
    for (auto& pair : processes_) {
        auto processId = pair.first;
        auto processInfo = &pair.second;

        DWORD exitCode = 0;
        if (processInfo->mHandle != NULL &&
            GetExitCodeProcess(processInfo->mHandle, &exitCode) &&
            exitCode != STILL_ACTIVE) {
            uint64_t qpc = 0;
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&qpc));
            terminatedProcesses->emplace_back(processId, qpc);
            CloseHandle(processInfo->mHandle);
            processInfo->mHandle = NULL;
            // The tracked process has terminated. As multiple clients could be
            // tracking this process call stop streaming until the streamer
            // returns false and no longer holds an NSM for the process.
            streamer_.StopStreaming(processId);
            if ((streamer_.NumActiveStreams() == 0) && evtStreamingStarted_) {
                evtStreamingStarted_.Reset();
            }
        }
    }
}
