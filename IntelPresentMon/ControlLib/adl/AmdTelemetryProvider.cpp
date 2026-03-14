// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AmdTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Qpc.h"

#include <chrono>
#include <unordered_set>

using namespace pmon;
using namespace util;

namespace pmon::tel::adl
{
    namespace
    {
        constexpr int kAmdVendorId_ = 1002;
        constexpr int kOverdriveVersionMin_ = 5;
        constexpr int kOverdriveVersionMax_ = 8;
        constexpr int kOdnGpuTemperatureSensor_ = 1;

        bool HasFlag_(int value, int flag) noexcept
        {
            return (value & flag) == flag;
        }
    }

    AmdTelemetryProvider::AmdTelemetryProvider()
    {
        try {
            pAdl_ = std::make_unique<pwr::amd::Adl2Wrapper>();
        }
        catch (const TelemetrySubsystemAbsent&) {
            throw;
        }
        catch (...) {
            pmlog_error(util::ReportException("ADL wrapper construction failed"));
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize AMD Display Library");
        }

        int adapterCount = 0;
        const auto countResult = pAdl_->Adapter_NumberOfAdapters_Get(&adapterCount);
        if (!pwr::amd::Adl2Wrapper::Ok(countResult)) {
            pmlog_error("ADL2_Adapter_NumberOfAdapters_Get failed").code(countResult);
            throw Except<>("ADL adapter count query failed");
        }

        if (adapterCount <= 0) {
            return;
        }

        std::vector<AdapterInfo> adapterInfos((size_t)adapterCount);
        for (auto& adapterInfo : adapterInfos) {
            adapterInfo.iSize = sizeof(AdapterInfo);
        }

        const auto infoResult = pAdl_->Adapter_AdapterInfo_Get(
            adapterInfos.data(),
            (int)(adapterInfos.size() * sizeof(AdapterInfo)));
        if (!pwr::amd::Adl2Wrapper::Ok(infoResult)) {
            pmlog_error("ADL2_Adapter_AdapterInfo_Get failed").code(infoResult);
            throw Except<>("ADL adapter info query failed");
        }

        std::unordered_set<int> seenBusNumbers{};
        for (const auto& adapterInfo : adapterInfos) {
            if (adapterInfo.iVendorID != kAmdVendorId_) {
                continue;
            }
            if (adapterInfo.iBusNumber < 0) {
                pmlog_warn("Skipping AMD adapter with invalid PCI bus number")
                    .pmwatch(adapterInfo.iAdapterIndex)
                    .pmwatch(adapterInfo.strAdapterName);
                continue;
            }
            if (!seenBusNumbers.insert(adapterInfo.iBusNumber).second) {
                continue;
            }

            const auto providerDeviceId = nextProviderDeviceId_;
            const auto emplaceResult = devicesById_.try_emplace(providerDeviceId);
            if (!emplaceResult.second) {
                throw Except<>("Duplicate ADL provider device id encountered");
            }

            auto& device = emplaceResult.first->second;
            device.providerDeviceId = providerDeviceId;
            if (!TryInitializeDevice_(device, adapterInfo)) {
                devicesById_.erase(emplaceResult.first);
                continue;
            }

            device.caps = BuildCapsForDevice_(device);
            ++nextProviderDeviceId_;
        }
    }

    ProviderCapabilityMap AmdTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& AmdTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("ADL provider device not found");
        }
        return iDevice->second.fingerprint;
    }

    TelemetryMetricValue AmdTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("ADL provider device not found");
        }

        auto& device = iDevice->second;

        switch (metricId) {
        case PM_METRIC_GPU_VENDOR:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return (int)device.fingerprint.vendor;
        case PM_METRIC_GPU_NAME:
            throw Except<>("PM_METRIC_GPU_NAME is static-only and is not served by poll path");
        case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto pLimit = QuerySustainedPowerLimit_(device);
            return pLimit != nullptr ? *pLimit : 0.0;
        }
        case PM_METRIC_GPU_MEM_SIZE:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto pMemoryInfo = QueryMemoryInfo_(device);
            return pMemoryInfo != nullptr ? GetMemorySizeBytes_(*pMemoryInfo) : (uint64_t)0;
        }
        case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto pMemoryInfo = QueryMemoryInfo_(device);
            return pMemoryInfo != nullptr ? GetMemoryMaxBandwidthBitsPerSecond_(*pMemoryInfo) : (uint64_t)0;
        }
        default:
            break;
        }

        const auto& snapshot = PollDynamicSnapshot_(device, requestQpc);

        switch (metricId) {
        case PM_METRIC_GPU_POWER:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuPower ? snapshot.gpuPowerW : 0.0;
        case PM_METRIC_GPU_VOLTAGE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuVoltage ? snapshot.gpuVoltageV : 0.0;
        case PM_METRIC_GPU_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuFrequency ? snapshot.gpuFrequencyMhz : 0.0;
        case PM_METRIC_GPU_TEMPERATURE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuTemperature ? snapshot.gpuTemperatureC : 0.0;
        case PM_METRIC_GPU_UTILIZATION:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuUtilization ? snapshot.gpuUtilizationPercent : 0.0;
        case PM_METRIC_GPU_MEM_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuMemFrequency ? snapshot.gpuMemFrequencyMhz : 0.0;
        case PM_METRIC_GPU_MEM_USED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuMemUsed ? snapshot.gpuMemUsedBytes : (uint64_t)0;
        case PM_METRIC_GPU_MEM_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto pMemoryInfo = QueryMemoryInfo_(device);
            if (!snapshot.hasGpuMemUsed || pMemoryInfo == nullptr) {
                return 0.0;
            }
            const auto totalBytes = GetMemorySizeBytes_(*pMemoryInfo);
            if (totalBytes == 0) {
                return 0.0;
            }
            return 100.0 * ((double)snapshot.gpuMemUsedBytes / (double)totalBytes);
        }
        case PM_METRIC_GPU_MEM_TEMPERATURE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuMemTemperature ? snapshot.gpuMemTemperatureC : 0.0;
        case PM_METRIC_GPU_MEM_VOLTAGE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasGpuMemVoltage ? snapshot.gpuMemVoltageV : 0.0;
        case PM_METRIC_GPU_POWER_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasThrottleStatus ? snapshot.gpuPowerLimited : false;
        case PM_METRIC_GPU_TEMPERATURE_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasThrottleStatus ? snapshot.gpuTemperatureLimited : false;
        case PM_METRIC_GPU_CURRENT_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return snapshot.hasThrottleStatus ? snapshot.gpuCurrentLimited : false;
        case PM_METRIC_GPU_FAN_SPEED:
            if (arrayIndex >= snapshot.fanSpeedsRpm.size()) {
                throw Except<>("ADL array index out of range");
            }
            return snapshot.fanSpeedsRpm[(size_t)arrayIndex];
        case PM_METRIC_GPU_FAN_SPEED_PERCENT:
            if (arrayIndex >= snapshot.fanSpeedRatios.size()) {
                throw Except<>("ADL array index out of range");
            }
            return snapshot.fanSpeedRatios[(size_t)arrayIndex];
        default:
            throw Except<>("Unsupported metric for ADL provider");
        }
    }

    void AmdTelemetryProvider::ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex)
    {
        if (arrayIndex != 0) {
            throw Except<>("ADL scalar metric queried with nonzero array index");
        }
        (void)metricId;
    }

    uint64_t AmdTelemetryProvider::GetMemorySizeBytes_(const ADLMemoryInfoX4& memoryInfo) noexcept
    {
        return memoryInfo.iMemorySize > 0 ? (uint64_t)memoryInfo.iMemorySize : 0ull;
    }

    uint64_t AmdTelemetryProvider::GetMemoryMaxBandwidthBitsPerSecond_(const ADLMemoryInfoX4& memoryInfo) noexcept
    {
        if (memoryInfo.iMemoryBitRateX2 > 0) {
            return (uint64_t)memoryInfo.iMemoryBitRateX2;
        }
        if (memoryInfo.iMemoryBandwidthX2 > 0) {
            return (uint64_t)memoryInfo.iMemoryBandwidthX2 * 8ull;
        }
        if (memoryInfo.iMemoryBandwidth > 0) {
            return (uint64_t)memoryInfo.iMemoryBandwidth * 8ull * 1000000ull;
        }
        return 0ull;
    }

    bool AmdTelemetryProvider::TryGetPmLogSensorValue_(
        const ADLPMLogDataOutput& data,
        int sensorIndex,
        int& value) noexcept
    {
        if (sensorIndex < 0 || sensorIndex >= ADL_PMLOG_MAX_SENSORS) {
            value = 0;
            return false;
        }

        const auto& sensor = data.sensors[(size_t)sensorIndex];
        if (!sensor.supported) {
            value = 0;
            return false;
        }

        value = sensor.value;
        return true;
    }

    bool AmdTelemetryProvider::TryInitializeDevice_(
        DeviceState_& device,
        const AdapterInfo& adapterInfo) const
    {
        device.adlAdapterIndex = adapterInfo.iAdapterIndex;
        device.fingerprint.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
        device.fingerprint.vendor = PM_DEVICE_VENDOR_AMD;
        device.fingerprint.deviceName =
            adapterInfo.strAdapterName[0] != '\0' ? adapterInfo.strAdapterName : "Unknown AMD Adapter";
        device.fingerprint.pciBusId = (uint32_t)adapterInfo.iBusNumber;

        int overdriveSupported = 0;
        int overdriveEnabled = 0;
        int overdriveVersion = 0;
        const auto capsResult = pAdl_->Overdrive_Caps(
            device.adlAdapterIndex,
            &overdriveSupported,
            &overdriveEnabled,
            &overdriveVersion);
        if (!pwr::amd::Adl2Wrapper::Ok(capsResult)) {
            pmlog_warn("ADL2_Overdrive_Caps failed").code(capsResult)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return false;
        }

        if (!overdriveSupported || !overdriveEnabled) {
            pmlog_warn("Skipping AMD adapter without enabled Overdrive telemetry")
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(overdriveSupported)
                .pmwatch(overdriveEnabled);
            return false;
        }

        if (overdriveVersion < kOverdriveVersionMin_ || overdriveVersion > kOverdriveVersionMax_) {
            pmlog_warn("Skipping AMD adapter with unsupported Overdrive version")
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(overdriveVersion);
            return false;
        }

        device.overdriveVersion = overdriveVersion;
        if (device.overdriveVersion == 5) {
            EnumerateOd5ThermalControllers_(device);
        }

        return true;
    }

    void AmdTelemetryProvider::EnumerateOd5ThermalControllers_(DeviceState_& device) const
    {
        device.od5ThermalControllerIndices.clear();

        for (int thermalControllerIndex = 0; thermalControllerIndex < 10; ++thermalControllerIndex) {
            ADLThermalControllerInfo thermalInfo{ .iSize = sizeof(ADLThermalControllerInfo) };
            const auto result = pAdl_->Overdrive5_ThermalDevices_Enum(
                device.adlAdapterIndex,
                thermalControllerIndex,
                &thermalInfo);
            if (!pwr::amd::Adl2Wrapper::Ok(result)) {
                if (result != ADL_ERR_INVALID_CONTROLLER_IDX && result != ADL_ERR) {
                    pmlog_warn("ADL2_Overdrive5_ThermalDevices_Enum failed").code(result)
                        .pmwatch(device.providerDeviceId)
                        .pmwatch(device.fingerprint.deviceName)
                        .pmwatch(thermalControllerIndex);
                }
                break;
            }
            if (thermalInfo.iThermalDomain == ADL_DL_THERMAL_DOMAIN_GPU) {
                device.od5ThermalControllerIndices.push_back(thermalControllerIndex);
            }
        }
    }

    ipc::MetricCapabilities AmdTelemetryProvider::BuildCapsForDevice_(DeviceState_& device) const
    {
        ipc::MetricCapabilities caps{};
        const auto requestQpc = GetCurrentTimestamp();

        caps.Set(PM_METRIC_GPU_VENDOR, 1);
        caps.Set(PM_METRIC_GPU_NAME, 1);

        if (QuerySustainedPowerLimit_(device) != nullptr) {
            caps.Set(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, 1);
        }

        if (const auto pMemoryInfo = QueryMemoryInfo_(device); pMemoryInfo != nullptr) {
            const auto memSize = GetMemorySizeBytes_(*pMemoryInfo);
            const auto memBw = GetMemoryMaxBandwidthBitsPerSecond_(*pMemoryInfo);
            if (memSize != 0) {
                caps.Set(PM_METRIC_GPU_MEM_SIZE, 1);
            }
            if (memBw != 0) {
                caps.Set(PM_METRIC_GPU_MEM_MAX_BANDWIDTH, 1);
            }
        }

        const auto& snapshot = PollDynamicSnapshot_(device, requestQpc);
        if (snapshot.hasGpuPower) {
            caps.Set(PM_METRIC_GPU_POWER, 1);
        }
        if (snapshot.hasGpuVoltage) {
            caps.Set(PM_METRIC_GPU_VOLTAGE, 1);
        }
        if (snapshot.hasGpuFrequency) {
            caps.Set(PM_METRIC_GPU_FREQUENCY, 1);
        }
        if (snapshot.hasGpuTemperature) {
            caps.Set(PM_METRIC_GPU_TEMPERATURE, 1);
        }
        if (snapshot.hasGpuUtilization) {
            caps.Set(PM_METRIC_GPU_UTILIZATION, 1);
        }
        if (snapshot.hasGpuMemFrequency) {
            caps.Set(PM_METRIC_GPU_MEM_FREQUENCY, 1);
        }
        if (snapshot.hasGpuMemUsed) {
            caps.Set(PM_METRIC_GPU_MEM_USED, 1);
            if (const auto pMemoryInfo = QueryMemoryInfo_(device);
                pMemoryInfo != nullptr && GetMemorySizeBytes_(*pMemoryInfo) != 0) {
                caps.Set(PM_METRIC_GPU_MEM_UTILIZATION, 1);
            }
        }
        if (snapshot.hasGpuMemTemperature) {
            caps.Set(PM_METRIC_GPU_MEM_TEMPERATURE, 1);
        }
        if (snapshot.hasGpuMemVoltage) {
            caps.Set(PM_METRIC_GPU_MEM_VOLTAGE, 1);
        }
        if (!snapshot.fanSpeedsRpm.empty()) {
            caps.Set(PM_METRIC_GPU_FAN_SPEED, (uint32_t)snapshot.fanSpeedsRpm.size());
        }
        if (!snapshot.fanSpeedRatios.empty()) {
            caps.Set(PM_METRIC_GPU_FAN_SPEED_PERCENT, (uint32_t)snapshot.fanSpeedRatios.size());
        }
        if (snapshot.hasThrottleStatus) {
            caps.Set(PM_METRIC_GPU_POWER_LIMITED, 1);
            caps.Set(PM_METRIC_GPU_TEMPERATURE_LIMITED, 1);
            caps.Set(PM_METRIC_GPU_CURRENT_LIMITED, 1);
        }

        return caps;
    }

    const ADLMemoryInfoX4* AmdTelemetryProvider::QueryMemoryInfo_(DeviceState_& device) const
    {
        if (device.memoryInfoQueried) {
            return device.memoryInfo ? &*device.memoryInfo : nullptr;
        }

        device.memoryInfoQueried = true;
        ADLMemoryInfoX4 memoryInfo{};
        const auto result = pAdl_->Adapter_MemoryInfoX4_Get(device.adlAdapterIndex, &memoryInfo);
        if (!pwr::amd::Adl2Wrapper::Ok(result)) {
            pmlog_warn("ADL2_Adapter_MemoryInfoX4_Get failed").code(result)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return nullptr;
        }

        device.memoryInfo = memoryInfo;
        return &*device.memoryInfo;
    }

    const double* AmdTelemetryProvider::QuerySustainedPowerLimit_(DeviceState_& device) const
    {
        if (device.sustainedPowerLimitQueried) {
            return device.sustainedPowerLimitW ? &*device.sustainedPowerLimitW : nullptr;
        }

        device.sustainedPowerLimitQueried = true;

        switch (device.overdriveVersion) {
        case 5:
        {
            int powerControlSupported = 0;
            auto result = pAdl_->Overdrive5_PowerControl_Caps(
                device.adlAdapterIndex,
                &powerControlSupported);
            if (!pwr::amd::Adl2Wrapper::Ok(result)) {
                pmlog_warn("ADL2_Overdrive5_PowerControl_Caps failed").code(result)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
                return nullptr;
            }
            if (!powerControlSupported) {
                return nullptr;
            }

            int powerControlCurrent = 0;
            int powerControlDefault = 0;
            result = pAdl_->Overdrive5_PowerControl_Get(
                device.adlAdapterIndex,
                &powerControlCurrent,
                &powerControlDefault);
            if (!pwr::amd::Adl2Wrapper::Ok(result)) {
                pmlog_warn("ADL2_Overdrive5_PowerControl_Get failed").code(result)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
                return nullptr;
            }

            device.sustainedPowerLimitW = (double)powerControlCurrent;
            (void)powerControlDefault;
            return &*device.sustainedPowerLimitW;
        }
        case 6:
        {
            int powerControlSupported = 0;
            auto result = pAdl_->Overdrive6_PowerControl_Caps(
                device.adlAdapterIndex,
                &powerControlSupported);
            if (!pwr::amd::Adl2Wrapper::Ok(result)) {
                pmlog_warn("ADL2_Overdrive6_PowerControl_Caps failed").code(result)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
                return nullptr;
            }
            if (!powerControlSupported) {
                return nullptr;
            }

            int powerControlCurrent = 0;
            int powerControlDefault = 0;
            result = pAdl_->Overdrive6_PowerControl_Get(
                device.adlAdapterIndex,
                &powerControlCurrent,
                &powerControlDefault);
            if (!pwr::amd::Adl2Wrapper::Ok(result)) {
                pmlog_warn("ADL2_Overdrive6_PowerControl_Get failed").code(result)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
                return nullptr;
            }

            device.sustainedPowerLimitW = (double)powerControlCurrent;
            (void)powerControlDefault;
            return &*device.sustainedPowerLimitW;
        }
        default:
            return nullptr;
        }
    }

    const AmdTelemetryProvider::DynamicSnapshot_& AmdTelemetryProvider::PollDynamicSnapshot_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.dynamicSnapshotCache;
        if (cache.Matches(requestQpc)) {
            return cache.output;
        }

        cache.output = {};
        cache.requestQpc = requestQpc;
        auto& snapshot = cache.output;

        int vramUsageMb = 0;
        const auto vramUsageResult = pAdl_->Adapter_VRAMUsage_Get(device.adlAdapterIndex, &vramUsageMb);
        if (pwr::amd::Adl2Wrapper::Ok(vramUsageResult)) {
            snapshot.hasGpuMemUsed = true;
            snapshot.gpuMemUsedBytes = (uint64_t)vramUsageMb * 1000000ull;
        }
        else {
            pmlog_warn("ADL2_Adapter_VRAMUsage_Get failed").code(vramUsageResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        switch (device.overdriveVersion) {
        case 5:
            PopulateOverdrive5Snapshot_(device, snapshot);
            break;
        case 6:
            PopulateOverdrive6Snapshot_(device, snapshot);
            break;
        case 7:
            PopulateOverdrive7Snapshot_(device, snapshot);
            break;
        case 8:
            PopulateOverdrive8Snapshot_(device, snapshot);
            break;
        default:
            pmlog_warn("Unexpected ADL Overdrive version while polling snapshot")
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.overdriveVersion);
            break;
        }

        return cache.output;
    }

    void AmdTelemetryProvider::PopulateOverdrive5Snapshot_(
        DeviceState_& device,
        DynamicSnapshot_& snapshot) const
    {
        for (const auto thermalControllerIndex : device.od5ThermalControllerIndices) {
            ADLTemperature temperature{ .iSize = sizeof(ADLTemperature) };
            const auto tempResult = pAdl_->Overdrive5_Temperature_Get(
                device.adlAdapterIndex,
                thermalControllerIndex,
                &temperature);
            if (pwr::amd::Adl2Wrapper::Ok(tempResult)) {
                if (!snapshot.hasGpuTemperature) {
                    snapshot.gpuTemperatureC = (double)temperature.iTemperature / 1000.0;
                    snapshot.hasGpuTemperature = true;
                }
            }
            else {
                pmlog_warn("ADL2_Overdrive5_Temperature_Get failed").code(tempResult).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(thermalControllerIndex);
            }

            ADLFanSpeedInfo fanInfo{ .iSize = sizeof(ADLFanSpeedInfo) };
            const auto fanInfoResult = pAdl_->Overdrive5_FanSpeedInfo_Get(
                device.adlAdapterIndex,
                thermalControllerIndex,
                &fanInfo);
            if (!pwr::amd::Adl2Wrapper::Ok(fanInfoResult)) {
                pmlog_warn("ADL2_Overdrive5_FanSpeedInfo_Get failed").code(fanInfoResult).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(thermalControllerIndex);
                continue;
            }

            if (HasFlag_(fanInfo.iFlags, ADL_DL_FANCTRL_SUPPORTS_RPM_READ)) {
                ADLFanSpeedValue fanValue{
                    .iSize = sizeof(ADLFanSpeedValue),
                    .iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM,
                };
                const auto fanResult = pAdl_->Overdrive5_FanSpeed_Get(
                    device.adlAdapterIndex,
                    thermalControllerIndex,
                    &fanValue);
                if (pwr::amd::Adl2Wrapper::Ok(fanResult)) {
                    snapshot.fanSpeedsRpm.push_back((double)fanValue.iFanSpeed);
                }
                else {
                    pmlog_warn("ADL2_Overdrive5_FanSpeed_Get failed for RPM").code(fanResult).every(std::chrono::seconds{ 60 })
                        .pmwatch(device.providerDeviceId)
                        .pmwatch(device.fingerprint.deviceName)
                        .pmwatch(thermalControllerIndex);
                }
            }

            if (HasFlag_(fanInfo.iFlags, ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ)) {
                ADLFanSpeedValue fanValue{
                    .iSize = sizeof(ADLFanSpeedValue),
                    .iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT,
                };
                const auto fanResult = pAdl_->Overdrive5_FanSpeed_Get(
                    device.adlAdapterIndex,
                    thermalControllerIndex,
                    &fanValue);
                if (pwr::amd::Adl2Wrapper::Ok(fanResult)) {
                    snapshot.fanSpeedRatios.push_back((double)fanValue.iFanSpeed / 100.0);
                }
                else {
                    pmlog_warn("ADL2_Overdrive5_FanSpeed_Get failed for percent").code(fanResult).every(std::chrono::seconds{ 60 })
                        .pmwatch(device.providerDeviceId)
                        .pmwatch(device.fingerprint.deviceName)
                        .pmwatch(thermalControllerIndex);
                }
            }
        }

        ADLPMActivity activity{ .iSize = sizeof(ADLPMActivity) };
        const auto activityResult = pAdl_->Overdrive5_CurrentActivity_Get(
            device.adlAdapterIndex,
            &activity);
        if (pwr::amd::Adl2Wrapper::Ok(activityResult)) {
            snapshot.gpuUtilizationPercent = (double)activity.iActivityPercent;
            snapshot.hasGpuUtilization = true;

            snapshot.gpuFrequencyMhz = (double)activity.iEngineClock / 100.0;
            snapshot.hasGpuFrequency = true;

            snapshot.gpuMemFrequencyMhz = (double)activity.iMemoryClock / 100.0;
            snapshot.hasGpuMemFrequency = true;

            snapshot.gpuVoltageV = (double)activity.iVddc;
            snapshot.hasGpuVoltage = true;
        }
        else {
            pmlog_warn("ADL2_Overdrive5_CurrentActivity_Get failed").code(activityResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }
    }

    void AmdTelemetryProvider::PopulateOverdrive6Snapshot_(
        DeviceState_& device,
        DynamicSnapshot_& snapshot) const
    {
        ADLOD6ThermalControllerCaps thermalCaps{};
        const auto thermalCapsResult = pAdl_->Overdrive6_ThermalController_Caps(
            device.adlAdapterIndex,
            &thermalCaps);
        if (pwr::amd::Adl2Wrapper::Ok(thermalCapsResult)) {
            if (HasFlag_(thermalCaps.iCapabilities, ADL_OD6_TCCAPS_THERMAL_CONTROLLER)) {
                int temperature = 0;
                const auto tempResult = pAdl_->Overdrive6_Temperature_Get(device.adlAdapterIndex, &temperature);
                if (pwr::amd::Adl2Wrapper::Ok(tempResult)) {
                    snapshot.gpuTemperatureC = (double)temperature / 1000.0;
                    snapshot.hasGpuTemperature = true;
                }
                else {
                    pmlog_warn("ADL2_Overdrive6_Temperature_Get failed").code(tempResult).every(std::chrono::seconds{ 60 })
                        .pmwatch(device.providerDeviceId)
                        .pmwatch(device.fingerprint.deviceName);
                }
            }

            ADLOD6FanSpeedInfo fanInfo{};
            const auto fanResult = pAdl_->Overdrive6_FanSpeed_Get(device.adlAdapterIndex, &fanInfo);
            if (pwr::amd::Adl2Wrapper::Ok(fanResult)) {
                if (HasFlag_(fanInfo.iSpeedType, ADL_OD6_FANSPEED_TYPE_RPM)) {
                    snapshot.fanSpeedsRpm.push_back((double)fanInfo.iFanSpeedRPM);
                }
                if (HasFlag_(fanInfo.iSpeedType, ADL_OD6_FANSPEED_TYPE_PERCENT)) {
                    snapshot.fanSpeedRatios.push_back((double)fanInfo.iFanSpeedPercent / 100.0);
                }
            }
            else {
                pmlog_warn("ADL2_Overdrive6_FanSpeed_Get failed").code(fanResult).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
            }
        }
        else {
            pmlog_warn("ADL2_Overdrive6_ThermalController_Caps failed").code(thermalCapsResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        ADLOD6CurrentStatus currentStatus{};
        const auto currentStatusResult = pAdl_->Overdrive6_CurrentStatus_Get(device.adlAdapterIndex, &currentStatus);
        if (pwr::amd::Adl2Wrapper::Ok(currentStatusResult)) {
            snapshot.gpuFrequencyMhz = (double)currentStatus.iEngineClock / 100.0;
            snapshot.hasGpuFrequency = true;

            snapshot.gpuMemFrequencyMhz = (double)currentStatus.iMemoryClock / 100.0;
            snapshot.hasGpuMemFrequency = true;

            ADLOD6Capabilities capabilities{};
            const auto capResult = pAdl_->Overdrive6_Capabilities_Get(device.adlAdapterIndex, &capabilities);
            if (pwr::amd::Adl2Wrapper::Ok(capResult)) {
                if (HasFlag_(capabilities.iCapabilities, ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR)) {
                    snapshot.gpuUtilizationPercent = (double)currentStatus.iActivityPercent;
                    snapshot.hasGpuUtilization = true;
                }
            }
            else {
                pmlog_warn("ADL2_Overdrive6_Capabilities_Get failed").code(capResult).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
            }
        }
        else {
            pmlog_warn("ADL2_Overdrive6_CurrentStatus_Get failed").code(currentStatusResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        int currentPower = 0;
        const auto powerResult = pAdl_->Overdrive6_CurrentPower_Get(device.adlAdapterIndex, 0, &currentPower);
        if (pwr::amd::Adl2Wrapper::Ok(powerResult)) {
            snapshot.gpuPowerW = (double)currentPower / 256.0;
            snapshot.hasGpuPower = true;
        }
        else {
            pmlog_warn("ADL2_Overdrive6_CurrentPower_Get failed").code(powerResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }
    }

    void AmdTelemetryProvider::PopulateOverdrive7Snapshot_(
        DeviceState_& device,
        DynamicSnapshot_& snapshot) const
    {
        ADLODNCapabilitiesX2 capabilities{};
        const auto capsResult = pAdl_->OverdriveN_CapabilitiesX2_Get(device.adlAdapterIndex, &capabilities);
        if (!pwr::amd::Adl2Wrapper::Ok(capsResult)) {
            pmlog_warn("ADL2_OverdriveN_CapabilitiesX2_Get failed").code(capsResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        ADLODNPerformanceStatus performanceStatus{};
        const auto statusResult = pAdl_->OverdriveN_PerformanceStatus_Get(device.adlAdapterIndex, &performanceStatus);
        if (pwr::amd::Adl2Wrapper::Ok(statusResult)) {
            snapshot.gpuFrequencyMhz = (double)performanceStatus.iCoreClock / 100.0;
            snapshot.hasGpuFrequency = true;

            snapshot.gpuMemFrequencyMhz = (double)performanceStatus.iMemoryClock / 100.0;
            snapshot.hasGpuMemFrequency = true;

            snapshot.gpuVoltageV = (double)performanceStatus.iVDDC / 1000.0;
            snapshot.hasGpuVoltage = true;

            snapshot.gpuUtilizationPercent = (double)performanceStatus.iGPUActivityPercent;
            snapshot.hasGpuUtilization = true;
        }
        else {
            pmlog_warn("ADL2_OverdriveN_PerformanceStatus_Get failed").code(statusResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        int temperature = 0;
        const auto tempResult = pAdl_->OverdriveN_Temperature_Get(
            device.adlAdapterIndex,
            kOdnGpuTemperatureSensor_,
            &temperature);
        if (pwr::amd::Adl2Wrapper::Ok(tempResult)) {
            snapshot.gpuTemperatureC = (double)temperature / 1000.0;
            snapshot.hasGpuTemperature = true;
        }
        else {
            pmlog_warn("ADL2_OverdriveN_Temperature_Get failed").code(tempResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        ADLODNFanControl fanControl{};
        const auto fanResult = pAdl_->OverdriveN_FanControl_Get(device.adlAdapterIndex, &fanControl);
        if (pwr::amd::Adl2Wrapper::Ok(fanResult)) {
            if (HasFlag_(fanControl.iCurrentFanSpeedMode, ADL_OD6_FANSPEED_TYPE_RPM)) {
                snapshot.fanSpeedsRpm.push_back((double)fanControl.iCurrentFanSpeed);
            }
            else if (HasFlag_(fanControl.iCurrentFanSpeedMode, ADL_OD6_FANSPEED_TYPE_PERCENT)) {
                snapshot.fanSpeedRatios.push_back((double)fanControl.iCurrentFanSpeed / 100.0);
            }
            else {
                snapshot.fanSpeedsRpm.push_back((double)fanControl.iCurrentFanSpeed);
            }
        }
        else {
            pmlog_warn("ADL2_OverdriveN_FanControl_Get failed").code(fanResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        int currentPower = 0;
        const auto powerResult = pAdl_->Overdrive6_CurrentPower_Get(device.adlAdapterIndex, 0, &currentPower);
        if (pwr::amd::Adl2Wrapper::Ok(powerResult)) {
            snapshot.gpuPowerW = (double)currentPower / 256.0;
            snapshot.hasGpuPower = true;
        }
        else {
            pmlog_warn("ADL2_Overdrive6_CurrentPower_Get failed").code(powerResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
        }

        (void)capabilities;
    }

    void AmdTelemetryProvider::PopulateOverdrive8Snapshot_(
        DeviceState_& device,
        DynamicSnapshot_& snapshot) const
    {
        ADLPMLogDataOutput dataOutput{};
        dataOutput.size = sizeof(ADLPMLogDataOutput);
        const auto result = pAdl_->New_QueryPMLogData_Get(device.adlAdapterIndex, &dataOutput);
        if (!pwr::amd::Adl2Wrapper::Ok(result)) {
            pmlog_warn("ADL2_New_QueryPMLogData_Get failed").code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        int value = 0;
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_CLK_GFXCLK, value)) {
            snapshot.gpuFrequencyMhz = (double)value;
            snapshot.hasGpuFrequency = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_CLK_MEMCLK, value)) {
            snapshot.gpuMemFrequencyMhz = (double)value;
            snapshot.hasGpuMemFrequency = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_FAN_RPM, value)) {
            snapshot.fanSpeedsRpm.push_back((double)value);
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_FAN_PERCENTAGE, value)) {
            snapshot.fanSpeedRatios.push_back((double)value / 100.0);
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_TEMPERATURE_EDGE, value)) {
            snapshot.gpuTemperatureC = (double)value;
            snapshot.hasGpuTemperature = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_INFO_ACTIVITY_GFX, value)) {
            snapshot.gpuUtilizationPercent = (double)value;
            snapshot.hasGpuUtilization = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_GFX_VOLTAGE, value)) {
            snapshot.gpuVoltageV = (double)value / 1000.0;
            snapshot.hasGpuVoltage = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_ASIC_POWER, value)) {
            snapshot.gpuPowerW = (double)value;
            snapshot.hasGpuPower = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_TEMPERATURE_MEM, value)) {
            snapshot.gpuMemTemperatureC = (double)value;
            snapshot.hasGpuMemTemperature = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_MEM_VOLTAGE, value)) {
            snapshot.gpuMemVoltageV = (double)value / 1000.0;
            snapshot.hasGpuMemVoltage = true;
        }
        if (TryGetPmLogSensorValue_(dataOutput, PMLOG_THROTTLER_STATUS, value)) {
            snapshot.hasThrottleStatus = true;
            snapshot.gpuPowerLimited = HasFlag_(value, ADL_PMLOG_THROTTLE_POWER);
            snapshot.gpuTemperatureLimited = HasFlag_(value, ADL_PMLOG_THROTTLE_THERMAL);
            snapshot.gpuCurrentLimited = HasFlag_(value, ADL_PMLOG_THROTTLE_CURRENT);
        }
    }
}
