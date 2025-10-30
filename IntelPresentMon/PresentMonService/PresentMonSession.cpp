// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMonSession.h"

pmon::test::service::Status PresentMonSession::GetTestingStatus() const
{
    return pmon::test::service::Status{
        .nsmStreamedPids = streamer_.GetActiveStreamPids(),
        .activeAdapterId = current_telemetry_adapter_id_,
        .telemetryPeriodMs = gpu_telemetry_period_ms_,
        .etwFlushPeriodMs = etw_flush_period_ms_,
    };
}

void PresentMonSession::SetCpu(const std::shared_ptr<pwr::cpu::CpuTelemetry>& pCpu) {
    cpu_ = pCpu.get();
}

std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> PresentMonSession::EnumerateAdapters() {
    if (telemetry_container_) {
        return telemetry_container_->GetPowerTelemetryAdapters();
    }
    else {
        return {};
    }
}

std::string PresentMonSession::GetCpuName() {
    if (cpu_) {
        return cpu_->GetCpuName();
    }
    else {
        return std::string{ "UNKOWN_CPU" };
    }
}

double PresentMonSession::GetCpuPowerLimit() {
    if (cpu_) {
        return cpu_->GetCpuPowerLimit();
    }
    else {
        return 0.;
    }
}

PM_STATUS PresentMonSession::SelectAdapter(uint32_t adapter_id) {
    if (telemetry_container_) {
        if (adapter_id > telemetry_container_->GetPowerTelemetryAdapters().size()) {
            return PM_STATUS_INVALID_ADAPTER_ID;
        }
        current_telemetry_adapter_id_ = adapter_id;
    }
    return PM_STATUS::PM_STATUS_SUCCESS;
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

int PresentMonSession::GetActiveStreams() {
    return streamer_.NumActiveStreams();
}

void PresentMonSession::SetPowerTelemetryContainer(PowerTelemetryContainer* ptc) {
    telemetry_container_ = ptc;
}