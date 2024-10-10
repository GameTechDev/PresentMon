#pragma once
#include "../CommonUtilities/cli/CliFramework.h"
#include "../CommonUtilities/log/Level.h"
#include "GlobalIdentifiers.h"

namespace clio
{
	using namespace pmon::util::cli;
	using namespace pmon::util::log;
	struct Options : public OptionsBase<Options>
	{
	private:
		CLI::CheckedTransformer logLevelTf_{ GetLevelMapNarrow(), CLI::ignore_case };

	private: Group gc_{ this, "Connection", "Control client connection" }; public:
		Option<std::string> etwSessionName{ this, "--etw-session-name", "", "Name to use when creating the ETW session" };
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> nsmPrefix{ this, "--nsm-prefix", "", "Prefix to use when naming named shared memory segments created for frame data circular buffers" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };

	private: Group gd_{ this, "Debugging", "Aids in debugging this tool" }; public:
		Flag debug{ this, "--debug,-d", "Stall service by running in a loop after startup waiting for debugger to connect" };
		Option<long long> timedStop{ this, "--timed-stop", -1, "Signal stop event after specified number of milliseconds" };
		Option<std::string> etlTestFile{ this, "--etl-test-file", "", "Etl test file including necessary path" };

	private: Group gl_{ this, "Logging", "Control logging behavior" }; public:
		Option<std::string> logDir{ this, "--log-dir", "", "Enable logging to a file in the specified directory" };
		Option<std::string> logPipeName{ this, "--log-pipe-name", pmon::gid::defaultLogPipeBaseName, "Name of the pipe to connect to for log IPC" };
		Flag enableStdioLog{ this, "--enable-stdio-log", "Disable logging to stderr" };
		Flag enableDebuggerLog{ this, "--enable-debugger-log", "Disable logging to system debugger" };
		Flag disableIpcLog{ this, "--disable-ipc-log", "Disable logging to named pipe connection" };
		Option<Level> logLevel{ this, "--log-level", Level::Error, "Severity to log at", logLevelTf_ };


		static constexpr const char* description = "Intel PresentMon service for frame and system performance measurement";
		static constexpr const char* name = "PresentMonService.exe";
	};
}