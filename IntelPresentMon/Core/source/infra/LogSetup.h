#pragma once
#include <string>

namespace p2c
{
	// LogChannelManager should be instantiated as early as possible in the entry point of the process/module
	// It acts as a guard to prevent stack trace resolution in the channel worker thread after exiting main
	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
	};
	// call after command line arguments have been parsed
	void ConfigureLogging() noexcept;
	// add stdio logging for when running headless
	void ConfigureHeadlessLogging() noexcept;
	// call when ready to connect to a logging source (logging pipe server)
	void ConnectToLoggingSourcePipe(const std::string& pipePrefix);
}