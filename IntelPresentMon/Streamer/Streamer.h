// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <windows.h>
#include <thread>
#include <string>
#include <map>
#include <set>

#include "../PresentMonUtils/StreamFormat.h"
#include "gtest/gtest.h"
#include "NamedSharedMemory.h"

class Streamer {
 public:
     Streamer();
  ~Streamer() = default;

  // Client API, start streaming data for process by name
  PM_STATUS StartStreaming(uint32_t client_process_id,
                           uint32_t target_process_id,
                           std::string& mapfile_name,
      bool isPlayback,
      bool isPlaybackPaced,
      bool isPlaybackRetimed,
      bool isPlaybackBackpressured,
      bool isPlaybackResetOldest);

  // Set streaming mode. Default value is real time streaming for single process.
  void StopStreaming(uint32_t client_process_id, uint32_t target_process_id);
  void StopStreaming(uint32_t process_id);
  void StopAllStreams();
  // Last producer and last consumer are internal fields
  // Remove for public build
  void ProcessPresentEvent(
      PresentEvent* present_event,
      PresentMonPowerTelemetryInfo* power_telemetry_info,
      CpuTelemetryInfo* cpu_telemetry_info, uint64_t last_present_qpc,
      uint64_t last_displayed_qpc, std::wstring app_name,
      std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
          gpu_telemetry_cap_bits,
      std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
          cpu_telemetry_cap_bits);

  void WriteFrameData(
      uint32_t process_id, PmNsmFrameData* data,
      std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
          gpu_telemetry_cap_bits,
      std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
          cpu_telemetry_cap_bits);
  std::string GetMapFileName(DWORD process_id);
  std::set<uint32_t> GetActiveStreamPids() const;
  void SetStartQpc(uint64_t start_qpc) { start_qpc_ = start_qpc; };
  bool IsTimedOut() { return write_timedout_; };
  int NumActiveStreams() { return (int)process_shared_mem_map_.size(); }

 private:
  FRIEND_TEST(NamedSharedMemoryTest, CreateNamedSharedMemory);
  FRIEND_TEST(NamedSharedMemoryTestCustomSize, CreateNamedSharedMemory);
  bool CreateNamedSharedMemory(DWORD process_id, uint64_t nsm_size_in_bytes,
      bool isPlayback,
      bool isPlaybackPaced,
      bool isPlaybackRetimed,
      bool isPlaybackBackpressured,
      bool isPlaybackResetOldest);
  void CopyFromPresentMonPresentEvent(PresentEvent* present_event,
                                      PmNsmPresentEvent* nsm_present_event);
  bool UpdateNSMAttachments(uint32_t process_id, int& ref_count);
  std::string mapfileNamePrefix_;
  // Shared mem buffer map of process id and share mem handle
  std::map<DWORD, std::unique_ptr<NamedSharedMem>> process_shared_mem_map_;
  std::multimap<uint32_t, uint32_t> client_map_;
  uint64_t shared_mem_size_;
  uint64_t start_qpc_;
  // This flag is currently used during etl processing. If the client 
  // misbehaved or died such that streamer is blocked to write frame data, 
  // write_timedout_ would be set to true and etl_session_ of PresentMon would 
  // stop the trace session. 
  bool write_timedout_;
  mutable std::mutex nsm_map_mutex_;
};
