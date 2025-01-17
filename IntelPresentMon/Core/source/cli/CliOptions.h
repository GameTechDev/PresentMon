#pragma once
#include <CommonUtilities/cli/CliFramework.h>
#include <CommonUtilities/log/Level.h>
#include <PresentMonService/GlobalIdentifiers.h>
#include <format>

namespace p2c::cli
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
	private:
		CLI::CheckedTransformer logLevelTf_{ log::GetLevelMapNarrow(), CLI::ignore_case };

	private: Group gs_{ this, "Standard", "Useful to end users in production"}; public:
		Flag allowTearing{ this, "--p2c-allow-tearing", "Allow tearing presents for overlay (optional, might affect VRR)" };
		Flag disableAlpha{ this, "--p2c-disable-alpha", "Disable alpha blend composition of overlay" };
		Flag enableTimestampColumn{ this, "--p2c-enable-timestamp-column", "Enable timestamp column in capture CSV" };

	private: Group gd_{ this, "Debugging", "Aids in debugging this tool" }; public:
		Option<std::string> url{ this, "--p2c-url", "", "URL to load instead of app files" };
		Option<std::string> controlPipe{ this, "--p2c-control-pipe", R"(\\.\pipe\pm-ctrl)", "Named pipe to connect to the service with" };
		Option<std::string> shmName{ this, "--p2c-shm-name", "pm-intro-shm", "Shared memory to connect to the service with" };
		Option<std::string> etwSessionName{ this, "--p2c-etw-session-name", "pm-child-etw-session", "ETW session name when lauching service as child" };
		Flag svcAsChild{ this, "--p2c-svc-as-child", "Launch service as child console app" };
		Flag noNetFail{ this, "--p2c-no-net-fail", "Disable error modal for bad url accesses" };
		Flag debugWaitRender{ this, "--p2c-debug-wait-render", "Force all render child processes to wait for debugger connection" };
		Flag debugWaitClient{ this, "--p2c-debug-wait-client", "Force main client process to wait for debugger connection" };
		Flag filesWorking{ this, "--p2c-files-working", "Use the working directory for file storage" };
		Flag traceExceptions{ this, "--p2c-trace-exceptions", "Add stack trace to all thrown exceptions (including SEH exceptions)" };
		Flag enableDiagnostic{ this, "--p2c-enable-diagnostic", "Enable debug diagnostic layer forwarding (duplicates exiisting log entries)" };
		Flag enableUiDevOptions{ this, "--p2c-enable-ui-dev-options", "Enable advanced UI elements useful during development" };


	private: Group gl_{ this, "Logging", "Customize logging for this tool"}; public:
		Option<log::Level> logLevel{ this, "--p2c-log-level", log::Level::Error, "Severity to log at", logLevelTf_ };
		Option<log::Level> logTraceLevel{ this, "--p2c-log-trace-level", log::Level::Error, "Severity to print stacktrace at", logLevelTf_ };
		Option<std::string> logDenyList{ this, "--p2c-log-deny-list", "", "Path to log deny list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logAllowList{ this, "--p2c-log-allow-list", "", "Path to log allow list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logFolder{ this, "--p2c-log-folder", "", "Path to directory in which to store log files", CLI::ExistingDirectory };
		Option<std::string> logSvcPipe{ this, "--p2c-log-svc-pipe", ::pmon::gid::defaultLogPipeBaseName, "Base name of pipe to use when connecting to service IPC log" };
		Flag logSvcPipeEnable{ this, "--p2c-log-svc-pipe-enable", "Enable pipe connection to service IPC log stream" };

	private: Group gi_{ this, "Internal", "Internal options, do not supply manually"}; public:
		Option<std::string> cefType{ this, "--type", "", "Type of the current chromium process" };
		Option<std::string> logPipeName{ this, "--p2c-log-pipe-name", "", "The postfix used to create the named pipe for logging source server" };

		static constexpr const char* description = "PresentMon performance overlay and trace capture application";
		static constexpr const char* name = "PresentMon.exe";

	private:
		MutualExclusion excl_{ logDenyList, logAllowList };
		Dependency incl_{ etwSessionName, svcAsChild };
		NoForward noForward_{ cefType, logPipeName };
		AllowExtras ext_{ this };
	};
}