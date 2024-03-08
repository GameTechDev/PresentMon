// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMon.h"

#include <VersionHelpers.h>
#include <shlwapi.h>
#include <span>

static const std::wstring kEtlSessionName = L"ETLProcessing";
static const std::wstring kRealTimeSessionName = L"PMService";

PresentMonSession::PresentMonSession()
    : target_process_count_(0),
      quit_output_thread_(false),
      process_trace_finished_(false) {
  pm_session_name_.clear();
  processes_.clear();
  etl_file_name_.clear();
  HANDLE temp_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
  streaming_started_.reset(temp_handle);
  pm_consumer_.reset();
}

PM_STATUS PresentMonSession::StartTraceSession() {
  process_trace_finished_ = false;
  std::lock_guard<std::mutex> lock(session_mutex_);

  if (pm_consumer_) {
    return PM_STATUS::PM_STATUS_SESSION_ALREADY_EXISTS;
  }

  auto expectFilteredEvents = IsWindows8Point1OrGreater();
  auto filterProcessIds =
      false;  // Does not support process names at this point

  // Create consumers
  try {
    pm_consumer_ = std::make_unique<PMTraceConsumer>();
  } catch (...) {
    return PM_STATUS::PM_STATUS_ERROR;
  }

  pm_consumer_->mFilteredEvents = expectFilteredEvents;
  pm_consumer_->mFilteredProcessIds = filterProcessIds;
  pm_consumer_->mTrackDisplay = true;
  pm_consumer_->mTrackGPU = true;
  pm_consumer_->mTrackGPUVideo = true;
  pm_consumer_->mTrackInput = true;

  const wchar_t* etl_file_name = nullptr;
  if (etl_file_name_.size() > 0) {
    etl_file_name = etl_file_name_.c_str();
    pm_session_name_ = kEtlSessionName;
  } else {
    pm_session_name_ = kRealTimeSessionName;
  }

  // Start the session. If a session with this name is already running, we stop
  // it and start a new session. This is useful if a previous process failed to
  // properly shut down the session for some reason.
  auto status = trace_session_.Start(pm_consumer_.get(), nullptr, etl_file_name,
                                     pm_session_name_.c_str());

  if (status == ERROR_ALREADY_EXISTS) {
    status = TraceSession::StopNamedSession(pm_session_name_.c_str());
    if (status == ERROR_SUCCESS) {
      status = trace_session_.Start(pm_consumer_.get(), nullptr, etl_file_name,
                                    pm_session_name_.c_str());
    }
  }

  // Report error if we failed to start a new session
  if (status != ERROR_SUCCESS) {
    pm_consumer_.reset();
    switch (status) {
      case ERROR_ALREADY_EXISTS:
        return PM_STATUS::PM_STATUS_SESSION_ALREADY_EXISTS;
      case ERROR_FILE_NOT_FOUND:
        return PM_STATUS::PM_STATUS_INVALID_ETL_FILE;
      default:
        return PM_STATUS::PM_STATUS_ERROR;
    }
  }

  // Start the consumer and output threads
  StartConsumerThread(trace_session_.mTraceHandle);
  StartOutputThread();
  return PM_STATUS::PM_STATUS_SUCCESS;
}

void PresentMonSession::StopTraceSession() {
  std::lock_guard<std::mutex> lock(session_mutex_);
  // Stop the trace session.
  trace_session_.Stop();

  // Wait for the consumer and output threads to end (which are using the
  // consumers).
  WaitForConsumerThreadToExit();
  StopOutputThread();

  // Stop all streams
  streamer_.StopAllStreams();
  if (streaming_started_.get() != INVALID_HANDLE_VALUE) {
    ResetEvent(streaming_started_.get());
  }

  if (pm_consumer_) {
    pm_consumer_.reset();
  }
}

PM_STATUS PresentMonSession::ProcessEtlFile(uint32_t client_process_id,
                                            const std::wstring& etl_file_name,
                                            std::string& nsm_file_name) {
  if (pm_consumer_ != nullptr) {
    // There is a current consumer running. For now,
    // only support a single consumer.
    return PM_STATUS::PM_STATUS_CREATE_SESSION_FAILED;
  }

  streamer_.SetStreamMode(StreamMode::kOfflineEtl);
  PM_STATUS status = streamer_.StartStreaming(
      client_process_id, static_cast<uint32_t>(StreamPidOverride::kEtlPid),
      nsm_file_name);
  if (status != PM_STATUS::PM_STATUS_SUCCESS) {
    return PM_STATUS::PM_STATUS_CREATE_SESSION_FAILED;
  }

  // Start the trace session
  etl_file_name_ = etl_file_name;
  // Start the trace session using the passed in etl file
  status = StartTraceSession();
  if (status != PM_STATUS::PM_STATUS_SUCCESS) {
    streamer_.StopStreaming(client_process_id);
  }
  return status;
}

void PresentMonSession::StartConsumerThread(TRACEHANDLE traceHandle) {
  consumer_thread_ = std::thread(&PresentMonSession::Consume, this, traceHandle);
}

void PresentMonSession::WaitForConsumerThreadToExit() {
  if (consumer_thread_.joinable()) {
    consumer_thread_.join();
  }
}

void PresentMonSession::Consume(TRACEHANDLE traceHandle) {
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

  // This is only needed if we are processing an ETL file and ProcessTrace()
  // returned because the ETL is done.
  process_trace_finished_ = true;
}

void PresentMonSession::DequeueAnalyzedInfo(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents) {
  pm_consumer_->DequeueProcessEvents(*processEvents);
  pm_consumer_->DequeuePresentEvents(*presentEvents);
  pm_consumer_->DequeueLostPresentEvents(*lostPresentEvents);
}

void PresentMonSession::StartOutputThread() {
  quit_output_thread_ = false;
  output_thread_ = std::thread(&PresentMonSession::Output, this);
}

void PresentMonSession::StopOutputThread() {
  quit_output_thread_ = true;
  if (output_thread_.joinable()) {
    output_thread_.join();
  }
}

ProcessInfo* PresentMonSession::GetProcessInfo(uint32_t processId) {
  std::lock_guard<std::mutex> lock(process_mutex_);
  auto result = processes_.emplace(processId, ProcessInfo());
  auto processInfo = &result.first->second;
  auto newProcess = result.second;

  if (newProcess) {
    // Try to open a limited handle into the process in order to query its
    // name and also periodically check if it has terminated.  This will
    // fail (with GetLastError() == ERROR_ACCESS_DENIED) if the process was
    // run on another account, unless we're running with SeDebugPrivilege.
    wchar_t const* processName = L"<error>";
    wchar_t path[MAX_PATH];
    DWORD numChars = sizeof(path);
    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (handle != NULL) {
      if (QueryFullProcessImageNameW(handle, 0, path, &numChars)) {
        processName = PathFindFileNameW(path);
      }
      CloseHandle(handle);
    }

    InitProcessInfo(processInfo, processId, handle, processName);
  }

  return processInfo;
}

void PresentMonSession::InitProcessInfo(ProcessInfo* processInfo, uint32_t processId,
                                 HANDLE handle,
                                 std::wstring const& processName) {
  processInfo->mHandle = handle;
  processInfo->mModuleName = processName;
  processInfo->mTargetProcess = true;

  target_process_count_ += 1;
}

void PresentMonSession::UpdateProcesses(
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
    } else {
      // Note any process termination in terminatedProcess, to be handled
      // once the present event stream catches up to the termination time.
      terminatedProcesses->emplace_back(processEvent.ProcessId,
                                        processEvent.QpcTime);
    }
  }
}

void PresentMonSession::AddPresents(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex, bool recording, bool checkStopQpc,
    uint64_t stopQpc, bool* hitStopQpc) {
  auto i = *presentEventIndex;

  if (trace_session_.mStartTimestamp.QuadPart != 0) {
    // If mStartTimestamp contains a value an etl file is being processed.
    // Set this value in the streamer to have the correct start time.
    streamer_.SetStartQpc(trace_session_.mStartTimestamp.QuadPart);
  }

  for (auto n = presentEvents.size(); i < n; ++i) {
    auto presentEvent = presentEvents[i];
    assert(presentEvent->IsCompleted);

    // Stop processing events if we hit the next stop time.
    if (checkStopQpc && presentEvent->PresentStartTime >= stopQpc) {
      *hitStopQpc = true;
      break;
    }

    // Stop processing if streamer timed out and output_thread termination is triggered
    if (quit_output_thread_) {
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
    // Only grab control lib data when performing real time
    // tracing
    if ((pm_session_name_.compare(kRealTimeSessionName) == 0) &&
        (telemetry_container_)) {
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
    // Only grab control lib data when performing real time
    // tracing
    if ((pm_session_name_.compare(kRealTimeSessionName) == 0) &&
        (cpu_)) {
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

    if (chain->mPresentHistoryCount > 0) {
      // Last producer and last consumer are internal fields
      // Remove for public build
      // Send data to streamer if we have more than single present event
      streamer_.ProcessPresentEvent(
          presentEvent.get(), &power_telemetry, &cpu_telemetry,
          chain->mLastPresentQPC, chain->mLastDisplayedPresentQPC,
          processInfo->mModuleName, gpu_telemetry_cap_bits,
          cpu_telemetry_cap_bits);
    }

    chain->mLastPresentQPC = presentEvent->PresentStartTime;
    if (presentEvent->FinalState == PresentResult::Presented) {
      chain->mLastDisplayedPresentQPC = presentEvent->ScreenTime;
    } else if (chain->mLastDisplayedPresentQPC == chain->mLastPresentQPC) {
      chain->mLastDisplayedPresentQPC = 0;
    }

    chain->mPresentHistoryCount += 1;
  }

  *presentEventIndex = i;
}

void PresentMonSession::HandleTerminatedProcess(uint32_t processId) {
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

void PresentMonSession::ProcessEvents(
    std::vector<ProcessEvent>* processEvents,
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents,
    std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses) {
  bool eventProcessingDone = false;

  // Copy any analyzed information from ConsumerThread and early-out if there
  // isn't any.
  DequeueAnalyzedInfo(processEvents, presentEvents, lostPresentEvents);
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

  if (!eventProcessingDone && !quit_output_thread_) {
    // Process all present events. The PresentMon service is always recording
    // and in this instance we are not concerned with checking for a stop QPC.
    auto hitStopQPC = false;
    AddPresents(*presentEvents, &presentEventIndex, true, false, 0,
                &hitStopQPC);
  }

  // Clear events processed.
  processEvents->clear();
  presentEvents->clear();
  lostPresentEvents->clear();

  // Finished processing all events.  Erase the terminated processes that we
  // handled now.
  if (terminatedProcessIndex > 0) {
    terminatedProcesses->erase(
        terminatedProcesses->begin(),
        terminatedProcesses->begin() + terminatedProcessIndex);
  }

  return;
}

// Check if any realtime processes terminated and add them to the terminated
// list.
//
// We assume that the process terminated now, which is wrong but conservative
// and functionally ok because no other process should start with the same PID
// as long as we're still holding a handle to it.
void PresentMonSession::CheckForTerminatedRealtimeProcesses(
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
      if ((streamer_.NumActiveStreams() == 0) &&
          (streaming_started_.get() != INVALID_HANDLE_VALUE)) {
        ResetEvent(streaming_started_.get());
      }
    }
  }
}

void PresentMonSession::Output() {
  // Structures to track processes and statistics from recorded events.
  std::vector<ProcessEvent> processEvents;
  std::vector<std::shared_ptr<PresentEvent>> presentEvents;
  std::vector<std::shared_ptr<PresentEvent>> lostPresentEvents;
  std::vector<std::pair<uint32_t, uint64_t>> terminatedProcesses;
  processEvents.reserve(128);
  presentEvents.reserve(4096);
  terminatedProcesses.reserve(16);

  for (;;) {
    // Read quit_output_thread_ here, but then check it after processing
    // queued events. This ensures that we call DequeueAnalyzedInfo() at
    // least once after events have stopped being collected so that all
    // events are included.
    auto quit = quit_output_thread_ == true ? true:false;

    // Copy and process all the collected events, and update the various
    // tracking and statistics data structures.
    ProcessEvents(&processEvents, &presentEvents, &lostPresentEvents,
                  &terminatedProcesses);

    // Everything is processed and output out at this point, so if we're
    // quiting we don't need to update the rest.
    if (quit) {
      break;
    }

    // Update tracking information.
    CheckForTerminatedRealtimeProcesses(&terminatedProcesses);

    // Sleep to reduce overhead.
    Sleep(100);
    
  }

  // Output warning if events were lost.
  ULONG eventsLost = 0;
  ULONG buffersLost = 0;
  trace_session_.CheckLostReports(&eventsLost, &buffersLost);

  if (buffersLost > 0) {
    // TODO(megalvan): Need to add this to the debug service output
    // fprintf(stderr, "warning: %lu ETW buffers were lost.\n", buffersLost);
  }
  if (eventsLost > 0) {
    // TODO(megalvan): Need to add this to the debug service output
    // fprintf(stderr, "warning: %lu ETW events were lost.\n", eventsLost);
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

  return;
}

PM_STATUS PresentMonSession::StartStreaming(uint32_t client_process_id,
                                            uint32_t target_process_id,
                                            std::string& nsmFileName) {
  
  if (target_process_id != (uint32_t)StreamPidOverride::kStreamAllPid) {
    // Check to see if the target process is valid
    HANDLE target_process_handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION, FALSE, target_process_id);
    if (target_process_handle == nullptr) {
      return PM_STATUS::PM_STATUS_INVALID_PID;
    }
    CloseHandle(target_process_handle);
  }
  
  PM_STATUS status = streamer_.StartStreaming(client_process_id,
                                              target_process_id, nsmFileName);
  if (status != PM_STATUS::PM_STATUS_SUCCESS) {
    return status;
  } else{
    // Add the client process id to be monitored
    GetProcessInfo(client_process_id);
    auto status = StartTraceSession();
    if (status == PM_STATUS_ERROR) {
      // Unable to start a trace session. Destroy the NSM and
      // return status
      streamer_.StopStreaming(target_process_id);
      return status;
    }
    // Only signal the streaming started event when we have
    // exactly one stream after returning from StartStreaming.
    if ((streamer_.NumActiveStreams() == 1) &&
        (streaming_started_.get() != INVALID_HANDLE_VALUE)) {
      SetEvent(streaming_started_.get());
    }
    // Also monitor the target process id
    GetProcessInfo(target_process_id);
    return PM_STATUS::PM_STATUS_SUCCESS;
  }
}

void PresentMonSession::StopStreaming(uint32_t client_process_id,
                                      uint32_t target_process_id) {
  streamer_.StopStreaming(client_process_id, target_process_id);
  if ((streamer_.NumActiveStreams() == 0) &&
      (streaming_started_.get() != INVALID_HANDLE_VALUE)) {
    ResetEvent(streaming_started_.get());
    StopTraceSession();
  }
}

// Checks to see If there is a consumer AND all ETL processing has completed 
// or streamer timed out due to lost of client or client not reading data fast enough
bool PresentMonSession::IsProcessTraceFinishedOrTimedOut() {
  if (pm_consumer_ && (process_trace_finished_ == true || streamer_.IsTimedOut())) {
    return true;
  }
  return false;
}

std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>>
PresentMonSession::EnumerateAdapters() {
  if (telemetry_container_) {
    return telemetry_container_->GetPowerTelemetryAdapters();
  } else {
    return {};
  }
}

PM_STATUS PresentMonSession::SelectAdapter(uint32_t adapter_id) {

  if (telemetry_container_) {
    if (adapter_id > telemetry_container_->GetPowerTelemetryAdapters().size()) {
      return PM_STATUS_INVALID_ADAPTER_ID;
    }
    current_telemetry_adapter_id_ = adapter_id;
  }
  return PM_STATUS::PM_STATUS_SUCCESS;
}

PM_STATUS PresentMonSession::SetGpuTelemetryPeriod(uint32_t period_ms) {
  if (period_ms < MIN_PM_TELEMETRY_PERIOD ||
      period_ms > MAX_PM_TELEMETRY_PERIOD) {
    return PM_STATUS::PM_STATUS_OUT_OF_RANGE;
  } else {
    gpu_telemetry_period_ms_ = period_ms;
    return PM_STATUS_SUCCESS;
  }
}

PresentMon::~PresentMon() {
  real_time_session_.StopTraceSession();
  etl_session_.StopTraceSession();
}

PM_STATUS PresentMon::StartTraceSession() {
  return real_time_session_.StartTraceSession();
}

void PresentMon::StopTraceSession() {
  return real_time_session_.StopTraceSession();
}

PM_STATUS PresentMon::StartStreaming(uint32_t client_process_id,
                                     uint32_t target_process_id,
                                     std::string& nsm_file_name) {
  if (target_process_id == static_cast<uint32_t>(StreamPidOverride::kEtlPid)) {
    return etl_session_.StartStreaming(client_process_id, target_process_id,
                                       nsm_file_name);
  } else {
    return real_time_session_.StartStreaming(client_process_id,
                                             target_process_id, nsm_file_name);
  }
  
}

void PresentMon::StopStreaming(uint32_t client_process_id,
                               uint32_t target_process_id) {
  return real_time_session_.StopStreaming(client_process_id, target_process_id);
}

PM_STATUS PresentMon::ProcessEtlFile(uint32_t client_process_id,
                                     const std::wstring& etl_file_name,
                                     std::string& nsm_file_name) {
  return etl_session_.ProcessEtlFile(client_process_id, etl_file_name,
                                     nsm_file_name);
}

std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>>
PresentMon::EnumerateAdapters() {
  // Only the real time trace uses the control libary interface
  return real_time_session_.EnumerateAdapters();
}

PM_STATUS PresentMon::SelectAdapter(uint32_t adapter_id) {
  // Only the real time trace uses the control libary interface
  return real_time_session_.SelectAdapter(adapter_id);
}

void PresentMon::CheckTraceSessions() {
  if (etl_session_.IsProcessTraceFinishedOrTimedOut() == true) {
    etl_session_.StopTraceSession();
  }
  if ((real_time_session_.GetActiveStreams() == 0) &&
      (real_time_session_.IsTraceSessionActive() == true)) {
    real_time_session_.StopTraceSession();
  }
}