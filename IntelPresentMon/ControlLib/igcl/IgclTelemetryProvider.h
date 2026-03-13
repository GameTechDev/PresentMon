// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>

#include "../EndpointCache.h"
#include "../TelemetryProvider.h"
#include "igcl_api.h"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace pmon::tel::igcl
{
    class IgclTelemetryProvider : public TelemetryProvider
    {
    public:
        IgclTelemetryProvider();
        ~IgclTelemetryProvider() override;
        ProviderCapabilityMap GetCaps() override;
        const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const override;
        TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) override;

    private:
        class TelemetrySampleBuffer_
        {
        public:
            TelemetrySampleBuffer_() noexcept;
            bool Matches(int64_t requestQpc) const noexcept;
            ctl_power_telemetry_t& PrepareForWrite(int64_t requestQpc) noexcept;
            const ctl_power_telemetry_t& Current() const noexcept;
            const ctl_power_telemetry_t* Previous() const noexcept;
            int64_t PreviousRequestQpc() const noexcept;

        private:
            static ctl_power_telemetry_t MakeEmptySample_() noexcept;

        private:
            ctl_power_telemetry_t samples_[2]{};
            int64_t requestQpcs_[2]{};
            uint32_t currentIndex_ = 0;
            uint32_t previousIndex_ = 1;
            bool hasCurrent_ = false;
            bool hasPrevious_ = false;
        };

        struct DeviceState_
        {
            ProviderDeviceId providerDeviceId = 0;
            TelemetryDeviceFingerprint fingerprint{};
            ipc::MetricCapabilities caps{};

            ctl_device_adapter_handle_t handle = nullptr;
            LUID deviceLuid{};
            ctl_device_adapter_properties_t properties{};
            std::vector<ctl_mem_handle_t> memoryModules{};
            std::vector<ctl_pwr_handle_t> powerDomains{};
            std::vector<int32_t> maxFanSpeedsRpm{};
            uint32_t fanSpeedCount = 0;
            uint32_t fanSpeedPercentCount = 0;
            bool isAlchemist = false;

            bool useNewBandwidthTelemetry = true;

            TelemetrySampleBuffer_ telemetrySamples{};
            pmon::tele::EndpointCache<ctl_mem_state_t> memoryStateEndpointCache{};

            // Runtime telemetry workarounds carried over from the legacy Intel path.
            double gpuMemReadBwCacheValueBps = 0.0;
            uint64_t gpuMemMaxBwCacheValueBps = 0;
            double gpuMemPowerCacheValueW = 0.0;
        };

        static void ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex);
        bool TryInitializeDevice_(DeviceState_& device, ctl_device_adapter_handle_t handle) const;
        void EnumerateMemoryModules_(DeviceState_& device) const;
        void EnumeratePowerDomains_(DeviceState_& device) const;
        void EnumerateFans_(DeviceState_& device) const;
        ipc::MetricCapabilities BuildCapsForDevice_(DeviceState_& device) const;

        const ctl_power_telemetry_t& PollTelemetryEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        const ctl_mem_state_t* PollMemoryStateEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        std::optional<ctl_mem_bandwidth_t> PollMemoryBandwidthEndpoint_(
            DeviceState_& device) const;
        std::optional<ctl_power_limits_t> PollPowerLimitsEndpoint_(
            DeviceState_& device) const;

        TelemetryMetricValue PollTelemetryMetric_(
            DeviceState_& device,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) const;

        static bool TryGetInstantaneousTelemetryItem_(
            const ctl_oc_telemetry_item_t& telemetryItem,
            double& value);
        static bool TryGetUsagePercentTelemetryItem_(
            const ctl_oc_telemetry_item_t& currentItem,
            const ctl_oc_telemetry_item_t& previousItem,
            double timeDelta,
            double& value);
        static bool TryGetUsageTelemetryItem_(
            DeviceState_& device,
            PM_METRIC metricId,
            const ctl_oc_telemetry_item_t& currentItem,
            const ctl_oc_telemetry_item_t& previousItem,
            double timeDelta,
            double& value);

        static bool IsInstantaneousTelemetryItemSupported_(const ctl_oc_telemetry_item_t& telemetryItem);
        static bool IsUsageTelemetryItemSupported_(const ctl_oc_telemetry_item_t& telemetryItem);
        static bool IsUsagePercentTelemetryItemSupported_(const ctl_oc_telemetry_item_t& telemetryItem);
        static double ConvertMegabytesPerSecondToBitsPerSecond_(double megabytesPerSecond);

    private:
        ctl_api_handle_t apiHandle_ = nullptr;
        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
        ProviderDeviceId nextProviderDeviceId_ = 1;
    };
}
