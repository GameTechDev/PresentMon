#pragma once
#include <optional>
#include <string>

namespace pmon::test
{
	void SetupTestLogging(const std::string& logFolder,
		const std::string& logLevel,
		const std::optional<std::string>& logVerboseModules) noexcept;

	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
	};
}
