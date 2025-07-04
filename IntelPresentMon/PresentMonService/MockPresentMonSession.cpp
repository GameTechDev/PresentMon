// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MockPresentMonSession.h"
#include "CliOptions.h"
#include "..\CommonUtilities\str\String.h"
#include "Logging.h"

static const std::wstring kMockEtwSessionName = L"MockETWSession";

using namespace std::literals;

MockPresentMonSession::MockPresentMonSession()
    :
    quit_output_thread_(false),
    target_process_count_(0) {
    pm_session_name_.clear();
    pm_consumer_.reset();
}

bool MockPresentMonSession::IsTraceSessionActive() {
    return (pm_consumer_ != nullptr);
}

PM_STATUS MockPresentMonSession::StartStreaming(uint32_t client_process_id,
    uint32_t target_process_id,
    std::string& nsmFileName) {

    auto& opt = clio::Options::Get();

    // In a mock PresentMon session we must have an ETL file
    // if we are starting a trace session
    if (opt.etlTestFile.AsOptional().has_value() == false) {
        pmlog_error("--etl-test-file requried for mock presentmon session");
        return PM_STATUS::PM_STATUS_FAILURE;
    }

    // TODO: hook up all cli options
    PM_STATUS status = streamer_.StartStreaming(client_process_id,
        target_process_id, nsmFileName, true, opt.pacePlayback, opt.pacePlayback, !opt.pacePlayback, true);
    if (status != PM_STATUS::PM_STATUS_SUCCESS) {
        return status;
    }

    std::wstring sessionName;
    if (opt.etwSessionName.AsOptional().has_value()) {
        sessionName = pmon::util::str::ToWide(opt.etwSessionName.AsOptional().value());
    }
    else {
        sessionName = kMockEtwSessionName;
    }

    // TODO: hook up all cli options
    status = StartTraceSession(target_process_id, *opt.etlTestFile, sessionName,
        true, opt.pacePlayback, opt.pacePlayback, !opt.pacePlayback, true);
    if (status == PM_STATUS_FAILURE) {
        // Unable to start a trace session. Destroy the NSM and
        // return status
        streamer_.StopStreaming(target_process_id);
        return status;
    }

    return PM_STATUS::PM_STATUS_SUCCESS;
}

void MockPresentMonSession::StopStreaming(uint32_t client_process_id,
    uint32_t target_process_id) {
    streamer_.StopStreaming(client_process_id, target_process_id);
    if (streamer_.NumActiveStreams() == 0) {
        StopTraceSession();
    }
}

bool MockPresentMonSession::CheckTraceSessions(bool forceTerminate) {
    if (pm_consumer_ && stop_playback_requested_ == true) {
        StopTraceSession();
        return true;
    }

    if (forceTerminate) {
        StopTraceSession();
        return true;
    }
    return false;
}

HANDLE MockPresentMonSession::GetStreamingStartHandle() {
    return evtStreamingStarted_;
}

void MockPresentMonSession::StartPlayback()
{
}

void MockPresentMonSession::StopPlayback()
{
    stop_playback_requested_ = true;
}

PM_STATUS MockPresentMonSession::StartTraceSession(uint32_t processId, const std::string& etlPath,
    const std::wstring& etwSessionName,
    bool isPlayback,
    bool isPlaybackPaced,
    bool isPlaybackRetimed,
    bool isPlaybackBackpressured,
    bool isPlaybackResetOldest) {

    std::lock_guard<std::mutex> lock(session_mutex_);

    if (pm_consumer_) {
        pmlog_error("pmconsumer already created when start trace session called");
        return PM_STATUS::PM_STATUS_SERVICE_ERROR;
    }

    auto expectFilteredEvents = IsWindows8Point1OrGreater();
    auto filterProcessIds = false;  // Does not support process names at this point

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
    pm_consumer_->mPaceEvents = isPlaybackPaced;
    pm_consumer_->mRetimeEvents = isPlaybackRetimed;

    pm_session_name_ = etwSessionName;

    const auto etl_file_name = pmon::util::str::ToWide(etlPath);

    // Start the session. If a session with this name is already running, we stop
    // it and start a new session. This is useful if a previous process failed to
    // properly shut down the session for some reason.
    trace_session_.mPMConsumer = pm_consumer_.get();
    auto status = trace_session_.Start(etl_file_name.c_str(), pm_session_name_.c_str());

    if (status == ERROR_ALREADY_EXISTS) {
        status = StopNamedTraceSession(pm_session_name_.c_str());
        if (status == ERROR_SUCCESS) {
            status = trace_session_.Start(etl_file_name.c_str(), pm_session_name_.c_str());
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

    // Set the process id for this etl session
    etlProcessId_ = processId;

    // Start the consumer and output threads
    StartConsumerThread(trace_session_.mTraceHandle);
    StartOutputThread();
    return PM_STATUS::PM_STATUS_SUCCESS;
}

void MockPresentMonSession::StopTraceSession() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    // Stop the trace session.
    trace_session_.Stop();

    // Wait for the consumer and output threads to end (which are using the
    // consumers).
    WaitForConsumerThreadToExit();
    StopOutputThread();

    // Stop all streams
    streamer_.StopAllStreams();

    if (pm_consumer_) {
        pm_consumer_.reset();
    }
}

void MockPresentMonSession::StartConsumerThread(TRACEHANDLE traceHandle) {
    consumer_thread_ = std::thread(&MockPresentMonSession::Consume, this, traceHandle);
}

void MockPresentMonSession::WaitForConsumerThreadToExit() {
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

void MockPresentMonSession::DequeueAnalyzedInfo(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents) {
    pm_consumer_->DequeueProcessEvents(*processEvents);
    pm_consumer_->DequeuePresentEvents(*presentEvents);
}

void MockPresentMonSession::AddPresents(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex, bool recording, bool checkStopQpc,
    uint64_t stopQpc, bool* hitStopQpc) {
    auto i = *presentEventIndex;

    assert(trace_session_.mStartTimestamp.QuadPart != 0);
    // If mStartTimestamp contains a value an etl file is being processed.
    // Set this value in the streamer to have the correct start time.
    streamer_.SetStartQpc(trace_session_.mStartTimestamp.QuadPart);

    for (auto n = presentEvents.size(); i < n; ++i) {
        auto presentEvent = presentEvents[i];
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

        // Last producer and last consumer are internal fields
        // Remove for public build
        // Send data to streamer if we have more than single present event
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
    }

    *presentEventIndex = i;
}

void MockPresentMonSession::ProcessEvents(
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
        AddPresents(*presentEvents, &presentEventIndex, true, false,
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

void MockPresentMonSession::Consume(TRACEHANDLE traceHandle) {
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

    // consider setting nsm header flag here to indicate end of playback without destroying nsm/trace session
}

void MockPresentMonSession::Output() {
    // Structures to track processes and statistics from recorded events.
    std::vector<ProcessEvent> processEvents;
    std::vector<std::shared_ptr<PresentEvent>> presentEvents;
    std::vector<std::pair<uint32_t, uint64_t>> terminatedProcesses;
    processEvents.reserve(128);
    presentEvents.reserve(4096);
    terminatedProcesses.reserve(16);

    for (;;) {
        // Read quit_output_thread_ here, but then check it after processing
        // queued events. This ensures that we call DequeueAnalyzedInfo() at
        // least once after events have stopped being collected so that all
        // events are included.
        const auto quit = quit_output_thread_.load();

        // Copy and process all the collected events, and update the various
        // tracking and statistics data structures.
        ProcessEvents(&processEvents, &presentEvents, &terminatedProcesses);

        // Everything is processed and output out at this point, so if we're
        // quiting we don't need to update the rest.
        if (quit) {
            break;
        }

        // Sleep to reduce overhead.
        // TODO: sync this to eliminate overhead / lag
        std::this_thread::sleep_for(10ms);
    }
}

void MockPresentMonSession::StartOutputThread() {
    quit_output_thread_ = false;
    output_thread_ = std::thread(&MockPresentMonSession::Output, this);
}

void MockPresentMonSession::StopOutputThread() {
    if (output_thread_.joinable()) {
        quit_output_thread_ = true;
        output_thread_.join();
    }
}

ProcessInfo* MockPresentMonSession::GetProcessInfo(uint32_t processId) {
    auto result = processes_.emplace(processId, ProcessInfo());
    auto processInfo = &result.first->second;
    auto newProcess = result.second;

    if (newProcess) {
        InitProcessInfo(processInfo, processId, INVALID_HANDLE_VALUE, L"MockProcess.exe");
    }
    return processInfo;
}

void MockPresentMonSession::InitProcessInfo(ProcessInfo* processInfo, uint32_t processId,
    HANDLE handle, std::wstring const& processName) {

    processInfo->mHandle = handle;
    processInfo->mModuleName = processName;
    processInfo->mTargetProcess = true;

    target_process_count_ += 1;
}

void MockPresentMonSession::UpdateProcesses(
    std::vector<ProcessEvent> const& processEvents,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
    for (auto const& processEvent : processEvents) {
        if (processEvent.IsStartEvent) {
            // This event is a new process starting, the pid should not already be
            // in processes_.
            auto result = processes_.emplace(processEvent.ProcessId, ProcessInfo());
            auto processInfo = &result.first->second;
            auto newProcess = result.second;
            if (newProcess) {
                InitProcessInfo(processInfo, processEvent.ProcessId, NULL,
                    processEvent.ImageFileName);
            }
        }
    }
}

void MockPresentMonSession::HandleTerminatedProcess(uint32_t processId) {
}