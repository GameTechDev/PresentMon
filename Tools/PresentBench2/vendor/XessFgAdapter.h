// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../Dx12PresentComponent.h"
#include <cstdint>
#include <memory>

namespace pb2
{
	std::unique_ptr<IDx12PresentComponent> CreateXessFgPresentComponent(
		uint32_t interpolatedFrames);
}
