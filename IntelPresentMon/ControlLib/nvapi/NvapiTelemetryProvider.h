// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../EndpointCache.h"
#include "../TelemetryProvider.h"
#include "NvapiWrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace pmon::tel::nvapi
{
    class NvapiTelemetryProvider : public TelemetryProvider
    {
    public:
        NvapiTelemetryProvider();
        ~NvapiTelemetryProvider() override = default;
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

            NvPhysicalGpuHandle handle = nullptr;

            pmon::tele::EndpointCache<NV_GPU_THERMAL_SETTINGS> thermalEndpointCache{};
            pmon::tele::EndpointCache<NV_GPU_CLOCK_FREQUENCIES> clockEndpointCache{};
            pmon::tele::EndpointCache<NV_GPU_DYNAMIC_PSTATES_INFO_EX> utilizationEndpointCache{};
            pmon::tele::EndpointCache<std::optional<NvU32>> tachEndpointCache{};
        };

        static void ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex);
        bool TryInitializeDevice_(DeviceState_& device, NvPhysicalGpuHandle handle) const;
        ipc::MetricCapabilities BuildCapsForDevice_(DeviceState_& device) const;

        const NV_GPU_THERMAL_SETTINGS* PollThermalEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        const NV_GPU_CLOCK_FREQUENCIES* PollClockEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        const NV_GPU_DYNAMIC_PSTATES_INFO_EX* PollUtilizationEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        const NvU32* PollTachEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;

        TelemetryMetricValue PollNvapiMetric_(
            DeviceState_& device,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) const;

        static bool TryGetThermalValue_(
            const NV_GPU_THERMAL_SETTINGS& thermalSettings,
            NV_THERMAL_TARGET target,
            bool requireInternalController,
            double& value);
        static bool TryGetClockValue_(
            const NV_GPU_CLOCK_FREQUENCIES& clockFrequencies,
            NV_GPU_PUBLIC_CLOCK_ID clockDomain,
            double& value);
        static bool TryGetUtilizationValue_(
            const NV_GPU_DYNAMIC_PSTATES_INFO_EX& utilization,
            NVAPI_GPU_UTILIZATION_DOMAIN utilizationDomain,
            double& value);

    private:
        std::unique_ptr<NvapiWrapper> pNvapi_{};
        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
        ProviderDeviceId nextProviderDeviceId_ = 1;
    };
}
