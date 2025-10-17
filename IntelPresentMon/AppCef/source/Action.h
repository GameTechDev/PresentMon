// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace p2c::client
{
	enum class Action : uint32_t
	{
		BeginCapture,
		EndCapture,
		ToggleCapture,
		ShowOverlay,
		HideOverlay,
		ToggleOverlay,
		CyclePreset,
		ToggleEtlLogging,
		Count_,
	};

	struct ActionDescriptor
	{
		uint32_t code;
		std::string text;
	};

	inline std::vector<ActionDescriptor> EnumerateActions()
	{
		return {
			{ (uint32_t)Action::BeginCapture, "BeginCapture" },
			{ (uint32_t)Action::EndCapture, "EndCapture" },
			{ (uint32_t)Action::ToggleCapture, "ToggleCapture" },
			{ (uint32_t)Action::ShowOverlay, "ShowOverlay" },
			{ (uint32_t)Action::HideOverlay, "HideOverlay" },
			{ (uint32_t)Action::ToggleOverlay, "ToggleOverlay" },
			{ (uint32_t)Action::CyclePreset, "CyclePreset" },
			{ (uint32_t)Action::ToggleEtlLogging, "ToggleEtlLogging" },
		};
	}
}