// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <string>

namespace p2c::kern
{
	class KernelHandler
	{
	public:
		virtual ~KernelHandler() = default;
		virtual void OnTargetLost(uint32_t pid) = 0;
		virtual void OnOverlayDied() = 0;
		virtual void OnPresentmonInitFailed() = 0;
		virtual void OnStalePidSelected() = 0;
	};
}