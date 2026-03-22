// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryCoordinator.h"
#include "Exceptions.h"
#include "TelemetryDeviceFingerprint.h"
#include "Logging.h"
#include "adl/AmdTelemetryProvider.h"
#include "igcl/IgclTelemetryProvider.h"
#include "nvapi/NvapiTelemetryProvider.h"
#include "nvml/NvmlTelemetryProvider.h"
#include "uci/UciTelemetryProvider.h"
#include "wmi/WmiTelemetryProvider.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Qpc.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/metadata/MetricList.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <sstream>
#include <span>
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

        bool IsStaticMetric_(PM_METRIC metricId) noexcept
        {
            switch (metricId) {
#define X_(id_, type_, ...) case id_: return type_ == PM_METRIC_TYPE_STATIC;
                METRIC_LIST(X_)
#undef X_
            default:
                return false;
            }
        }

        const char* GetDeviceNameOrUnknown_(const TelemetryDeviceFingerprint& fingerprint) noexcept
        {
            return fingerprint.deviceName.empty() ? "UNKNOWN_GPU" : fingerprint.deviceName.c_str();
        }

    }

    TelemetryCoordinator::TelemetryCoordinator(uint32_t pollRateMs)
        :
        pollRateMs_{ std::max(pollRateMs, 1u) }
    {
        TryCreateConcreteProviders_();
        BuildLogicalDevicesAndRoutes_();
    }

    std::vector<TelemetryCoordinator::AdapterInfo> TelemetryCoordinator::EnumerateAdapters() const
    {
        std::vector<AdapterInfo> adapters;
        adapters.reserve(logicalDevicesById_.size());

        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            if (logicalDeviceId == ipc::kSystemDeviceId) {
                continue;
            }

            AdapterInfo adapter{};
            adapter.id = logicalDeviceId;
            const auto fingerprint = ResolveLogicalDeviceFingerprint_(logicalDevice);
            adapter.vendor = fingerprint.vendor;
            if (!fingerprint.deviceName.empty()) {
                adapter.name = fingerprint.deviceName;
            }
            else {
                adapter.name = "UNKNOWN_GPU";
            }

            adapter.gpuMemorySize = QueryGpuMemorySize_(logicalDevice).value;
            const auto requestQpc = util::GetCurrentTimestamp();
            const auto populateStaticMetric = [&]<typename T>(PM_METRIC metricId, T& destination) {
                if (metricId == PM_METRIC_GPU_MEM_SIZE) {
                    if constexpr (std::is_same_v<T, uint64_t>) {
                        destination = adapter.gpuMemorySize;
                    }
                    return;
                }
                if (!logicalDevice.routes.contains(metricId)) {
                    return;
                }

                try {
                    const auto value = PollMetricForRoute_(logicalDevice, metricId, 0, requestQpc);
                    if (const auto pVal = std::get_if<T>(&value)) {
                        destination = *pVal;
                    }
                    else {
                        throw util::Except<>("Type mismatch while enumerating adapter static metric");
                    }
                }
                catch (...) {
                    pmlog_error(util::ReportException("Adapter static metric query failed"))
                        .pmwatch(logicalDeviceId)
                        .pmwatch((int)metricId);
                }
            };

            populateStaticMetric(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, adapter.gpuSustainedPowerLimit);
            populateStaticMetric(PM_METRIC_GPU_MEM_MAX_BANDWIDTH, adapter.gpuMemoryMaxBandwidth);

            adapters.push_back(std::move(adapter));
        }

        std::sort(adapters.begin(), adapters.end(), [](const AdapterInfo& lhs, const AdapterInfo& rhs) {
            if (lhs.gpuMemorySize != rhs.gpuMemorySize) {
                return lhs.gpuMemorySize > rhs.gpuMemorySize;
            }
            return lhs.id < rhs.id;
        });

        return adapters;
    }

    void TelemetryCoordinator::RegisterDevicesToIpc(ipc::ServiceComms& comms) const
    {
        bool cpuRegistered = false;

        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            auto fingerprint = ResolveLogicalDeviceFingerprint_(logicalDevice);
            auto caps = BuildRoutedCapabilities_(logicalDevice);

            const auto vendor = fingerprint.vendor;
            const auto name = !fingerprint.deviceName.empty() ?
                fingerprint.deviceName : std::string{ "UNKNOWN_CPU" };

            try {
                if (logicalDeviceId != ipc::kSystemDeviceId) {
                    comms.RegisterGpuDevice(
                        logicalDeviceId,
                        vendor,
                        !fingerprint.deviceName.empty() ?
                            fingerprint.deviceName : std::string{ "UNKNOWN_GPU" },
                        caps,
                        fingerprint.luid);
                }
                else {
                    comms.RegisterCpuDevice(vendor, name, caps);
                    cpuRegistered = true;
                }
            }
            catch (...) {
                pmlog_error(util::ReportException("RegisterDevicesToIpc failed for logical device"))
                    .pmwatch(logicalDeviceId)
                    .pmwatch((int)vendor)
                    .pmwatch(fingerprint.deviceName);
            }
        }

        if (!cpuRegistered) {
            comms.RegisterCpuDevice(PM_DEVICE_VENDOR_UNKNOWN, "UNKNOWN_CPU", {});
        }
        comms.FinalizeGpuDevices();
    }

    void TelemetryCoordinator::PopulateStaticsToIpc(ipc::ServiceComms& comms) const
    {
        const auto requestQpc = util::GetCurrentTimestamp();
        // TODO: make static population dynamic by metric id and metadata instead of hardcoded checks.
        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            const auto providerFingerprint = ResolveLogicalDeviceFingerprint_(logicalDevice);

            try {
                if (logicalDeviceId != ipc::kSystemDeviceId) {
                    auto& store = comms.GetGpuDataStore(logicalDevice.logicalDeviceId);
                    store.statics.vendor = providerFingerprint.vendor;
                    store.statics.name = providerFingerprint.deviceName.c_str();

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
                    store.statics.cpuVendor = providerFingerprint.vendor;
                    store.statics.cpuName = providerFingerprint.deviceName.c_str();

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

        for (const auto& entry : logicalDevicesById_) {
            const auto& logicalDevice = entry.second;
            const auto routedCaps = BuildRoutedCapabilities_(logicalDevice);
            for (const auto& [metricId, arraySize] : routedCaps) {
                const auto existingSize = availability.Check(metricId);
                if (arraySize > existingSize) {
                    availability.Set(metricId, arraySize);
                }
            }
        }

        return availability;
    }

    void TelemetryCoordinator::SetPollRate(uint32_t pollRateMs)
    {
        pollRateMs_ = std::max(pollRateMs, 1u);
        for (const auto& pProvider : providerPtrs_) {
            try {
                pProvider->SetPollRate(pollRateMs_);
            }
            catch (...) {
                pmlog_error(util::ReportException("Telemetry provider poll-rate update failed"));
            }
        }
    }

    void TelemetryCoordinator::SetMetricUse(const svc::DeviceMetricUse& metricUse)
    {
        for (const auto& pProvider : providerPtrs_) {
            try {
                pProvider->SetMetricUse(metricUse);
            }
            catch (...) {
                pmlog_error(util::ReportException("Telemetry provider metric-use update failed"));
            }
        }
    }

    size_t TelemetryCoordinator::PollToIpc(
        const svc::DeviceMetricUse& metricUse,
        ipc::ServiceComms& comms) const
    {
        const auto requestQpc = util::GetCurrentTimestamp();
        size_t samplesWritten = 0;

        for (const auto& [deviceId, deviceMetricUse] : metricUse) {
            if (deviceId == 0) {
                continue;
            }

            const auto itLogical = logicalDevicesById_.find(deviceId);
            if (itLogical == logicalDevicesById_.end()) {
                pmlog_warn("Unknown logical device id in PollToIpc request")
                    .pmwatch(deviceId);
                continue;
            }

            const auto& logicalDevice = itLogical->second;
            for (const auto& use : deviceMetricUse) {
                if (use.deviceId != deviceId) {
                    pmlog_warn("Metric use device id did not match owning device bucket")
                        .pmwatch(deviceId)
                        .pmwatch(use.deviceId)
                        .pmwatch((int)use.metricId)
                        .pmwatch(use.arrayIdx);
                    continue;
                }
                if (use.metricId >= PM_METRIC_COUNT_) {
                    pmlog_warn("Invalid metric id in PollToIpc request")
                        .pmwatch((int)use.metricId)
                        .pmwatch(deviceId)
                        .pmwatch(use.arrayIdx);
                    continue;
                }
                if (IsStaticMetric_(use.metricId)) {
                    continue;
                }

                try {
                    const auto value = PollMetricForRoute_(
                        logicalDevice, use.metricId, use.arrayIdx, requestQpc);
                    const auto qpc = (uint64_t)util::GetCurrentTimestamp();

                    if (logicalDevice.logicalDeviceId != ipc::kSystemDeviceId) {
                        auto& store = comms.GetGpuDataStore(logicalDevice.logicalDeviceId);
                        PushValueToTelemetryMap_(store.telemetryData, use.metricId, use.arrayIdx, value, qpc);
                    }
                    else {
                        auto& store = comms.GetSystemDataStore();
                        PushValueToTelemetryMap_(store.telemetryData, use.metricId, use.arrayIdx, value, qpc);
                    }

                    ++samplesWritten;
                }
                catch (...) {
                    pmlog_error(util::ReportException("PollToIpc failed while writing sample"))
                        .pmwatch(logicalDevice.logicalDeviceId)
                        .pmwatch((int)use.metricId)
                        .pmwatch(use.arrayIdx);
                }
            }
        }

        return samplesWritten;
    }

    void TelemetryCoordinator::TryCreateConcreteProviders_()
    {
        providerPtrs_.clear();

        const auto tryAddProvider = [this]<class ProviderT>(const char* missingMessage, const char* failureMessage) {
            try {
                auto pProvider = std::make_shared<ProviderT>();
                pProvider->SetPollRate(pollRateMs_);
                providerPtrs_.push_back(std::move(pProvider));
            }
            catch (const TelemetrySubsystemAbsent&) {
                pmlog_dbg(util::ReportException(missingMessage));
            }
            catch (...) {
                pmlog_error(util::ReportException(failureMessage));
            }
        };

        tryAddProvider.operator()<wmi::WmiTelemetryProvider>(
            "WMI telemetry provider unavailable",
            "WMI telemetry provider construction failed");
        tryAddProvider.operator()<uci::UciTelemetryProvider>(
            "UCI telemetry provider unavailable",
            "UCI telemetry provider construction failed");
        tryAddProvider.operator()<adl::AmdTelemetryProvider>(
            "ADL telemetry provider unavailable",
            "ADL telemetry provider construction failed");
        tryAddProvider.operator()<igcl::IgclTelemetryProvider>(
            "IGCL telemetry provider unavailable",
            "IGCL telemetry provider construction failed");
        tryAddProvider.operator()<nvapi::NvapiTelemetryProvider>(
            "NVAPI telemetry provider unavailable",
            "NVAPI telemetry provider construction failed");
        tryAddProvider.operator()<nvml::NvmlTelemetryProvider>(
            "NVML telemetry provider unavailable",
            "NVML telemetry provider construction failed");
    }

    void TelemetryCoordinator::BuildLogicalDevicesAndRoutes_()
    {
        logicalDevicesById_.clear();
        nextLogicalDeviceId_ = 1;

        using CandidateMap = std::unordered_map<PM_METRIC, std::vector<RouteCandidate_>>;
        std::unordered_map<uint32_t, CandidateMap> routeCandidatesByLogicalId;

        for (const auto& pProvider : providerPtrs_) {
            try {
                const auto capabilityMap = pProvider->GetCaps();
                for (const auto& [providerDeviceId, capabilities] : capabilityMap) {
                    const auto& fingerprint = pProvider->GetFingerPrint(providerDeviceId);
                    pmlog_dbg("Provider device LUID")
                        .pmwatch((int)fingerprint.vendor)
                        .pmwatch(fingerprint.deviceName)
                        .pmwatch(providerDeviceId)
                        .pmwatch(fingerprint.LuidAsString());
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

        ReassignGpuLogicalDeviceIdsByMemorySize_();
    }

    void TelemetryCoordinator::ReassignGpuLogicalDeviceIdsByMemorySize_()
    {
        struct GpuLogicalDeviceOrdering_
        {
            uint32_t oldId = 0;
            std::string name{};
            bool isIntegratedAdapter = false;
            bool hasSustainedPowerLimit = false;
            double sustainedPowerLimit = 0.0;
            bool hasMemorySize = false;
            uint64_t gpuMemorySize = 0;
        };

        std::vector<GpuLogicalDeviceOrdering_> gpuDevices;
        gpuDevices.reserve(logicalDevicesById_.size());

        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            if (logicalDeviceId == ipc::kSystemDeviceId) {
                continue;
            }
            const auto fingerprint = ResolveLogicalDeviceFingerprint_(logicalDevice);
            const auto powerInfo = QueryGpuSustainedPowerLimit_(logicalDevice);
            const auto memInfo = QueryGpuMemorySize_(logicalDevice);

            gpuDevices.push_back(GpuLogicalDeviceOrdering_{
                .oldId = logicalDeviceId,
                .name = GetDeviceNameOrUnknown_(fingerprint),
                .isIntegratedAdapter = fingerprint.isIntegratedAdapter,
                .hasSustainedPowerLimit = powerInfo.hasValue,
                .sustainedPowerLimit = powerInfo.value,
                .hasMemorySize = memInfo.hasValue,
                .gpuMemorySize = memInfo.value,
            });
        }

        const bool allHaveSustainedPowerLimit = !gpuDevices.empty() &&
            std::ranges::all_of(gpuDevices, [](const GpuLogicalDeviceOrdering_& gpuDevice) {
                return gpuDevice.hasSustainedPowerLimit;
            });

        std::sort(gpuDevices.begin(), gpuDevices.end(),
            [allHaveSustainedPowerLimit](const GpuLogicalDeviceOrdering_& lhs, const GpuLogicalDeviceOrdering_& rhs) {
                if (lhs.isIntegratedAdapter != rhs.isIntegratedAdapter) {
                    return rhs.isIntegratedAdapter;
                }
                if (allHaveSustainedPowerLimit) {
                    if (lhs.sustainedPowerLimit != rhs.sustainedPowerLimit) {
                        return lhs.sustainedPowerLimit > rhs.sustainedPowerLimit;
                    }
                }
                else {
                    if (lhs.hasMemorySize != rhs.hasMemorySize) {
                        return lhs.hasMemorySize > rhs.hasMemorySize;
                    }
                    if (lhs.gpuMemorySize != rhs.gpuMemorySize) {
                        return lhs.gpuMemorySize > rhs.gpuMemorySize;
                    }
                }
                return lhs.oldId < rhs.oldId;
            });

        std::unordered_map<uint32_t, LogicalDevice_> reorderedLogicalDevices;
        reorderedLogicalDevices.reserve(logicalDevicesById_.size());

        if (const auto itSystem = logicalDevicesById_.find(ipc::kSystemDeviceId);
            itSystem != logicalDevicesById_.end()) {
            reorderedLogicalDevices.emplace(ipc::kSystemDeviceId, itSystem->second);
        }

        uint32_t nextGpuLogicalId = 1;
        for (const auto& gpuDevice : gpuDevices) {
            auto nh = logicalDevicesById_.extract(gpuDevice.oldId);
            if (nh.empty()) {
                pmlog_error("Failed extracting logical GPU device during renumber")
                    .pmwatch(gpuDevice.oldId);
                continue;
            }

            if (gpuDevice.oldId != nextGpuLogicalId) {
                std::string name = "UNKNOWN_GPU";
                try {
                    const auto fingerprint = ResolveLogicalDeviceFingerprint_(nh.mapped());
                    name = GetDeviceNameOrUnknown_(fingerprint);
                }
                catch (...) {
                    pmlog_warn(util::ReportException("Failed resolving GPU fingerprint while logging ID reassignment"))
                        .pmwatch(gpuDevice.oldId)
                        .pmwatch(nextGpuLogicalId);
                }
                pmlog_dbg(std::format(
                    "GPU logical device id reassigned: name=\"{}\" oldId={} newId={} integrated={} sustainedPowerLimit={} hasSustainedPowerLimit={} memorySize={} hasMemorySize={} rankingBasis={}",
                    name,
                    gpuDevice.oldId,
                    nextGpuLogicalId,
                    gpuDevice.isIntegratedAdapter,
                    gpuDevice.sustainedPowerLimit,
                    gpuDevice.hasSustainedPowerLimit,
                    gpuDevice.gpuMemorySize,
                    gpuDevice.hasMemorySize,
                    allHaveSustainedPowerLimit ? "sustainedPower" : "memory"));
            }
            nh.key() = nextGpuLogicalId;
            nh.mapped().logicalDeviceId = nextGpuLogicalId;
            reorderedLogicalDevices.insert(std::move(nh));
            ++nextGpuLogicalId;
        }

        logicalDevicesById_ = std::move(reorderedLogicalDevices);
        nextLogicalDeviceId_ = std::max(nextGpuLogicalId, 1u);
        if (nextLogicalDeviceId_ == ipc::kSystemDeviceId) {
            ++nextLogicalDeviceId_;
        }

        struct FinalGpuOrderEntry_
        {
            uint32_t id = 0;
            PM_DEVICE_VENDOR vendor = PM_DEVICE_VENDOR_UNKNOWN;
            std::string name{};
            bool isIntegratedAdapter = false;
            bool hasSustainedPowerLimit = false;
            double sustainedPowerLimit = 0.0;
            bool hasMemorySize = false;
            uint64_t gpuMemorySize = 0;
        };

        std::vector<FinalGpuOrderEntry_> finalGpuOrder;
        finalGpuOrder.reserve(logicalDevicesById_.size());

        for (const auto& [logicalDeviceId, logicalDevice] : logicalDevicesById_) {
            if (logicalDeviceId == ipc::kSystemDeviceId) {
                continue;
            }

            FinalGpuOrderEntry_ entry{};
            entry.id = logicalDeviceId;
            const auto powerInfo = QueryGpuSustainedPowerLimit_(logicalDevice);
            const auto memInfo = QueryGpuMemorySize_(logicalDevice);
            entry.hasSustainedPowerLimit = powerInfo.hasValue;
            entry.sustainedPowerLimit = powerInfo.value;
            entry.hasMemorySize = memInfo.hasValue;
            entry.gpuMemorySize = memInfo.value;
            try {
                const auto fingerprint = ResolveLogicalDeviceFingerprint_(logicalDevice);
                entry.vendor = fingerprint.vendor;
                entry.name = GetDeviceNameOrUnknown_(fingerprint);
                entry.isIntegratedAdapter = fingerprint.isIntegratedAdapter;
            }
            catch (...) {
                pmlog_warn(util::ReportException("Failed resolving GPU fingerprint while logging final order"))
                    .pmwatch(logicalDeviceId);
                entry.name = "UNKNOWN_GPU";
            }
            finalGpuOrder.push_back(std::move(entry));
        }

        std::sort(finalGpuOrder.begin(), finalGpuOrder.end(), [](const FinalGpuOrderEntry_& lhs, const FinalGpuOrderEntry_& rhs) {
            return lhs.id < rhs.id;
        });

        std::ostringstream oss;
        oss << "Final GPU logical device ordering:";
        oss << "\n  rankingBasis: " << (allHaveSustainedPowerLimit ? "sustainedPower" : "memory");
        for (size_t i = 0; i < finalGpuOrder.size(); ++i) {
            const auto& entry = finalGpuOrder[i];
            oss << "\n" << entry.name;
            oss << "\n  id: " << entry.id;
            oss << "\n  vendor: " << (int)entry.vendor;
            oss << "\n  integrated: " << entry.isIntegratedAdapter;
            oss << "\n  hasSustainedPowerLimit: " << entry.hasSustainedPowerLimit;
            oss << "\n  sustainedPowerLimit: " << entry.sustainedPowerLimit;
            oss << "\n  hasMemorySize: " << entry.hasMemorySize;
            oss << "\n  memorySize: " << entry.gpuMemorySize;
        }
        pmlog_dbg(oss.str());
    }

    TelemetryCoordinator::GpuMemoryOrderingInfo_ TelemetryCoordinator::QueryGpuMemorySize_(const LogicalDevice_& logicalDevice) const
    {
        GpuMemoryOrderingInfo_ out{};
        if (logicalDevice.logicalDeviceId == ipc::kSystemDeviceId ||
            !logicalDevice.routes.contains(PM_METRIC_GPU_MEM_SIZE)) {
            return out;
        }

        try {
            const auto value = PollMetricForRoute_(
                logicalDevice, PM_METRIC_GPU_MEM_SIZE, 0, util::GetCurrentTimestamp());
            if (const auto pVal = std::get_if<uint64_t>(&value)) {
                out.hasValue = true;
                out.value = *pVal;
                return out;
            }

            throw util::Except<>("Type mismatch while querying GPU memory size");
        }
        catch (...) {
            pmlog_error(util::ReportException("GPU memory size query failed while ordering logical devices"))
                .pmwatch(logicalDevice.logicalDeviceId);
            return out;
        }
    }

    TelemetryCoordinator::GpuPowerOrderingInfo_ TelemetryCoordinator::QueryGpuSustainedPowerLimit_(const LogicalDevice_& logicalDevice) const
    {
        GpuPowerOrderingInfo_ out{};
        if (logicalDevice.logicalDeviceId == ipc::kSystemDeviceId ||
            !logicalDevice.routes.contains(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT)) {
            return out;
        }

        try {
            const auto value = PollMetricForRoute_(
                logicalDevice, PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, 0, util::GetCurrentTimestamp());
            if (const auto pVal = std::get_if<double>(&value)) {
                out.hasValue = true;
                out.value = *pVal;
                return out;
            }

            throw util::Except<>("Type mismatch while querying GPU sustained power limit");
        }
        catch (...) {
            pmlog_error(util::ReportException("GPU sustained power limit query failed while ordering logical devices"))
                .pmwatch(logicalDevice.logicalDeviceId);
            return out;
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

    TelemetryDeviceFingerprint TelemetryCoordinator::ResolveLogicalDeviceFingerprint_(
        const LogicalDevice_& logicalDevice) const
    {
        TelemetryDeviceFingerprint fingerprint{};
        bool haveFingerprint = false;

        for (const auto& providerDevice : logicalDevice.providerDevices) {
            const auto pProvider = providerDevice.pProvider.lock();
            if (!pProvider) {
                pmlog_warn("Expired provider while resolving logical-device fingerprint")
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch(providerDevice.providerDeviceId);
                continue;
            }

            try {
                const auto providerFingerprint = pProvider->GetFingerPrint(providerDevice.providerDeviceId);
                if (!haveFingerprint) {
                    fingerprint = providerFingerprint;
                    haveFingerprint = true;
                }
                else {
                    if (!fingerprint.luid.empty() &&
                        !providerFingerprint.luid.empty() &&
                        fingerprint.luid != providerFingerprint.luid) {
                        pmlog_warn("Conflicting provider LUID while resolving logical-device fingerprint")
                            .pmwatch(logicalDevice.logicalDeviceId)
                            .pmwatch(providerDevice.providerDeviceId);
                    }
                    MergeTelemetryDeviceFingerprint(fingerprint, providerFingerprint);
                }
            }
            catch (...) {
                pmlog_error(util::ReportException("Logical-device fingerprint query failed"))
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch(providerDevice.providerDeviceId);
            }
        }

        if (!haveFingerprint) {
            pmlog_error("No provider-device fingerprint resolved for logical device")
                .pmwatch(logicalDevice.logicalDeviceId);
            throw util::Except<>("No provider-device fingerprint resolved for logical device");
        }

        return fingerprint;
    }

    ipc::MetricCapabilities TelemetryCoordinator::BuildRoutedCapabilities_(
        const LogicalDevice_& logicalDevice) const
    {
        ipc::MetricCapabilities caps{};

        for (const auto& [metricId, providerDeviceIndex] : logicalDevice.routes) {
            if (providerDeviceIndex >= logicalDevice.providerDevices.size()) {
                pmlog_error("Route points outside provider device list in BuildRoutedCapabilities")
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch((int)metricId)
                    .pmwatch(providerDeviceIndex)
                    .pmwatch(logicalDevice.providerDevices.size());
                continue;
            }

            const auto& providerDevice = logicalDevice.providerDevices[providerDeviceIndex];
            const auto pProvider = providerDevice.pProvider.lock();
            if (!pProvider) {
                pmlog_error("Route provider expired in BuildRoutedCapabilities")
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch((int)metricId)
                    .pmwatch(providerDevice.providerDeviceId);
                continue;
            }

            TelemetryDeviceFingerprint providerFingerprint{};
            try {
                providerFingerprint = pProvider->GetFingerPrint(providerDevice.providerDeviceId);
                const auto capabilityMap = pProvider->GetCaps();
                const auto itDeviceCaps = capabilityMap.find(providerDevice.providerDeviceId);
                if (itDeviceCaps == capabilityMap.end()) {
                    pmlog_error("Provider did not return caps for routed device")
                        .pmwatch(logicalDevice.logicalDeviceId)
                        .pmwatch((int)providerFingerprint.vendor)
                        .pmwatch(providerFingerprint.deviceName)
                        .pmwatch(providerDevice.providerDeviceId)
                        .pmwatch((int)metricId);
                    continue;
                }

                const auto arraySize = itDeviceCaps->second.Check(metricId);
                if (arraySize == 0) {
                    pmlog_error("Provider caps missing routed metric")
                        .pmwatch(logicalDevice.logicalDeviceId)
                        .pmwatch((int)providerFingerprint.vendor)
                        .pmwatch(providerFingerprint.deviceName)
                        .pmwatch(providerDevice.providerDeviceId)
                        .pmwatch((int)metricId);
                    continue;
                }

                caps.Set(metricId, arraySize);
            }
            catch (...) {
                pmlog_error(util::ReportException("BuildRoutedCapabilities provider query failed"))
                    .pmwatch(logicalDevice.logicalDeviceId)
                    .pmwatch((int)providerFingerprint.vendor)
                    .pmwatch(providerFingerprint.deviceName)
                    .pmwatch(providerDevice.providerDeviceId)
                    .pmwatch((int)metricId);
            }
        }

        return caps;
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
