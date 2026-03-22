// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <ranges>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <VersionHelpers.h>

#include "../ControlLib/TelemetryCoordinator.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../../PresentData/PresentMonTraceSession.hpp"
#include "FrameBroadcaster.h"

#include "../PresentMonAPI2Tests/TestCommands.h"


struct ProcessInfo {
    std::wstring mModuleName;
    HANDLE mHandle = INVALID_HANDLE_VALUE;
};

using namespace pmon;

class PresentMonSession {
public:
    virtual ~PresentMonSession() = default;
    virtual bool IsTraceSessionActive() = 0;
    virtual PM_STATUS UpdateTracking(const std::unordered_set<uint32_t>& trackedPids) = 0;
    virtual bool CheckTraceSessions(bool forceTerminate) = 0;
    virtual HANDLE GetStreamingStartHandle() = 0;
    virtual void FlushEvents() {}
    virtual void ResetEtwFlushPeriod() = 0;

    std::vector<pmon::tel::TelemetryCoordinator::AdapterInfo> EnumerateAdapters() const;
    pmon::test::service::Status GetTestingStatus() const;
    PM_STATUS SetGpuTelemetryPeriod(std::optional<uint32_t> period_ms);
    PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs);
    std::optional<uint32_t> GetEtwFlushPeriod();
    uint32_t GetGpuTelemetryPeriod();
    bool HasLiveTargets() const;
    void SetTelemetryAdapters(std::vector<pmon::tel::TelemetryCoordinator::AdapterInfo> adapters);

    void MarkProcessExited(uint32_t pid);
    bool IsProcessTracked(uint32_t pid) const;
    bool HasTrackedProcesses() const;
    bool HasLiveTrackedProcesses() const;
    void ClearTrackedProcesses();

protected:

    // TODO: review all of these members and consider fixing the unsound thread safety aspects
    // data
    std::wstring pm_session_name_;

    std::vector<pmon::tel::TelemetryCoordinator::AdapterInfo> telemetry_adapters_{};

    // Set the initial telemetry period to 16ms
    static constexpr uint32_t default_gpu_telemetry_period_ms_ = 16;
    uint32_t gpu_telemetry_period_ms_ = default_gpu_telemetry_period_ms_;
    // empty optional means manual ETW flushing is disabled
    std::atomic<std::optional<uint32_t>> etw_flush_period_ms_;

    svc::FrameBroadcaster* pBroadcaster = nullptr;

    void SyncTrackedPidState(const std::unordered_set<uint32_t>& trackedPids);

    mutable std::mutex tracked_processes_mutex_;
    std::unordered_map<uint32_t, bool> tracked_pid_live_;
};

