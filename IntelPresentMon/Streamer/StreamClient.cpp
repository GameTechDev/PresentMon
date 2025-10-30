// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "StreamClient.h"
#include "../PresentMonUtils/QPCUtils.h"
#include "../PresentMonUtils/PresentDataUtils.h"

#include "../CommonUtilities/log/GlogShim.h"

StreamClient::StreamClient()
    : initialized_(false),
      next_dequeue_idx_(0),
      recording_frame_data_(false),
      current_dequeue_frame_num_(0) {}

StreamClient::StreamClient(std::string mapfile_name, bool is_etl_stream_client)
    : next_dequeue_idx_(0),
      recording_frame_data_(false),
      current_dequeue_frame_num_(0) {
  Initialize(std::move(mapfile_name));
}

StreamClient::~StreamClient() {
}

void StreamClient::Initialize(std::string mapfile_name) {
	shared_mem_view_ = std::make_unique<NamedSharedMem>();
	shared_mem_view_->OpenSharedMemView(mapfile_name);
	mapfile_name_ = std::move(mapfile_name);

	// Query qpc frequency
    if (!QueryPerformanceFrequency(&qpcFrequency_)) {
        pmlog_error("QueryPerformanceFrequency failed").hr();
    }

	initialized_ = true;
	LOG(INFO) << "Stream client initialized.";
}

void StreamClient::CloseSharedMemView() { shared_mem_view_.reset(nullptr); }

PmNsmFrameData* StreamClient::ReadLatestFrame() {
  PmNsmFrameData* data = nullptr;

  if (shared_mem_view_ == nullptr) {
    LOG(ERROR)
        << "Shared mem view is null. Initialze client with mapfile name.";
    return data;
  }

  if (shared_mem_view_->IsEmpty()) {
    LOG(INFO) << "Shared mem view is empty. start ETW tracing or wait for "
                 "more data";
    return data;
  }

  uint64_t index = GetLatestFrameIndex();

  return ReadFrameByIdx(index, true);
}

PmNsmFrameData* StreamClient::ReadFrameByIdx(uint64_t frame_idx, bool checked) {
  PmNsmFrameData* data = nullptr;
  uint64_t read_offset = 0;

  if (shared_mem_view_ == nullptr) {
    LOG(ERROR)
        << "Shared mem view is null. Initialze client with mapfile name.";
    return nullptr;
  }

  if (shared_mem_view_->IsEmpty()) {
      if (checked) {
          pmlog_warn("Trying to read from empty nsm ring").pmwatch(frame_idx);
      }
    return nullptr;
  }

  auto p_header = shared_mem_view_->GetHeader();

  if (!p_header->process_active) {
    pmlog_warn("Process is not active. Shared mem view to be destroyed.");
    CloseSharedMemView();
    return nullptr;
  }

  if (frame_idx > p_header->max_entries - 1) {
      pmlog_warn("Bad out of bounds index in circular buffer").pmwatch(frame_idx);
      return nullptr;
  }

  if (checked) {
    // when ring is full, every frame is valid for reading
    // in realtime usage (head not updated on read), this is always the case once the ring wraps once
    if (!shared_mem_view_->IsFull()) {
        bool invalid = false;
        // when ring is not full, there are two cases (head before tail, or vice versa)
        // head before tail (invalid range potentially non-contiguous/wrapping)
        if (p_header->tail_idx > p_header->head_idx) {
            if (frame_idx >= p_header->tail_idx || frame_idx < p_header->head_idx) {
                invalid = true;
            }
        }
        else { // tail before head (invalid range contiguous in middle)
            if (frame_idx >= p_header->tail_idx && frame_idx < p_header->head_idx) {
                invalid = true;
            }
        }
        if (invalid) {
            pmlog_warn("Invalid frame idx").pmwatch(frame_idx);
            return nullptr;
        }
    }
  }

  read_offset = frame_idx * sizeof(PmNsmFrameData) + shared_mem_view_->GetBaseOffset();

  data = reinterpret_cast<PmNsmFrameData*>(
      static_cast<char*>((shared_mem_view_->GetBuffer())) + read_offset);

  return data;
}

void StreamClient::PeekNextFrames(const PmNsmFrameData** pNextFrame,
                                  const PmNsmFrameData** pNextDisplayedFrame)
{
    *pNextFrame          = nullptr;
    *pNextDisplayedFrame = nullptr;
    if (recording_frame_data_) {
        auto nsm_view = GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            // Service destroyed the named shared memory.
            return;
        }

        uint64_t peekIndex{ next_dequeue_idx_ };
        auto pTempFrameData = ReadFrameByIdx(peekIndex, false);
        *pNextFrame = pTempFrameData;
        while (pTempFrameData) {
            if (pTempFrameData->present_event.FinalState == PresentResult::Presented &&
                pTempFrameData->present_event.DisplayedCount > 0) {
                *pNextDisplayedFrame = pTempFrameData;
                return;
            }
            // advance to next frame with circular buffer wrapping behavior
            peekIndex = (peekIndex + 1) % nsm_view->GetHeader()->max_entries;
            pTempFrameData = ReadFrameByIdx(peekIndex, false);
        }
    }
    return;
}

bool StreamClient::IsAppPresentedFrame(const PmNsmFrameData* frame) const {
    if (!frame || frame->present_event.DisplayedCount == 0) {
        return false;
    }
    size_t lastDisplayedIndex = frame->present_event.DisplayedCount - 1;
    return frame->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::NotSet ||
        frame->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::Application;
}

bool StreamClient::IsDisplayedFrame(const PmNsmFrameData* frame) const {
    return frame && frame->present_event.FinalState == PresentResult::Presented &&
        frame->present_event.DisplayedCount > 0;
}

bool StreamClient::IsAppDisplayedFrame(const PmNsmFrameData* frame) const {
    if (!frame || frame->present_event.DisplayedCount == 0) {
        return false;
    }
    size_t lastDisplayedIndex = frame->present_event.DisplayedCount - 1;
    return frame->present_event.FinalState == PresentResult::Presented &&
        (frame->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::NotSet ||
            frame->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::Application);
}

// Function to peek previous frames from the current frame
void StreamClient::PeekPreviousFrames(const PmNsmFrameData** pFrameDataOfLastPresented,
                                      const PmNsmFrameData** pFrameDataOfLastAppPresented,
                                      const PmNsmFrameData** pFrameDataOfLastDisplayed,
                                      const PmNsmFrameData** pFrameDataOfLastAppDisplayed,
                                      const PmNsmFrameData** pFrameDataOfPreviousAppFrameOfLastAppDisplayed)
{
    *pFrameDataOfLastPresented         = nullptr;
    *pFrameDataOfLastAppPresented      = nullptr;
    *pFrameDataOfLastDisplayed         = nullptr;
    *pFrameDataOfLastAppDisplayed      = nullptr;
    *pFrameDataOfPreviousAppFrameOfLastAppDisplayed = nullptr;
    if (recording_frame_data_) {
        auto nsm_view = GetNamedSharedMemView();
        auto nsm_hdr = nsm_view->GetHeader();
        if (!nsm_hdr->process_active) {
            // Service destroyed the named shared memory.
            return;
        }

        // if nsm is not fully initialized, last frame to read is at idx 0, so stop condition is when we wrap to last idx
        // otherwise, last frame to read is the one that follows the latest one (which is the oldest one, next one to be overwritten)
        const auto stopIdx = nsm_view->HasUninitializedFrames() ? nsm_hdr->max_entries - 1 : this->GetLatestFrameIndex();
        // here, peek index is starting at the frame which follows the current one
        // this frame is guaranteed to be valid since peekNext call prior to this would exit early if it were not
        uint64_t peekIndex{ next_dequeue_idx_ };
        uint32_t numFramesTraversed = 0;
        while (true) {
            // seek back one frame with ring buffer cycling behavior
            peekIndex = peekIndex == 0 ? (nsm_hdr->max_entries - 1) : (peekIndex - 1);
            // exit when we hit stopIdx (which is the first frame encountered we should *not* use)
            if (peekIndex == stopIdx) {
                return;
            }
            numFramesTraversed++;
            // TODO: we can improve checking by independently enabling for forward and backward cases
            auto pTempFrameData = ReadFrameByIdx(peekIndex, false);
            // We need to traverse back two frames from the next_dequeue_idx to
            // get to the start of the previous frames
            if (numFramesTraversed > 1) {
                if (pTempFrameData) {
                    if (numFramesTraversed == 2) {
                        // If we made it here we were able to go back two frames
                        // from the current next_deque_index. This is the frame
                        // data of the last presented frame before the current frame.
                        *pFrameDataOfLastPresented = pTempFrameData;
                    }
                    if (*pFrameDataOfLastAppPresented == nullptr) {
                        // If we haven't found the last app presented frame check to see if
                        // the current one is presented. This could point to the same frame
                        // as the last presented one above.
                        auto displayedCount = pTempFrameData->present_event.DisplayedCount;
                        if (displayedCount > 0) {
                            if (pTempFrameData->present_event.Displayed_FrameType[displayedCount - 1] == FrameType::NotSet ||
                                pTempFrameData->present_event.Displayed_FrameType[displayedCount - 1] == FrameType::Application) {
                                *pFrameDataOfLastAppPresented = pTempFrameData;
                            }
                        } else {
                            // If the displayed count is 0 we assume this is an app frame
                            *pFrameDataOfLastAppPresented = pTempFrameData;
                        }
                    }
                    if (*pFrameDataOfLastDisplayed == nullptr) {
                        // If we haven't found the last displayed frame check to see if
                        // the current one is presented. This could point to the same frame
                        // as the last presented one above.
                        if (pTempFrameData->present_event.FinalState == PresentResult::Presented &&
                            pTempFrameData->present_event.DisplayedCount > 0) {
                            *pFrameDataOfLastDisplayed = pTempFrameData;
                        }
                    }
                    if (*pFrameDataOfLastAppDisplayed == nullptr) {
                        // If we haven't found the last displayed app frame check to see if
                        // the current one is presented. This could point to the same frame
                        // as the last displayed frame above.
                        if (pTempFrameData->present_event.DisplayedCount > 0) {
                            size_t lastDisplayedIndex = pTempFrameData->present_event.DisplayedCount - 1;
                            if (pTempFrameData->present_event.FinalState == PresentResult::Presented &&
                                (pTempFrameData->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::NotSet ||
                                    pTempFrameData->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::Application)) {
                                *pFrameDataOfLastAppDisplayed = pTempFrameData;
                            }
                        }
                    } else {
                        // If we have found the last displayed app frame we now need to find the previously
                        // presented frame to determine the cpu start time. The frame does not need to be
                        // displayed but must be from the application
                        if (pTempFrameData->present_event.DisplayedCount > 0) {
                            size_t lastDisplayedIndex = pTempFrameData->present_event.DisplayedCount - 1;
                            if (pTempFrameData->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::NotSet ||
                                pTempFrameData->present_event.Displayed_FrameType[lastDisplayedIndex] == FrameType::Application) {
                                *pFrameDataOfPreviousAppFrameOfLastAppDisplayed = pTempFrameData;
                            }
                        } else {
                            // If the displayed count is 0 we assume this is an app frame
                            *pFrameDataOfPreviousAppFrameOfLastAppDisplayed = pTempFrameData;
                        }
                    }
                    if (*pFrameDataOfLastPresented != nullptr &&
                        *pFrameDataOfLastAppPresented != nullptr &&
                        *pFrameDataOfLastDisplayed != nullptr &&
                        *pFrameDataOfPreviousAppFrameOfLastAppDisplayed != nullptr &&
                        *pFrameDataOfLastAppDisplayed != nullptr) {
                        // We have found all the frames we need to find. We can exit early.
                        return;
                    }
                }
                else {
                    // The read frame data is null.
                    // TODO log this as the index is supposed to be valid!
                    return;
                }
            }
        }
    }
    return;
}


PM_STATUS StreamClient::ConsumePtrToNextNsmFrameData(const PmNsmFrameData** pNsmData,
                                                     const PmNsmFrameData** pNextFrame,
                                                     const PmNsmFrameData** pFrameDataOfNextDisplayed,
                                                     const PmNsmFrameData** pFrameDataOfLastPresented,
                                                     const PmNsmFrameData** pFrameDataOfLastAppPresented,
                                                     const PmNsmFrameData** pFrameDataOfLastDisplayed,
                                                     const PmNsmFrameData** pFrameDataOfLastAppDisplayed,
                                                     const PmNsmFrameData** pFrameDataOfPreviousAppFrameOfLastAppDisplayed)
{
    if (pNsmData == nullptr || pNextFrame == nullptr || pFrameDataOfNextDisplayed == nullptr ||
        pFrameDataOfLastPresented == nullptr || pFrameDataOfLastAppPresented == nullptr ||
        pFrameDataOfLastDisplayed == nullptr || pFrameDataOfLastAppDisplayed == nullptr ||
        pFrameDataOfPreviousAppFrameOfLastAppDisplayed == nullptr) {
        return PM_STATUS::PM_STATUS_FAILURE;
    }

    // nullify point so that if we exit early it will be null
    *pNsmData = nullptr;

    auto nsm_view = GetNamedSharedMemView();
    auto nsm_hdr = nsm_view->GetHeader();
    if (!nsm_hdr->process_active) {
        // Service destroyed the named shared memory.
        return PM_STATUS::PM_STATUS_INVALID_PID;
    }

    if (recording_frame_data_ == false) {
        // Get the current number of frames written and set it as the current
        // dequeue frame number. This will be used to track data overruns if
        // the client does not read data fast enough.
        recording_frame_data_ = true;
        if (nsm_hdr->isPlaybackResetOldest) {
            // during playback it is desirable to start at the very first frame even if we start consuming late
            if (nsm_view->IsFull()) {
                // if nsm is full, we have to dequeue all frames currently in the buffer
                current_dequeue_frame_num_ = nsm_hdr->num_frames_written - nsm_hdr->max_entries;
            }
            else {
                // if nsm is not full, then we must be in the initial state so set to zero (start from the beginning)
                current_dequeue_frame_num_ = 0;
            }
            // always start from the head (oldest frame)
            next_dequeue_idx_ = nsm_hdr->head_idx;
        }
        else {
            // at start or after overrun, reset to most recent frame data
            current_dequeue_frame_num_ = nsm_hdr->num_frames_written;
            next_dequeue_idx_ = (nsm_hdr->tail_idx + 1) % nsm_hdr->max_entries;
        }
    }

    // Check to see if the number of pending read frames is greater
    // than the maximum number of entries. If so we have lost frame
    // data
    uint64_t num_pending_frames = CheckPendingReadFrames();
    if (num_pending_frames > nsm_hdr->max_entries) {
        recording_frame_data_ = false;
        return PM_STATUS::PM_STATUS_SUCCESS;
    }
    else if (num_pending_frames < 2) {
        // we need at least 2 pending frames to enable peek-ahead
        return PM_STATUS::PM_STATUS_SUCCESS;
    }

    // Set the rest of the incoming frame pointers in
    // preparation for the various frame data peeks and
    // reads
    *pNextFrame                                     = nullptr;
    *pFrameDataOfNextDisplayed                      = nullptr;
    *pFrameDataOfLastPresented                      = nullptr;
    *pFrameDataOfLastAppPresented                   = nullptr;
    *pFrameDataOfLastDisplayed                      = nullptr;
    *pFrameDataOfLastAppDisplayed                   = nullptr;
    *pFrameDataOfPreviousAppFrameOfLastAppDisplayed = nullptr;

    // First read the current frame. next_dequeue_idx_ sits
    // at next frame we need to dequeue.
    *pNsmData = ReadFrameByIdx(next_dequeue_idx_, true);
    if (*pNsmData) {
        // Good so far. Save off the queue index in case
        // we need to reset
        auto previous_dequeue_idx = next_dequeue_idx_;
        // Increment the index to the next frame to dequeue as PeekNextDisplayedFrame expects
        // the frame to be incremented. Can change this when done debugging so we don't have to
        // reset the dequeue index.
        next_dequeue_idx_ = (next_dequeue_idx_ + 1) % nsm_hdr->max_entries;
        PeekNextFrames(pNextFrame, pFrameDataOfNextDisplayed);
        if (*pFrameDataOfNextDisplayed == nullptr) {
            // We were unable to get the next displayed frame. It might not have been displayed
            // yet. Reset the next_dequeue_idx back to where we first started.
            next_dequeue_idx_ = previous_dequeue_idx;
            // Also reset the current and next frame data pointers
            *pNsmData = nullptr;
            *pNextFrame = nullptr;
            return PM_STATUS::PM_STATUS_SUCCESS;
        }
        PeekPreviousFrames(
            pFrameDataOfLastPresented,
            pFrameDataOfLastAppPresented,
            pFrameDataOfLastDisplayed,
            pFrameDataOfLastAppDisplayed,
            pFrameDataOfPreviousAppFrameOfLastAppDisplayed);

        current_dequeue_frame_num_++;
        if (nsm_hdr->isPlaybackBackpressured) {
            nsm_view->DequeueFrameData();
        }
        return PM_STATUS::PM_STATUS_SUCCESS;
    }
    else {
        return PM_STATUS::PM_STATUS_FAILURE;
    }
}

void StreamClient::CopyFrameData(uint64_t start_qpc,
                                 const PmNsmFrameData* src_frame,
                                 GpuTelemetryBitset gpu_telemetry_cap_bits,
                                 CpuTelemetryBitset cpu_telemetry_cap_bits,
                                 PM_FRAME_DATA* dst_frame) {
  memset(dst_frame, 0, sizeof(PM_FRAME_DATA));
  dst_frame->qpc_time = src_frame->present_event.PresentStartTime;

  dst_frame->ms_between_presents =
      QpcDeltaToMs(src_frame->present_event.PresentStartTime -
                       src_frame->present_event.last_present_qpc,
                   GetQpcFrequency());

  strncpy_s(dst_frame->application, src_frame->present_event.application, _TRUNCATE);

  dst_frame->process_id = src_frame->present_event.ProcessId;
  dst_frame->swap_chain_address = src_frame->present_event.SwapChainAddress;
  strncpy_s(dst_frame->runtime,
           RuntimeToString(src_frame->present_event.Runtime), _TRUNCATE);
  dst_frame->sync_interval = src_frame->present_event.SyncInterval;
  dst_frame->present_flags = src_frame->present_event.PresentFlags;

  if (src_frame->present_event.FinalState == PresentResult::Presented) {
    dst_frame->dropped = false;
    // If the last_displayed_qpc is zero this means there has not been
    // displayed frame. 
    if (src_frame->present_event.last_displayed_qpc > 0) {
      dst_frame->ms_between_display_change =
          QpcDeltaToMs(src_frame->present_event.Displayed_ScreenTime[0] -
                           src_frame->present_event.last_displayed_qpc,
                       GetQpcFrequency());
    }
    dst_frame->ms_until_displayed = QpcDeltaToMs(src_frame->present_event.Displayed_ScreenTime[0] -
                         src_frame->present_event.PresentStartTime,
        GetQpcFrequency());
  } else {
    dst_frame->dropped = true;
  }

  dst_frame->time_in_seconds = QpcDeltaToSeconds(
      src_frame->present_event.PresentStartTime - start_qpc, GetQpcFrequency());

  dst_frame->ms_in_present_api =
        QpcDeltaToMs(src_frame->present_event.TimeInPresent,
                     GetQpcFrequency());

  dst_frame->allows_tearing = src_frame->present_event.SupportsTearing;
  dst_frame->present_mode =
      TranslatePresentMode(src_frame->present_event.PresentMode);

  // If the ReadyTime is BEFORE the present event start time denote this
  // with a negative render complete
  if (src_frame->present_event.ReadyTime == 0) {
    dst_frame->ms_until_render_complete = 0.0;
  } else {
    if (src_frame->present_event.ReadyTime <
        src_frame->present_event.PresentStartTime) {
      dst_frame->ms_until_render_complete =
          -1.0 * QpcDeltaToMs(src_frame->present_event.PresentStartTime -
                                  src_frame->present_event.ReadyTime,
                              GetQpcFrequency());
    } else {
      dst_frame->ms_until_render_complete =
          QpcDeltaToMs(src_frame->present_event.ReadyTime -
                           src_frame->present_event.PresentStartTime,
                       GetQpcFrequency());
    }
  }
  // If the GPUStartTime is BEFORE the present event start time denote this
  // with a negative render start
  if (src_frame->present_event.GPUStartTime == 0) {
    dst_frame->ms_until_render_start = 0.0;
  } else {
    if (src_frame->present_event.GPUStartTime <
        src_frame->present_event.PresentStartTime) {
      dst_frame->ms_until_render_start =
          -1.0 * QpcDeltaToMs(src_frame->present_event.PresentStartTime -
                                  src_frame->present_event.GPUStartTime,
                              GetQpcFrequency());

    } else {
      dst_frame->ms_until_render_start =
          QpcDeltaToMs(src_frame->present_event.GPUStartTime -
                           src_frame->present_event.PresentStartTime,
                       GetQpcFrequency());
    }
  }
  dst_frame->ms_gpu_active =
      QpcDeltaToMs(src_frame->present_event.GPUDuration, GetQpcFrequency());
  dst_frame->ms_gpu_video_active = QpcDeltaToMs(
      src_frame->present_event.GPUVideoDuration, GetQpcFrequency());
  dst_frame->ms_since_input = 0.;
  if (src_frame->present_event.InputTime != 0) {
    dst_frame->ms_since_input =
        QpcDeltaToMs(src_frame->present_event.PresentStartTime -
                         src_frame->present_event.InputTime,
                     GetQpcFrequency());
  }

  // power telemetry
  dst_frame->gpu_power_w.data = src_frame->power_telemetry.gpu_power_w;
  dst_frame->gpu_power_w.valid = gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::gpu_power)];

  dst_frame->gpu_sustained_power_limit_w.data =
      src_frame->power_telemetry.gpu_sustained_power_limit_w;
  dst_frame->gpu_sustained_power_limit_w.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_sustained_power_limit)];

  dst_frame->gpu_voltage_v.data =
      src_frame->power_telemetry.gpu_voltage_v;
  dst_frame->gpu_voltage_v.valid = gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::gpu_voltage)];

  dst_frame->gpu_frequency_mhz.data =
      src_frame->power_telemetry.gpu_frequency_mhz;
  dst_frame->gpu_frequency_mhz.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_frequency)];

  dst_frame->gpu_temperature_c.data =
      src_frame->power_telemetry.gpu_temperature_c;
  dst_frame->gpu_temperature_c.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_temperature)];

  dst_frame->gpu_utilization.data =
      src_frame->power_telemetry.gpu_utilization;
  dst_frame->gpu_utilization.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_utilization)];

  dst_frame->gpu_render_compute_utilization.data =
      src_frame->power_telemetry.gpu_render_compute_utilization;
  dst_frame->gpu_render_compute_utilization.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::gpu_render_compute_utilization)];

  dst_frame->gpu_media_utilization.data =
      src_frame->power_telemetry.gpu_media_utilization;
  dst_frame->gpu_media_utilization.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_media_utilization)];

  dst_frame->vram_power_w.data = src_frame->power_telemetry.vram_power_w;
  dst_frame->vram_power_w.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_power)];

  dst_frame->vram_voltage_v.data = src_frame->power_telemetry.vram_voltage_v;
  dst_frame->vram_voltage_v.valid = gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::vram_voltage)];

  dst_frame->vram_frequency_mhz.data = src_frame->power_telemetry.vram_frequency_mhz;
  dst_frame->vram_frequency_mhz.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::vram_frequency)];

  dst_frame->vram_effective_frequency_gbs.data =
      src_frame->power_telemetry.vram_effective_frequency_gbps;
  dst_frame->vram_effective_frequency_gbs.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_effective_frequency)];

  dst_frame->vram_temperature_c.data = src_frame->power_telemetry.vram_temperature_c;
  dst_frame->vram_temperature_c.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_temperature)];

  for (size_t i = 0; i < MAX_PM_FAN_COUNT; i++) {
    dst_frame->fan_speed_rpm[i].data = src_frame->power_telemetry.fan_speed_rpm[i];
    dst_frame->fan_speed_rpm[i].valid =
        gpu_telemetry_cap_bits[static_cast<size_t>(
            GpuTelemetryCapBits::fan_speed_0) + i];
  }

  for (size_t i = 0; i < MAX_PM_PSU_COUNT; i++) {
    dst_frame->psu_type[i].data =
        TranslatePsuType(src_frame->power_telemetry.psu[i].psu_type);
    dst_frame->psu_type[i].valid = dst_frame->psu_power[i].valid =
        gpu_telemetry_cap_bits
            [static_cast<size_t>(GpuTelemetryCapBits::psu_info_0) + i];

    dst_frame->psu_power[i].data = src_frame->power_telemetry.psu[i].psu_power;
    dst_frame->psu_power[i].valid = gpu_telemetry_cap_bits
        [static_cast<size_t>(GpuTelemetryCapBits::psu_info_0) + i];

    dst_frame->psu_voltage[i].data = src_frame->power_telemetry.psu[i].psu_voltage;
    dst_frame->psu_voltage[i].valid = gpu_telemetry_cap_bits
        [static_cast<size_t>(GpuTelemetryCapBits::psu_info_0) + i];
  }

  // Gpu memory telemetry
  dst_frame->gpu_mem_total_size_b.data =
      src_frame->power_telemetry.gpu_mem_total_size_b;
  dst_frame->gpu_mem_total_size_b.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_mem_size)];

  dst_frame->gpu_mem_used_b.data =
      src_frame->power_telemetry.gpu_mem_used_b;
  dst_frame->gpu_mem_used_b.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_mem_used)];

  dst_frame->gpu_mem_max_bandwidth_bps.data =
      src_frame->power_telemetry.gpu_mem_max_bandwidth_bps;
  dst_frame->gpu_mem_max_bandwidth_bps.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
      GpuTelemetryCapBits::gpu_mem_max_bandwidth)];

  dst_frame->gpu_mem_read_bandwidth_bps.data =
      src_frame->power_telemetry.gpu_mem_read_bandwidth_bps;
  dst_frame->gpu_mem_read_bandwidth_bps.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_mem_read_bandwidth)];

  dst_frame->gpu_mem_write_bandwidth_bps.data =
      src_frame->power_telemetry.gpu_mem_write_bandwidth_bps;
  dst_frame->gpu_mem_write_bandwidth_bps.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_mem_write_bandwidth)];

  // Throttling flags
  dst_frame->gpu_power_limited.data = src_frame->power_telemetry.gpu_power_limited;
  dst_frame->gpu_power_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_power_limited)];

  dst_frame->gpu_temperature_limited.data =
      src_frame->power_telemetry.gpu_temperature_limited;
  dst_frame->gpu_temperature_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_temperature_limited)];

  dst_frame->gpu_current_limited.data =
      src_frame->power_telemetry.gpu_current_limited;
  dst_frame->gpu_current_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_current_limited)];

  dst_frame->gpu_voltage_limited.data =
      src_frame->power_telemetry.gpu_voltage_limited;
  dst_frame->gpu_voltage_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_voltage_limited)];

  dst_frame->gpu_utilization_limited.data =
      src_frame->power_telemetry.gpu_utilization_limited;
  dst_frame->gpu_utilization_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::gpu_utilization_limited)];

  dst_frame->vram_power_limited.data =
      src_frame->power_telemetry.vram_power_limited;
  dst_frame->vram_power_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_utilization_limited)];

  dst_frame->vram_temperature_limited.data =
      src_frame->power_telemetry.vram_temperature_limited;
  dst_frame->vram_temperature_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_temperature_limited)];

  dst_frame->vram_current_limited.data =
      src_frame->power_telemetry.vram_current_limited;
  dst_frame->vram_current_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_current_limited)];

  dst_frame->vram_voltage_limited.data =
      src_frame->power_telemetry.vram_voltage_limited;
  dst_frame->vram_voltage_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_voltage_limited)];

  dst_frame->vram_utilization_limited.data =
      src_frame->power_telemetry.vram_utilization_limited;
  dst_frame->vram_utilization_limited.valid =
      gpu_telemetry_cap_bits[static_cast<size_t>(
          GpuTelemetryCapBits::vram_utilization_limited)];

  // cpu telemetry - only available in INTERNAL builds
  dst_frame->cpu_utilization.data = src_frame->cpu_telemetry.cpu_utilization;
  dst_frame->cpu_utilization.valid =
      cpu_telemetry_cap_bits[static_cast<size_t>(
          CpuTelemetryCapBits::cpu_utilization)];

  dst_frame->cpu_power_w.data = src_frame->cpu_telemetry.cpu_power_w;
  dst_frame->cpu_power_w.valid = cpu_telemetry_cap_bits[static_cast<size_t>(
      CpuTelemetryCapBits::cpu_power)];

  dst_frame->cpu_power_limit_w.data = src_frame->cpu_telemetry.cpu_power_limit_w;
  dst_frame->cpu_power_limit_w.valid =
      cpu_telemetry_cap_bits[static_cast<size_t>(
      CpuTelemetryCapBits::cpu_power_limit)];

  dst_frame->cpu_temperature_c.data = src_frame->cpu_telemetry.cpu_temperature;
  dst_frame->cpu_temperature_c.valid =
      cpu_telemetry_cap_bits[static_cast<size_t>(
          CpuTelemetryCapBits::cpu_temperature)];

  dst_frame->cpu_frequency.data = src_frame->cpu_telemetry.cpu_frequency;
  dst_frame->cpu_frequency.valid =
      cpu_telemetry_cap_bits[static_cast<size_t>(
          CpuTelemetryCapBits::cpu_frequency)];
}

uint64_t StreamClient::GetLatestFrameIndex() {
  if (shared_mem_view_->IsEmpty()) {
    return UINT_MAX;
  }

  auto p_header = shared_mem_view_->GetHeader();

  if (!p_header->process_active) {
    LOG(ERROR) << "\n Process is not active. Shared mem view to be destroyed.";
    CloseSharedMemView();
    return UINT_MAX;
  }

  if (p_header->tail_idx == 0) {
    return p_header->max_entries - 1;
  } else {
    return (p_header->tail_idx - 1);
  }
}

// Calculate the number of frames written since the last dequue
uint64_t StreamClient::CheckPendingReadFrames() {
  uint64_t num_pending_read_frames = 0;

  auto p_header = shared_mem_view_->GetHeader();
  if (p_header->num_frames_written < current_dequeue_frame_num_) {
    // Wrap case where p_header->num_frames_written has wrapped to zero
    num_pending_read_frames = ULLONG_MAX - current_dequeue_frame_num_;
    num_pending_read_frames += p_header->num_frames_written;
  } else {
    num_pending_read_frames =
        p_header->num_frames_written - current_dequeue_frame_num_;
  }

  return num_pending_read_frames;
}

std::optional<
    std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>>
StreamClient::GetGpuTelemetryCaps() {
  if (shared_mem_view_->IsEmpty()) {
    return {};
  }

  auto p_header = shared_mem_view_->GetHeader();
  if (!p_header->process_active) {
    return {};
  }

  return p_header->gpuTelemetryCapBits;
}

std::optional<
    std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>>
StreamClient::GetCpuTelemetryCaps() {
  if (shared_mem_view_->IsEmpty()) {
    return {};
  }

  auto p_header = shared_mem_view_->GetHeader();
  if (!p_header->process_active) {
    return {};
  }

  return p_header->cpuTelemetryCapBits;
}
