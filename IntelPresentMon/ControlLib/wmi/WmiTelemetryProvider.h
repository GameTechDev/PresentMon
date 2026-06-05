// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../../CommonUtilities/win/WinAPI.h"
#include "../EndpointCache.h"
#include "../TelemetryProvider.h"

#include <pdh.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace pmon::tel::wmi
{
    class WmiTelemetryProvider : public TelemetryProvider
    {
    public:
        WmiTelemetryProvider();
        ~WmiTelemetryProvider() override = default;
        ProviderCapabilityMap GetCaps() override;
        const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const override;
        TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) override;

    private:
        struct QueryDeleter_
        {
            void operator()(PDH_HQUERY query) const noexcept;
        };

        struct CounterSample_
        {
            double frequencyMhz = 0.0;
            double utilizationPercent = 0.0;
            bool hasFrequency = false;
            bool hasUtilization = false;
        };

        struct DeviceState_
        {
            ProviderDeviceId providerDeviceId = 0;
            TelemetryDeviceFingerprint fingerprint{};
            ipc::MetricCapabilities caps{};

            std::unique_ptr<std::remove_pointer_t<PDH_HQUERY>, QueryDeleter_> query{};
            HCOUNTER processorFrequencyCounter = {};
            HCOUNTER processorPerformanceCounter = {};
            HCOUNTER processorIdleTimeCounter = {};
            uint64_t qpcFrequency = 0;
            int64_t nextSampleQpc = 0;

            pmon::tele::EndpointCache<CounterSample_> counterSampleEndpointCache{};
        };

        static void ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex);
        void InitializeDevice_(DeviceState_& device) const;
        void PopulateFingerprintFromWmi_(DeviceState_& device) const;
        ipc::MetricCapabilities BuildCapsForDevice_(DeviceState_& device) const;
        const CounterSample_* PollCounterSampleEndpoint_(
            DeviceState_& device,
            int64_t requestQpc) const;
        static bool TryReadCounterValue_(
            HCOUNTER counter,
            const char* label,
            const DeviceState_& device,
            double& value);

    private:
        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
        ProviderDeviceId nextProviderDeviceId_ = 1;
    };
}
