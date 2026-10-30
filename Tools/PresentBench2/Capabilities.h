// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pb2
{
	struct FeatureCapability
	{
		bool compiled = false;
		bool runtime = false;
		std::string reason;
	};

	struct XessFgAdapterCapability
	{
		std::string name;
		bool graphics = false;
		bool integrated = false;
		bool selected = false;
		bool igclFrameGeneration = false;
		bool igclFrameGenerationControl = false;
		uint32_t pciVendorId = 0;
		uint32_t pciDeviceId = 0;
	};

	struct XessFgCapability
	{
		FeatureCapability feature;
		std::vector<XessFgAdapterCapability> adapters;
		std::string igclReason;
	};

	struct CapabilityReport
	{
		XessFgCapability xessFg;
	};

	CapabilityReport ProbeCapabilities(uint32_t adapterIndex);
	std::string CapabilitiesToJson(const CapabilityReport& report);
}
