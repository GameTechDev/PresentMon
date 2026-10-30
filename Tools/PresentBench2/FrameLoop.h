// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "Backend.h"
#include "Config.h"
#include "RunStatus.h"
#include "Window.h"
#include "markers/FrameMarkers.h"

namespace pb2
{
	struct FrameLoopResult
	{
		uint32_t appFrames = 0;
		uint32_t presents = 0;
		bool ok = true;
		std::string error;
	};

	FrameLoopResult RunFrameLoop(const BenchConfig& config, IFrameLoopWindow& window,
		IRenderBackend& backend, MarkerHub& markers, RunStatusBoard& board);
}
