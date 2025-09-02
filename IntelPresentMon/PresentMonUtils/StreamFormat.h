/*
 *
 * Copyright (C) 2021,2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 *
 * File Name:  StreamFormat.h
 *
 * Abstract:   Header for Intel PresentMon Named Shared Memory IPC
 *
 */
#pragma once
#include <Windows.h>
#include <tchar.h>
#include <bitset>
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../ControlLib/PowerTelemetryProvider.h"
#include "../ControlLib/CpuTelemetryInfo.h"
#include "LegacyAPIDefines.h"

 // We use system reserved pid (0: System Idle Process, 4: System) that will
 // never show up in present mon for StreamAll and ETL PIDs
enum class StreamPidOverride : uint32_t { kStreamAllPid = 0, kEtlPid = 4 };

struct NamedSharedMemoryHeader
{
	NamedSharedMemoryHeader()
		: start_qpc(0),
		last_displayed_qpc(0),
		buf_size(0),
		max_entries(0),
		current_write_offset(0),
		num_frames_written(0),
		head_idx(0),
		tail_idx(0),
		process_active(true) {}
	// start QPC time of the very first frame recorderd after PmStartStream
	char application[MAX_PATH] = {};
	uint64_t start_qpc;
	LARGE_INTEGER qpc_frequency = {};
	uint64_t last_displayed_qpc;
	uint64_t buf_size;
	uint64_t max_entries;
	uint64_t current_write_offset;
	uint64_t num_frames_written;
	uint64_t head_idx;
	uint64_t tail_idx;
	bool process_active;
	std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>
		gpuTelemetryCapBits{};
	std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)>
		cpuTelemetryCapBits{};
	bool isPlayback = false;
	bool isPlaybackPaced = false;
	bool isPlaybackRetimed = false;
	bool isPlaybackBackpressured = false;
	bool isPlaybackResetOldest = false;
};

struct PmNsmPresentEvent
{
	uint64_t PresentStartTime;  // QPC value of the first event related to the
	// Present (D3D9, DXGI, or DXGK Present_Start)
	uint32_t ProcessId;         // ID of the process that presented
	uint32_t ThreadId;          // ID of the thread that presented
	uint64_t TimeInPresent;     // QPC duration between runtime present start and end
	uint64_t GPUStartTime;		// QPC value when the frame's first DMA packet started
	uint64_t ReadyTime;			// QPC value when the frame's last DMA packet completed

	uint64_t GPUDuration;		// QPC duration during which a frame's DMA packet was
								// running on any node
	uint64_t GPUVideoDuration;	// QPC duration during which a frame's DMA packet was
								// running on a video node (if mTrackGPUVideo==true)
	uint64_t InputTime;			// Earliest QPC value for all keyboard/mouse input used by this frame
	uint64_t MouseClickTime;	// Earliest QPC value when the mouse was clicked and used by this frame

	// Used to track the application work when Intel XeSS-FG is enabled
	uint64_t AppPropagatedPresentStartTime;  // Propogated QPC value of the first event related to the Present (D3D9, DXGI, or DXGK Present_Start)
	uint64_t AppPropagatedTimeInPresent;     // Propogated  QPC duration of the Present call (only applicable for D3D9/DXGI)
	uint64_t AppPropagatedGPUStartTime;      // Propogated QPC value when the frame's first DMA packet started
	uint64_t AppPropagatedReadyTime;         // Propogated QPC value when the frame's last DMA packet completed
	uint64_t AppPropagatedGPUDuration;       // Propogated QPC duration during which a frame's DMA packet was running on
	uint64_t AppPropagatedGPUVideoDuration;  // Propogated QPC duration during which a frame's DMA packet was running on 
	                                         // a video node (if mTrackGPUVideo==true)

	uint64_t AppSleepStartTime;         // QPC value of app sleep start time provided by Intel App Provider
	uint64_t AppSleepEndTime;           // QPC value of app sleep end time provided by Intel App Provider
	uint64_t AppSimStartTime;           // QPC value of app sim start time provided by Intel App Provider
	uint64_t AppSimEndTime;             // QPC value of app sim end time provided by Intel App Provider
	uint64_t AppRenderSubmitStartTime;  // QPC value of app render submit start time provided by Intel App Provider
	uint64_t AppRenderSubmitEndTime;    // QPC value of app render submit end time provided by Intel App Provider
	uint64_t AppPresentStartTime;       // QPC value of app present start time provided by Intel App Provider
	uint64_t AppPresentEndTime;         // QPC value of app present end time provided by Intel App Provider
	uint64_t AppInputTime;              // QPC value of app input time provided by Intel App Provider
	InputDeviceType AppInputType;		// Input type provided by Intel App Provider

	uint64_t PclInputPingTime;          // QPC value of input ping time provided by the PC Latency ETW event
	uint64_t PclSimStartTime;           // QPC value of app sim start time provided by the PC Latency ETW event
    uint64_t FlipDelay;                 // QPC timestamp delta of FlipDelay calculated using the NV DisplayDriver FlipRequest event.
    uint32_t FlipToken;                 // Flip token from the NV DisplayDriver FlipRequest event.

	// Extra present parameters obtained through DXGI or D3D9 present
	uint64_t SwapChainAddress;
	int32_t SyncInterval;
	uint32_t PresentFlags;

	// (FrameType, DisplayedQPC) for each time the frame was displayed
	uint64_t Displayed_ScreenTime[16];
	FrameType Displayed_FrameType[16];
	uint32_t DisplayedCount;

	// Keys used to index into PMTraceConsumer's tracking data structures:
	uint64_t CompositionSurfaceLuid;   // mPresentByWin32KPresentHistoryToken
	uint64_t Win32KPresentCount;       // mPresentByWin32KPresentHistoryToken
	uint64_t Win32KBindId;             // mPresentByWin32KPresentHistoryToken
	uint64_t DxgkPresentHistoryToken;  // mPresentByDxgkPresentHistoryToken
	uint64_t
		DxgkPresentHistoryTokenData;     // mPresentByDxgkPresentHistoryTokenData
	uint64_t DxgkContext;                // mPresentByDxgkContext
	uint64_t Hwnd;                       // mLastPresentByWindow
	uint32_t QueueSubmitSequence;        // mPresentBySubmitSequence
	uint32_t RingIndex;                   // mTrackedPresents and mCompletedPresents

	// Properties deduced by watching events through present pipeline
	uint32_t DestWidth;
	uint32_t DestHeight;
	uint32_t DriverThreadId;

	uint32_t FrameId;           // ID for the logical frame that this Present is associated with.

	Runtime Runtime;
	PresentMode PresentMode;
	PresentResult FinalState;
	InputDeviceType InputType;
	FrameType FrameType;

	bool SupportsTearing;
	bool WaitForFlipEvent;
	bool WaitForMPOFlipEvent;
	bool SeenDxgkPresent;
	bool SeenWin32KEvents;
	bool SeenInFrameEvent;   // This present has gotten a Win32k TokenStateChanged
	// event into InFrame state
	bool GpuFrameCompleted;  // This present has already seen an event that caused
	// GpuTrace::CompleteFrame() to be called.
	bool IsCompleted;        // All expected events have been observed
	bool IsLost;  // This PresentEvent was found in an unexpected state or is too
	// old

	// Whether this PresentEvent is currently stored in
	// PMTraceConsumer::mPresentsWaitingForDWM
	bool PresentInDwmWaitingStruct;

	// QPC time of last presented frame
	uint64_t last_present_qpc;
	// QPC time of the last displayed frame
	uint64_t last_displayed_qpc;

	// Application name
	char application[MAX_PATH];
};

struct PmNsmFrameData
{
	PmNsmPresentEvent present_event;
	PresentMonPowerTelemetryInfo power_telemetry;
	CpuTelemetryInfo cpu_telemetry;
};
