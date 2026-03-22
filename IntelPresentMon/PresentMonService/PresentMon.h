// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "PresentMonSession.h"
#include "EtwLogger.h"
#include "FrameBroadcaster.h"
#include "MetricUse.h"
#include "../CommonUtilities/win/Event.h"
#include <memory>
#include <span>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <source_location>
#include <utility>
#include <cstdint>
#include <optional>
#include <atomic>

using namespace pmon;

class PresentMon
{
public:
	using DeviceMetricUsage = svc::DeviceMetricUse;

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
    PM_STATUS UpdateTracking(const std::unordered_set<uint32_t>& trackedPids);
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
	HANDLE GetStreamingStartHandle()
	{
		return pSession_->GetStreamingStartHandle();
	}
	bool HasLiveTargets() const
	{
		return pSession_->HasLiveTargets();
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
	bool CheckDeviceMetricUsage(std::optional<uint32_t> deviceId,
		std::optional<PM_METRIC> metricId = std::nullopt,
		std::optional<uint32_t> arrayIdx = std::nullopt) const
	{
		const auto usageData = metricDeviceUsage_.load(std::memory_order_acquire);
		if (!usageData) {
			return false;
		}

		const auto selectionMatches = [&](const std::unordered_set<svc::MetricUse>& selection) -> bool {
			if (!metricId) {
				// array index without metric is not a valid query form
				if (arrayIdx) {
					return false;
				}
				return !selection.empty();
			}
			for (const auto& metricUse : selection) {
				if (metricUse.metricId != *metricId) {
					continue;
				}
				if (!arrayIdx || metricUse.arrayIdx == *arrayIdx) {
					return true;
				}
			}
			return false;
		};

		if (deviceId) {
			if (auto it = usageData->find(*deviceId); it != usageData->end()) {
				return selectionMatches(it->second);
			}
			return false;
		}

		for (const auto& usageEntry : *usageData) {
			if (selectionMatches(usageEntry.second)) {
				return true;
			}
		}
		return false;
	}
	std::shared_ptr<const DeviceMetricUsage> GetDeviceMetricUsageSnapshot() const
	{
		return metricDeviceUsage_.load(std::memory_order_acquire);
	}
	void SetDeviceMetricUsage(std::shared_ptr<const DeviceMetricUsage> usage)
	{
		if (!usage) {
			usage = std::make_shared<DeviceMetricUsage>();
		}
		metricDeviceUsage_.store(std::move(usage), std::memory_order_release);
		// keep shared lock now to prevent modification to event set while we are iterating it
		// if this were non-shared, it would cause the listeners to block immediately on wake
		std::shared_lock lk2{ deviceUsageEvtMtx_ };
		for (auto& kv : deviceUsageEvts_) {
			kv.second.Set();
		}
	}
	HANDLE GetDeviceUsageEvent(std::source_location loc = std::source_location::current()) const
	{
		const DeviceUsageEvtKey key{ loc.file_name(), (uint32_t)loc.line() };
		{
			std::shared_lock lk{ deviceUsageEvtMtx_ };
			if (auto it = deviceUsageEvts_.find(key); it != deviceUsageEvts_.end()) {
				return it->second.Get();
			}
		}
		// get non-shared lock for modification purposes (add new event)
		std::lock_guard lk2{ deviceUsageEvtMtx_ };
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
	std::atomic<std::shared_ptr<const DeviceMetricUsage>> metricDeviceUsage_{
		std::make_shared<DeviceMetricUsage>() };
	mutable std::shared_mutex deviceUsageEvtMtx_;
	using DeviceUsageEvtKey = std::pair<const char*, uint32_t>;
	mutable std::unordered_map<DeviceUsageEvtKey, util::win::Event> deviceUsageEvts_;
};
