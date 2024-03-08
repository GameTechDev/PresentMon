// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Windows.h>

#include <cmath>
#include <random>
#include <atomic>

#include "..\ControlLib\PowerTelemetryProvider.h"
#include "..\ControlLib\CpuTelemetry.h"
#include "..\Streamer\Streamer.h"
#include "..\..\PresentData\PresentMonTraceConsumer.hpp"
#include "..\..\PresentData\TraceSession.hpp"
#include "PowerTelemetryContainer.h"


struct SwapChainData {
  uint32_t mPresentHistoryCount;
  uint64_t mLastPresentQPC;
  uint64_t mLastDisplayedPresentQPC;
};

struct ProcessInfo {
  std::wstring mModuleName;
  std::unordered_map<uint64_t, SwapChainData> mSwapChain;
  HANDLE mHandle;
  bool mTargetProcess;
};

struct HandleDeleter {
  void operator()(HANDLE handle) const { CloseHandle(handle); }
};

class PresentMonSession {
 public:
  PresentMonSession();
  PresentMonSession(const PresentMonSession& t) = delete;
  PresentMonSession& operator=(const PresentMonSession& t) = delete;

  PM_STATUS StartTraceSession();
  void StopTraceSession();
  bool IsTraceSessionActive() { return (pm_consumer_ != nullptr); }

  PM_STATUS ProcessEtlFile(uint32_t client_process_id,
                           const std::wstring& etl_file_name,
                           std::string& nsm_file_name);

  PM_STATUS StartStreaming(uint32_t client_process_id,
                           uint32_t target_process_id,
                           std::string& nsmFileName);

  void StopStreaming(uint32_t client_process_id, uint32_t target_process_id);

  bool IsProcessTraceFinishedOrTimedOut();

  void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu) {
    cpu_ = pCpu.get();
  }

  std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
  std::string GetCpuName() {
    if (cpu_) {
      return cpu_->GetCpuName();
    } else {
      return std::string{"UNKOWN_CPU"};
    }
  }

  PM_STATUS SelectAdapter(uint32_t adapter_id);
  PM_STATUS SetGpuTelemetryPeriod(uint32_t period_ms);
  uint32_t GetGpuTelemetryPeriod() { return gpu_telemetry_period_ms_; }

  HANDLE GetStreamingStartHandle() { return streaming_started_.get(); }
  int GetActiveStreams() { return streamer_.NumActiveStreams(); }
  void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc) {
    telemetry_container_ = ptc;
  }

 private:
  void StartConsumerThread(TRACEHANDLE traceHandle);
  void WaitForConsumerThreadToExit();
  void DequeueAnalyzedInfo(
      std::vector<ProcessEvent>* processEvents,
      std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
      std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents);
  void Consume(TRACEHANDLE traceHandle);

  void StartOutputThread();
  void StopOutputThread();
  void Output();
  void AddPresents(
      std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
      size_t* presentEventIndex, bool recording, bool checkStopQpc,
      uint64_t stopQpc, bool* hitStopQpc);
  ProcessInfo* GetProcessInfo(uint32_t processId);
  void InitProcessInfo(ProcessInfo* processInfo, uint32_t processId,
                       HANDLE handle, std::wstring const& processName);
  void UpdateProcesses(
      std::vector<ProcessEvent> const& processEvents,
      std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses);
  void HandleTerminatedProcess(uint32_t processId);
  void ProcessEvents(
      std::vector<ProcessEvent>* processEvents,
      std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
      std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents,
      std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses);
  void CheckForTerminatedRealtimeProcesses(
      std::vector<std::pair<uint32_t, uint64_t>>* terminatedProcesses);

  std::wstring pm_session_name_;

  std::unique_ptr<PMTraceConsumer> pm_consumer_;
  TraceSession trace_session_;

  std::thread consumer_thread_;
  std::thread output_thread_;

  std::unordered_map<uint32_t, ProcessInfo> processes_;
  uint32_t target_process_count_;

  pwr::cpu::CpuTelemetry* cpu_ = nullptr;
  PowerTelemetryContainer* telemetry_container_ = nullptr;
  uint32_t current_telemetry_adapter_id_ = 0;

  // Set the initial telemetry period to 16ms
  uint32_t gpu_telemetry_period_ms_ = 16;

  Streamer streamer_;

  std::atomic<bool> quit_output_thread_;
  std::atomic<bool> process_trace_finished_;

  std::wstring etl_file_name_;
  
  // Event for when streaming has started
  std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>
      streaming_started_;
  mutable std::mutex session_mutex_;
  mutable std::mutex process_mutex_;
};

class PresentMon {
 public:
  PresentMon() {}
  ~PresentMon();

  PM_STATUS StartTraceSession();
  void StopTraceSession();

  // Check the status of both ETW logfile and real time trace sessions.
  // When an ETW logfile has finished processing the associated
  // trace session must be destroyed to allow for other etl sessions
  // to be processed. In the case of real-time session if for some reason
  // there are zero active streams and a trace session is still active
  // clean it up.
  void CheckTraceSessions();

  PM_STATUS StartStreaming(uint32_t client_process_id,
                           uint32_t target_process_id,
                           std::string& nsm_file_name);
  void StopStreaming(uint32_t client_process_id, uint32_t target_process_id);

  PM_STATUS ProcessEtlFile(uint32_t client_process_id,
                           const std::wstring& etl_file_name,
                           std::string& nsm_file_name);

  std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
  std::string GetCpuName() { return real_time_session_.GetCpuName(); }

  PM_STATUS SelectAdapter(uint32_t adapter_id);
  
  PM_STATUS SetGpuTelemetryPeriod(uint32_t period_ms) {
    // Only the real time trace sets GPU telemetry period
    return real_time_session_.SetGpuTelemetryPeriod(period_ms);

  }
  uint32_t GetGpuTelemetryPeriod() {
    // Only the real time trace sets GPU telemetry period
    return real_time_session_.GetGpuTelemetryPeriod();
  }

  void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu) {
    // Only the real time trace uses the control libary interface
    real_time_session_.SetCpu(pCpu);
  }

  HANDLE GetStreamingStartHandle() {
    // Only the real time trace uses the control libary interface
    return real_time_session_.GetStreamingStartHandle();
  }

  int GetActiveStreams() {
    // Only the real time trace uses the control libary interface
    return real_time_session_.GetActiveStreams();
  }

  void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc) {
    // the real time trace session uses the control library
    // interface
    return real_time_session_.SetPowerTelemetryContainer(ptc);
  }

 private:
  PresentMonSession real_time_session_;
  PresentMonSession etl_session_;
};