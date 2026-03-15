// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "IgclTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Qpc.h"
#include "../../CommonUtilities/ref/GeneratedReflection.h"

#include <algorithm>
#include <chrono>
#include <regex>
#include <stdexcept>
#include <string>

using namespace pmon;
using namespace util;
using v = log::V;

namespace pmon::tel::igcl
{
    IgclTelemetryProvider::TelemetrySampleBuffer_::TelemetrySampleBuffer_() noexcept
        : samples_{ MakeEmptySample_(), MakeEmptySample_() }
    {}

    bool IgclTelemetryProvider::TelemetrySampleBuffer_::Matches(int64_t requestQpc) const noexcept
    {
        return hasCurrent_ && requestQpcs_[currentIndex_] == requestQpc;
    }

    ctl_power_telemetry_t& IgclTelemetryProvider::TelemetrySampleBuffer_::PrepareForWrite(int64_t requestQpc) noexcept
    {
        if (hasCurrent_ && requestQpcs_[currentIndex_] != requestQpc) {
            previousIndex_ = currentIndex_;
            currentIndex_ = currentIndex_ == 0 ? 1u : 0u;
            hasPrevious_ = true;
        }

        hasCurrent_ = true;
        requestQpcs_[currentIndex_] = requestQpc;
        samples_[currentIndex_] = MakeEmptySample_();
        return samples_[currentIndex_];
    }

    const ctl_power_telemetry_t& IgclTelemetryProvider::TelemetrySampleBuffer_::Current() const noexcept
    {
        return samples_[currentIndex_];
    }

    const ctl_power_telemetry_t* IgclTelemetryProvider::TelemetrySampleBuffer_::Previous() const noexcept
    {
        if (!hasPrevious_) {
            return nullptr;
        }
        return &samples_[previousIndex_];
    }

    int64_t IgclTelemetryProvider::TelemetrySampleBuffer_::PreviousRequestQpc() const noexcept
    {
        if (!hasPrevious_) {
            return 0;
        }
        return requestQpcs_[previousIndex_];
    }

    ctl_power_telemetry_t IgclTelemetryProvider::TelemetrySampleBuffer_::MakeEmptySample_() noexcept
    {
        return {
            .Size = sizeof(ctl_power_telemetry_t),
            .Version = 1,
        };
    }

    IgclTelemetryProvider::IgclTelemetryProvider()
    {
        ctl_init_args_t ctlInitArgs{
            .Size = sizeof(ctl_init_args_t),
            .AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION),
            .flags = CTL_INIT_FLAG_USE_LEVEL_ZERO,
        };

        const auto initResult = ctlInit(&ctlInitArgs, &apiHandle_);
        if (initResult != CTL_RESULT_SUCCESS) {
            if (initResult != CTL_RESULT_ERROR_NOT_INITIALIZED) {
                pmlog_error("ctlInit failed").code(initResult);
            }
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize Intel Graphics Control Library");
        }
        pmlog_verb(v::tele_gpu)("ctlInit input args")
            .pmwatch(ref::DumpGenerated(ctlInitArgs));

        std::vector<ctl_device_adapter_handle_t> handles{};
        {
            uint32_t count = 0;
            const auto enumCountResult = ctlEnumerateDevices(apiHandle_, &count, nullptr);
            if (enumCountResult != CTL_RESULT_SUCCESS) {
                pmlog_error("ctlEnumerateDevices(count) failed").code(enumCountResult);
                throw std::runtime_error{ "IGCL device enumeration (count) failed" };
            }
            pmlog_verb(v::tele_gpu)("ctlEnumerateDevices(count) output")
                .pmwatch(count);

            handles.resize((size_t)count);
            const auto enumListResult = ctlEnumerateDevices(apiHandle_, &count, handles.data());
            if (enumListResult != CTL_RESULT_SUCCESS) {
                pmlog_error("ctlEnumerateDevices(list) failed").code(enumListResult);
                throw std::runtime_error{ "IGCL device enumeration (list) failed" };
            }
        }

        for (const auto handle : handles) {
            const auto providerDeviceId = nextProviderDeviceId_;
            const auto emplaceResult = devicesById_.try_emplace(providerDeviceId);
            if (!emplaceResult.second) {
                throw std::runtime_error{ "Duplicate IGCL provider device id encountered" };
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

    IgclTelemetryProvider::~IgclTelemetryProvider()
    {
        devicesById_.clear();
        if (apiHandle_ != nullptr) {
            ctlClose(apiHandle_);
            apiHandle_ = nullptr;
        }
    }

    ProviderCapabilityMap IgclTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& IgclTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        return devicesById_.at(providerDeviceId).fingerprint;
    }

    TelemetryMetricValue IgclTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw std::out_of_range{ "IGCL provider device not found" };
        }

        auto& device = iDevice->second;

        switch (metricId) {
        case PM_METRIC_GPU_VENDOR:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return (int)device.fingerprint.vendor;
        case PM_METRIC_GPU_NAME:
            throw std::invalid_argument{ "PM_METRIC_GPU_NAME is static-only and is not served by poll path" };
        case PM_METRIC_GPU_SUSTAINED_POWER_LIMIT:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto limits = PollPowerLimitsEndpoint_(device);
            if (!limits || !limits->sustainedPowerLimit.enabled) {
                return 0.0;
            }
            return (double)limits->sustainedPowerLimit.power / 1000.0;
        }
        case PM_METRIC_GPU_MEM_SIZE:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemState = PollMemoryStateEndpoint_(device, requestQpc);
            if (pMemState == nullptr) {
                return (uint64_t)0;
            }
            return (uint64_t)pMemState->size;
        }
        case PM_METRIC_GPU_MEM_USED:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemState = PollMemoryStateEndpoint_(device, requestQpc);
            if (pMemState == nullptr) {
                return (uint64_t)0;
            }
            return (uint64_t)(pMemState->size - pMemState->free);
        }
        case PM_METRIC_GPU_MEM_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pMemState = PollMemoryStateEndpoint_(device, requestQpc);
            if (pMemState == nullptr || pMemState->size == 0) {
                return 0.0;
            }
            return 100.0 * ((double)(pMemState->size - pMemState->free) / (double)pMemState->size);
        }
        case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto memBandwidth = PollMemoryBandwidthEndpoint_(device);
            if (!memBandwidth) {
                return (uint64_t)0;
            }
            return (uint64_t)memBandwidth->maxBandwidth;
        }
        default:
            return PollTelemetryMetric_(device, metricId, arrayIndex, requestQpc);
        }
    }

    void IgclTelemetryProvider::ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex)
    {
        if (arrayIndex != 0) {
            throw std::out_of_range{ "IGCL scalar metric queried with nonzero array index" };
        }
        (void)metricId;
    }

    bool IgclTelemetryProvider::TryInitializeDevice_(
        DeviceState_& device,
        ctl_device_adapter_handle_t handle) const
    {
        device.handle = handle;
        device.deviceLuid.resize(sizeof(LUID));
        device.properties = {
            .Size = sizeof(ctl_device_adapter_properties_t),
            .pDeviceID = device.deviceLuid.empty() ? nullptr : device.deviceLuid.data(),
            .device_id_size = (uint32_t)device.deviceLuid.size(),
        };

        const auto propertiesResult = ctlGetDeviceProperties(device.handle, &device.properties);
        if (propertiesResult != CTL_RESULT_SUCCESS) {
            pmlog_error("ctlGetDeviceProperties failed").code(propertiesResult);
            return false;
        }
        pmlog_verb(v::tele_gpu)("ctlGetDeviceProperties output")
            .pmwatch(device.properties.name)
            .pmwatch(device.providerDeviceId)
            .pmwatch(ref::DumpGenerated(device.properties));

        if (device.properties.device_type != CTL_DEVICE_TYPE_GRAPHICS) {
            return false;
        }

        device.fingerprint.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
        device.fingerprint.vendor = PM_DEVICE_VENDOR_INTEL;
        device.fingerprint.deviceName = device.properties.name;
        if (!device.deviceLuid.empty()) {
            device.fingerprint.luid = device.deviceLuid;
        }

        device.isAlchemist = std::regex_search(device.fingerprint.deviceName, std::regex{ R"(Arc.*A\d{3})" });
        pmlog_verb(v::tele_gpu)("Alchemist detection")
            .pmwatch(device.fingerprint.deviceName)
            .pmwatch(device.providerDeviceId)
            .pmwatch(device.isAlchemist);

        EnumerateMemoryModules_(device);
        EnumeratePowerDomains_(device);
        EnumerateFans_(device);

        return true;
    }

    void IgclTelemetryProvider::EnumerateMemoryModules_(DeviceState_& device) const
    {
        uint32_t memoryModuleCount = 0;
        const auto enumCountResult = ctlEnumMemoryModules(device.handle, &memoryModuleCount, nullptr);
        if (enumCountResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumMemoryModules(count) failed").code(enumCountResult)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        device.memoryModules.resize((size_t)memoryModuleCount);
        const auto enumListResult = ctlEnumMemoryModules(
            device.handle, &memoryModuleCount, device.memoryModules.data());
        if (enumListResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumMemoryModules(list) failed").code(enumListResult)
                .pmwatch(device.fingerprint.deviceName);
            device.memoryModules.clear();
            return;
        }
        pmlog_verb(v::tele_gpu)("ctlEnumMemoryModules output")
            .pmwatch(device.fingerprint.deviceName)
            .pmwatch(device.providerDeviceId)
            .pmwatch(ref::DumpGenerated(device.memoryModules));
    }

    void IgclTelemetryProvider::EnumeratePowerDomains_(DeviceState_& device) const
    {
        uint32_t powerDomainCount = 0;
        const auto enumCountResult = ctlEnumPowerDomains(device.handle, &powerDomainCount, nullptr);
        if (enumCountResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumPowerDomains(count) failed").code(enumCountResult)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        device.powerDomains.resize((size_t)powerDomainCount);
        const auto enumListResult = ctlEnumPowerDomains(
            device.handle, &powerDomainCount, device.powerDomains.data());
        if (enumListResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumPowerDomains(list) failed").code(enumListResult)
                .pmwatch(device.fingerprint.deviceName);
            device.powerDomains.clear();
            return;
        }
        pmlog_verb(v::tele_gpu)("ctlEnumPowerDomains(output)")
            .pmwatch(device.fingerprint.deviceName)
            .pmwatch(device.providerDeviceId)
            .pmwatch(ref::DumpGenerated(device.powerDomains));
    }

    void IgclTelemetryProvider::EnumerateFans_(DeviceState_& device) const
    {
        device.maxFanSpeedsRpm.clear();
        device.maxFanSpeedsRpm.resize((size_t)CTL_FAN_COUNT, 0);

        uint32_t fanCount = 0;
        const auto enumCountResult = ctlEnumFans(device.handle, &fanCount, nullptr);
        if (enumCountResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumFans(count) failed").code(enumCountResult)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        if (fanCount == 0) {
            return;
        }

        std::vector<ctl_fan_handle_t> fanHandles{};
        fanHandles.resize((size_t)fanCount);
        const auto enumListResult = ctlEnumFans(device.handle, &fanCount, fanHandles.data());
        if (enumListResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlEnumFans(list) failed").code(enumListResult)
                .pmwatch(device.fingerprint.deviceName);
            return;
        }

        const auto fanCountToRead = (uint32_t)std::min((size_t)fanCount, (size_t)CTL_FAN_COUNT);
        for (uint32_t iFan = 0; iFan < fanCountToRead; ++iFan) {
            const auto hFan = fanHandles[(size_t)iFan];
            if (hFan == nullptr) {
                continue;
            }

            ctl_fan_properties_t fanProperties{ .Size = sizeof(ctl_fan_properties_t) };
            const auto fanPropsResult = ctlFanGetProperties(hFan, &fanProperties);
            if (fanPropsResult != CTL_RESULT_SUCCESS) {
                pmlog_warn("ctlFanGetProperties failed").code(fanPropsResult)
                    .pmwatch(device.fingerprint.deviceName).pmwatch(iFan);
                continue;
            }
            pmlog_verb(v::tele_gpu)("ctlFanGetProperties output")
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.providerDeviceId)
                .pmwatch(iFan)
                .pmwatch(ref::DumpGenerated(fanProperties));

            device.maxFanSpeedsRpm[(size_t)iFan] = fanProperties.maxRPM;
        }
    }

    ipc::MetricCapabilities IgclTelemetryProvider::BuildCapsForDevice_(DeviceState_& device) const
    {
        ipc::MetricCapabilities caps{};
        const auto requestQpc = GetCurrentTimestamp();

        // Static capabilities can exist even if they do not route through dynamic polling.
        caps.Set(PM_METRIC_GPU_VENDOR, 1);
        caps.Set(PM_METRIC_GPU_NAME, 1);

        const auto powerLimits = PollPowerLimitsEndpoint_(device);
        if (powerLimits && powerLimits->sustainedPowerLimit.enabled) {
            caps.Set(PM_METRIC_GPU_SUSTAINED_POWER_LIMIT, 1);
        }

        const auto* pMemoryState = PollMemoryStateEndpoint_(device, requestQpc);
        if (pMemoryState != nullptr) {
            caps.Set(PM_METRIC_GPU_MEM_SIZE, 1);
            caps.Set(PM_METRIC_GPU_MEM_USED, 1);
            caps.Set(PM_METRIC_GPU_MEM_UTILIZATION, 1);
        }

        const auto memoryBandwidth = PollMemoryBandwidthEndpoint_(device);
        if (memoryBandwidth) {
            caps.Set(PM_METRIC_GPU_MEM_MAX_BANDWIDTH, 1);
            device.gpuMemMaxBwCacheValueBps = memoryBandwidth->maxBandwidth;
        }

        const auto& sample = PollTelemetryEndpoint_(device, requestQpc);

        if (IsUsageTelemetryItemSupported_(sample.gpuEnergyCounter)) {
            caps.Set(PM_METRIC_GPU_POWER, 1);
        }
        if (IsUsageTelemetryItemSupported_(sample.totalCardEnergyCounter)) {
            caps.Set(PM_METRIC_GPU_CARD_POWER, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuVoltage)) {
            caps.Set(PM_METRIC_GPU_VOLTAGE, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuCurrentClockFrequency)) {
            caps.Set(PM_METRIC_GPU_FREQUENCY, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuCurrentTemperature)) {
            caps.Set(PM_METRIC_GPU_TEMPERATURE, 1);
        }
        if (IsUsagePercentTelemetryItemSupported_(sample.globalActivityCounter)) {
            caps.Set(PM_METRIC_GPU_UTILIZATION, 1);
        }
        if (IsUsagePercentTelemetryItemSupported_(sample.renderComputeActivityCounter)) {
            caps.Set(PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION, 1);
        }
        if (IsUsagePercentTelemetryItemSupported_(sample.mediaActivityCounter)) {
            caps.Set(PM_METRIC_GPU_MEDIA_UTILIZATION, 1);
        }

        caps.Set(PM_METRIC_GPU_POWER_LIMITED, 1);
        caps.Set(PM_METRIC_GPU_TEMPERATURE_LIMITED, 1);
        caps.Set(PM_METRIC_GPU_VOLTAGE_LIMITED, 1);
        caps.Set(PM_METRIC_GPU_UTILIZATION_LIMITED, 1);
        if (device.isAlchemist) {
            caps.Set(PM_METRIC_GPU_CURRENT_LIMITED, 1);
        }

        if (IsInstantaneousTelemetryItemSupported_(sample.gpuEffectiveClock)) {
            caps.Set(PM_METRIC_GPU_EFFECTIVE_FREQUENCY, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuVrTemp)) {
            caps.Set(PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.vramCurrentEffectiveFrequency)) {
            caps.Set(PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY, 1);
            caps.Set(PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuOverVoltagePercent)) {
            caps.Set(PM_METRIC_GPU_OVERVOLTAGE_PERCENT, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuTemperaturePercent)) {
            caps.Set(PM_METRIC_GPU_TEMPERATURE_PERCENT, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.gpuPowerPercent)) {
            caps.Set(PM_METRIC_GPU_POWER_PERCENT, 1);
        }

        if (IsUsageTelemetryItemSupported_(sample.vramEnergyCounter)) {
            caps.Set(PM_METRIC_GPU_MEM_POWER, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.vramVoltage)) {
            caps.Set(PM_METRIC_GPU_MEM_VOLTAGE, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.vramCurrentClockFrequency)) {
            caps.Set(PM_METRIC_GPU_MEM_FREQUENCY, 1);
        }
        if (IsInstantaneousTelemetryItemSupported_(sample.vramCurrentTemperature)) {
            caps.Set(PM_METRIC_GPU_MEM_TEMPERATURE, 1);
        }

        const bool hasV1ReadBandwidth = IsInstantaneousTelemetryItemSupported_(sample.vramReadBandwidth);
        const bool hasV1WriteBandwidth = IsInstantaneousTelemetryItemSupported_(sample.vramWriteBandwidth);
        const bool hasCounterReadBandwidth = IsUsageTelemetryItemSupported_(sample.vramReadBandwidthCounter);
        const bool hasCounterWriteBandwidth = IsUsageTelemetryItemSupported_(sample.vramWriteBandwidthCounter);
        if (hasV1ReadBandwidth && hasV1WriteBandwidth) {
            device.useNewBandwidthTelemetry = true;
            caps.Set(PM_METRIC_GPU_MEM_READ_BANDWIDTH, 1);
            caps.Set(PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, 1);
        }
        else if (hasCounterReadBandwidth && hasCounterWriteBandwidth) {
            device.useNewBandwidthTelemetry = false;
            caps.Set(PM_METRIC_GPU_MEM_READ_BANDWIDTH, 1);
            caps.Set(PM_METRIC_GPU_MEM_WRITE_BANDWIDTH, 1);
        }

        device.fanSpeedCount = 0;
        while (device.fanSpeedCount < CTL_FAN_COUNT &&
            IsInstantaneousTelemetryItemSupported_(sample.fanSpeed[(size_t)device.fanSpeedCount])) {
            ++device.fanSpeedCount;
        }
        for (uint32_t iFan = device.fanSpeedCount; iFan < CTL_FAN_COUNT; ++iFan) {
            if (IsInstantaneousTelemetryItemSupported_(sample.fanSpeed[(size_t)iFan])) {
                pmlog_warn("Detected sparse fan-speed telemetry population; assuming dense indices")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.fanSpeedCount)
                    .pmwatch(iFan);
                break;
            }
        }

        device.fanSpeedPercentCount = 0;
        while (device.fanSpeedPercentCount < device.fanSpeedCount &&
            device.fanSpeedPercentCount < (uint32_t)device.maxFanSpeedsRpm.size() &&
            device.maxFanSpeedsRpm[(size_t)device.fanSpeedPercentCount] > 0) {
            ++device.fanSpeedPercentCount;
        }
        for (uint32_t iFan = device.fanSpeedPercentCount; iFan < device.fanSpeedCount; ++iFan) {
            if (iFan < (uint32_t)device.maxFanSpeedsRpm.size() &&
                device.maxFanSpeedsRpm[(size_t)iFan] > 0) {
                pmlog_warn("Detected sparse fan-speed-percent telemetry population; assuming dense indices")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.fanSpeedPercentCount)
                    .pmwatch(iFan);
                break;
            }
        }

        if (device.fanSpeedCount > 0) {
            caps.Set(PM_METRIC_GPU_FAN_SPEED, device.fanSpeedCount);
        }
        if (device.fanSpeedPercentCount > 0) {
            caps.Set(PM_METRIC_GPU_FAN_SPEED_PERCENT, device.fanSpeedPercentCount);
        }

        return caps;
    }

    const ctl_power_telemetry_t& IgclTelemetryProvider::PollTelemetryEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        if (device.telemetrySamples.Matches(requestQpc)) {
            return device.telemetrySamples.Current();
        }

        auto& currentSample = device.telemetrySamples.PrepareForWrite(requestQpc);

        pmlog_verb(v::tele_gpu)("telemetry poll tick")
            .pmwatch(device.fingerprint.deviceName)
            .pmwatch(device.providerDeviceId);
        const auto pollResult = ctlPowerTelemetryGet(device.handle, &currentSample);
        if (pollResult != CTL_RESULT_SUCCESS) {
            pmlog_warn("ctlPowerTelemetryGet failed").code(pollResult).every(std::chrono::seconds{ 60 })
                .pmwatch(device.fingerprint.deviceName);
            currentSample = {
                .Size = sizeof(ctl_power_telemetry_t),
                .Version = 1,
            };
        }
        pmlog_verb(v::tele_gpu)("ctlPowerTelemetryGet output")
            .pmwatch(device.fingerprint.deviceName)
            .pmwatch(device.providerDeviceId)
            .pmwatch(ref::DumpGenerated(currentSample));

        return device.telemetrySamples.Current();
    }

    const ctl_mem_state_t* IgclTelemetryProvider::PollMemoryStateEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.memoryStateEndpointCache;
        if (cache.Matches(requestQpc)) {
            return cache.output.Size == sizeof(ctl_mem_state_t) ? &cache.output : nullptr;
        }

        cache.output = { .Size = sizeof(ctl_mem_state_t) };
        cache.requestQpc = requestQpc;

        if (!device.memoryModules.empty()) {
            const auto result = ctlMemoryGetState(device.memoryModules[0], &cache.output);
            if (result != CTL_RESULT_SUCCESS) {
                pmlog_warn("ctlMemoryGetState failed").code(result).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.fingerprint.deviceName);
                cache.output.Size = 0;
            }
            pmlog_verb(v::tele_gpu)("ctlMemoryGetState output")
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.providerDeviceId)
                .pmwatch(ref::DumpGenerated(cache.output));
        }
        else {
            cache.output.Size = 0;
        }
        return cache.output.Size == sizeof(ctl_mem_state_t) ? &cache.output : nullptr;
    }

    std::optional<ctl_mem_bandwidth_t> IgclTelemetryProvider::PollMemoryBandwidthEndpoint_(
        DeviceState_& device) const
    {
        ctl_mem_bandwidth_t memoryBandwidth{
            .Size = sizeof(ctl_mem_bandwidth_t),
            .Version = 1,
        };

        if (!device.memoryModules.empty()) {
            const auto result = ctlMemoryGetBandwidth(device.memoryModules[0], &memoryBandwidth);
            if (result == CTL_RESULT_SUCCESS) {
                device.gpuMemMaxBwCacheValueBps = memoryBandwidth.maxBandwidth;
                pmlog_verb(v::tele_gpu)("ctlMemoryGetBandwidth output")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(ref::DumpGenerated(memoryBandwidth));
                return memoryBandwidth;
            }
            else {
                pmlog_warn("ctlMemoryGetBandwidth failed").code(result).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.fingerprint.deviceName);
            }
            pmlog_verb(v::tele_gpu)("ctlMemoryGetBandwidth output")
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.providerDeviceId)
                .pmwatch(ref::DumpGenerated(memoryBandwidth));
        }

        return {};
    }

    std::optional<ctl_power_limits_t> IgclTelemetryProvider::PollPowerLimitsEndpoint_(
        DeviceState_& device) const
    {
        ctl_power_limits_t powerLimits{
            .Size = sizeof(ctl_power_limits_t),
        };

        if (!device.powerDomains.empty()) {
            const auto result = ctlPowerGetLimits(device.powerDomains[0], &powerLimits);
            if (result == CTL_RESULT_SUCCESS) {
                pmlog_verb(v::tele_gpu)("ctlPowerGetLimits output")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(ref::DumpGenerated(powerLimits));
                return powerLimits;
            }
            else {
                pmlog_warn("ctlPowerGetLimits failed").code(result).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.fingerprint.deviceName);
            }
        }

        return {};
    }

    TelemetryMetricValue IgclTelemetryProvider::PollTelemetryMetric_(
        DeviceState_& device,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc) const
    {
        const auto& currentSample = PollTelemetryEndpoint_(device, requestQpc);
        const auto* pPreviousSample = device.telemetrySamples.Previous();
        double timeDelta = 0.0;
        if (device.telemetrySamples.PreviousRequestQpc() != 0 &&
            pPreviousSample != nullptr &&
            currentSample.timeStamp.type == CTL_DATA_TYPE_DOUBLE &&
            pPreviousSample->timeStamp.type == CTL_DATA_TYPE_DOUBLE) {
            timeDelta = currentSample.timeStamp.value.datadouble -
                pPreviousSample->timeStamp.value.datadouble;
        }

        double value = 0.0;

        switch (metricId) {
        case PM_METRIC_GPU_POWER:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsageTelemetryItem_(
                    device, metricId, currentSample.gpuEnergyCounter, pPreviousSample->gpuEnergyCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_CARD_POWER:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsageTelemetryItem_(
                    device, metricId, currentSample.totalCardEnergyCounter, pPreviousSample->totalCardEnergyCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_VOLTAGE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuVoltage, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuCurrentClockFrequency, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_TEMPERATURE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuCurrentTemperature, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_UTILIZATION:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsagePercentTelemetryItem_(
                    currentSample.globalActivityCounter, pPreviousSample->globalActivityCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsagePercentTelemetryItem_(
                    currentSample.renderComputeActivityCounter, pPreviousSample->renderComputeActivityCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEDIA_UTILIZATION:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsagePercentTelemetryItem_(
                    currentSample.mediaActivityCounter, pPreviousSample->mediaActivityCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_POWER_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return currentSample.gpuPowerLimited;
        case PM_METRIC_GPU_TEMPERATURE_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return currentSample.gpuTemperatureLimited;
        case PM_METRIC_GPU_CURRENT_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return currentSample.gpuCurrentLimited;
        case PM_METRIC_GPU_VOLTAGE_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return currentSample.gpuVoltageLimited;
        case PM_METRIC_GPU_UTILIZATION_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return currentSample.gpuUtilizationLimited;
        case PM_METRIC_GPU_MEM_POWER:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (pPreviousSample &&
                TryGetUsageTelemetryItem_(
                    device, metricId, currentSample.vramEnergyCounter, pPreviousSample->vramEnergyCounter, timeDelta, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_VOLTAGE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.vramVoltage, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.vramCurrentClockFrequency, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.vramCurrentEffectiveFrequency, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_TEMPERATURE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.vramCurrentTemperature, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (device.useNewBandwidthTelemetry) {
                pmlog_verb(v::tele_gpu)("ctlPowerTelemetryGet VRAM bandwidth V1 path")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.providerDeviceId);
                if (TryGetInstantaneousTelemetryItem_(currentSample.vramWriteBandwidth, value)) {
                    return ConvertMegabytesPerSecondToBitsPerSecond_(value);
                }
                device.useNewBandwidthTelemetry = false;
            }
            pmlog_verb(v::tele_gpu)("ctlPowerTelemetryGet VRAM bandwidth counter path")
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.providerDeviceId);
            if (pPreviousSample &&
                TryGetUsageTelemetryItem_(
                    device,
                    metricId,
                    currentSample.vramWriteBandwidthCounter,
                    pPreviousSample->vramWriteBandwidthCounter,
                    timeDelta,
                    value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (device.useNewBandwidthTelemetry) {
                pmlog_verb(v::tele_gpu)("ctlPowerTelemetryGet VRAM bandwidth V1 path")
                    .pmwatch(device.fingerprint.deviceName)
                    .pmwatch(device.providerDeviceId);
                if (TryGetInstantaneousTelemetryItem_(currentSample.vramReadBandwidth, value)) {
                    return ConvertMegabytesPerSecondToBitsPerSecond_(value);
                }
                device.useNewBandwidthTelemetry = false;
            }
            pmlog_verb(v::tele_gpu)("ctlPowerTelemetryGet VRAM bandwidth counter path")
                .pmwatch(device.fingerprint.deviceName)
                .pmwatch(device.providerDeviceId);
            if (pPreviousSample &&
                TryGetUsageTelemetryItem_(
                    device,
                    metricId,
                    currentSample.vramReadBandwidthCounter,
                    pPreviousSample->vramReadBandwidthCounter,
                    timeDelta,
                    value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_EFFECTIVE_FREQUENCY:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuEffectiveClock, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuVrTemp, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.vramCurrentEffectiveFrequency, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_OVERVOLTAGE_PERCENT:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuOverVoltagePercent, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_TEMPERATURE_PERCENT:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuTemperaturePercent, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_POWER_PERCENT:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            if (TryGetInstantaneousTelemetryItem_(currentSample.gpuPowerPercent, value)) {
                return value;
            }
            return 0.0;
        case PM_METRIC_GPU_FAN_SPEED:
        {
            if (arrayIndex >= device.fanSpeedCount) {
                throw Except<>("IGCL array index out of range");
            }
            if (TryGetInstantaneousTelemetryItem_(currentSample.fanSpeed[(size_t)arrayIndex], value)) {
                return value;
            }
            return 0.0;
        }
        case PM_METRIC_GPU_FAN_SPEED_PERCENT:
        {
            if (arrayIndex >= device.fanSpeedPercentCount) {
                throw Except<>("IGCL array index out of range");
            }
            if (!TryGetInstantaneousTelemetryItem_(currentSample.fanSpeed[(size_t)arrayIndex], value)) {
                return 0.0;
            }
            if (arrayIndex >= (uint32_t)device.maxFanSpeedsRpm.size()) {
                return 0.0;
            }
            const auto maxFanRpm = device.maxFanSpeedsRpm[(size_t)arrayIndex];
            if (maxFanRpm <= 0) {
                return 0.0;
            }
            return value / (double)maxFanRpm;
        }
        case PM_METRIC_GPU_MEM_POWER_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return false;
        case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return false;
        case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return false;
        case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return false;
        case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return false;
        default:
            throw std::invalid_argument{ "Unsupported metric for IGCL provider" };
        }
    }

    bool IgclTelemetryProvider::TryGetInstantaneousTelemetryItem_(
        const ctl_oc_telemetry_item_t& telemetryItem,
        double& value)
    {
        if (telemetryItem.bSupported && telemetryItem.type == CTL_DATA_TYPE_DOUBLE) {
            value = telemetryItem.value.datadouble;
            return true;
        }

        value = 0.0;
        return false;
    }

    bool IgclTelemetryProvider::TryGetUsagePercentTelemetryItem_(
        const ctl_oc_telemetry_item_t& currentItem,
        const ctl_oc_telemetry_item_t& previousItem,
        double timeDelta,
        double& value)
    {
        if (!currentItem.bSupported || currentItem.type != CTL_DATA_TYPE_DOUBLE || timeDelta <= 0.0) {
            value = 0.0;
            return false;
        }

        const auto dataDelta = currentItem.value.datadouble - previousItem.value.datadouble;
        value = (dataDelta / timeDelta) * 100.0;
        return true;
    }

    bool IgclTelemetryProvider::TryGetUsageTelemetryItem_(
        DeviceState_& device,
        PM_METRIC metricId,
        const ctl_oc_telemetry_item_t& currentItem,
        const ctl_oc_telemetry_item_t& previousItem,
        double timeDelta,
        double& value)
    {
        if (!currentItem.bSupported || timeDelta <= 0.0) {
            value = 0.0;
            return false;
        }

        if (currentItem.type == CTL_DATA_TYPE_DOUBLE) {
            const auto dataDelta = currentItem.value.datadouble - previousItem.value.datadouble;
            value = dataDelta / timeDelta;

            if (metricId == PM_METRIC_GPU_MEM_POWER) {
                if (currentItem.value.datadouble < previousItem.value.datadouble) {
                    value = device.gpuMemPowerCacheValueW;
                }
                else {
                    device.gpuMemPowerCacheValueW = value;
                }
            }

            return true;
        }

        if (currentItem.type == CTL_DATA_TYPE_INT64) {
            const auto dataDelta = currentItem.value.data64 - previousItem.value.data64;
            value = (double)dataDelta / timeDelta;
            return true;
        }

        if (currentItem.type == CTL_DATA_TYPE_UINT64) {
            const auto dataDelta = currentItem.value.datau64 - previousItem.value.datau64;
            value = (double)dataDelta / timeDelta;

            if (metricId == PM_METRIC_GPU_MEM_READ_BANDWIDTH && !device.useNewBandwidthTelemetry) {
                const auto overflowOrWrap = currentItem.value.datau64 < previousItem.value.datau64;
                const auto overMax = dataDelta > device.gpuMemMaxBwCacheValueBps;
                if (overflowOrWrap || overMax) {
                    value = device.gpuMemReadBwCacheValueBps;
                }
                else {
                    device.gpuMemReadBwCacheValueBps = value;
                }
            }

            return true;
        }

        value = 0.0;
        return false;
    }

    bool IgclTelemetryProvider::IsInstantaneousTelemetryItemSupported_(
        const ctl_oc_telemetry_item_t& telemetryItem)
    {
        return telemetryItem.bSupported && telemetryItem.type == CTL_DATA_TYPE_DOUBLE;
    }

    bool IgclTelemetryProvider::IsUsageTelemetryItemSupported_(
        const ctl_oc_telemetry_item_t& telemetryItem)
    {
        if (!telemetryItem.bSupported) {
            return false;
        }

        return telemetryItem.type == CTL_DATA_TYPE_DOUBLE ||
            telemetryItem.type == CTL_DATA_TYPE_INT64 ||
            telemetryItem.type == CTL_DATA_TYPE_UINT64;
    }

    bool IgclTelemetryProvider::IsUsagePercentTelemetryItemSupported_(
        const ctl_oc_telemetry_item_t& telemetryItem)
    {
        return telemetryItem.bSupported && telemetryItem.type == CTL_DATA_TYPE_DOUBLE;
    }

    double IgclTelemetryProvider::ConvertMegabytesPerSecondToBitsPerSecond_(double megabytesPerSecond)
    {
        return megabytesPerSecond * 8.0 * 1000000.0;
    }
}
