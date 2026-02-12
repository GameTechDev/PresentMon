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

#include "../ControlLib/PowerTelemetryProvider.h"
#include "../ControlLib/CpuTelemetry.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../../PresentData/PresentMonTraceSession.hpp"
#include "PowerTelemetryContainer.h"
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

    void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu);
    std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
    std::string GetCpuName();
    double GetCpuPowerLimit();
    pmon::test::service::Status GetTestingStatus() const;
    PM_STATUS SetGpuTelemetryPeriod(std::optional<uint32_t> period_ms);
    PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs);
    std::optional<uint32_t> GetEtwFlushPeriod();
    uint32_t GetGpuTelemetryPeriod();
    bool HasLiveTargets() const;
    void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc);

    void MarkProcessExited(uint32_t pid);
    bool IsProcessTracked(uint32_t pid) const;
    bool HasTrackedProcesses() const;
    bool HasLiveTrackedProcesses() const;
    void ClearTrackedProcesses();

protected:

    // TODO: review all of these members and consider fixing the unsound thread safety aspects
    // data
    std::wstring pm_session_name_;

    pwr::cpu::CpuTelemetry* cpu_ = nullptr;
    PowerTelemetryContainer* telemetry_container_ = nullptr;

    // Set the initial telemetry period to 16ms
    static constexpr uint32_t default_gpu_telemetry_period_ms_ = 16;
    uint32_t gpu_telemetry_period_ms_ = default_gpu_telemetry_period_ms_;
    // initial default etw flush period for realtime is 1000ms
    // realtime trace sessions always manually flush
    static constexpr uint32_t default_realtime_etw_flush_period_ms_ = 1000;
    // empty optional means automatic flushing active
    std::atomic<std::optional<uint32_t>> etw_flush_period_ms_;

    svc::FrameBroadcaster* pBroadcaster = nullptr;

    void SyncTrackedPidState(const std::unordered_set<uint32_t>& trackedPids);

    mutable std::mutex tracked_processes_mutex_;
    std::unordered_map<uint32_t, bool> tracked_pid_live_;
};

