// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>
#include <bitset>
#include "PresentMonPowerTelemetry.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pwr
{
    class PowerTelemetryAdapter
    {
    public:
        // types
        using SetTelemetryCapBitset = std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)>;
        // functions
        virtual ~PowerTelemetryAdapter() = default;
        virtual PresentMonPowerTelemetryInfo Sample() noexcept = 0;
        virtual PM_DEVICE_VENDOR GetVendor() const noexcept = 0;
        virtual std::string GetName() const noexcept = 0;
        virtual uint64_t GetDedicatedVideoMemory() const noexcept = 0;
        virtual uint64_t GetVideoMemoryMaxBandwidth() const noexcept = 0;
        virtual double GetSustainedPowerLimit() const noexcept = 0;        
        void SetTelemetryCapBit(GpuTelemetryCapBits telemetryCapBit) noexcept;
        SetTelemetryCapBitset GetPowerTelemetryCapBits();
        bool HasTelemetryCapBit(GpuTelemetryCapBits bit) const;
       private:
        // data
        SetTelemetryCapBitset gpuTelemetryCapBits_{};
    };
}
