// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "IntelPowerTelemetryAdapter.h"
#include "Logging.h"
#include "../CommonUtilities/Math.h"
#include "../CommonUtilities/ref/GeneratedReflection.h"
#include "../CommonUtilities/ref/StaticReflection.h"
#include <regex>
#include <ranges>

using namespace pmon;
using namespace util;
namespace vi = std::views;
using v = log::V;

namespace pwr::intel
{
    namespace {
        GpuTelemetryCapBits GetFanSpeedTelemetryCapBit_(
            uint32_t fan_index) {
            switch (fan_index) {
            case 0:
                return GpuTelemetryCapBits::fan_speed_0;
            case 1:
                return GpuTelemetryCapBits::fan_speed_1;
            case 2:
                return GpuTelemetryCapBits::fan_speed_2;
            case 3:
                return GpuTelemetryCapBits::fan_speed_3;
            case 4:
                return GpuTelemetryCapBits::fan_speed_4;
            default:
                throw std::runtime_error{ "Invalid fan speed index" };
            }
        }
        GpuTelemetryCapBits GetPsuTelemetryCapBit_(
            uint32_t psu_index) {
            switch (psu_index) {
            case 0:
                return GpuTelemetryCapBits::psu_info_0;
            case 1:
                return GpuTelemetryCapBits::psu_info_1;
            case 2:
                return GpuTelemetryCapBits::psu_info_2;
            case 3:
                return GpuTelemetryCapBits::psu_info_3;
            case 4:
                return GpuTelemetryCapBits::psu_info_4;
            default:
                throw std::runtime_error{ "Invalid PSU index" };
            }
        }
        GpuTelemetryCapBits GetMaxFanSpeedTelemetryCapBit_(
            uint32_t index) {
            switch (index) {
            case 0:
                return GpuTelemetryCapBits::max_fan_speed_0;
            case 1:
                return GpuTelemetryCapBits::max_fan_speed_1;
            case 2:
                return GpuTelemetryCapBits::max_fan_speed_2;
            case 3:
                return GpuTelemetryCapBits::max_fan_speed_3;
            case 4:
                return GpuTelemetryCapBits::max_fan_speed_4;
            default:
                throw std::runtime_error{ "Invalid max fan speed index" };
            }
        }
    }

    // public interface functions

    IntelPowerTelemetryAdapter::IntelPowerTelemetryAdapter(ctl_device_adapter_handle_t handle)
        :
        deviceHandle{ handle }
    {
        properties = {
            .Size = sizeof(ctl_device_adapter_properties_t),
            .pDeviceID = &deviceId,
            .device_id_size = sizeof(deviceId),
        };

        if (auto result = ctlGetDeviceProperties(deviceHandle, &properties);
            result != CTL_RESULT_SUCCESS) {
            throw std::runtime_error{ "Failure to get device properties" };
        }
        pmlog_verb(v::tele_gpu)("Device properties").pmwatch(ref::DumpGenerated(properties));

        if (properties.device_type != CTL_DEVICE_TYPE_GRAPHICS) {
            throw NonGraphicsDeviceException{};
        }

        // check for alchemist (used to enable features whose support not reported by IGCL)
        // use device name that match Arc followed by A### part number pattern
        isAlchemist = std::regex_search(GetName(), std::regex{ R"(Arc.*A\d{3})" });
        pmlog_verb(v::tele_gpu)("Detecting Alchemist").pmwatch(GetName()).pmwatch(isAlchemist);

        // errors are reported inside this function
        // do not hard-fail on memory module issues, so no need to check error return code
        EnumerateMemoryModules();

        // get count of power domains
        uint32_t powerDomainCount = 0;
        if (auto result = ctlEnumPowerDomains(deviceHandle, &powerDomainCount, nullptr);
            result == CTL_RESULT_SUCCESS) {
            powerDomains.resize(powerDomainCount);
            // get power domain handles
            if (auto result = ctlEnumPowerDomains(deviceHandle, &powerDomainCount, powerDomains.data());
                result == CTL_RESULT_SUCCESS) {
                pmlog_verb(v::tele_gpu)("Power domains enumerated").pmwatch(GetName()).pmwatch(ref::DumpGenerated(powerDomains));
            }
            else {
                pmlog_error("ctlEnumPowerDomains failed enumeration").code(result).pmwatch(GetName());
            }
        }
        else {
            pmlog_error("ctlEnumPowerDomains failed to get count").code(result).pmwatch(GetName());
        }

        // enumerate fans
        uint32_t fanCount = 0;
        if (auto res = ctlEnumFans(deviceHandle, &fanCount, nullptr); res == CTL_RESULT_SUCCESS) {
            if (fanCount > 0) {
                std::vector<ctl_fan_handle_t> fanHandles(fanCount);
                if (auto res = ctlEnumFans(deviceHandle, &fanCount, fanHandles.data()); res == CTL_RESULT_SUCCESS) {
                    for (auto&&[iFan, hFan] : vi::enumerate(fanHandles)) {
                        if (hFan) {
                            ctl_fan_properties_t props{ .Size = sizeof(ctl_fan_properties_t) };
                            if (auto res = ctlFanGetProperties(hFan, &props); res != CTL_RESULT_SUCCESS) {
                                pmlog_error(std::format("failed to get properties for fan #{}", iFan)).code(res);
                                maxFanSpeedsRpm_.push_back(0);
                            }
                            pmlog_verb(v::tele_gpu)("Got fan properties").pmwatch(GetName()).pmwatch(iFan)
                                .pmwatch(ref::DumpGenerated(props));
                            maxFanSpeedsRpm_.push_back(props.maxRPM);
                        }
                        else {
                            pmlog_warn("null handle received from ctlEnumFans");
                            maxFanSpeedsRpm_.push_back(0);
                        }
                    }
                }
                else {
                    pmlog_error("failed getting fan handles").code(res);
                }
            }
        }
        else {
            pmlog_error("failed getting fan count").code(res);
        }
    }

    bool IntelPowerTelemetryAdapter::Sample() noexcept
    {
        pmlog_verb(v::tele_gpu)("Sample called").pmwatch(GetName());

        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        bool success = true;

        decltype(previousSampleVariant) currentSampleVariant;

        if (useV1PowerTelemetry) {
            currentSampleVariant = ctl_power_telemetry2_t{
                .Size = sizeof(ctl_power_telemetry2_t),
                .Version = 1
            };
            auto currentSample = std::get_if<ctl_power_telemetry2_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            if (const auto result = ctlPowerTelemetryGet(deviceHandle,
                (ctl_power_telemetry_t*)currentSample); result != CTL_RESULT_SUCCESS) {
                // treating any error as unavailability of the API version since some driver version do not correctly report
                // version error
                useV1PowerTelemetry = false;
                useNewBandwidthTelemetry = false;
                success = false;
                pmlog_warn("Failed to access ctlPowerTelemetryGet.v1, falling back to .v0");
            }
            pmlog_verb(v::tele_gpu)("get power telemetry V1").pmwatch(GetName()).pmwatch(ref::DumpGenerated(*currentSample));
        }
        if (!useV1PowerTelemetry) {
            currentSampleVariant = ctl_power_telemetry_t{
                .Size = sizeof(ctl_power_telemetry_t),
                .Version = 0
            };
            auto currentSample = std::get_if<ctl_power_telemetry_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            if (const auto result = ctlPowerTelemetryGet(deviceHandle, currentSample);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::tele_gpu)("get power telemetry V0").pmwatch(GetName()).pmwatch(ref::DumpGenerated(*currentSample));
        }

        // Query memory state and bandwidth if supported
        ctl_mem_state_t memory_state = {.Size = sizeof(ctl_mem_state_t)};
        ctl_mem_bandwidth_t memory_bandwidth = {
            .Size = sizeof(ctl_mem_bandwidth_t),
            .Version = 1,
        };
        // Question: why are we only using the first element of memoryModules here?
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetState(memoryModules[0], &memory_state);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::tele_gpu)("get memory state").pmwatch(GetName()).pmwatch(ref::DumpGenerated(memory_state));
            
            if (const auto result = ctlMemoryGetBandwidth(memoryModules[0], &memory_bandwidth);
                result != CTL_RESULT_SUCCESS) {
                success = false;
                IGCL_ERR(result);
            }
            pmlog_verb(v::tele_gpu)("get memory bandwidth").pmwatch(GetName()).pmwatch(ref::DumpGenerated(memory_bandwidth));
        }

        std::optional<double> gpu_sustained_power_limit_mw;
        if (!powerDomains.empty()) {
            ctl_power_limits_t limits{
                .Size = sizeof(ctl_power_limits_t),
            };
            if (const auto result = ctlPowerGetLimits(powerDomains[0], &limits);
                result == CTL_RESULT_SUCCESS) {
                if (limits.sustainedPowerLimit.enabled) {
                    gpu_sustained_power_limit_mw = (double)limits.sustainedPowerLimit.power;
                }
                pmlog_verb(v::tele_gpu)(std::format("ctlPowerGetLimits output")).pmwatch(GetName())
                    .pmwatch(ref::DumpGenerated(limits));
            }
            else {
                success = false;
                IGCL_ERR(result);
            }
        }

        if (useV1PowerTelemetry) {
            auto currentSample = std::get_if<ctl_power_telemetry2_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            else {
                success = GatherSampleData(*currentSample, memory_state,
                    memory_bandwidth, gpu_sustained_power_limit_mw, (uint64_t)qpc.QuadPart) && success;
            }
        }
        else {
            auto currentSample = std::get_if<ctl_power_telemetry_t>(&currentSampleVariant);
            // sanity check; should never fail
            if (!currentSample) {
                success = false;
                IGCL_ERR(CTL_RESULT_ERROR_INVALID_ARGUMENT);
            }
            else {
                success = GatherSampleData(*currentSample, memory_state,
                    memory_bandwidth, gpu_sustained_power_limit_mw, (uint64_t)qpc.QuadPart) && success;
            }
        }

        return success;
    }

    std::optional<PresentMonPowerTelemetryInfo> IntelPowerTelemetryAdapter::GetClosest(uint64_t qpc) const noexcept
    {
        std::lock_guard<std::mutex> lock(historyMutex);
        const auto nearest = history.GetNearest(qpc);
        if constexpr (PMLOG_BUILD_LEVEL_ >= pmon::util::log::Level::Verbose) {
            if (!nearest) {
                pmlog_verb(v::tele_gpu)("Empty telemetry info sample returned").pmwatch(GetName()).pmwatch(qpc);
            }
            else {
                pmlog_verb(v::tele_gpu)("Nearest telemetry info sampled").pmwatch(GetName()).pmwatch(qpc).pmwatch(ref::DumpStatic(*nearest));
            }
        }
        return nearest;
    }

    PM_DEVICE_VENDOR IntelPowerTelemetryAdapter::GetVendor() const noexcept
    {
        return PM_DEVICE_VENDOR::PM_DEVICE_VENDOR_INTEL;
    }

    std::string IntelPowerTelemetryAdapter::GetName() const noexcept
    {
        return properties.name;
    }

    uint64_t IntelPowerTelemetryAdapter::GetDedicatedVideoMemory() const noexcept
    {
        ctl_mem_state_t memory_state = {.Size = sizeof(ctl_mem_state_t)};
        uint64_t video_mem_size = 0;
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetState(memoryModules[0], &memory_state);
                result == CTL_RESULT_SUCCESS) {
                video_mem_size = memory_state.size;
            }
            else {
                IGCL_ERR(result);
            }
            pmlog_verb(v::tele_gpu)("get memory state").pmwatch(GetName()).pmwatch(ref::DumpGenerated(memory_state));
        }
        return video_mem_size;
    }

    uint64_t IntelPowerTelemetryAdapter::GetVideoMemoryMaxBandwidth() const noexcept
    {
        ctl_mem_bandwidth_t memoryBandwidth = {
            .Size = sizeof(ctl_mem_bandwidth_t),
            .Version = 1,
        };
        uint64_t videoMemMaxBandwidth = 0;
        if (memoryModules.size() > 0) {
            if (const auto result = ctlMemoryGetBandwidth(memoryModules[0], &memoryBandwidth);
                result == CTL_RESULT_SUCCESS) {
                videoMemMaxBandwidth = memoryBandwidth.maxBandwidth;
            }
            else {
                IGCL_ERR(result);
            }
            pmlog_verb(v::tele_gpu)("get memory bandwidth").pmwatch(GetName()).pmwatch(ref::DumpGenerated(memoryBandwidth));
        }
        return videoMemMaxBandwidth;
    }

    double IntelPowerTelemetryAdapter::GetSustainedPowerLimit() const noexcept
    {
        double gpuSustainedPowerLimit = 0.;
        if (!powerDomains.empty()) {
            ctl_power_limits_t limits{
                .Size = sizeof(ctl_power_limits_t),
            };
            if (const auto result = ctlPowerGetLimits(powerDomains[0], &limits);
                result == CTL_RESULT_SUCCESS) {
                gpuSustainedPowerLimit = (double)limits.sustainedPowerLimit.power;
                pmlog_verb(v::tele_gpu)(std::format("ctlPowerGetLimits output")).pmwatch(GetName())
                    .pmwatch(ref::DumpGenerated(limits));
            }
            else {
                IGCL_ERR(result);
            }
        }
        // Control lib returns back in milliwatts
        return gpuSustainedPowerLimit / 1000.;
    }

    uint64_t IntelPowerTelemetryAdapter::GetAdapterId() const noexcept
    {
        // LUID is a struct with LowPart (DWORD) and HighPart (LONG)
        // We pack it into a uint64_t
        uint64_t id = 0;
        id |= (uint64_t)deviceId.LowPart;
        id |= (uint64_t)deviceId.HighPart << 32;
        return id;
    }
    // private implementation functions

    ctl_result_t IntelPowerTelemetryAdapter::EnumerateMemoryModules()
    {
        // first call ctlEnumMemoryModules with nullptr to get number of modules
        // and resize vector to accomodate
        uint32_t memory_module_count = 0;
        if (auto result = ctlEnumMemoryModules(deviceHandle, &memory_module_count,
            nullptr); result != CTL_RESULT_SUCCESS)
        {
            pmlog_error("ctlEnumMemoryModules getting count").code(result).pmwatch(GetName());
            return result;
        }
        memoryModules.resize(size_t(memory_module_count));
        // call ctlEnumMemoryModules to get the actual module data now
        if (auto result = ctlEnumMemoryModules(deviceHandle, &memory_module_count,
            memoryModules.data()); result != CTL_RESULT_SUCCESS)
        {
            pmlog_error("ctlEnumMemoryModules getting module data").code(result).pmwatch(GetName());
            memoryModules.clear();
            return result;
        }
        pmlog_verb(v::tele_gpu)("Memory modules enumerated").pmwatch(GetName()).pmwatch(ref::DumpGenerated(memoryModules));

        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    bool IntelPowerTelemetryAdapter::GatherSampleData(T& currentSample,
        ctl_mem_state_t& memory_state,
        ctl_mem_bandwidth_t& memory_bandwidth,
        std::optional<double> gpu_sustained_power_limit_mw,
        uint64_t qpc)
    {
        bool success = true;

        if (const auto result = GetTimeDelta(currentSample);
            result != CTL_RESULT_SUCCESS)
        {
            success = false;
            IGCL_ERR(result);
        }

        PresentMonPowerTelemetryInfo pm_gpu_power_telemetry_info{ .qpc = qpc };

        if (previousSampleVariant.index()) {

            if (const auto result = GetGPUPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetVramPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetFanPowerTelemetryData(currentSample,
                pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            if (const auto result = GetPsuPowerTelemetryData(
                currentSample, pm_gpu_power_telemetry_info); result != CTL_RESULT_SUCCESS)
            {
                success = false;
                IGCL_ERR(result);
            }

            // Get memory state and bandwidth data
            if (memoryModules.size() > 0) {
                GetMemStateTelemetryData(memory_state,
                    pm_gpu_power_telemetry_info);
                GetMemBandwidthData(memory_bandwidth,
                    pm_gpu_power_telemetry_info);
            }

            // Save and convert the gpu sustained power limit
            pm_gpu_power_telemetry_info.gpu_sustained_power_limit_w =
                gpu_sustained_power_limit_mw.value_or(0.) / 1000.;
            if (gpu_sustained_power_limit_mw) {
                SetTelemetryCapBit(GpuTelemetryCapBits::gpu_sustained_power_limit);                
            }

            // Save off the calculated PresentMon power telemetry values. These are
            // saved off for clients to extrace out timing information based on QPC
            SavePmPowerTelemetryData(pm_gpu_power_telemetry_info);
        }

        // Save off the raw control library data for calculating time delta
        // and usage data.
        if (const auto result = SaveTelemetry(currentSample, memory_bandwidth);
            result != CTL_RESULT_SUCCESS)
        {
            success = false;
            IGCL_ERR(result);
        }

        return success;
    }

    // TODO: stop using CTL stuff for non-ctl logic
    // TODO: better functional programming
    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetTimeDelta(const T& currentSample)
    {
        if (!previousSampleVariant.index()) {
            // We do not have a previous power telemetry item to calculate time
            // delta against.
            time_delta_ = 0.f;
        }
        else {
            auto previousSample = std::get_if<T>(&previousSampleVariant);
            if (previousSample && currentSample.timeStamp.type == CTL_DATA_TYPE_DOUBLE) {
                time_delta_ = currentSample.timeStamp.value.datadouble -
                    previousSample->timeStamp.value.datadouble;
            }
            else {
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }

        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetGPUPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.timeStamp,
            pm_gpu_power_telemetry_info.time_stamp,
            GpuTelemetryCapBits::time_stamp);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuVoltage,
            pm_gpu_power_telemetry_info.gpu_voltage_v,
            GpuTelemetryCapBits::gpu_voltage);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuCurrentClockFrequency,
            pm_gpu_power_telemetry_info.gpu_frequency_mhz,
            GpuTelemetryCapBits::gpu_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.gpuCurrentTemperature,
            pm_gpu_power_telemetry_info.gpu_temperature_c,
            GpuTelemetryCapBits::gpu_temperature);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsage(
            currentSample.gpuEnergyCounter,
            previousSample->gpuEnergyCounter,
            pm_gpu_power_telemetry_info.gpu_power_w,
            GpuTelemetryCapBits::gpu_power);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsage(
            currentSample.totalCardEnergyCounter,
            previousSample->totalCardEnergyCounter,
            pm_gpu_power_telemetry_info.gpu_card_power_w,
            GpuTelemetryCapBits::gpu_card_power);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.globalActivityCounter,
            previousSample->globalActivityCounter,
            pm_gpu_power_telemetry_info.gpu_utilization,
            GpuTelemetryCapBits::gpu_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.renderComputeActivityCounter,
            previousSample->renderComputeActivityCounter,
            pm_gpu_power_telemetry_info.gpu_render_compute_utilization,
            GpuTelemetryCapBits::gpu_render_compute_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetPowerTelemetryItemUsagePercent(
            currentSample.mediaActivityCounter,
            previousSample->mediaActivityCounter,
            pm_gpu_power_telemetry_info.gpu_media_utilization,
            GpuTelemetryCapBits::gpu_media_utilization);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        pm_gpu_power_telemetry_info.gpu_power_limited =
            currentSample.gpuPowerLimited;
        pm_gpu_power_telemetry_info.gpu_temperature_limited =
            currentSample.gpuTemperatureLimited;
        pm_gpu_power_telemetry_info.gpu_current_limited =
            currentSample.gpuCurrentLimited;
        pm_gpu_power_telemetry_info.gpu_voltage_limited =
            currentSample.gpuVoltageLimited;
        pm_gpu_power_telemetry_info.gpu_utilization_limited =
            currentSample.gpuUtilizationLimited;

        // On Intel all GPU limitation indicators are active except...
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_voltage_limited);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization_limited);

        // gpu_current_limited perf limit reason seems not supported on BMG
        // because there is no bSupported flags for the perf limit reasons
        // we detect alchemist and use this as a proxy for support
        if (isAlchemist) {
            SetTelemetryCapBit(GpuTelemetryCapBits::gpu_current_limited);
        }

        // these metrics only available with V1 struct
        if constexpr (std::same_as<T, ctl_power_telemetry2_t>) {
            result = GetInstantaneousPowerTelemetryItem(
                currentSample.gpuEffectiveClock,
                pm_gpu_power_telemetry_info.gpu_effective_frequency_mhz,
                GpuTelemetryCapBits::gpu_effective_frequency);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }

            result = GetInstantaneousPowerTelemetryItem(
                currentSample.gpuVrTemp,
                pm_gpu_power_telemetry_info.gpu_voltage_regulator_temperature_c,
                GpuTelemetryCapBits::gpu_voltage_regulator_temperature);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }

            result = GetInstantaneousPowerTelemetryItem(
                currentSample.vramCurrentEffectiveFrequency,
                pm_gpu_power_telemetry_info.gpu_mem_effective_bandwidth_gbps,
                GpuTelemetryCapBits::gpu_mem_effective_bandwidth);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }

            result = GetInstantaneousPowerTelemetryItem(
                currentSample.gpuOverVoltagePercent,
                pm_gpu_power_telemetry_info.gpu_overvoltage_percent,
                GpuTelemetryCapBits::gpu_overvoltage_percent);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }

            result = GetInstantaneousPowerTelemetryItem(
                currentSample.gpuTemperaturePercent,
                pm_gpu_power_telemetry_info.gpu_temperature_percent,
                GpuTelemetryCapBits::gpu_temperature_percent);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }

            result = GetInstantaneousPowerTelemetryItem(
                currentSample.gpuPowerPercent,
                pm_gpu_power_telemetry_info.gpu_power_percent,
                GpuTelemetryCapBits::gpu_power_percent);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }
        }

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetVramPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        result = GetInstantaneousPowerTelemetryItem(currentSample.vramVoltage,
            pm_gpu_power_telemetry_info.vram_voltage_v,
            GpuTelemetryCapBits::vram_voltage);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentClockFrequency,
            pm_gpu_power_telemetry_info.vram_frequency_mhz,
            GpuTelemetryCapBits::vram_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentEffectiveFrequency,
            pm_gpu_power_telemetry_info.vram_effective_frequency_gbps,
            GpuTelemetryCapBits::vram_effective_frequency);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        result = GetInstantaneousPowerTelemetryItem(
            currentSample.vramCurrentTemperature,
            pm_gpu_power_telemetry_info.vram_temperature_c,
            GpuTelemetryCapBits::vram_temperature);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        // bandwidth telemetry has 2 possible paths for aquisition
        if constexpr (std::same_as<T, ctl_power_telemetry2_t>) {
            using namespace pmon::util;
            if (useNewBandwidthTelemetry) {
                pmlog_verb(v::tele_gpu)("Processing VRAM BW V1").pmwatch(GetName());
                double gpuMemReadBandwidthMegabytesPerSecond = 0;
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.vramReadBandwidth,
                    gpuMemReadBandwidthMegabytesPerSecond,
                    GpuTelemetryCapBits::gpu_mem_read_bandwidth);
                // we need bandwidth in bits per second, IGCL V1 gives in megabytes per second
                pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps = ConvertMagnitudePrefix(
                    gpuMemReadBandwidthMegabytesPerSecond * 8.,
                    MagnitudePrefix::Mega,
                    MagnitudePrefix::Base);
                if (result != CTL_RESULT_SUCCESS ||
                    !(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_read_bandwidth))) {
                    useNewBandwidthTelemetry = false;
                    pmlog_info("V1 vram bandwidth not available, falling back to V0 counters")
                        .code(result).pmwatch(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_read_bandwidth));
                }
            }
            if (useNewBandwidthTelemetry) {
                double gpuMemWriteBandwidthMegabytesPerSecond = 0;
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.vramWriteBandwidth,
                    gpuMemWriteBandwidthMegabytesPerSecond,
                    GpuTelemetryCapBits::gpu_mem_write_bandwidth);
                // we need bandwidth in bits per second, IGCL V1 gives in megabytes per second
                pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps = ConvertMagnitudePrefix(
                    gpuMemWriteBandwidthMegabytesPerSecond * 8.,
                    MagnitudePrefix::Mega,
                    MagnitudePrefix::Base);
                if (result != CTL_RESULT_SUCCESS ||
                    !(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_write_bandwidth))) {
                    useNewBandwidthTelemetry = false;
                    pmlog_info("V1 vram bandwidth not available, falling back to V0 counters")
                        .code(result).pmwatch(HasTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_write_bandwidth));
                }
            }
        }
        if (!useNewBandwidthTelemetry) {
            pmlog_verb(v::tele_gpu)("Processing VRAM BW V0").pmwatch(GetName());
            result = GetPowerTelemetryItemUsage(
                currentSample.vramReadBandwidthCounter,
                previousSample->vramReadBandwidthCounter,
                pm_gpu_power_telemetry_info.gpu_mem_read_bandwidth_bps,
                GpuTelemetryCapBits::gpu_mem_read_bandwidth);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }
            result = GetPowerTelemetryItemUsage(
                currentSample.vramWriteBandwidthCounter,
                previousSample->vramWriteBandwidthCounter,
                pm_gpu_power_telemetry_info.gpu_mem_write_bandwidth_bps,
                GpuTelemetryCapBits::gpu_mem_write_bandwidth);
            if (result != CTL_RESULT_SUCCESS) {
                return result;
            }
        }

        result = GetPowerTelemetryItemUsage(currentSample.vramEnergyCounter,
            previousSample->vramEnergyCounter,
            pm_gpu_power_telemetry_info.vram_power_w,
            GpuTelemetryCapBits::vram_power);
        if (result != CTL_RESULT_SUCCESS) {
            return result;
        }

        // On Intel all VRAM limitation indicators are deprecated / return false
        // GpuTelemetryCapBits::vram_power_limited
        // GpuTelemetryCapBits::vram_temperature_limited
        // GpuTelemetryCapBits::vram_current_limited
        // GpuTelemetryCapBits::vram_voltage_limited
        // GpuTelemetryCapBits::vram_utilization_limited

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetFanPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result = CTL_RESULT_SUCCESS;

        for (uint32_t i = 0; i < CTL_FAN_COUNT; i++) {
            try {
                const auto fanSpeedCapBit = GetFanSpeedTelemetryCapBit_(i);
                result = GetInstantaneousPowerTelemetryItem(
                    currentSample.fanSpeed[i],
                    pm_gpu_power_telemetry_info.fan_speed_rpm[i],
                    fanSpeedCapBit);
                if (result != CTL_RESULT_SUCCESS) {
                    break;
                }
                if (HasTelemetryCapBit(fanSpeedCapBit) && maxFanSpeedsRpm_[i] > 0) {
                    SetTelemetryCapBit(GetMaxFanSpeedTelemetryCapBit_(i));
                    pm_gpu_power_telemetry_info.max_fan_speed_rpm[i] = maxFanSpeedsRpm_[i];
                }
            } catch (...) {
                result = CTL_RESULT_ERROR_INVALID_ARGUMENT;
            } 
        }

        return result;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::GetPsuPowerTelemetryData(
        const T& currentSample,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info)
    {
        ctl_result_t result = CTL_RESULT_SUCCESS;

        auto previousSample = std::get_if<T>(&previousSampleVariant);
        if (!previousSample) {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        for (uint32_t i = 0; i < CTL_PSU_COUNT; i++) {
            if (currentSample.psu[i].bSupported) {
                GpuTelemetryCapBits psu_telemetry_bit;
                try {
                  psu_telemetry_bit = GetPsuTelemetryCapBit_(i);
                  pm_gpu_power_telemetry_info.psu[i].psu_type =
                      PresentMonPsuType(currentSample.psu[i].psuType);
                  result = GetInstantaneousPowerTelemetryItem(
                      currentSample.psu[i].voltage,
                      pm_gpu_power_telemetry_info.psu[i].psu_voltage,
                      psu_telemetry_bit);
                  if (result != CTL_RESULT_SUCCESS) {
                    break;
                  }
                  result = GetPowerTelemetryItemUsage(
                      currentSample.psu[i].energyCounter,
                      previousSample->psu[i].energyCounter,
                      pm_gpu_power_telemetry_info.psu[i].psu_power,
                      psu_telemetry_bit);
                  if (result != CTL_RESULT_SUCCESS) {
                    break;
                  }
                } catch (...) {
                  result = CTL_RESULT_ERROR_INVALID_ARGUMENT;
                  break;
                } 
            }
            else {
                pm_gpu_power_telemetry_info.psu[i].psu_type =
                    PresentMonPsuType::None;
                pm_gpu_power_telemetry_info.psu[i].psu_power = 0.0f;
                pm_gpu_power_telemetry_info.psu[i].psu_voltage = 0.0f;
            }
        }

        return result;
    }

    void IntelPowerTelemetryAdapter::GetMemStateTelemetryData(
        const ctl_mem_state_t& mem_state,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info) {
        pm_gpu_power_telemetry_info.gpu_mem_total_size_b = mem_state.size;
        pm_gpu_power_telemetry_info.gpu_mem_used_b = mem_state.size - mem_state.free;
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_size);
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_used);
        return;
    }

    void IntelPowerTelemetryAdapter::GetMemBandwidthData(
        const ctl_mem_bandwidth_t& mem_bandwidth,
        PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info) {
        pm_gpu_power_telemetry_info.gpu_mem_max_bandwidth_bps = mem_bandwidth.maxBandwidth;
        gpu_mem_max_bw_cache_value_bps_ = mem_bandwidth.maxBandwidth;
        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_max_bandwidth);
        return;
    }
    
    ctl_result_t IntelPowerTelemetryAdapter::GetInstantaneousPowerTelemetryItem(
        const ctl_oc_telemetry_item_t& telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (telemetry_item.bSupported) {
            if (telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                pm_telemetry_value = telemetry_item.value.datadouble;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        else {
            pm_telemetry_value = 0.0;
        }
        return CTL_RESULT_SUCCESS;
    }

    ctl_result_t IntelPowerTelemetryAdapter::GetPowerTelemetryItemUsagePercent(
        const ctl_oc_telemetry_item_t& current_telemetry_item,
        const ctl_oc_telemetry_item_t& previous_telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (current_telemetry_item.bSupported) {
            if (current_telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                auto data_delta = current_telemetry_item.value.datadouble -
                    previous_telemetry_item.value.datadouble;
                pm_telemetry_value = (data_delta / time_delta_) * 100.0f;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        return CTL_RESULT_SUCCESS;
    }

    ctl_result_t IntelPowerTelemetryAdapter::GetPowerTelemetryItemUsage(
        const ctl_oc_telemetry_item_t& current_telemetry_item,
        const ctl_oc_telemetry_item_t& previous_telemetry_item,
        double& pm_telemetry_value, GpuTelemetryCapBits telemetry_cap_bit)
    {
        if (current_telemetry_item.bSupported) {
            if (current_telemetry_item.type == CTL_DATA_TYPE_DOUBLE) {
                auto data_delta = current_telemetry_item.value.datadouble -
                    previous_telemetry_item.value.datadouble;
                pm_telemetry_value = (data_delta / time_delta_);
                SetTelemetryCapBit(telemetry_cap_bit);
                if (telemetry_cap_bit == GpuTelemetryCapBits::vram_power && useV1PowerTelemetry) {
                    if (current_telemetry_item.value.datadouble < previous_telemetry_item.value.datadouble) {
                        pm_telemetry_value = gpu_mem_power_cache_value_w_;
                    }
                    else {
                        gpu_mem_power_cache_value_w_ = pm_telemetry_value;
                    }
                }
            } 
            else if (current_telemetry_item.type == CTL_DATA_TYPE_INT64) {
                auto data_delta = current_telemetry_item.value.data64 -
                    previous_telemetry_item.value.data64;
                pm_telemetry_value = static_cast<double>(data_delta) / time_delta_;
                SetTelemetryCapBit(telemetry_cap_bit);
            }
            else if (current_telemetry_item.type == CTL_DATA_TYPE_UINT64) {
              auto data_delta = current_telemetry_item.value.datau64 -
                                previous_telemetry_item.value.datau64;
              pm_telemetry_value =
                  static_cast<double>(data_delta) / time_delta_;
              SetTelemetryCapBit(telemetry_cap_bit);
              // stopgap measure for bad vram bandwidth telemetry coming out of V0 api
              if (telemetry_cap_bit == GpuTelemetryCapBits::gpu_mem_read_bandwidth && !useNewBandwidthTelemetry) {
                  if ((current_telemetry_item.value.datau64 < previous_telemetry_item.value.datau64) ||
                      ((current_telemetry_item.value.datau64 - previous_telemetry_item.value.datau64) > gpu_mem_max_bw_cache_value_bps_)) {
                      pm_telemetry_value = gpu_mem_read_bw_cache_value_bps_;
                  }
                  else {
                      gpu_mem_read_bw_cache_value_bps_ = pm_telemetry_value;
                  }
              }
            }
            else {
                // Expecting a double return type here
                return CTL_RESULT_ERROR_INVALID_ARGUMENT;
            }
        }
        return CTL_RESULT_SUCCESS;
    }

    template<class T>
    ctl_result_t IntelPowerTelemetryAdapter::SaveTelemetry(
        const T& currentSample,
        const ctl_mem_bandwidth_t& currentMemBandwidthSample)
    {
        if (currentSample.timeStamp.type == CTL_DATA_TYPE_DOUBLE) {
            previousSampleVariant = currentSample;
        }
        else {
            return CTL_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return CTL_RESULT_SUCCESS;
    }

    void IntelPowerTelemetryAdapter::SavePmPowerTelemetryData(PresentMonPowerTelemetryInfo& info)
    {
        pmlog_verb(v::tele_gpu)("Saving gathered telemetry info to history").pmwatch(GetName()).pmwatch(ref::DumpStatic(info));
        std::lock_guard<std::mutex> lock(historyMutex);
        history.Push(info);
    }

    }