// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Env.h"
#include "Exception.h"
#include <cstdlib>


namespace pmon::util
{
	std::string GetEnv(const std::string& name, bool shouldThrow)
	{
		size_t valueSize = 0;
		if (getenv_s(&valueSize, nullptr, 0, name.c_str()) != 0) {
			if (shouldThrow) {
				throw Except<Exception>("Failed reading environment variable: " + name);
			}
			return {};
		}

		if (valueSize == 0) {
			if (shouldThrow) {
				throw Except<Exception>("Environment variable not found: " + name);
			}
			return {};
		}

		std::string value(valueSize, '\0');
		if (getenv_s(&valueSize, value.data(), value.size(), name.c_str()) != 0 || valueSize == 0) {
			if (shouldThrow) {
				throw Except<Exception>("Failed reading environment variable: " + name);
			}
			return {};
		}

		value.resize(valueSize - 1);
		return value;
	}
}
