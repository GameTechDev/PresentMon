// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../EndpointCache.h"
#include "../TelemetryProvider.h"
#include "NvmlWrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace pmon::tel::nvml
{
    class NvmlTelemetryProvider : public TelemetryProvider
    {
    public:
        NvmlTelemetryProvider();
        ~NvmlTelemetryProvider() override = default;
        ProviderCapabilityMap GetCaps() override;
        const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const override;
        TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) override;

    private:
        struct DeviceState_
        {
            ProviderDeviceId providerDeviceId = 0;
            TelemetryDeviceFingerprint fingerprint{};
            ipc::MetricCapabilities caps{};

            nvmlDevice_t handle = nullptr;

            pmon::tele::EndpointCache<std::optional<nvmlMemory_t>> memoryInfoEndpointCache{};
        };

        static void ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex);
        static uint64_t GetLegacyTotalMemoryBytes_(const nvmlMemory_t& memoryInfo) noexcept;

        bool TryInitializeDevice_(DeviceState_& device, nvmlDevice_t handle) const;
        ipc::MetricCapabilities BuildCapsForDevice_(DeviceState_& device) const;

        const nvmlMemory_t* PollMemoryInfoEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        std::optional<unsigned int> PollPowerEndpoint_(const DeviceState_& device) const;
        std::optional<unsigned int> PollPowerLimitEndpoint_(const DeviceState_& device) const;

    private:
        std::unique_ptr<NvmlWrapper> pNvml_{};
        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
        ProviderDeviceId nextProviderDeviceId_ = 1;
    };
}
