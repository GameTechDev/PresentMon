// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryCoordinator.h"
#include "Exceptions.h"
#include "TelemetryDeviceFingerprint.h"
#include "Logging.h"
#include "igcl/IgclTelemetryProvider.h"
#include "nvapi/NvapiTelemetryProvider.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Qpc.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace pmon::tel
{
    namespace
    {
        struct RouteCandidate_
        {
            uint32_t providerDeviceIndex = 0;
            uint32_t arraySize = 0;
            uint32_t providerCoverageScore = 0;
        };
    }

    TelemetryCoordinator::TelemetryCoordinator()
    {
        TryCreateConcreteProviders_();
        BuildLogicalDevicesAndRoutes_();
    }

    void TelemetryCoordinator::PopulateStaticsToIpc(ipc::ServiceComms& comms) const
    {
        const auto requestQpc = util::GetCurrentTimestamp();
        // TODO: make static population dynamic by metric id and metadata instead of hardcoded checks.
        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            TelemetryDeviceFingerprint providerFingerprint{};
            bool haveProviderFingerprint = false;
            for (const auto& providerDevice : logicalDevice.providerDevices) {
                const auto pProvider = providerDevice.pProvider.lock();
                if (!pProvider) {
                    pmlog_warn("Expired provider while resolving provider-device fingerprint for statics")
                        .pmwatch(logicalDevice.logicalDeviceId)
                        .pmwatch(providerDevice.providerDeviceId);
                    continue;
                }

                try {
                    providerFingerprint = pProvider->GetFingerPrint(providerDevice.providerDeviceId);
                    haveProviderFingerprint = true;
                    break;
                }
                catch (...) {
                    pmlog_error(util::ReportException("Provider-device fingerprint query failed for statics"))
                        .pmwatch(logicalDevice.logicalDeviceId)
                        .pmwatch(providerDevice.providerDeviceId);
                }
            }

            try {
                if (logicalDeviceId != ipc::kSystemDeviceId) {
                    auto& store = comms.GetGpuDataStore(logicalDevice.logicalDeviceId);
                    if (haveProviderFingerprint) {
                        store.statics.vendor = providerFingerprint.vendor;
                        store.statics.name = providerFingerprint.deviceName.c_str();
                    }
                    else {
                        pmlog_warn("No provider-device fingerprint available for GPU static identity")
                            .pmwatch(logicalDevice.logicalDeviceId);
                    }

                    if (logicalDevice.routes.contains(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT)) {
                        const auto value = PollMetricForRoute_(
                            logicalDevice, PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, 0, requestQpc);
                        if (const auto pVal = std::get_if<double>(&value)) {
                            store.statics.sustainedPowerLimit = *pVal;
                        }
                        else {
                            throw util::Except<>("Type mismatch for PM_METRIC_GPU_SUSTAINED_POWER_LIMIT static metric");
                        }
                    }
                    if (logicalDevice.routes.contains(PM_METRIC_GPU_MEM_SIZE)) {
                        const auto value = PollMetricForRoute_(
                            logicalDevice, PM_METRIC_GPU_MEM_SIZE, 0, requestQpc);
                        if (const auto pVal = std::get_if<uint64_t>(&value)) {
                            store.statics.memSize = *pVal;
                        }
                        else {
                            throw util::Except<>("Type mismatch for PM_METRIC_GPU_MEM_SIZE static metric");
                        }
                    }
                    if (logicalDevice.routes.contains(PM_METRIC_GPU_MEM_MAX_BANDWIDTH)) {
                        const auto value = PollMetricForRoute_(
                            logicalDevice, PM_METRIC_GPU_MEM_MAX_BANDWIDTH, 0, requestQpc);
                        if (const auto pVal = std::get_if<uint64_t>(&value)) {
                            store.statics.maxMemBandwidth = *pVal;
                        }
                        else {
                            throw util::Except<>("Type mismatch for PM_METRIC_GPU_MEM_MAX_BANDWIDTH static metric");
                        }
                    }
                }
                else {
                    auto& store = comms.GetSystemDataStore();
                    if (haveProviderFingerprint) {
                        store.statics.cpuVendor = providerFingerprint.vendor;
                        store.statics.cpuName = providerFingerprint.deviceName.c_str();
                    }
                    else {
                        pmlog_warn("No provider-device fingerprint available for system static identity")
                            .pmwatch(logicalDevice.logicalDeviceId);
                    }

                    if (logicalDevice.routes.contains(PM_METRIC_CPU_POWER_LIMIT)) {
                        const auto value = PollMetricForRoute_(
                            logicalDevice, PM_METRIC_CPU_POWER_LIMIT, 0, requestQpc);
                        if (const auto pVal = std::get_if<double>(&value)) {
                            store.statics.cpuPowerLimit = *pVal;
                        }
                        else {
                            throw util::Except<>("Type mismatch for PM_METRIC_CPU_POWER_LIMIT static metric");
                        }
                    }
                }
            }
            catch (...) {
                pmlog_error(util::ReportException("PopulateStaticsToIpc failed for logical device"))
                    .pmwatch(logicalDevice.logicalDeviceId);
            }
        }
    }

    ipc::MetricCapabilities TelemetryCoordinator::GetAvailability() const
    {
        ipc::MetricCapabilities availability;

        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            for (const auto& [metricId, providerDeviceIndex] : logicalDevice.routes) {
                if (providerDeviceIndex >= logicalDevice.providerDevices.size()) {
                    pmlog_error("Route points outside provider device list in GetAvailability")
                        .pmwatch(logicalDeviceId)
                        .pmwatch((int)metricId)
                        .pmwatch(providerDeviceIndex)
                        .pmwatch(logicalDevice.providerDevices.size());
                    continue;
                }

                const auto& providerDevice = logicalDevice.providerDevices[providerDeviceIndex];
                const auto pProvider = providerDevice.pProvider.lock();
                if (!pProvider) {
                    pmlog_error("Route provider expired in GetAvailability")
                        .pmwatch(logicalDeviceId)
                        .pmwatch((int)metricId)
                        .pmwatch(providerDevice.providerDeviceId);
                    continue;
                }

                TelemetryDeviceFingerprint providerFingerprint{};
                bool haveProviderFingerprint = false;
                try {
                    providerFingerprint = pProvider->GetFingerPrint(providerDevice.providerDeviceId);
                    haveProviderFingerprint = true;
                    const auto capabilityMap = pProvider->GetCaps();
                    const auto itDeviceCaps = capabilityMap.find(providerDevice.providerDeviceId);
                    if (itDeviceCaps == capabilityMap.end()) {
                        pmlog_error("Provider did not return caps for routed device in GetAvailability")
                            .pmwatch(logicalDeviceId)
                            .pmwatch((int)providerFingerprint.vendor)
                            .pmwatch(providerFingerprint.deviceName)
                            .pmwatch(providerDevice.providerDeviceId)
                            .pmwatch((int)metricId);
                        continue;
                    }

                    const auto arraySize = itDeviceCaps->second.Check(metricId);
                    if (arraySize == 0) {
                        pmlog_error("Provider caps missing routed metric in GetAvailability")
                            .pmwatch(logicalDeviceId)
                            .pmwatch((int)providerFingerprint.vendor)
                            .pmwatch(providerFingerprint.deviceName)
                            .pmwatch(providerDevice.providerDeviceId)
                            .pmwatch((int)metricId);
                        continue;
                    }

                    const auto existingSize = availability.Check(metricId);
                    if (arraySize > existingSize) {
                        availability.Set(metricId, arraySize);
                    }
                }
                catch (...) {
                    pmlog_error(util::ReportException("GetAvailability provider query failed"))
                        .pmwatch(logicalDeviceId)
                        .pmwatch((int)providerFingerprint.vendor)
                        .pmwatch(providerFingerprint.deviceName)
                        .pmwatch(haveProviderFingerprint)
                        .pmwatch(providerDevice.providerDeviceId)
                        .pmwatch((int)metricId);
                }
            }
        }

        return availability;
    }

    size_t TelemetryCoordinator::PollToIpc(
        const std::span<const PM_QUERY_ELEMENT>& metricUse,
        ipc::ServiceComms& comms) const
    {
        const auto requestQpc = util::GetCurrentTimestamp();
        size_t samplesWritten = 0;

        for (const auto& use : metricUse) {
            if (use.metric >= PM_METRIC_COUNT_) {
                pmlog_warn("Invalid metric id in PollToIpc request")
                    .pmwatch((int)use.metric)
                    .pmwatch(use.deviceId)
                    .pmwatch(use.arrayIndex);
                continue;
            }

            const auto itLogical = logicalDevicesById_.find(use.deviceId);
            if (itLogical == logicalDevicesById_.end()) {
                pmlog_warn("Unknown logical device id in PollToIpc request")
                    .pmwatch(use.deviceId)
                    .pmwatch((int)use.metric)
                    .pmwatch(use.arrayIndex);
                continue;
            }

            const auto& logicalDevice = itLogical->second;
            try {
                const auto value = PollMetricForRoute_(
                    logicalDevice, use.metric, use.arrayIndex, requestQpc);
                const auto qpc = (uint64_t)util::GetCurrentTimestamp();

                if (logicalDevice.logicalDeviceId != ipc::kSystemDeviceId) {
                    auto& store = comms.GetGpuDataStore(logicalDevice.logicalDeviceId);
                    PushValueToTelemetryMap_(store.telemetryData, use.metric, use.arrayIndex, value, qpc);
                }
                else {
                    auto& store = comms.GetSystemDataStore();
                    PushValueToTelemetryMap_(store.telemetryData, use.metric, use.arrayIndex, value, qpc);
                }

                ++samplesWritten;
            }
            catch (...) {
                pmlog_error(util::ReportException("PollToIpc failed while writing sample"))
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch((int)use.metric)
                    .pmwatch(use.arrayIndex);
            }
        }

        return samplesWritten;
    }

    void TelemetryCoordinator::TryCreateConcreteProviders_()
    {
        providerPtrs_.clear();

        try {
            providerPtrs_.push_back(std::make_shared<igcl::IgclTelemetryProvider>());
        }
        catch (const TelemetrySubsystemAbsent&) {
            pmlog_warn(util::ReportException("IGCL telemetry provider unavailable"));
        }
        catch (...) {
            pmlog_error(util::ReportException("IGCL telemetry provider construction failed"));
        }

        try {
            providerPtrs_.push_back(std::make_shared<nvapi::NvapiTelemetryProvider>());
        }
        catch (const TelemetrySubsystemAbsent&) {
            pmlog_warn(util::ReportException("NVAPI telemetry provider unavailable"));
        }
        catch (...) {
            pmlog_error(util::ReportException("NVAPI telemetry provider construction failed"));
        }
    }

    void TelemetryCoordinator::BuildLogicalDevicesAndRoutes_()
    {
        logicalDevicesById_.clear();
        nextLogicalDeviceId_ = 1;

        using CandidateMap = std::unordered_map<PM_METRIC, std::vector<RouteCandidate_>>;
        std::unordered_map<uint32_t, CandidateMap> routeCandidatesByLogicalId;

        for (const auto& pProvider : providerPtrs_) {
            if (!pProvider) {
                pmlog_error("Null pointer in provider ptrs");
                continue;
            }

            try {
                const auto capabilityMap = pProvider->GetCaps();
                for (const auto& [providerDeviceId, capabilities] : capabilityMap) {
                    const auto& fingerprint = pProvider->GetFingerPrint(providerDeviceId);
                    auto& logicalDevice = GetOrCreateLogicalDevice_(fingerprint);
                    const auto logicalDeviceId = logicalDevice.logicalDeviceId;

                    uint32_t providerDeviceIndex = std::numeric_limits<uint32_t>::max();
                    for (uint32_t i = 0; i < logicalDevice.providerDevices.size(); ++i) {
                        const auto& existingProviderDevice = logicalDevice.providerDevices[i];
                        if (existingProviderDevice.providerDeviceId != providerDeviceId) {
                            continue;
                        }
                        const auto pExisting = existingProviderDevice.pProvider.lock();
                        if (pExisting.get() == pProvider.get()) {
                            providerDeviceIndex = i;
                            break;
                        }
                    }

                    if (providerDeviceIndex == std::numeric_limits<uint32_t>::max()) {
                        logicalDevice.providerDevices.push_back(ProviderDevice_{ pProvider, providerDeviceId });
                        providerDeviceIndex = (uint32_t)(logicalDevice.providerDevices.size() - 1u);
                    }

                    auto& metricCandidates = routeCandidatesByLogicalId[logicalDeviceId];
                    for (const auto& [metricId, arraySize] : capabilities) {
                        if (metricId >= PM_METRIC_COUNT_ || arraySize == 0) {
                            pmlog_error("Invalid capability entry from provider")
                                .pmwatch((int)fingerprint.vendor)
                                .pmwatch(fingerprint.deviceName)
                                .pmwatch(providerDeviceId)
                                .pmwatch((int)metricId)
                                .pmwatch(arraySize);
                            continue;
                        }

                        const auto clampedArraySize = (uint32_t)(
                            std::min(arraySize, size_t(std::numeric_limits<uint32_t>::max())));
                        metricCandidates[metricId].push_back(RouteCandidate_{
                            providerDeviceIndex,
                            clampedArraySize,
                            0
                        });
                    }
                }
            }
            catch (...) {
                pmlog_error(util::ReportException("BuildLogicalDevicesAndRoutes provider pass failed"));
                continue;
            }
        }

        for (auto& [logicalDeviceId, metricCandidates] : routeCandidatesByLogicalId) {
            const auto itLogical = logicalDevicesById_.find(logicalDeviceId);
            if (itLogical == logicalDevicesById_.end()) {
                pmlog_error("Missing logical device while finalizing routes")
                    .pmwatch(logicalDeviceId);
                continue;
            }

            auto& logicalDevice = itLogical->second;
            std::unordered_map<uint32_t, uint32_t> providerCoverage;
            for (const auto& metricCandidatesEntry : metricCandidates) {
                const auto& candidates = metricCandidatesEntry.second;
                std::unordered_set<uint32_t> countedProviderIndexes;
                for (const auto& candidate : candidates) {
                    if (countedProviderIndexes.insert(candidate.providerDeviceIndex).second) {
                        ++providerCoverage[candidate.providerDeviceIndex];
                    }
                }
            }

            for (auto& [metricId, candidates] : metricCandidates) {
                for (auto& candidate : candidates) {
                    if (const auto itCoverage = providerCoverage.find(candidate.providerDeviceIndex);
                        itCoverage != providerCoverage.end()) {
                        candidate.providerCoverageScore = itCoverage->second;
                    }
                }

                std::sort(candidates.begin(), candidates.end(), [](const RouteCandidate_& lhs, const RouteCandidate_& rhs) {
                    if (lhs.providerCoverageScore != rhs.providerCoverageScore) {
                        return lhs.providerCoverageScore > rhs.providerCoverageScore;
                    }
                    if (lhs.arraySize != rhs.arraySize) {
                        return lhs.arraySize > rhs.arraySize;
                    }
                    return lhs.providerDeviceIndex < rhs.providerDeviceIndex;
                });

                candidates.erase(std::unique(candidates.begin(), candidates.end(),
                    [](const RouteCandidate_& lhs, const RouteCandidate_& rhs) {
                        return lhs.providerDeviceIndex == rhs.providerDeviceIndex;
                    }), candidates.end());

                if (candidates.empty()) {
                    pmlog_error("No route candidate available for metric")
                        .pmwatch(logicalDeviceId)
                        .pmwatch((int)metricId);
                    continue;
                }

                const auto& selected = candidates.front();
                logicalDevice.routes[metricId] = selected.providerDeviceIndex;
            }
        }

    }

    TelemetryCoordinator::LogicalDevice_& TelemetryCoordinator::GetOrCreateLogicalDevice_(
        const TelemetryDeviceFingerprint& fingerprint)
    {
        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            for (const auto& providerDevice : logicalDevice.providerDevices) {
                const auto pProvider = providerDevice.pProvider.lock();
                if (!pProvider) {
                    pmlog_warn("Expired provider while matching logical devices")
                        .pmwatch(logicalDeviceId)
                        .pmwatch(providerDevice.providerDeviceId);
                    continue;
                }

                try {
                    const auto& candidateFingerprint = pProvider->GetFingerPrint(providerDevice.providerDeviceId);
                    if (TelemetryDeviceFingerprintMatches(candidateFingerprint, fingerprint)) {
                        return logicalDevicesById_.at(logicalDeviceId);
                    }
                }
                catch (...) {
                    pmlog_error(util::ReportException("GetOrCreateLogicalDevice fingerprint query failed"))
                        .pmwatch(logicalDeviceId)
                        .pmwatch(providerDevice.providerDeviceId);
                }
            }
        }

        uint32_t newId = 0;
        if (fingerprint.deviceType == PM_DEVICE_TYPE_SYSTEM &&
            logicalDevicesById_.find(ipc::kSystemDeviceId) == logicalDevicesById_.end()) {
            newId = ipc::kSystemDeviceId;
        }
        else {
            while (true) {
                if (nextLogicalDeviceId_ == ipc::kSystemDeviceId) {
                    ++nextLogicalDeviceId_;
                }
                newId = nextLogicalDeviceId_++;
                if (logicalDevicesById_.find(newId) == logicalDevicesById_.end()) {
                    break;
                }
            }
        }

        LogicalDevice_ logicalDevice{};
        logicalDevice.logicalDeviceId = newId;
        auto itLogical = logicalDevicesById_.emplace(newId, std::move(logicalDevice)).first;
        return itLogical->second;
    }

    void TelemetryCoordinator::PushValueToTelemetryMap_(
        ipc::TelemetryMap& telemetryMap,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        const TelemetryMetricValue& value,
        uint64_t qpc)
    {
        try {
            std::visit([&](const auto& sampleValue) -> void {
                using ValueT = std::decay_t<decltype(sampleValue)>;
                if (auto& rings = telemetryMap.FindRing<ValueT>(metricId); arrayIndex < rings.size()) {
                    if (!rings[arrayIndex].Push(sampleValue, qpc)) {
                        pmlog_error("Failed pushing telemetry sample to ring (backpressured timeout)")
                            .pmwatch((int)metricId)
                            .pmwatch(arrayIndex);
                        throw util::Except<>("Failed pushing telemetry sample to ring (backpressured timeout)");
                    }
                    return;
                }
                else {
                    pmlog_error("Index out of bounds in telemetry ring")
                        .pmwatch((int)metricId)
                        .pmwatch(rings.size())
                        .pmwatch(arrayIndex);
                    throw util::Except<>("Index out of bounds in telemetry ring");
                }
            }, value);
        }
        catch (...) {
            pmlog_error(util::ReportException("PushValueToTelemetryMap failed"))
                .pmwatch((int)metricId)
                .pmwatch(arrayIndex);
            throw;
        }
    }

    TelemetryMetricValue TelemetryCoordinator::PollMetricForRoute_(
        const LogicalDevice_& logicalDevice,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc) const
    {
        const auto itRoute = logicalDevice.routes.find(metricId);
        if (itRoute == logicalDevice.routes.end()) {
            pmlog_error("No route found for metric on logical device")
                .pmwatch(logicalDevice.logicalDeviceId)
                .pmwatch((int)metricId)
                .pmwatch(arrayIndex);
            throw util::Except<>("No route found for metric on logical device");
        }

        const auto providerDeviceIndex = itRoute->second;
        if (providerDeviceIndex >= logicalDevice.providerDevices.size()) {
            pmlog_error("Route points outside provider device list")
                .pmwatch(logicalDevice.logicalDeviceId)
                .pmwatch((int)metricId)
                .pmwatch(providerDeviceIndex)
                .pmwatch(logicalDevice.providerDevices.size());
            throw util::Except<>("Route points outside provider device list");
        }

        const auto& providerDevice = logicalDevice.providerDevices[providerDeviceIndex];
        const auto pProvider = providerDevice.pProvider.lock();
        if (!pProvider) {
            pmlog_error("Route provider expired")
                .pmwatch(logicalDevice.logicalDeviceId)
                .pmwatch((int)metricId)
                .pmwatch(providerDevice.providerDeviceId)
                .pmwatch(arrayIndex);
            throw util::Except<>("Route provider expired");
        }

        try {
            return pProvider->PollMetric(providerDevice.providerDeviceId, metricId, arrayIndex, requestQpc);
        }
        catch (...) {
            pmlog_error(util::ReportException("PollMetricForRoute failed"))
                .pmwatch(logicalDevice.logicalDeviceId)
                .pmwatch(providerDevice.providerDeviceId)
                .pmwatch((int)metricId)
                .pmwatch(arrayIndex);
            throw;
        }
    }

}
