// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../EndpointCache.h"
#include "../TelemetryProvider.h"
#include "Adl2Wrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace pmon::tel::adl
{
    class AmdTelemetryProvider : public TelemetryProvider
    {
    public:
        AmdTelemetryProvider();
        ~AmdTelemetryProvider() override = default;
        ProviderCapabilityMap GetCaps() override;
        const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const override;
        TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) override;

    private:
        struct DynamicSnapshot_
        {
            bool hasGpuPower = false;
            double gpuPowerW = 0.0;

            bool hasGpuVoltage = false;
            double gpuVoltageV = 0.0;

            bool hasGpuFrequency = false;
            double gpuFrequencyMhz = 0.0;

            bool hasGpuTemperature = false;
            double gpuTemperatureC = 0.0;

            bool hasGpuUtilization = false;
            double gpuUtilizationPercent = 0.0;

            bool hasGpuMemFrequency = false;
            double gpuMemFrequencyMhz = 0.0;

            bool hasGpuMemUsed = false;
            uint64_t gpuMemUsedBytes = 0;

            bool hasGpuMemTemperature = false;
            double gpuMemTemperatureC = 0.0;

            bool hasGpuMemVoltage = false;
            double gpuMemVoltageV = 0.0;

            bool hasThrottleStatus = false;
            bool gpuPowerLimited = false;
            bool gpuTemperatureLimited = false;
            bool gpuCurrentLimited = false;

            std::vector<double> fanSpeedsRpm{};
            std::vector<double> fanSpeedRatios{};
        };

        struct DeviceState_
        {
            ProviderDeviceId providerDeviceId = 0;
            TelemetryDeviceFingerprint fingerprint{};
            ipc::MetricCapabilities caps{};

            int adlAdapterIndex = -1;
            int overdriveVersion = 0;
            std::vector<int> od5ThermalControllerIndices{};

            bool memoryInfoQueried = false;
            std::optional<ADLMemoryInfoX4> memoryInfo{};

            pmon::tele::EndpointCache<DynamicSnapshot_> dynamicSnapshotCache{};
        };

        static void ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex);
        static uint64_t GetMemorySizeBytes_(const ADLMemoryInfoX4& memoryInfo) noexcept;
        static uint64_t GetMemoryMaxBandwidthBitsPerSecond_(const ADLMemoryInfoX4& memoryInfo) noexcept;
        static bool TryGetPmLogSensorValue_(
            const ADLPMLogDataOutput& data,
            int sensorIndex,
            int& value) noexcept;

        bool TryInitializeDevice_(DeviceState_& device, const AdapterInfo& adapterInfo) const;
        void EnumerateOd5ThermalControllers_(DeviceState_& device) const;
        ipc::MetricCapabilities BuildCapsForDevice_(DeviceState_& device) const;

        const ADLMemoryInfoX4* QueryMemoryInfo_(DeviceState_& device) const;
        std::optional<double> QuerySustainedPowerLimit_(const DeviceState_& device) const;
        const DynamicSnapshot_& PollDynamicSnapshot_(
            DeviceState_& device,
            int64_t requestQpc) const;

        void PopulateOverdrive5Snapshot_(
            DeviceState_& device,
            DynamicSnapshot_& snapshot) const;
        void PopulateOverdrive6Snapshot_(
            DeviceState_& device,
            DynamicSnapshot_& snapshot) const;
        void PopulateOverdrive7Snapshot_(
            DeviceState_& device,
            DynamicSnapshot_& snapshot) const;
        void PopulateOverdrive8Snapshot_(
            DeviceState_& device,
            DynamicSnapshot_& snapshot) const;

    private:
        std::unique_ptr<Adl2Wrapper> pAdl_{};
        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
        ProviderDeviceId nextProviderDeviceId_ = 1;
    };
}
