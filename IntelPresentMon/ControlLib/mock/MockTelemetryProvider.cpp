// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MockTelemetryProvider.h"

#include "../Exceptions.h"
#include "../TelemetryMetricDiscovery.h"
#include "../../CommonUtilities/Qpc.h"

#include <cmath>

using namespace pmon;
using namespace util;

namespace pmon::tel::mock
{
    namespace
    {
        inline constexpr double kTwoPi = 6.283185307179586;

        ProviderDeviceId kMockGpuProviderDeviceId_ = 1;
        ProviderDeviceId kMockSystemProviderDeviceId_ = 2;

        inline DiscoverOutcome AvailableScalar_()
        {
            return DiscoverOutcome{ .arraySize = 1, .availability = PM_METRIC_AVAILABILITY_AVAILABLE };
        }

        ipc::MetricCapabilities BuildMockGpuCaps_()
        {
            ipc::MetricCapabilities caps{};
            const MetricDiscoverSpec specs[] = {
                { PM_METRIC_GPU_VENDOR, AvailableScalar_ },
                { PM_METRIC_GPU_NAME, AvailableScalar_ },
                { PM_METRIC_GPU_FAN_SPEED, []() {
                    return DiscoverOutcome{
                        .arraySize = kMockGpuFanArraySize,
                        .availability = PM_METRIC_AVAILABILITY_AVAILABLE,
                    };
                } },
            };
            DiscoverMetricsFromSpecs(specs, caps, PM_DEVICE_TYPE_GRAPHICS_ADAPTER);
            return caps;
        }

        ipc::MetricCapabilities BuildMockSystemCaps_()
        {
            ipc::MetricCapabilities caps{};
            const MetricDiscoverSpec specs[] = {
                { PM_METRIC_CPU_CORE_TEMPERATURE, []() {
                    return DiscoverOutcome{
                        .arraySize = kMockCoreTempArraySize,
                        .availability = PM_METRIC_AVAILABILITY_AVAILABLE,
                    };
                } },
            };
            DiscoverMetricsFromSpecs(specs, caps, PM_DEVICE_TYPE_SYSTEM);
            return caps;
        }
    }

    MockTelemetryProvider::MockTelemetryProvider()
    {
        {
            DeviceState_ gpu{};
            gpu.providerDeviceId = kMockGpuProviderDeviceId_;
            gpu.fingerprint.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
            gpu.fingerprint.vendor = PM_DEVICE_VENDOR_UNKNOWN;
            gpu.fingerprint.deviceName = kMockGpuDeviceName;
            gpu.fingerprint.isIntegratedAdapter = false;
            gpu.fingerprint.luid = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x01 };
            gpu.caps = BuildMockGpuCaps_();
            devicesById_.emplace(gpu.providerDeviceId, std::move(gpu));
        }
        {
            DeviceState_ system{};
            system.providerDeviceId = kMockSystemProviderDeviceId_;
            system.fingerprint.deviceType = PM_DEVICE_TYPE_SYSTEM;
            system.fingerprint.vendor = PM_DEVICE_VENDOR_INTEL;
            system.fingerprint.deviceName = kMockSystemDeviceName;
            system.caps = BuildMockSystemCaps_();
            devicesById_.emplace(system.providerDeviceId, std::move(system));
        }
    }

    ProviderCapabilityMap MockTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& MockTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<MockTelemetryException>("Mock telemetry provider device not found");
        }
        return iDevice->second.fingerprint;
    }

    double MockTelemetryProvider::ComputeSineSample_(int64_t requestQpc, uint32_t arrayIndex)
    {
        const double qpcFrequency = GetTimestampFrequencyDouble();
        const double tSec = (double)requestQpc / qpcFrequency;
        const double phase = (double)arrayIndex * kMockSinePhasePerIndex;
        return 50.0 + 50.0 * std::sin(kTwoPi * (tSec / kMockSinePeriodSec + phase));
    }

    TelemetryMetricValue MockTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<MockTelemetryException>("Mock telemetry provider device not found");
        }

        const auto& device = iDevice->second;
        const auto arraySize = device.caps.Check(metricId);
        if (arraySize == 0) {
            throw Except<MockTelemetryException>("Unsupported metric for mock telemetry provider");
        }
        if (arrayIndex >= arraySize) {
            throw Except<MockTelemetryException>("Mock telemetry metric queried with out-of-range array index");
        }

        switch (metricId) {
        case PM_METRIC_GPU_FAN_SPEED:
        case PM_METRIC_CPU_CORE_TEMPERATURE:
            return ComputeSineSample_(requestQpc, arrayIndex);
        default:
            throw Except<MockTelemetryException>("Unsupported metric for mock telemetry provider");
        }
    }
}
