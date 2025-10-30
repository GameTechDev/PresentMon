// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"

#include <cmath>
#include <random>
#include <atomic>
#include <VersionHelpers.h>

#include "../ControlLib/PowerTelemetryProvider.h"
#include "../ControlLib/CpuTelemetry.h"
#include "../Streamer/Streamer.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../../PresentData/PresentMonTraceSession.hpp"
#include "PowerTelemetryContainer.h"

#include "../PresentMonAPI2Tests/TestCommands.h"


struct SwapChainData {
    uint32_t mPresentHistoryCount;
    uint64_t mLastPresentQPC;
    uint64_t mLastDisplayedPresentQPC;
};

struct ProcessInfo {
    std::wstring mModuleName;
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    HANDLE mHandle = INVALID_HANDLE_VALUE;
    bool mTargetProcess = false;
};

class PresentMonSession {
public:
    virtual ~PresentMonSession() = default;
    virtual bool IsTraceSessionActive() = 0;
    virtual PM_STATUS StartStreaming(uint32_t client_process_id, uint32_t target_process_id, std::string& nsmFileName) = 0;
    virtual void StopStreaming(uint32_t client_process_id, uint32_t target_process_id) = 0;
    virtual bool CheckTraceSessions(bool forceTerminate) = 0;
    virtual HANDLE GetStreamingStartHandle() = 0;
    virtual void FlushEvents() {}
    virtual void ResetEtwFlushPeriod() = 0;

    void SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu);
    std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> EnumerateAdapters();
    std::string GetCpuName();
    double GetCpuPowerLimit();
    pmon::test::service::Status GetTestingStatus() const;

    PM_STATUS SelectAdapter(uint32_t adapter_id);
    PM_STATUS SetGpuTelemetryPeriod(std::optional<uint32_t> period_ms);
    PM_STATUS SetEtwFlushPeriod(std::optional<uint32_t> periodMs);
    std::optional<uint32_t> GetEtwFlushPeriod();
    uint32_t GetGpuTelemetryPeriod();
    int GetActiveStreams();
    void SetPowerTelemetryContainer(PowerTelemetryContainer* ptc);

    // TODO: review all of these members and consider fixing the unsound thread safety aspects
    // data
    std::wstring pm_session_name_;

    pwr::cpu::CpuTelemetry* cpu_ = nullptr;
    PowerTelemetryContainer* telemetry_container_ = nullptr;

    uint32_t current_telemetry_adapter_id_ = 0;

    // Set the initial telemetry period to 16ms
    static constexpr uint32_t default_gpu_telemetry_period_ms_ = 16;
    uint32_t gpu_telemetry_period_ms_ = default_gpu_telemetry_period_ms_;
    // initial default etw flush period for realtime is 1000ms
    // realtime trace sessions always manually flush
    static constexpr uint32_t default_realtime_etw_flush_period_ms_ = 1000;
    // empty optional means automatic flushing active
    std::atomic<std::optional<uint32_t>> etw_flush_period_ms_;

    Streamer streamer_;
};

