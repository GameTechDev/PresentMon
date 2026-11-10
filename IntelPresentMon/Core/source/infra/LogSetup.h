#pragma once
#include <CommonUtilities/log/GlobalPolicy.h>
#include <CommonUtilities/log/Level.h>
#include <cstdint>
#include <filesystem>
#include <string>

namespace p2c
{
	struct DiagnosticLoggingPaths
	{
		std::filesystem::path client;
		std::filesystem::path middleware;
	};

	struct DiagnosticLoggingConfiguration
	{
		::pmon::util::log::Level logLevel =
			::pmon::util::log::GlobalPolicy::GetDefaultLogLevel();
		::pmon::util::log::Level traceLevel =
			::pmon::util::log::GlobalPolicy::GetDefaultTraceLevel();
		uint64_t verboseModules = 0;
		std::filesystem::path folder;
		bool exceptionTrace = false;
	};

	// LogChannelManager should be instantiated as early as possible in the entry point of the process/module
	// It acts as a guard to prevent stack trace resolution in the channel worker thread after exiting main
	struct LogChannelManager
	{
		LogChannelManager() noexcept;
		~LogChannelManager();
	};
	// call after command line arguments have been parsed
	void ConfigureLogging() noexcept;
	DiagnosticLoggingPaths ConfigureDiagnosticLogging(const DiagnosticLoggingConfiguration& config);
	void FlushDiagnosticLogging();
	// add stdio logging for when running headless
	void ConfigureHeadlessLogging() noexcept;
	// call when ready to connect to a logging source (logging pipe server)
	void ConnectToLoggingSourcePipe(const std::string& pipePrefix);
}