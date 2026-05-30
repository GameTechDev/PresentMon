// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>

namespace pmon::util
{
	std::string GetEnv(const std::string& name, bool shouldThrow = true);
}
