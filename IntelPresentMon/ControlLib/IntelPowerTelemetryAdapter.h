// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#define NOMINMAX
#include <Windows.h>
#include "igcl_api.h"
#include "PowerTelemetryAdapter.h"
#include "TelemetryHistory.h"
#include "ctlpvttemp_api.h"
#include <mutex>
#include <optional>
#include <variant>

namespace pwr::intel
{
	class IntelPowerTelemetryAdapter : public PowerTelemetryAdapter
	{
	public:
		IntelPowerTelemetryAdapter(ctl_device_adapter_handle_t handle);
		bool Sample() noexcept override;
		std::optional<PresentMonPowerTelemetryInfo> GetClosest(uint64_t qpc) const noexcept override;
		const PresentMonPowerTelemetryInfo& GetNewest() const noexcept override;
		PM_DEVICE_VENDOR GetVendor() const noexcept override;
		std::string GetName() const noexcept override;
        uint64_t GetDedicatedVideoMemory() const noexcept override;
		uint64_t GetVideoMemoryMaxBandwidth() const noexcept override;
		double GetSustainedPowerLimit() const noexcept override;

		// types
		class NonGraphicsDeviceException : public std::exception {};

	private:
		// types
		using SampleVariantType = std::variant<std::monostate, ctl_power_telemetry_t, ctl_power_telemetry2_t>;
		// functions
		template<class T>
		bool GatherSampleData(T& currentSample,
			ctl_mem_state_t& memory_state,
			ctl_mem_bandwidth_t& memory_bandwidth,
			std::optional<double> gpu_sustained_power_limit_mw,
			uint64_t qpc);

		ctl_result_t EnumerateMemoryModules();

		template<class T>
		ctl_result_t GetTimeDelta(const T& power_telemetry);

		// TODO: meld these into the sample function
		template<class T>
		ctl_result_t GetGPUPowerTelemetryData(
			const T& power_telemetry,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);
		template<class T>
		ctl_result_t GetVramPowerTelemetryData(
			const T& power_telemetry,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);
		template<class T>
		ctl_result_t GetFanPowerTelemetryData(
			const T& power_telemetry,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);
		template<class T>
		ctl_result_t GetPsuPowerTelemetryData(
			const T& power_telemetry,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);

		void GetMemStateTelemetryData(
			const ctl_mem_state_t& mem_state,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);
		void GetMemBandwidthData(
			const ctl_mem_bandwidth_t& mem_bandwidth,
			PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);

		void SavePmPowerTelemetryData(PresentMonPowerTelemetryInfo& pm_gpu_power_telemetry_info);
		template<class T>
        ctl_result_t SaveTelemetry(
            const T& power_telemetry,
            const ctl_mem_bandwidth_t& mem_bandwidth);

		// TODO: put these as part of the telemetry data object
		ctl_result_t GetInstantaneousPowerTelemetryItem(
			const ctl_oc_telemetry_item_t& telemetry_item,
			double& pm_telemetry_value,
			GpuTelemetryCapBits telemetry_cap_bit);
		ctl_result_t GetPowerTelemetryItemUsagePercent(
			const ctl_oc_telemetry_item_t& current_telemetry_item,
			const ctl_oc_telemetry_item_t& previous_telemetry_item,
			double& pm_telemetry_value,
			GpuTelemetryCapBits telemetry_cap_bit);
		ctl_result_t GetPowerTelemetryItemUsage(
			const ctl_oc_telemetry_item_t& current_telemetry_item,
			const ctl_oc_telemetry_item_t& previous_telemetry_item,
			double& pm_telemetry_value,
			GpuTelemetryCapBits telemetry_cap_bit);
		// data
		ctl_device_adapter_handle_t deviceHandle = nullptr;
		LUID deviceId; // pointed to by a device_adapter_properties member, written to by igcl api
		ctl_device_adapter_properties_t properties{};
		std::vector<ctl_mem_handle_t> memoryModules;
		std::vector<ctl_pwr_handle_t> powerDomains;
		mutable std::mutex historyMutex;
		TelemetryHistory<PresentMonPowerTelemetryInfo> history{ PowerTelemetryAdapter::defaultHistorySize };
		SampleVariantType previousSampleVariant;
		bool useV1PowerTelemetry = true;
		bool useNewBandwidthTelemetry = true;
		double time_delta_ = 0.f;
		// in V0 api readbandwidth occasionally returns what appears to be an invalid counter value
		// this is a stopgap to cover for cases where IGCL is reporting bad data in V0 bandwidth telemetry
		double gpu_mem_read_bw_cache_value_bps_ = 0.;
		uint64_t gpu_mem_max_bw_cache_value_bps_ = 0;
		// in V1 api vramEnergyCounter rolls over after hitting 1000.0 causing the current sample to be
		// less than the previous sample. Working with IGCL to determine the correct behavior for roll
		// over occasions
		double gpu_mem_power_cache_value_w_ = 0;
		// we have special handling for GPU current perf limitation on Alchemist
		// workaround for lack of discoverablity of perf limitation availability
		bool isAlchemist = false;
		// populated on init, used to calculate fan %
		std::vector<int32_t> maxFanSpeedsRpm_;
	};
}