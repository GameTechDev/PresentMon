// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "PresentMonSession.h"
#include "EtwLogger.h"
#include "FrameBroadcaster.h"
#include <memory>
#include <span>

using namespace pmon;

class PresentMon
{
public:
	PresentMon(svc::FrameBroadcaster& broadcaster, bool isRealtime);
	~PresentMon();

	// Check the status of both ETW logfile and real time trace sessions.
	// When an ETW logfile has finished processing the associated
	// trace session must be destroyed to allow for other etl sessions
	// to be processed. In the case of real-time session if for some reason
	// there are zero active streams and a trace session is still active
	// clean it up.
	void CheckTraceSessions();
	// Force stop trace sessions
	void StopTraceSessions();
	PM_STATUS StartStreaming(uint32_t client_process_id,
		uint32_t target_process_id,
		std::string& nsm_file_name);
	void StopStreaming(uint32_t client_process_id, uint32_t target_process_id);
	std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
	std::string GetCpuName() { return pSession_->GetCpuName(); }
	double GetCpuPowerLimit() { return pSession_->GetCpuPowerLimit(); }
	PM_STATUS SelectAdapter(uint32_t adapter_id);
	PM_STATUS SetGpuTelemetryPeriod(std::optional<uint32_t> telemetryPeriodRequestsMs)
	{
		return pSession_->SetGpuTelemetryPeriod(telemetryPeriodRequestsMs);
	}
	uint32_t GetGpuTelemetryPeriod()
	{
		// Only the real time trace sets GPU telemetry period
		return pSession_->GetGpuTelemetryPeriod();
	}
	PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs)
	{
		// Only the real time trace sets ETW flush period
		return pSession_->SetEtwFlushPeriod(periodMs);
	}
	std::optional<uint32_t> GetEtwFlushPeriod()
	{
		// Only the real time trace sets ETW flush period
		return pSession_->GetEtwFlushPeriod();
	}
	void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu)
	{
		// Only the real time trace uses the control libary interface
		pSession_->SetCpu(pCpu);
	}
	HANDLE GetStreamingStartHandle()
	{
		return pSession_->GetStreamingStartHandle();
	}
	int GetActiveStreams()
	{
		// Only the real time trace uses the control libary interface
		return pSession_->GetActiveStreams();
	}
	void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc)
	{
		// Only the real time trace session uses the control library interface
		return pSession_->SetPowerTelemetryContainer(ptc);
	}
	void FlushEvents()
	{
		pSession_->FlushEvents();
	}
	auto GetTestingStatus() const
	{
		return pSession_->GetTestingStatus();
	}
	auto& GetEtwLogger()
	{
		return etwLogger_;
	}
	auto& GetBroadcaster()
	{
		return broadcaster_;
	}
	void StartPlayback();
	void StopPlayback();
private:
	svc::FrameBroadcaster& broadcaster_;
	svc::EtwLogger etwLogger_;
	std::unique_ptr<PresentMonSession> pSession_;
};