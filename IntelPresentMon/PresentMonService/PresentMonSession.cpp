// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMonSession.h"
#include <set>

pmon::test::service::Status PresentMonSession::GetTestingStatus() const
{
    std::set<uint32_t> trackedPids;
    {
        std::lock_guard lock(tracked_processes_mutex_);
        for (auto const& entry : tracked_pid_live_) {
            trackedPids.emplace(entry.first);
        }
    }
    std::set<uint32_t> frameStorePids;
    if (pBroadcaster) {
        for (auto pid : pBroadcaster->GetPids()) {
            frameStorePids.emplace(pid);
        }
    }
    return pmon::test::service::Status{
        .trackedPids = std::move(trackedPids),
        .frameStorePids = std::move(frameStorePids),
        .telemetryPeriodMs = gpu_telemetry_period_ms_,
        .etwFlushPeriodMs = etw_flush_period_ms_,
    };
}

std::vector<pmon::tel::TelemetryCoordinator::AdapterInfo> PresentMonSession::EnumerateAdapters() const {
    return telemetry_adapters_;
}

PM_STATUS PresentMonSession::SetGpuTelemetryPeriod(std::optional<uint32_t> period_ms)
{
    gpu_telemetry_period_ms_ = period_ms.value_or(default_gpu_telemetry_period_ms_);
    return PM_STATUS_SUCCESS;
}

uint32_t PresentMonSession::GetGpuTelemetryPeriod() {
    return gpu_telemetry_period_ms_;
}

PM_STATUS PresentMonSession::SetEtwFlushPeriod(std::optional<uint32_t> periodMs)
{
    if (periodMs) {
        etw_flush_period_ms_ = *periodMs;
    }
    else {
        ResetEtwFlushPeriod();
    }
    return PM_STATUS_SUCCESS;
}

std::optional<uint32_t> PresentMonSession::GetEtwFlushPeriod()
{
    return etw_flush_period_ms_;
}

bool PresentMonSession::HasLiveTargets() const {
    return HasLiveTrackedProcesses();
}

void PresentMonSession::SetTelemetryAdapters(
    std::vector<pmon::tel::TelemetryCoordinator::AdapterInfo> adapters) {
    telemetry_adapters_ = std::move(adapters);
}

void PresentMonSession::SyncTrackedPidState(const std::unordered_set<uint32_t>& trackedPids)
{
    // TODO: consider theoretical rare race condition where exited process is added and never gets
    // marked "dead" while action client maintains session and nevers stops tracking
    std::lock_guard lock(tracked_processes_mutex_);
    std::erase_if(tracked_pid_live_, [&](auto const& entry) {
        return !trackedPids.contains(entry.first);
    });
    for (auto pid : trackedPids) {
        if (!tracked_pid_live_.contains(pid)) {
            tracked_pid_live_.emplace(pid, true);
        }
    }
}

void PresentMonSession::MarkProcessExited(uint32_t pid)
{
    std::lock_guard lock(tracked_processes_mutex_);
    if (auto it = tracked_pid_live_.find(pid); it != tracked_pid_live_.end()) {
        it->second = false;
    }
}

bool PresentMonSession::IsProcessTracked(uint32_t pid) const
{
    std::lock_guard lock(tracked_processes_mutex_);
    return tracked_pid_live_.find(pid) != tracked_pid_live_.end();
}

bool PresentMonSession::HasTrackedProcesses() const
{
    std::lock_guard lock(tracked_processes_mutex_);
    return std::ranges::any_of(tracked_pid_live_, [](auto const&) { return true; });
}

bool PresentMonSession::HasLiveTrackedProcesses() const
{
    std::lock_guard lock(tracked_processes_mutex_);
    return std::ranges::any_of(tracked_pid_live_, [](auto const& entry) { return entry.second; });
}

void PresentMonSession::ClearTrackedProcesses()
{
    std::lock_guard lock(tracked_processes_mutex_);
    tracked_pid_live_.clear();
}
