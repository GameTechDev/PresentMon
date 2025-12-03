#pragma once
#include "../CommonUtilities/cli/CliFramework.h"
#include "../CommonUtilities/log/Level.h"
#include "../CommonUtilities/log/Verbose.h"
#include "GlobalIdentifiers.h"

namespace clio
{
	using namespace pmon::util::cli;
	using namespace pmon::util::log;
	struct Options : public OptionsBase<Options>
	{
	private:
		CLI::CheckedTransformer logLevelTf_{ GetLevelMapNarrow(), CLI::ignore_case };
		CLI::CheckedTransformer logVmodTf_{ GetVerboseModuleMapNarrow(), CLI::ignore_case };

	private: Group gc_{ this, "Connection", "Control client connection" }; public:
		Option<std::string> etwSessionName{ this, "--etw-session-name", "PMService", "Name to use when creating the ETW session" };
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> shmNamePrefix{ this, "--shm-name-prefix", "", "Prefix to use when naming shared memory segments" };

	private: Group gd_{ this, "Debugging", "Aids in debugging this tool" }; public:
		Flag debug{ this, "--debug,-d", "Stall service by running in a loop after startup waiting for debugger to connect" };
		Option<long long> timedStop{ this, "--timed-stop", -1, "Signal stop event after specified number of milliseconds" };

	private: Group gr_{ this, "Playback", "Playback of recorded ETL files" }; public:
		Option<std::string> etlTestFile{ this, "--etl-test-file", "", "Etl test file including necessary path", CLI::ExistingFile };
		Flag pacePlayback{ this, "--pace-playback", "Process ETL events at similar cadence to realtime processing and adjust timestamps" };

	private: Group gl_{ this, "Logging", "Control logging behavior" }; public:
		Option<std::string> logDir{ this, "--log-dir", "", "Enable logging to a file in the specified directory" };
		Option<std::string> logPipeName{ this, "--log-pipe-name", pmon::gid::defaultLogPipeBaseName, "Name of the pipe to connect to for log IPC" };
		Flag enableStdioLog{ this, "--enable-stdio-log", "Enable logging to stderr" };
		Flag enableDebuggerLog{ this, "--enable-debugger-log", "Enable logging to system debugger" };
		Flag disableIpcLog{ this, "--disable-ipc-log", "Disable logging to named pipe connection" };
		Option<Level> logLevel{ this, "--log-level", Level::Error, "Severity to log at", logLevelTf_ };
		Flag logNamePid{ this, "--log-name-pid", "Append PID to log files instead of timestamp" };
		Option<std::vector<V>> logVerboseModules{ this, "--log-verbose-modules", {}, "Verbose logging modules to enable", logVmodTf_ };

	private: Group gt_{ this, "Testing", "Automated testing features" }; public:
		Flag enableTestControl{ this, "--enable-test-control", "Enable test control over stdio" };

		static constexpr const char* description = "Intel PresentMon service for frame and system performance measurement";
		static constexpr const char* name = "PresentMonService.exe";
	};
}