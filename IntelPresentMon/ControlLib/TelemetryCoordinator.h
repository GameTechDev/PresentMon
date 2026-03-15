// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../PresentMonService/MetricUse.h"
#include "TelemetryProvider.h"
#include <cstdint>
#include <optional>
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
        struct CpuInfo
        {
            PM_DEVICE_VENDOR vendor = PM_DEVICE_VENDOR_UNKNOWN;
            std::string name{};
            double cpuPowerLimit = 0.0;
        };

        struct AdapterInfo
        {
            uint32_t id = 0;
            PM_DEVICE_VENDOR vendor = PM_DEVICE_VENDOR_UNKNOWN;
            std::string name{};
            double gpuSustainedPowerLimit = 0.0;
            uint64_t gpuMemorySize = 0;
            uint64_t gpuMemoryMaxBandwidth = 0;
        };

        // Constructs all known concrete providers, correlates logical devices,
        // and builds metric polling routes.
        TelemetryCoordinator();
        // Returns system CPU static identity and routed static telemetry when present.
        std::optional<CpuInfo> GetCpuInfo() const;
        // Returns GPU adapter identity and routed static telemetry for legacy service enumeration.
        std::vector<AdapterInfo> EnumerateAdapters() const;
        // Registers logical CPU/GPU devices and per-device routed capabilities with IPC.
        void RegisterDevicesToIpc(ipc::ServiceComms& comms) const;
        // Populates available static data into IPC device stores.
        void PopulateStaticsToIpc(ipc::ServiceComms& comms) const;
        // Returns aggregate availability across all routed logical devices.
        ipc::MetricCapabilities GetAvailability() const;
        // Polls routed telemetry metrics and pushes samples directly to IPC rings.
        size_t PollToIpc(
            const svc::DeviceMetricUse& metricUse,
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
        // functions
        void TryCreateConcreteProviders_();
        void BuildLogicalDevicesAndRoutes_();
        LogicalDevice_& GetOrCreateLogicalDevice_(const TelemetryDeviceFingerprint& fingerprint);
        TelemetryDeviceFingerprint ResolveLogicalDeviceFingerprint_(
            const LogicalDevice_& logicalDevice,
            bool& haveFingerprint) const;
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
    };
}
