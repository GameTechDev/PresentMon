// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../TelemetryProvider.h"

#include <cstdint>
#include <unordered_map>

namespace pmon::tel::mock
{
    inline constexpr const char* kMockGpuDeviceName = "PresentMon Mock GPU";
    inline constexpr const char* kMockSystemDeviceName = "PresentMon Mock System";
    inline constexpr uint32_t kMockGpuFanArraySize = 3;
    inline constexpr uint32_t kMockCoreTempArraySize = 5;
    inline constexpr double kMockSinePeriodSec = 5.0;
    inline constexpr double kMockSinePhasePerIndex = 0.2;

    class MockTelemetryProvider : public TelemetryProvider
    {
    public:
        MockTelemetryProvider();
        ~MockTelemetryProvider() override = default;
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
        };

        static double ComputeSineSample_(int64_t requestQpc, uint32_t arrayIndex);

        std::unordered_map<ProviderDeviceId, DeviceState_> devicesById_{};
    };
}
