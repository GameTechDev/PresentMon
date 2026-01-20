// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "PresentMonSession.h"
#include "EtwLogger.h"
#include "FrameBroadcaster.h"
#include "../CommonUtilities/win/Event.h"
#include "../CommonUtilities/Hash.h"
#include <memory>
#include <span>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <source_location>
#include <utility>
#include <cstdint>

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
	bool IsPlayback() const
	{
		return !isRealtime_;
	}
	bool CheckDeviceMetricUsage(uint32_t deviceId) const
	{
		std::shared_lock lk{ metricDeviceUsageMtx_ };
		return metricDeviceUsage_.contains(deviceId);
	}
	void SetDeviceMetricUsage(std::unordered_set<uint32_t> usage)
	{
		// we need exclusive lock to prevent concurrent access to usage data while being modified
		{
			std::lock_guard lk{ metricDeviceUsageMtx_ };
			metricDeviceUsage_ = std::move(usage);
		}
		// keep shared lock now to prevent modification to event set while we are iterating it
		// if this were non-shared, it would cause the listeners to block immediately on wake
		std::shared_lock lk2{ metricDeviceUsageMtx_ };
		for (auto& kv : deviceUsageEvts_) {
			kv.second.Set();
		}
	}
	HANDLE GetDeviceUsageEvent(std::source_location loc = std::source_location::current()) const
	{
		const DeviceUsageEvtKey key{ loc.file_name(), (uint32_t)loc.line() };
		{
			std::shared_lock lk{ metricDeviceUsageMtx_ };
			if (auto it = deviceUsageEvts_.find(key); it != deviceUsageEvts_.end()) {
				return it->second.Get();
			}
		}
		// get non-shared lock for modification purposes (add new event)
		std::lock_guard lk2{ metricDeviceUsageMtx_ };
		auto it = deviceUsageEvts_.emplace(key, util::win::Event{ false, false }).first;
		return it->second.Get();
	}
	void StartPlayback();
	void StopPlayback();
private:
	svc::FrameBroadcaster& broadcaster_;
	svc::EtwLogger etwLogger_;
	std::unique_ptr<PresentMonSession> pSession_;
	bool isRealtime_ = true;
	mutable std::shared_mutex metricDeviceUsageMtx_;
	std::unordered_set<uint32_t> metricDeviceUsage_;
	using DeviceUsageEvtKey = std::pair<const char*, uint32_t>;
	mutable std::unordered_map<DeviceUsageEvtKey, util::win::Event> deviceUsageEvts_;
};
