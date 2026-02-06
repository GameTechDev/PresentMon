// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Logging.h"
#include "RealtimePresentMonSession.h"
#include "CliOptions.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/Event.h"
#include "../CommonUtilities/win/Utilities.h"
#include "../CommonUtilities/Qpc.h"
#include "../CommonUtilities/Exception.h"

using namespace pmon;
using namespace std::literals;
using v = util::log::V;

RealtimePresentMonSession::RealtimePresentMonSession(svc::FrameBroadcaster& broadcaster)
{
    pBroadcaster = &broadcaster;
    ResetEtwFlushPeriod();
}

bool RealtimePresentMonSession::IsTraceSessionActive() {
    return session_active_.load(std::memory_order_acquire);
}

PM_STATUS RealtimePresentMonSession::UpdateTracking(const std::unordered_set<uint32_t>& trackedPids) {
    const bool wasActive = HasLiveTargets();
    std::unordered_map<uint32_t, bool> previousState;
    {
        std::lock_guard lock(tracked_processes_mutex_);
        previousState = tracked_pid_live_;
    }
    SyncTrackedPidState(trackedPids);
    const bool isActive = HasLiveTargets();
    if (isActive && (!wasActive || !IsTraceSessionActive())) {
        auto status = StartTraceSession();
        if (status != PM_STATUS_SUCCESS) {
            {
                std::lock_guard lock(tracked_processes_mutex_);
                tracked_pid_live_ = std::move(previousState);
            }
            return status;
        }
    }
    if (isActive && evtStreamingStarted_) {
        evtStreamingStarted_.Set();
    }
    if (!isActive) {
        if (evtStreamingStarted_) {
            evtStreamingStarted_.Reset();
        }
        StopTraceSession();
    }
    return PM_STATUS::PM_STATUS_SUCCESS;
}

bool RealtimePresentMonSession::CheckTraceSessions(bool forceTerminate) {
    if (forceTerminate) {
        StopTraceSession();
        ClearTrackedProcesses();
        return true;
    }
    if (!HasLiveTargets() && (IsTraceSessionActive() == true)) {
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
    if (session_active_.load(std::memory_order_acquire)) {
        if (ControlTraceW(trace_session_.mSessionHandle, nullptr, &props, EVENT_TRACE_CONTROL_FLUSH)) {
            pmlog_warn("Failed manual flush of ETW event buffer").hr();
        }
    }

}

void RealtimePresentMonSession::ResetEtwFlushPeriod()
{
    etw_flush_period_ms_ = default_realtime_etw_flush_period_ms_;
}

PM_STATUS RealtimePresentMonSession::StartTraceSession() {
    std::lock_guard lock(session_mutex_);

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
    pm_session_name_ = util::str::ToWide(*opt.etwSessionName);

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

    // Mark session as active (atomic operation)
    session_active_.store(true, std::memory_order_release);

    // Start the consumer and output threads
    StartConsumerThread(trace_session_.mTraceHandle);
    StartOutputThread();
    return PM_STATUS::PM_STATUS_SUCCESS;
}

void RealtimePresentMonSession::StopTraceSession() {
    // PHASE 1: Signal shutdown and wait for threads to observe it
    // this also enforces "only_once" semantics for multiple stop callers
    if (session_active_.exchange(false, std::memory_order_acq_rel)) {

        // Stop the trace session to stop new events from coming in
        trace_session_.Stop();

        // Wait for threads to exit their critical sections and finish
        WaitForConsumerThreadToExit();
        StopOutputThread();

        // PHASE 2: Safe cleanup after threads have finished
        std::lock_guard<std::mutex> lock(session_mutex_);

        if (evtStreamingStarted_) {
            evtStreamingStarted_.Reset();
        }

        if (pm_consumer_) {
            pm_consumer_.reset();
        }
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
    // Check if session is active before accessing pm_consumer_ (atomic guard)
    if (session_active_.load(std::memory_order_acquire) && pm_consumer_) {        
        pm_consumer_->DequeueProcessEvents(*processEvents);
        pm_consumer_->DequeuePresentEvents(*presentEvents);
    }
}

void RealtimePresentMonSession::AddPresents(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex, bool recording, bool checkStopQpc,
    uint64_t stopQpc, bool* hitStopQpc) {
    auto i = *presentEventIndex;

    if (session_active_.load(std::memory_order_acquire)) {
        if (trace_session_.mStartTimestamp.QuadPart != 0) {
            if (pBroadcaster) {
                pBroadcaster->SetStartQpc(trace_session_.mStartTimestamp.QuadPart);
            }
        }
    }

    // logging of ETW latency
    if (util::log::GlobalPolicy::VCheck(v::etwq)) {
        pmlog_(util::log::Level::Verbose).note(std::format("Processing [{}] frames", presentEvents.size()));
        for (auto& p : presentEvents) {
            if (p->FinalState == PresentResult::Presented) {
                const auto per = util::GetTimestampPeriodSeconds();
                const auto now = util::GetCurrentTimestamp();
                // TODO: Presents can now have multiple displayed frames if we are tracking
                // frame types. For now take the first displayed frame for logging stats
                const auto lag = util::TimestampDeltaToSeconds(p->Displayed[0].second, now, per);
                pmlog_(util::log::Level::Verbose).note(std::format("Frame [{}] lag: {} ms", p->FrameId, lag * 1000.));
            }
        }
    }

    for (auto n = presentEvents.size(); i < n; ++i) {
        auto& presentEvent = presentEvents[i];
        assert(presentEvent->IsCompleted);

        // Ignore failed and lost presents.
        if (presentEvent->IsLost || presentEvent->PresentFailed) {
            // TODO: log these
            continue;
        }

        // Stop processing events if we hit the next stop time.
        if (checkStopQpc && presentEvent->PresentStartTime >= stopQpc) {
            *hitStopQpc = true;
            break;
        }

        if (!IsProcessTracked(presentEvent->ProcessId)) {
            continue;
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

        pBroadcaster->Broadcast(*presentEvent);

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
                if (idx == 0) {
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


void RealtimePresentMonSession::UpdateProcesses(
    std::vector<ProcessEvent> const& processEvents,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    for (auto const& processEvent : processEvents) {
        if (!IsProcessTracked(processEvent.ProcessId)) {
            continue;
        }
        if (!processEvent.IsStartEvent) {
            // Note any process termination in terminatedProcess, to be handled
            // once the present event stream catches up to the termination time.
            MarkProcessExited(processEvent.ProcessId);
            terminatedProcesses->emplace_back(processEvent.ProcessId,
                processEvent.QpcTime);
        }
    }
}

void RealtimePresentMonSession::HandleTerminatedProcess(uint32_t processId) {
    // TODO(megalvan): Need to figure this out
    // Close this process' CSV.
    // CloseOutputCsv(processInfo);
    MarkProcessExited(processId);
    if (!HasLiveTrackedProcesses() && evtStreamingStarted_) {
        evtStreamingStarted_.Reset();
    }
}

// Check if any realtime processes terminated and add them to the terminated
// list.
//
// Note: handle-based polling of target process lifetime is intentionally
// handled outside of the session layer.
void RealtimePresentMonSession::CheckForTerminatedRealtimeProcesses(
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    (void)terminatedProcesses;
}
