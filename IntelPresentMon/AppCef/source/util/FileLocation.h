// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace p2c::client::util
{
	enum class FileLocation : uint32_t
	{
		Install,
		Data,
		Documents,
		Count_,
	};
}