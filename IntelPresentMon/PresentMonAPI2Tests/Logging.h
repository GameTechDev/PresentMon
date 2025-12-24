#pragma once
#include <string>

namespace pmon::test
{
	void SetupTestLogging(const std::string& logFolder, const std::string& logLevel) noexcept;

	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
	};
}
