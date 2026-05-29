#pragma once

namespace logsetup
{
	// LogChannelManager should be instantiated as early as possible in the entry point of the process/module
	// It acts as a guard to prevent stack trace resolution in the channel worker thread after exiting main
	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
		LogChannelManager(const LogChannelManager& other) = delete;
		LogChannelManager(LogChannelManager&& other) = delete;
		LogChannelManager& operator=(const LogChannelManager& other) = delete;
	};
	// call after command line arguments have been parsed
	void ConfigureLogging(bool asApp) noexcept;
	void SetPeriodicLogFlushingEnabled(bool enabled) noexcept;
}
