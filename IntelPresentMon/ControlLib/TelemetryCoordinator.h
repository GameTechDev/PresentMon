// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonService/MetricUse.h"
#include "TelemetryProvider.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace pmon::ipc
{
    class ServiceComms;
    class TelemetryMap;
}

namespace pmon::tel
{
    class TelemetryCoordinator
    {
    public:
        // Constructs all known concrete providers, correlates logical devices,
        // and builds metric polling routes.
        explicit TelemetryCoordinator(uint32_t pollRateMs);
        // Registers logical CPU/GPU devices and per-device routed capabilities with IPC.
        void RegisterDevicesToIpc(ipc::ServiceComms& comms) const;
        // Populates available static data into IPC device stores.
        void PopulateStaticsToIpc(ipc::ServiceComms& comms) const;
        // Returns aggregate availability across all routed logical devices.
        ipc::MetricCapabilities GetAvailability() const;
        // Broadcasts requested active telemetry polling cadence to providers that need it.
        void SetPollRate(uint32_t pollRateMs);
        // Broadcasts current routed metric interest to providers that need active configuration.
        void SetMetricUse(const svc::DeviceMetricUse& metricUse);
        // Polls routed telemetry metrics and pushes samples directly to IPC rings.
        size_t PollToIpc(const svc::DeviceMetricUse& metricUse,
            ipc::ServiceComms& comms) const;

    private:
        // types
        struct ProviderDevice_
        {
            std::weak_ptr<TelemetryProvider> pProvider;
            uint32_t providerDeviceId;
        };
        struct LogicalDevice_
        {
            uint32_t logicalDeviceId = 0;
            std::vector<ProviderDevice_> providerDevices;
            // maps metrics to an index into providerDevices
            std::unordered_map<PM_METRIC, uint32_t> routes;
        };
        struct GpuMemoryOrderingInfo_
        {
            bool hasValue = false;
            uint64_t value = 0;
        };
        struct GpuPowerOrderingInfo_
        {
            bool hasValue = false;
            double value = 0.0;
        };
        // functions
        void TryCreateConcreteProviders_();
        void BuildLogicalDevicesAndRoutes_();
        void ReassignGpuLogicalDeviceIdsByMemorySize_();
        GpuMemoryOrderingInfo_ QueryGpuMemorySize_(const LogicalDevice_& logicalDevice) const;
        GpuPowerOrderingInfo_ QueryGpuSustainedPowerLimit_(const LogicalDevice_& logicalDevice) const;
        LogicalDevice_& GetOrCreateLogicalDevice_(const TelemetryDeviceFingerprint& fingerprint);
        TelemetryDeviceFingerprint ResolveLogicalDeviceFingerprint_(const LogicalDevice_& logicalDevice) const;
        ipc::MetricCapabilities BuildRoutedCapabilities_(const LogicalDevice_& logicalDevice) const;
        static void PushValueToTelemetryMap_(
            ipc::TelemetryMap& telemetryMap,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            const TelemetryMetricValue& value,
            uint64_t qpc);
        TelemetryMetricValue PollMetricForRoute_(
            const LogicalDevice_& logicalDevice,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) const;
        // data
        std::vector<std::shared_ptr<TelemetryProvider>> providerPtrs_;
        std::unordered_map<uint32_t, LogicalDevice_> logicalDevicesById_;
        uint32_t nextLogicalDeviceId_ = 1;
        uint32_t pollRateMs_ = 1000;
    };
}
