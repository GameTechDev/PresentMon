// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#define NOMINMAX
#include <Windows.h>
#include "NvidiaPowerTelemetryAdapter.h"
#include "Logging.h"
#include <optional>


namespace pwr::nv
{
    NvidiaPowerTelemetryAdapter::NvidiaPowerTelemetryAdapter(
        const NvapiWrapper* pNvapi,
        const NvmlWrapper* pNvml,
        NvPhysicalGpuHandle hGpuNvapi,
        std::optional<nvmlDevice_t> hGpuNvml)
        :
        nvapi{ pNvapi },
        nvml{ pNvml },
        hNvapi{ hGpuNvapi },
        hNvml{ hGpuNvml }
    {
        char adapterName[NVAPI_SHORT_STRING_MAX];
        if (nvapi->Ok(nvapi->GPU_GetFullName(hGpuNvapi, adapterName)))
        {
            name = adapterName;
        }
    }



    const PresentMonPowerTelemetryInfo& NvidiaPowerTelemetryAdapter::GetNewest() const noexcept
    {
        return *std::prev(history.end());
    }

    uint64_t NvidiaPowerTelemetryAdapter::GetDedicatedVideoMemory() const noexcept {
        uint64_t video_mem_size = 0;
        nvmlMemory_t mem{};
        if (nvml->Ok(nvml->DeviceGetMemoryInfo(*hNvml, &mem))) {
            // free and total might be swapped in nvml; accessing .free to get
            // total
            video_mem_size = uint64_t(mem.free);
        }
        return video_mem_size;
    }

    double NvidiaPowerTelemetryAdapter::GetSustainedPowerLimit() const noexcept
    {
        unsigned int limitMw = 0;
        if (nvml->Ok(nvml->DeviceGetPowerManagementLimit(*hNvml, &limitMw)))
        {
            return double(limitMw) / 1000.;
        }
        return 0.f;
    }

    bool NvidiaPowerTelemetryAdapter::Sample() noexcept
    {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);

        PresentMonPowerTelemetryInfo info{
            .qpc = (uint64_t)qpc.QuadPart,
        };
        
        // nvapi-sourced telemetry
        {// gpu and vram temperatures
            NV_GPU_THERMAL_SETTINGS thermals = {
                .version = NV_GPU_THERMAL_SETTINGS_VER_2
            };
            if (nvapi->Ok(nvapi->GPU_GetThermalSettings(hNvapi, NVAPI_THERMAL_TARGET_ALL, &thermals)))
            {
                // loop through all sensors, read those of interest into output struct
                for (const auto& sensor : thermals.sensor)
                {
                    // TODO: consider prioritizing sensor.controller == NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL when multiple GPU thermals are present
                    if ((sensor.target == NVAPI_THERMAL_TARGET_GPU) &&
                        (sensor.controller == NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL)) {
                        info.gpu_temperature_c = (double)sensor.currentTemp;
                        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
                    }
                    else if (sensor.target == NVAPI_THERMAL_TARGET_MEMORY)
                    {
                        info.vram_temperature_c = (double)sensor.currentTemp;
                        SetTelemetryCapBit(GpuTelemetryCapBits::vram_temperature);
                    }
                }
            }
            // TODO: consider logging failure (lower logging level perhaps)
        }

        {// gpu and vram clock frequencies
            NV_GPU_CLOCK_FREQUENCIES freqs{
                .version = NV_GPU_CLOCK_FREQUENCIES_VER_3
            };
            if (nvapi->Ok(nvapi->GPU_GetAllClockFrequencies(hNvapi, &freqs)))
            {
                if (const auto& gpuDomain = freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS];
                    gpuDomain.bIsPresent)
                {
                    info.gpu_frequency_mhz = double(gpuDomain.frequency) / 1000.;
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_frequency);
                }
                if (const auto& vramDomain = freqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY];
                    vramDomain.bIsPresent)
                {
                    info.vram_frequency_mhz = double(vramDomain.frequency) / 1000.;
                    SetTelemetryCapBit(GpuTelemetryCapBits::vram_frequency);
                }
            }
            // TODO: consider logging failure (lower logging level perhaps)
        }

        {// fan speed
            NvU32 tach = 0;
            if (nvapi->Ok(nvapi->GPU_GetTachReading(hNvapi, &tach)))
            {
                info.fan_speed_rpm[0] = double(tach);
                SetTelemetryCapBit(GpuTelemetryCapBits::fan_speed_0);
            }
            // TODO: consider logging failure (lower logging level perhaps)
        }

        {// gpu utilization
            NV_GPU_DYNAMIC_PSTATES_INFO_EX pstates{
                .version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER
            };
            if (nvapi->Ok(nvapi->GPU_GetDynamicPstatesInfoEx(hNvapi, &pstates)))
            {
                if (const auto& domain = pstates.utilization[NVAPI_GPU_UTILIZATION_DOMAIN_GPU];
                    domain.bIsPresent)
                {
                    info.gpu_utilization = double(domain.percentage);
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_utilization);
                }
                if (const auto& domain = pstates.utilization[NVAPI_GPU_UTILIZATION_DOMAIN_VID];
                    domain.bIsPresent)
                {
                    info.gpu_media_utilization = double(domain.percentage);
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_media_utilization);
                }
            }
            // TODO: consider logging failure (lower logging level perhaps)
        }


        // nvml-sourced telemetry (if we have a valid nvml handle)
        if (*hNvml)
        {
            {// gpu power
                unsigned int powerMw = 0;
                if (nvml->Ok(nvml->DeviceGetPowerUsage(*hNvml, &powerMw)))
                {
                    info.gpu_power_w = double(powerMw) / 1000.;
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_power);
                }
                // TODO: consider logging failure (lower logging level perhaps)
            }

            {// power limit
                unsigned int limitMw = 0;
                if (nvml->Ok(nvml->DeviceGetPowerManagementLimit(*hNvml, &limitMw)))
                {
                    info.gpu_sustained_power_limit_w = double(limitMw) / 1000.;
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_sustained_power_limit);
                }
                // TODO: consider logging failure (lower logging level perhaps)
            }

            {// memory usage
                nvmlMemory_t mem{};
                if (nvml->Ok(nvml->DeviceGetMemoryInfo(*hNvml, &mem)))
                {
                    // free and total might be swapped in nvml; accessing .free to get total
                    info.gpu_mem_total_size_b = uint64_t(mem.free);
                    info.gpu_mem_used_b = uint64_t(mem.used);
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_size);
                    SetTelemetryCapBit(GpuTelemetryCapBits::gpu_mem_used);
                }
                // TODO: consider logging failure (lower logging level perhaps)
            }

            {// temperature
                if (!GetPowerTelemetryCapBits().test(static_cast<size_t>(GpuTelemetryCapBits::gpu_temperature))||
                    (useNvmlTemperature))
                {
                    unsigned int temp = 0;
                    if (nvml->Ok(nvml->DeviceGetTemperature(*hNvml, nvmlTemperatureSensors_t::NVML_TEMPERATURE_GPU, &temp)))
                    {
                        info.gpu_temperature_c = (double)temp;
                        SetTelemetryCapBit(GpuTelemetryCapBits::gpu_temperature);
                        useNvmlTemperature = true;
                    }
                    // TODO: consider logging failure (lower logging level perhaps)
                }
            }
        }

        // insert telemetry into history
        std::lock_guard lock{ historyMutex };
        history.Push(info);

        return true;
    }

    std::optional<PresentMonPowerTelemetryInfo> NvidiaPowerTelemetryAdapter::GetClosest(uint64_t qpc) const noexcept
    {
        std::lock_guard lock{ historyMutex };
        return history.GetNearest(qpc);
    }

    PM_DEVICE_VENDOR NvidiaPowerTelemetryAdapter::GetVendor() const noexcept
    {
        return PM_DEVICE_VENDOR::PM_DEVICE_VENDOR_NVIDIA;
    }

    std::string NvidiaPowerTelemetryAdapter::GetName() const noexcept
    {
        return name;
    }
}
