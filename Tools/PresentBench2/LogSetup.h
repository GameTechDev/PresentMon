// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>

namespace pb2log
{
	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
		LogChannelManager(const LogChannelManager&) = delete;
		LogChannelManager& operator=(const LogChannelManager&) = delete;
	};

	void ConfigureLogging(const std::string& logPath) noexcept;
}
