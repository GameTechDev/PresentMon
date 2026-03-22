// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NvmlTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Qpc.h"

#include <chrono>

using namespace pmon;
using namespace util;
using namespace std::literals;

namespace pmon::tel::nvml
{
    NvmlTelemetryProvider::NvmlTelemetryProvider()
    {
        try {
            pNvml_ = std::make_unique<NvmlWrapper>();
        }
        catch (const TelemetrySubsystemAbsent&) {
            throw;
        }
        catch (...) {
            pmlog_error(util::ReportException("NVML wrapper construction failed"));
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize NVIDIA Management Library");
        }

        unsigned int count = 0;
        const auto countResult = pNvml_->DeviceGetCount_v2(&count);
        if (!NvmlWrapper::Ok(countResult)) {
            pmlog_error("nvmlDeviceGetCount_v2 failed").code(countResult);
            throw Except<>("NVML device count query failed");
        }

        for (unsigned int i = 0; i < count; ++i) {
            nvmlDevice_t handle = nullptr;
            const auto handleResult = pNvml_->DeviceGetHandleByIndex_v2(i, &handle);
            if (!NvmlWrapper::Ok(handleResult)) {
                pmlog_warn("nvmlDeviceGetHandleByIndex_v2 failed").code(handleResult)
                    .pmwatch(i);
                continue;
            }

            const auto providerDeviceId = nextProviderDeviceId_;
            const auto emplaceResult = devicesById_.try_emplace(providerDeviceId);
            if (!emplaceResult.second) {
                throw Except<>("Duplicate NVML provider device id encountered");
            }

            auto& device = emplaceResult.first->second;
            device.providerDeviceId = providerDeviceId;
            if (!TryInitializeDevice_(device, handle)) {
                devicesById_.erase(emplaceResult.first);
                continue;
            }

            device.caps = BuildCapsForDevice_(device);
            ++nextProviderDeviceId_;
        }
    }

    ProviderCapabilityMap NvmlTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& NvmlTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("NVML provider device not found");
        }
        return iDevice->second.fingerprint;
    }

    TelemetryMetricValue NvmlTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("NVML provider device not found");
        }

        auto& device = iDevice->second;

        switch (metricId) {
        case PM_METRIC_GPU_VENDOR:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return (int)device.fingerprint.vendor;
        case PM_METRIC_GPU_NAME:
            throw Except<>("PM_METRIC_GPU_NAME is static-only and is not served by poll path");
        case PM_METRIC_GPU_POWER:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto powerMw = PollPowerEndpoint_(device);
            if (!powerMw) {
                return 0.0;
            }
            return (double)*powerMw / 1000.0;
        }
        case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto limitMw = PollPowerLimitEndpoint_(device);
            if (!limitMw) {
                return 0.0;
            }
            return (double)*limitMw / 1000.0;
        }
        case PM_METRIC_GPU_MEM_SIZE:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemoryInfo = PollMemoryInfoEndpoint_(device, requestQpc);
            if (pMemoryInfo == nullptr) {
                return (uint64_t)0;
            }
            return GetLegacyTotalMemoryBytes_(*pMemoryInfo);
        }
        case PM_METRIC_GPU_MEM_USED:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemoryInfo = PollMemoryInfoEndpoint_(device, requestQpc);
            if (pMemoryInfo == nullptr) {
                return (uint64_t)0;
            }
            return (uint64_t)pMemoryInfo->used;
        }
        case PM_METRIC_GPU_MEM_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemoryInfo = PollMemoryInfoEndpoint_(device, requestQpc);
            if (pMemoryInfo == nullptr) {
                return 0.0;
            }

            const auto totalBytes = GetLegacyTotalMemoryBytes_(*pMemoryInfo);
            if (totalBytes == 0) {
                return 0.0;
            }
            return 100.0 * ((double)pMemoryInfo->used / (double)totalBytes);
        }
        default:
            throw Except<>("Unsupported metric for NVML provider");
        }
    }

    void NvmlTelemetryProvider::ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex)
    {
        if (arrayIndex != 0) {
            throw Except<>("NVML scalar metric queried with nonzero array index");
        }
        (void)metricId;
    }

    uint64_t NvmlTelemetryProvider::GetLegacyTotalMemoryBytes_(const nvmlMemory_t& memoryInfo) noexcept
    {
        return (uint64_t)memoryInfo.free;
    }

    bool NvmlTelemetryProvider::TryInitializeDevice_(DeviceState_& device, nvmlDevice_t handle) const
    {
        if (handle == nullptr) {
            pmlog_warn("Null NVML device handle encountered");
            return false;
        }

        device.handle = handle;
        device.fingerprint.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
        device.fingerprint.vendor = PM_DEVICE_VENDOR_NVIDIA;
        device.fingerprint.deviceName = "Unknown NVIDIA Adapter";

        char adapterName[NVML_DEVICE_NAME_BUFFER_SIZE]{};
        const auto nameResult = pNvml_->DeviceGetName(
            device.handle, adapterName, (unsigned int)NVML_DEVICE_NAME_BUFFER_SIZE);
        if (!NvmlWrapper::Ok(nameResult)) {
            pmlog_warn("nvmlDeviceGetName failed").code(nameResult)
                .pmwatch(device.providerDeviceId);
        }
        else if (adapterName[0] != '\0') {
            device.fingerprint.deviceName = adapterName;
        }
        else {
            pmlog_warn("nvmlDeviceGetName returned empty adapter name")
                .pmwatch(device.providerDeviceId);
        }

        nvmlPciInfo_t pciInfo{};
        const auto pciResult = pNvml_->DeviceGetPciInfo_v3(device.handle, &pciInfo);
        if (NvmlWrapper::Ok(pciResult)) {
            device.fingerprint.pciDeviceId = pciInfo.pciDeviceId;
            device.fingerprint.pciSubSystemId = pciInfo.pciSubSystemId;
            device.fingerprint.pciBusId = pciInfo.bus;
        }
        else {
            pmlog_warn("nvmlDeviceGetPciInfo_v3 failed").code(pciResult)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        return true;
    }

    ipc::MetricCapabilities NvmlTelemetryProvider::BuildCapsForDevice_(DeviceState_& device) const
    {
        ipc::MetricCapabilities caps{};
        const auto requestQpc = GetCurrentTimestamp();

        caps.Set(PM_METRIC_GPU_VENDOR, 1);
        caps.Set(PM_METRIC_GPU_NAME, 1);

        if (PollPowerEndpoint_(device)) {
            caps.Set(PM_METRIC_GPU_POWER, 1);
        }

        if (PollPowerLimitEndpoint_(device)) {
            caps.Set(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, 1);
        }

        if (const auto* pMemoryInfo = PollMemoryInfoEndpoint_(device, requestQpc)) {
            const auto totalBytes = GetLegacyTotalMemoryBytes_(*pMemoryInfo);
            if (totalBytes != 0) {
                caps.Set(PM_METRIC_GPU_MEM_SIZE, 1);
                caps.Set(PM_METRIC_GPU_MEM_USED, 1);
                caps.Set(PM_METRIC_GPU_MEM_UTILIZATION, 1);
            }
        }

        return caps;
    }

    const nvmlMemory_t* NvmlTelemetryProvider::PollMemoryInfoEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.memoryInfoEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output ? &*cache.output : nullptr;
        }

        cache.output.reset();
        cache.requestQpc = requestQpc;

        nvmlMemory_t memoryInfo{};
        const auto result = pNvml_->DeviceGetMemoryInfo(device.handle, &memoryInfo);
        if (!NvmlWrapper::Ok(result)) {
            pmlog_warn("nvmlDeviceGetMemoryInfo failed").code(result).every(60s)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return nullptr;
        }

        cache.output = memoryInfo;
        return &*cache.output;
    }

    std::optional<unsigned int> NvmlTelemetryProvider::PollPowerEndpoint_(const DeviceState_& device) const
    {
        unsigned int powerMw = 0;
        const auto result = pNvml_->DeviceGetPowerUsage(device.handle, &powerMw);
        if (!NvmlWrapper::Ok(result)) {
            pmlog_warn("nvmlDeviceGetPowerUsage failed").code(result).every(60s)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return {};
        }

        return powerMw;
    }

    std::optional<unsigned int> NvmlTelemetryProvider::PollPowerLimitEndpoint_(const DeviceState_& device) const
    {
        unsigned int limitMw = 0;
        const auto result = pNvml_->DeviceGetPowerManagementLimit(device.handle, &limitMw);
        if (!NvmlWrapper::Ok(result)) {
            pmlog_warn("nvmlDeviceGetPowerManagementLimit failed").code(result).every(60s)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return {};
        }

        return limitMw;
    }
}
