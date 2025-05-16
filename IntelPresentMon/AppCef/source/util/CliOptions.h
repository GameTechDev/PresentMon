#pragma once
#include <CommonUtilities/cli/CliFramework.h>
#include <CommonUtilities/log/Level.h>

namespace p2c::client::util::cli
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
	private:
		CLI::CheckedTransformer logLevelTf_{ log::GetLevelMapNarrow(), CLI::ignore_case };

	private: Group gd_{ this, "Debugging", "Aids in debugging this tool" }; public:
		Option<std::string> url{ this, "--p2c-url", "", "URL to load instead of app files" };
		Flag noNetFail{ this, "--p2c-no-net-fail", "Disable error modal for bad url accesses" };
		Flag debugWaitRender{ this, "--p2c-debug-wait-render", "Force all render child processes to wait for debugger connection" };
		Flag debugWaitClient{ this, "--p2c-debug-wait-client", "Force main client process to wait for debugger connection" };
		Flag filesWorking{ this, "--p2c-files-working", "Use the working directory for file storage" };
		Flag traceExceptions{ this, "--p2c-trace-exceptions", "Add stack trace to all thrown exceptions (including SEH exceptions)" };
		Flag enableUiDevOptions{ this, "--p2c-enable-ui-dev-options", "Enable advanced UI elements useful during development" };
		Option<std::string> webRoot{ this, "--p2c-web-root", "", "Filesystem path to directory holding SPA assets" };
		Flag enableChromiumDebug{ this, "--p2c-enable-chromium-debug", "Enable Chromium devtools connections on port 9009" };


	private: Group gl_{ this, "Logging", "Customize logging for this tool" }; public:
		Option<log::Level> logLevel{ this, "--p2c-log-level", log::Level::Error, "Severity to log at", logLevelTf_ };
		Option<log::Level> logTraceLevel{ this, "--p2c-log-trace-level", log::Level::Error, "Severity to print stacktrace at", logLevelTf_ };
		Option<std::string> logDenyList{ this, "--p2c-log-deny-list", "", "Path to log deny list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logAllowList{ this, "--p2c-log-allow-list", "", "Path to log allow list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logFolder{ this, "--p2c-log-folder", "", "Path to directory in which to store log files", CLI::ExistingDirectory };

	private: Group gi_{ this, "Internal", "Internal options, do not supply manually" }; public:
		Option<std::string> cefType{ this, "--type", "", "Type of the current chromium process" };
		Option<std::string> logPipeName{ this, "--p2c-log-pipe-name", "", "The postfix used to create the named pipe for logging source server" };
		Option<std::string> actName{ this, "--p2c-act-name", "", "Base name for the named pipes connecting CEF and the overlay Kernel process" };

		static constexpr const char* description = "PresentMon control UI";
		static constexpr const char* name = "PresentMonUI.exe";

	private:
		MutualExclusion excl_{ logDenyList, logAllowList };
		NoForward noForward_{ cefType, logPipeName };
		AllowExtras ext_{ this };
	};
}