#pragma once
#include "IntrospectionMetadata.h"
#include <array>
#include "../../ControlLib/PresentMonPowerTelemetry.h"
#include "../../ControlLib/CpuTelemetryInfo.h"



namespace pmon::ipc::intro
{
	// mapping of caps to metrics
	template<PM_METRIC metric> struct IntrospectionCapsLookup { using Universal = std::true_type; };
	// GPU caps
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_SUSTAINED_POWER_LIMIT> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_sustained_power_limit; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VOLTAGE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_voltage; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_TEMPERATURE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_EFFECTIVE_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_effective_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_render_compute_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEDIA_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_media_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_VOLTAGE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_voltage; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_effective_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_TEMPERATURE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VOLTAGE_REGULATOR_TEMPERATURE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_voltage_regulator_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_EFFECTIVE_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_effective_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_OVERVOLTAGE_PERCENT> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_overvoltage_percent; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_TEMPERATURE_PERCENT> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_temperature_percent; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER_PERCENT> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power_percent; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_CARD_POWER> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_card_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FAN_SPEED> {
		static constexpr auto gpuCapBitArray = std::array{ GpuTelemetryCapBits::fan_speed_0, GpuTelemetryCapBits::fan_speed_1,
			GpuTelemetryCapBits::fan_speed_2, GpuTelemetryCapBits::fan_speed_3, GpuTelemetryCapBits::fan_speed_4, };
	};
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_FAN_SPEED_PERCENT> {
		static constexpr auto gpuCapBitArray = std::array{ GpuTelemetryCapBits::max_fan_speed_0, GpuTelemetryCapBits::max_fan_speed_1,
			GpuTelemetryCapBits::max_fan_speed_2, GpuTelemetryCapBits::max_fan_speed_3, GpuTelemetryCapBits::max_fan_speed_4, };
	};
	//template<> struct IntrospectionCapsLookup<PM_METRIC_PSU_METRIC_NAME_HERE> { static constexpr auto gpuCapBitArray = std::array{
	//	GpuTelemetryCapBits::psu_info_0, GpuTelemetryCapBits::psu_info_1, GpuTelemetryCapBits::psu_info_2, GpuTelemetryCapBits::psu_info_3, GpuTelemetryCapBits::psu_info_4, }; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_SIZE> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_size; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_USED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_used; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_UTILIZATION> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_used; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_MAX_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_max_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_WRITE_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_write_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_READ_BANDWIDTH> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_mem_read_bandwidth; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_POWER_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_power_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_TEMPERATURE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_temperature_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_CURRENT_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_current_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VOLTAGE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_voltage_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_UTILIZATION_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::gpu_utilization_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_POWER_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_power_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_temperature_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_CURRENT_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_current_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_VOLTAGE_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_voltage_limited; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_MEM_UTILIZATION_LIMITED> { static constexpr auto gpuCapBit = GpuTelemetryCapBits::vram_utilization_limited; };
	// static GPU
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_NAME> { using GpuDeviceStatic = std::true_type; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_GPU_VENDOR> { using GpuDeviceStatic = std::true_type; };
	// CPU caps
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_UTILIZATION> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_utilization; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_POWER> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_power; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_TEMPERATURE> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_temperature; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_FREQUENCY> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_frequency; };
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_CORE_UTILITY> { using ManualDisable = std::true_type; };
	// static CPU
	template<> struct IntrospectionCapsLookup<PM_METRIC_CPU_POWER_LIMIT> { static constexpr auto cpuCapBit = CpuTelemetryCapBits::cpu_power_limit; };


	// concepts to help determine device-metric mapping type
	template<class T> concept IsUniversalMetric = requires { typename T::Universal; };
	template<class T> concept IsGpuDeviceMetric = requires { T::gpuCapBit; };
	template<class T> concept IsGpuDeviceMetricArray = requires { T::gpuCapBitArray; };
	template<class T> concept IsGpuDeviceStaticMetric = requires { typename T::GpuDeviceStatic; };
	template<class T> concept IsCpuMetric = requires { T::cpuCapBit; };
	template<class T> concept IsManualDisableMetric = requires { typename T::ManualDisable; };

	// TODO: compile-time verify that all cap bits are covered (how?)
}