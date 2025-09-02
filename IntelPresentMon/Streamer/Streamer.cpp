// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
// Streamer.cpp : Defines the functions for the static library.

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <algorithm>

#include "Streamer.h"
#include <functional>
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include "../PresentMonService/CliOptions.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/log/GlogShim.h"

static const std::chrono::milliseconds kTimeoutLimitMs =
    std::chrono::milliseconds(500);

Streamer::Streamer()
    : shared_mem_size_(kBufSize),
    start_qpc_(0),
    write_timedout_(false),
    mapfileNamePrefix_{ kGlobalPrefix }
{
    if (clio::Options::IsInitialized()) {
        mapfileNamePrefix_ = clio::Options::Get().nsmPrefix.AsOptional().value_or(mapfileNamePrefix_);
    }
}

void Streamer::WriteFrameData(
    uint32_t process_id, PmNsmFrameData* data,
    std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
        gpu_telemetry_cap_bits,
    std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
        cpu_telemetry_cap_bits) {
    if (data == NULL) {
        LOG(ERROR) << "Invalid data.";
        return;
    }

    // Lock the nsm mutex as stop streaming calls can occur at any time
    // and destory the named shared memory during writing of frame data.
    std::lock_guard<std::mutex> lock(nsm_map_mutex_);
    auto iter = process_shared_mem_map_.find(process_id);
    if (iter == process_shared_mem_map_.end()) {
        LOG(INFO) << "Corresponding named shared memory doesn't exist. Please call StartStreaming(process_id) first.";

        return;
    }

    auto shared_mem = iter->second.get();
    
    // Record start time if it's the first frame.
    if (shared_mem->IsEmpty()) {
      shared_mem->RecordFirstFrameTime(data->present_event.PresentStartTime);
    }
    shared_mem->WriteTelemetryCapBits(gpu_telemetry_cap_bits,
                                      cpu_telemetry_cap_bits);
    shared_mem->WriteFrameData(data);
}

void Streamer::CopyFromPresentMonPresentEvent(
    PresentEvent* present_event, PmNsmPresentEvent* nsm_present_event) {
    if (present_event == nullptr || nsm_present_event == nullptr) {
      return;
    }

    nsm_present_event->PresentStartTime = present_event->PresentStartTime;
    nsm_present_event->ProcessId = present_event->ProcessId;
    nsm_present_event->ThreadId = present_event->ThreadId;
    nsm_present_event->TimeInPresent = present_event->TimeInPresent;
    nsm_present_event->GPUStartTime = present_event->GPUStartTime;
    nsm_present_event->ReadyTime = present_event->ReadyTime;
    nsm_present_event->GPUDuration = present_event->GPUDuration;
    nsm_present_event->GPUVideoDuration = present_event->GPUVideoDuration;
    nsm_present_event->InputTime = present_event->InputTime;
    nsm_present_event->MouseClickTime = present_event->MouseClickTime;

    nsm_present_event->AppPropagatedPresentStartTime = present_event->AppPropagatedPresentStartTime;
    nsm_present_event->AppPropagatedTimeInPresent = present_event->AppPropagatedTimeInPresent;
    nsm_present_event->AppPropagatedGPUStartTime = present_event->AppPropagatedGPUStartTime;
    nsm_present_event->AppPropagatedReadyTime = present_event->AppPropagatedReadyTime;
    nsm_present_event->AppPropagatedGPUDuration = present_event->AppPropagatedGPUDuration;
    nsm_present_event->AppPropagatedGPUVideoDuration = present_event->AppPropagatedGPUVideoDuration;

    nsm_present_event->AppSleepStartTime = present_event->AppSleepStartTime;
    nsm_present_event->AppSleepEndTime = present_event->AppSleepEndTime;
    nsm_present_event->AppSimStartTime = present_event->AppSimStartTime;
    nsm_present_event->AppSimEndTime = present_event->AppSimEndTime;
    nsm_present_event->AppRenderSubmitStartTime = present_event->AppRenderSubmitStartTime;
    nsm_present_event->AppRenderSubmitEndTime = present_event->AppRenderSubmitEndTime;
    nsm_present_event->AppPresentStartTime = present_event->AppPresentStartTime;
    nsm_present_event->AppPresentEndTime = present_event->AppPresentEndTime;
    nsm_present_event->AppInputTime = present_event->AppInputSample.first;
    nsm_present_event->AppInputType = present_event->AppInputSample.second;

    nsm_present_event->PclInputPingTime = present_event->PclInputPingTime;
    nsm_present_event->PclSimStartTime = present_event->PclSimStartTime;

    nsm_present_event->FlipDelay = present_event->FlipDelay;
    nsm_present_event->FlipToken = present_event->FlipToken;

    nsm_present_event->SwapChainAddress = present_event->SwapChainAddress;
    nsm_present_event->SyncInterval = present_event->SyncInterval;
    nsm_present_event->PresentFlags = present_event->PresentFlags;

    nsm_present_event->DisplayedCount = (uint32_t) min(present_event->Displayed.size(), _countof(nsm_present_event->Displayed_ScreenTime));
    for (uint32_t i = 0; i < nsm_present_event->DisplayedCount; ++i) {
        nsm_present_event->Displayed_ScreenTime[i] = present_event->Displayed[i].second;
        nsm_present_event->Displayed_FrameType[i] = present_event->Displayed[i].first;
    }

    nsm_present_event->CompositionSurfaceLuid =
        present_event->CompositionSurfaceLuid;
    nsm_present_event->Win32KPresentCount = present_event->Win32KPresentCount;
    nsm_present_event->Win32KBindId = present_event->Win32KBindId;
    nsm_present_event->DxgkPresentHistoryToken =
        present_event->DxgkPresentHistoryToken;
    nsm_present_event->DxgkPresentHistoryTokenData =
        present_event->DxgkPresentHistoryTokenData;
    nsm_present_event->DxgkContext = present_event->DxgkContext;
    nsm_present_event->Hwnd = present_event->Hwnd;
    nsm_present_event->QueueSubmitSequence = present_event->QueueSubmitSequence;
    nsm_present_event->RingIndex =
        present_event->RingIndex;

    nsm_present_event->DestWidth = present_event->DestWidth;
    nsm_present_event->DestHeight = present_event->DestHeight;
    nsm_present_event->DriverThreadId = present_event->DriverThreadId;

    nsm_present_event->FrameId = present_event->FrameId;

    nsm_present_event->Runtime = present_event->Runtime;
    nsm_present_event->PresentMode = present_event->PresentMode;
    nsm_present_event->FinalState = present_event->FinalState;
    nsm_present_event->InputType = present_event->InputType;

    nsm_present_event->SupportsTearing = present_event->SupportsTearing;
    nsm_present_event->WaitForFlipEvent = present_event->WaitForFlipEvent;
    nsm_present_event->WaitForMPOFlipEvent = present_event->WaitForMPOFlipEvent;
    nsm_present_event->SeenDxgkPresent = present_event->SeenDxgkPresent;
    nsm_present_event->SeenWin32KEvents = present_event->SeenWin32KEvents;
    nsm_present_event->SeenInFrameEvent = present_event->SeenInFrameEvent;
    nsm_present_event->GpuFrameCompleted = present_event->GpuFrameCompleted;
    nsm_present_event->IsCompleted = present_event->IsCompleted;
    nsm_present_event->IsLost = present_event->IsLost;
    nsm_present_event->PresentInDwmWaitingStruct =
        present_event->PresentInDwmWaitingStruct;

    return;
}

void Streamer::ProcessPresentEvent(
    PresentEvent* present_event,
    PresentMonPowerTelemetryInfo* power_telemetry_info,
    CpuTelemetryInfo* cpu_telemetry_info, uint64_t last_present_qpc,
    uint64_t last_displayed_qpc, std::wstring app_name,
    std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
        gpu_telemetry_cap_bits,
    std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
        cpu_telemetry_cap_bits) {
    uint32_t process_id = present_event->ProcessId;

    // Lock the nsm mutex as stop streaming calls can occur at any time
    // and destroy the named shared memory during writing of frame data.
    std::lock_guard<std::mutex> lock(nsm_map_mutex_);

    // Search for the requested process
    auto iter = process_shared_mem_map_.find(process_id);
    NamedSharedMem* process_nsm = nullptr;
    if (iter != process_shared_mem_map_.end()) {
      process_nsm = iter->second.get();
      // Record start time if it's the first frame.
      if (process_nsm->IsEmpty()) {
          if (start_qpc_ && process_nsm->GetHeader()->isPlayback) {
              process_nsm->RecordFirstFrameTime(start_qpc_);
          }
          else {
              process_nsm->RecordFirstFrameTime(present_event->PresentStartTime);
          }
      }
    }

    // In addition search for the stream all process
    auto stream_all_iter = process_shared_mem_map_.find(
        (uint32_t)StreamPidOverride::kStreamAllPid);
    NamedSharedMem* stream_all_nsm = nullptr;
    if (stream_all_iter != process_shared_mem_map_.end()) {
      stream_all_nsm = stream_all_iter->second.get();
      // Record start time if it's the first frame.
      if (stream_all_nsm->IsEmpty()) {
        if (start_qpc_ && stream_all_nsm->GetHeader()->isPlayback) {
            stream_all_nsm->RecordFirstFrameTime(start_qpc_);
        }
        else {
            stream_all_nsm->RecordFirstFrameTime(present_event->PresentStartTime);
        }
      }
    }

    if ((process_nsm == nullptr) && (stream_all_nsm == nullptr)) {
      // process is not being monitored. Skip.
      return;
    }

    PmNsmFrameData data = {};
    // Copy the passed in PresentEvent data into the PmNsmFrameData
    // structure.
    CopyFromPresentMonPresentEvent(present_event, &data.present_event);
    // Now update the necessary qpcs and application name which
    // reside AFTER the PresentEvent members and hence were not
    // updated in the copy above.
    data.present_event.last_present_qpc = last_present_qpc;
    data.present_event.last_displayed_qpc = last_displayed_qpc;
    auto appNameNarrow = pmon::util::str::ToNarrow(app_name);
    std::size_t length = appNameNarrow.copy(data.present_event.application, appNameNarrow.size());
    data.present_event.application[length] = '\0';
    // Now copy the power telemetry data
    memcpy_s(&data.power_telemetry, sizeof(PresentMonPowerTelemetryInfo),
             power_telemetry_info, sizeof(PresentMonPowerTelemetryInfo));
    // Finally copy the cpu telemetry data
    memcpy_s(&data.cpu_telemetry, sizeof(CpuTelemetryInfo), cpu_telemetry_info,
             sizeof(CpuTelemetryInfo));

    if (process_nsm) {
      auto pHdr = process_nsm->GetHeader();
      // block here if nsm is full and backpressure is enabled (only in playback modes)
      if (pHdr->isPlaybackBackpressured && process_nsm->IsFull()) {
          const auto start = std::chrono::high_resolution_clock::now();
          do {
              const auto now = std::chrono::high_resolution_clock::now();
              const auto time_elapsed = now - start;
              LOG(INFO) << "NSM is full ...";
              using namespace std::literals;
              std::this_thread::sleep_for(25ms);

              if (time_elapsed >= kTimeoutLimitMs) {
                  LOG(ERROR) << "\nServer data write timed out.";
                  write_timedout_ = true;
                  return;
              }
          } while (process_nsm->IsFull());
      }
      process_nsm->WriteTelemetryCapBits(gpu_telemetry_cap_bits,
                                         cpu_telemetry_cap_bits);
      process_nsm->WriteFrameData(&data);
    }

    if (stream_all_nsm) {
      stream_all_nsm->WriteTelemetryCapBits(gpu_telemetry_cap_bits,
                                            cpu_telemetry_cap_bits);
      stream_all_nsm->WriteFrameData(&data);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief StartStreaming with target process_id
/// @return Returns corresponding mapfile_name string. If mapfile_name is empty, it means process is not found. 
/// 
  PM_STATUS Streamer::StartStreaming(uint32_t client_process_id,
                                   uint32_t target_process_id,
                                   std::string& mapfile_name,
      bool isPlayback,
      bool isPlaybackPaced,
      bool isPlaybackRetimed,
      bool isPlaybackBackpressured,
      bool isPlaybackResetOldest) {
    auto target_range = client_map_.equal_range(client_process_id);
    auto found = std::any_of(target_range.first, target_range.second,
                             [target_process_id](auto client_entry) {
                               return target_process_id == client_entry.second;
                             });
    if (found) {
      return PM_STATUS::PM_STATUS_ALREADY_TRACKING_PROCESS;
    }

    uint64_t mem_size = kBufSize;      
    #define _CRT_SECURE_NO_WARNINGS
    const char* name = "PM2_NSM_SIZE";
    char* pValue = nullptr;
    size_t len;
    errno_t err = _dupenv_s(&pValue, &len, name);
    if (err || pValue == nullptr) {
      LOG(INFO) << "Use default NSM size : " << kBufSize;
    } else {
      LOG(INFO) << "PM2_NSM_SIZE = " << pValue;
      char* pEnd;
      mem_size = strtoull(pValue, &pEnd,10);
    }
    free(pValue);

    // create new shared mem for particular process id
    if (CreateNamedSharedMemory(target_process_id, mem_size,
        isPlayback,
        isPlaybackPaced,
        isPlaybackRetimed,
        isPlaybackBackpressured,
        isPlaybackResetOldest) == false) {
      return PM_STATUS::PM_STATUS_UNABLE_TO_CREATE_NSM;
    }

    client_map_.insert(std::make_pair(client_process_id, target_process_id));
    LOG(INFO) << "Started streaming for process id:" << target_process_id;    
    mapfile_name = GetMapFileName(target_process_id);

    return PM_STATUS::PM_STATUS_SUCCESS;
}

// Update the named shared memory process attachments. If the number of
// attachments drops to zero release the NSM. Function assumes the
// NSM map mutex has been called PRIOR to calling this function.
bool Streamer::UpdateNSMAttachments(uint32_t process_id, int& ref_count) {
  ref_count = 0;
  auto iter = process_shared_mem_map_.find(process_id);
  if (iter != process_shared_mem_map_.end()) {
    // Check if refcount is going to be zero
    if (iter->second->GetRefCount() > 1) {
      iter->second->DecrementRefcount();
      ref_count = iter->second->GetRefCount();
    } else {
      iter->second->NotifyProcessKilled();
      process_shared_mem_map_.erase(std::move(iter));
      ref_count = 0;
    }
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Stop streaming with target process_id. 
/// Decrement refcount if more than one client is attached to it. 
/// Otherwise destroy corresponding mapfilename
/// 
void Streamer::StopStreaming(uint32_t process_id) {
  // Lock the nsm map mutex as stop streaming calls can occur at any time
  // from both the client and the output thread when it detects processes
  // have terminated
  std::lock_guard<std::mutex> lock(nsm_map_mutex_);

  int ref_count = 0;
  // Check to see if the incoming process id is a client process id
  if (client_map_.contains(process_id) == false) {
    // Not a client process id, assume a target process. Need to remove
    // the NSM.
    bool status = UpdateNSMAttachments(process_id, ref_count);
    while ((status == true) && (ref_count > 0)) {
      status = UpdateNSMAttachments(process_id, ref_count);
    }
    if ((status == true) && (ref_count == 0)) {
      // If the passed in target process id resulted in a destruction of the
      // named shared memory then go through the client maps and remove
      // any references to the target process.
      for (auto i = client_map_.begin(); i != client_map_.end();) {
        if (i->second == process_id) {
          i = client_map_.erase(i);
        } else {
          ++i;
        }
      }
    }
  } else {
    auto client_range = client_map_.equal_range(process_id);
    for (auto i = client_range.first; i != client_range.second; ++i) {
      UpdateNSMAttachments(i->second, ref_count);
    }
    client_map_.erase(client_range.first, client_range.second);
  }
  return;
}

void Streamer::StopStreaming(uint32_t client_process_id,
    uint32_t target_process_id) {
  // Lock the nsm map mutex as stop streaming calls can occur at any time
  // from both the client and the output thread when it detects processes
  // have terminated
  std::lock_guard<std::mutex> lock(nsm_map_mutex_);
  
  int ref_count = 0;
  bool status = UpdateNSMAttachments(target_process_id, ref_count);
  if ((status == true) && (ref_count == 0)) {
    // If the passed in target process id resulted in the destruction of the
    // named shared memory then go through the client maps and remove
    // any references to the target process.
    for (auto i = client_map_.begin(); i != client_map_.end();) {
      if (i->second == target_process_id) {
        i = client_map_.erase(i);
      } else {
        ++i;
      }
    }
  } else if ((status == true) && (ref_count > 0)) {
    // Succesfully found the NSM of the target process id but other clients
    // are still monitoring it. Only remove it from this clients mapping.
    for (auto i = client_map_.begin(); i != client_map_.end();) {
      if ((i->first == client_process_id) && (i->second == target_process_id)) {
        i = client_map_.erase(i);
      } else {
        ++i;
      }
    }
  }
  return;
}

void Streamer::StopAllStreams() {
  // Lock the nsm map mutex to ensure we don't destroy the NSMs
  // while writing frame data.
  std::lock_guard<std::mutex> lock(nsm_map_mutex_);
  for (auto const& it : process_shared_mem_map_) {
    it.second->NotifyProcessKilled();
  }
  process_shared_mem_map_.clear();
  client_map_.clear();
  write_timedout_ = false;
}

bool Streamer::CreateNamedSharedMemory(DWORD process_id,
    uint64_t nsm_size_in_bytes,
    bool isPlayback,
    bool isPlaybackPaced,
    bool isPlaybackRetimed,
    bool isPlaybackBackpressured,
    bool isPlaybackResetOldest) {
  std::string mapfile_name = mapfileNamePrefix_ + std::to_string(process_id);

  std::lock_guard<std::mutex> lock(nsm_map_mutex_);
  auto iter = process_shared_mem_map_.find(process_id);
  if (iter == process_shared_mem_map_.end()) {
    auto nsm =
        std::make_unique<NamedSharedMem>(std::move(mapfile_name), nsm_size_in_bytes,
            isPlayback,
            isPlaybackPaced,
            isPlaybackRetimed,
            isPlaybackBackpressured,
            isPlaybackResetOldest);
    if (nsm->IsNSMCreated()) {
        process_shared_mem_map_.emplace(process_id, std::move(nsm));
        return true;
    } else {
        LOG(INFO) << "Unabled to create NSM for process id:" << process_id;
        return false;
    }

  } else {
    LOG(INFO) << "Shared mem for process(" << process_id
              << ") already exists. Increment recount.";
    iter->second->IncrementRefcount();
    return true;
  }
}

std::string Streamer::GetMapFileName(DWORD process_id) {
  std::lock_guard<std::mutex> lock(nsm_map_mutex_);
  auto iter = process_shared_mem_map_.find(process_id);
  std::string mapfile_name;

  if (iter != process_shared_mem_map_.end()) {
    mapfile_name = iter->second->GetMapFileName();
  }

  return mapfile_name;
}