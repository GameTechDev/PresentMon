// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "PowerTelemetryAdapter.h"
#include <optional>
#include "NvapiWrapper.h"
#include "NvmlWrapper.h"

namespace pwr::nv
{
	class NvidiaPowerTelemetryAdapter : public PowerTelemetryAdapter
	{
	public:
		NvidiaPowerTelemetryAdapter(
            uint32_t deviceId,
			const NvapiWrapper* pNvapi,
			const NvmlWrapper* pNvml,
			NvPhysicalGpuHandle hGpuNvapi,
			std::optional<nvmlDevice_t> hGpuNvml);
		PresentMonPowerTelemetryInfo Sample() noexcept override;
		PM_DEVICE_VENDOR GetVendor() const noexcept override;
		std::string GetName() const noexcept override;
        uint64_t GetDedicatedVideoMemory() const noexcept override;
		uint64_t GetVideoMemoryMaxBandwidth() const noexcept override { return 0; }
		double GetSustainedPowerLimit() const noexcept override;

	private:
		// data
		const NvapiWrapper* nvapi;
		const NvmlWrapper* nvml;
		NvPhysicalGpuHandle hNvapi;
		std::optional<nvmlDevice_t> hNvml;
		std::string name = "Unknown Adapter Name";
		bool useNvmlTemperature = false;
	};
}
