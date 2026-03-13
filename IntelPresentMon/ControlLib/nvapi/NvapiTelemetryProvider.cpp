// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "NvapiTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"

#include <algorithm>
#include <chrono>

using namespace pmon;
using namespace util;

namespace pmon::tel::nvapi
{
    NvapiTelemetryProvider::NvapiTelemetryProvider()
    {
        try {
            pNvapi_ = std::make_unique<pwr::nv::NvapiWrapper>();
        }
        catch (const TelemetrySubsystemAbsent&) {
            throw;
        }
        catch (...) {
            pmlog_error(util::ReportException("NVAPI wrapper construction failed"));
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize NVIDIA API");
        }

        std::vector<NvPhysicalGpuHandle> handles((size_t)NVAPI_MAX_PHYSICAL_GPUS);
        NvU32 count = (NvU32)handles.size();
        const auto enumResult = pNvapi_->EnumPhysicalGPUs(handles.data(), &count);
        if (enumResult == NVAPI_NVIDIA_DEVICE_NOT_FOUND) {
            return;
        }
        if (!pwr::nv::NvapiWrapper::Ok(enumResult)) {
            pmlog_error("NvAPI_EnumPhysicalGPUs failed").code(enumResult);
            throw Except<>("NVAPI physical GPU enumeration failed");
        }

        handles.resize((size_t)count);
        for (const auto handle : handles) {
            const auto providerDeviceId = nextProviderDeviceId_;
            const auto emplaceResult = devicesById_.try_emplace(providerDeviceId);
            if (!emplaceResult.second) {
                throw Except<>("Duplicate NVAPI provider device id encountered");
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

    ProviderCapabilityMap NvapiTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& NvapiTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("NVAPI provider device not found");
        }
        return iDevice->second.fingerprint;
    }

    TelemetryMetricValue NvapiTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("NVAPI provider device not found");
        }

        auto& device = iDevice->second;

        switch (metricId) {
        case PM_METRIC_GPU_VENDOR:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return (int)device.fingerprint.vendor;
        case PM_METRIC_GPU_NAME:
            throw Except<>("PM_METRIC_GPU_NAME is static-only and is not served by poll path");
        default:
            return PollNvapiMetric_(device, metricId, arrayIndex, requestQpc);
        }
    }

    void NvapiTelemetryProvider::ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex)
    {
        if (arrayIndex != 0) {
            throw Except<>("NVAPI scalar metric queried with nonzero array index");
        }
        (void)metricId;
    }

    bool NvapiTelemetryProvider::TryInitializeDevice_(
        DeviceState_& device,
        NvPhysicalGpuHandle handle) const
    {
        if (handle == nullptr) {
            pmlog_warn("Null NVAPI physical GPU handle encountered");
            return false;
        }

        device.handle = handle;
        device.fingerprint.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
        device.fingerprint.vendor = PM_DEVICE_VENDOR_NVIDIA;
        device.fingerprint.deviceName = "Unknown NVIDIA Adapter";

        NvAPI_ShortString adapterName{};
        const auto fullNameResult = pNvapi_->GPU_GetFullName(device.handle, adapterName);
        if (pwr::nv::NvapiWrapper::Ok(fullNameResult)) {
            device.fingerprint.deviceName = adapterName;
        }
        else {
            pmlog_warn("NvAPI_GPU_GetFullName failed").code(fullNameResult)
                .pmwatch(device.providerDeviceId);
        }

        NvU32 deviceId = 0;
        NvU32 subSystemId = 0;
        NvU32 revisionId = 0;
        NvU32 extDeviceId = 0;
        const auto pciResult = pNvapi_->GPU_GetPCIIdentifiers(
            device.handle, &deviceId, &subSystemId, &revisionId, &extDeviceId);
        if (pwr::nv::NvapiWrapper::Ok(pciResult)) {
            device.fingerprint.pciDeviceId = deviceId;
            device.fingerprint.pciSubSystemId = subSystemId;
        }
        else {
            pmlog_warn("NvAPI_GPU_GetPCIIdentifiers failed").code(pciResult)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        NvU32 busId = 0;
        const auto busResult = pNvapi_->GPU_GetBusId(device.handle, &busId);
        if (pwr::nv::NvapiWrapper::Ok(busResult)) {
            device.fingerprint.pciBusId = busId;
        }
        else {
            pmlog_warn("NvAPI_GPU_GetBusId failed").code(busResult)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        (void)revisionId;
        (void)extDeviceId;

        return true;
    }

    ipc::MetricCapabilities NvapiTelemetryProvider::BuildCapsForDevice_(DeviceState_& device) const
    {
        ipc::MetricCapabilities caps{};

        caps.Set(PM_METRIC_GPU_VENDOR, 1);
        caps.Set(PM_METRIC_GPU_NAME, 1);

        double value = 0.0;

        if (const auto* pThermals = PollThermalEndpoint_(device, 0)) {
            if (TryGetThermalValue_(*pThermals, NVAPI_THERMAL_TARGET_GPU, true, value)) {
                caps.Set(PM_METRIC_GPU_TEMPERATURE, 1);
            }
            if (TryGetThermalValue_(*pThermals, NVAPI_THERMAL_TARGET_MEMORY, false, value)) {
                caps.Set(PM_METRIC_GPU_MEM_TEMPERATURE, 1);
            }
        }

        if (const auto* pClocks = PollClockEndpoint_(device, 0)) {
            if (TryGetClockValue_(*pClocks, NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS, value)) {
                caps.Set(PM_METRIC_GPU_FREQUENCY, 1);
            }
            if (TryGetClockValue_(*pClocks, NVAPI_GPU_PUBLIC_CLOCK_MEMORY, value)) {
                caps.Set(PM_METRIC_GPU_MEM_FREQUENCY, 1);
            }
        }

        if (const auto* pUtilization = PollUtilizationEndpoint_(device, 0)) {
            if (TryGetUtilizationValue_(
                *pUtilization, pwr::nv::NVAPI_GPU_UTILIZATION_DOMAIN_GPU, value)) {
                caps.Set(PM_METRIC_GPU_UTILIZATION, 1);
            }
            if (TryGetUtilizationValue_(
                *pUtilization, pwr::nv::NVAPI_GPU_UTILIZATION_DOMAIN_VID, value)) {
                caps.Set(PM_METRIC_GPU_MEDIA_UTILIZATION, 1);
            }
        }

        if (PollTachEndpoint_(device, 0) != nullptr) {
            caps.Set(PM_METRIC_GPU_FAN_SPEED, 1);
        }

        return caps;
    }

    const NV_GPU_THERMAL_SETTINGS* NvapiTelemetryProvider::PollThermalEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.thermalEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output.version == NV_GPU_THERMAL_SETTINGS_VER_2 ? &cache.output : nullptr;
        }

        cache.output = {
            .version = NV_GPU_THERMAL_SETTINGS_VER_2,
        };
        cache.requestQpc = requestQpc;

        const auto result = pNvapi_->GPU_GetThermalSettings(
            device.handle, NVAPI_THERMAL_TARGET_ALL, &cache.output);
        if (!pwr::nv::NvapiWrapper::Ok(result)) {
            pmlog_warn("NvAPI_GPU_GetThermalSettings failed").code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            cache.output.version = 0;
            return nullptr;
        }

        return &cache.output;
    }

    const NV_GPU_CLOCK_FREQUENCIES* NvapiTelemetryProvider::PollClockEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.clockEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output.version == NV_GPU_CLOCK_FREQUENCIES_VER_3 ? &cache.output : nullptr;
        }

        cache.output = {
            .version = NV_GPU_CLOCK_FREQUENCIES_VER_3,
        };
        cache.requestQpc = requestQpc;

        const auto result = pNvapi_->GPU_GetAllClockFrequencies(device.handle, &cache.output);
        if (!pwr::nv::NvapiWrapper::Ok(result)) {
            pmlog_warn("NvAPI_GPU_GetAllClockFrequencies failed").code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            cache.output.version = 0;
            return nullptr;
        }

        return &cache.output;
    }

    const NV_GPU_DYNAMIC_PSTATES_INFO_EX* NvapiTelemetryProvider::PollUtilizationEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.utilizationEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output.version == NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER ? &cache.output : nullptr;
        }

        cache.output = {
            .version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER,
        };
        cache.requestQpc = requestQpc;

        const auto result = pNvapi_->GPU_GetDynamicPstatesInfoEx(device.handle, &cache.output);
        if (!pwr::nv::NvapiWrapper::Ok(result)) {
            pmlog_warn("NvAPI_GPU_GetDynamicPstatesInfoEx failed").code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            cache.output.version = 0;
            return nullptr;
        }

        return &cache.output;
    }

    const NvU32* NvapiTelemetryProvider::PollTachEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.tachEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output ? &*cache.output : nullptr;
        }

        cache.output.reset();
        cache.requestQpc = requestQpc;

        NvU32 tach = 0;
        const auto result = pNvapi_->GPU_GetTachReading(device.handle, &tach);
        if (!pwr::nv::NvapiWrapper::Ok(result)) {
            pmlog_warn("NvAPI_GPU_GetTachReading failed").code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return nullptr;
        }

        cache.output = tach;
        return &*cache.output;
    }

    TelemetryMetricValue NvapiTelemetryProvider::PollNvapiMetric_(
        DeviceState_& device,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc) const
    {
        double value = 0.0;

        switch (metricId) {
        case PM_METRIC_GPU_TEMPERATURE:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pThermals = PollThermalEndpoint_(device, requestQpc);
            if (pThermals &&
                TryGetThermalValue_(*pThermals, NVAPI_THERMAL_TARGET_GPU, true, value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_MEM_TEMPERATURE:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pThermals = PollThermalEndpoint_(device, requestQpc);
            if (pThermals &&
                TryGetThermalValue_(*pThermals, NVAPI_THERMAL_TARGET_MEMORY, false, value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_FREQUENCY:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pClocks = PollClockEndpoint_(device, requestQpc);
            if (pClocks &&
                TryGetClockValue_(*pClocks, NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS, value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_MEM_FREQUENCY:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pClocks = PollClockEndpoint_(device, requestQpc);
            if (pClocks &&
                TryGetClockValue_(*pClocks, NVAPI_GPU_PUBLIC_CLOCK_MEMORY, value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_FAN_SPEED:
        {
            if (arrayIndex != 0) {
                throw Except<>("NVAPI array index out of range");
            }
            const auto* pTach = PollTachEndpoint_(device, requestQpc);
            if (pTach != nullptr) {
                return (double)*pTach;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pUtilization = PollUtilizationEndpoint_(device, requestQpc);
            if (pUtilization &&
                TryGetUtilizationValue_(
                    *pUtilization, pwr::nv::NVAPI_GPU_UTILIZATION_DOMAIN_GPU, value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_MEDIA_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pUtilization = PollUtilizationEndpoint_(device, requestQpc);
            if (pUtilization &&
                TryGetUtilizationValue_(
                    *pUtilization, pwr::nv::NVAPI_GPU_UTILIZATION_DOMAIN_VID, value)) {
                return value;
            }
            return 0.0;
        }
        default:
            throw Except<>("Unsupported metric for NVAPI provider");
        }
    }

    bool NvapiTelemetryProvider::TryGetThermalValue_(
        const NV_GPU_THERMAL_SETTINGS& thermalSettings,
        NV_THERMAL_TARGET target,
        bool requireInternalController,
        double& value)
    {
        const auto sensorCount = (uint32_t)std::min(
            (size_t)thermalSettings.count, (size_t)NVAPI_MAX_THERMAL_SENSORS_PER_GPU);

        if (requireInternalController) {
            for (uint32_t i = 0; i < sensorCount; ++i) {
                const auto& sensor = thermalSettings.sensor[(size_t)i];
                if (sensor.target == target &&
                    sensor.controller == NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL) {
                    value = (double)sensor.currentTemp;
                    return true;
                }
            }
        }

        for (uint32_t i = 0; i < sensorCount; ++i) {
            const auto& sensor = thermalSettings.sensor[(size_t)i];
            if (sensor.target == target && !requireInternalController) {
                value = (double)sensor.currentTemp;
                return true;
            }
        }

        value = 0.0;
        return false;
    }

    bool NvapiTelemetryProvider::TryGetClockValue_(
        const NV_GPU_CLOCK_FREQUENCIES& clockFrequencies,
        NV_GPU_PUBLIC_CLOCK_ID clockDomain,
        double& value)
    {
        if ((size_t)clockDomain >= (size_t)NVAPI_MAX_GPU_PUBLIC_CLOCKS) {
            value = 0.0;
            return false;
        }

        const auto& domain = clockFrequencies.domain[(size_t)clockDomain];
        if (!domain.bIsPresent) {
            value = 0.0;
            return false;
        }

        value = (double)domain.frequency / 1000.0;
        return true;
    }

    bool NvapiTelemetryProvider::TryGetUtilizationValue_(
        const NV_GPU_DYNAMIC_PSTATES_INFO_EX& utilization,
        pwr::nv::NVAPI_GPU_UTILIZATION_DOMAIN utilizationDomain,
        double& value)
    {
        if ((size_t)utilizationDomain >= (size_t)NVAPI_MAX_GPU_UTILIZATIONS) {
            value = 0.0;
            return false;
        }

        const auto& domain = utilization.utilization[(size_t)utilizationDomain];
        if (!domain.bIsPresent) {
            value = 0.0;
            return false;
        }

        value = (double)domain.percentage;
        return true;
    }
}
