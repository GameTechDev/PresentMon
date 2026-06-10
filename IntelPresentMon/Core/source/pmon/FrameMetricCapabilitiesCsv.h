// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <PresentMonAPIWrapperCommon/Introspection.h>
#include <filesystem>
#include <string>

namespace p2c::pmon
{
	void WriteFrameMetricCapabilitiesCsv(const pmapi::intro::Root& introRoot, const std::filesystem::path& outputPath,
		const std::string& systemCpuName = {});
}
