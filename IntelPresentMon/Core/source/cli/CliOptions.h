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
		CLI::CheckedTransformer logVmodTf_{ log::GetVerboseModuleMapNarrow(), CLI::ignore_case };

	private: Group gs_{ this, "Standard", "Useful to end users in production"}; public:
		Flag allowTearing{ this, "--allow-tearing", "Allow tearing presents for overlay (optional, might affect VRR)" };
		Flag disableAlpha{ this, "--disable-alpha", "Disable alpha blend composition of overlay" };
		Flag enableTimestampColumn{ this, "--enable-timestamp-column", "Enable timestamp column in capture CSV" };

	private: Group gd_{ this, "Debugging", "Aids in debugging this tool" }; public:
		Option<std::string> controlPipe{ this, "--control-pipe", R"(\\.\pipe\pm-ctrl)", "Named pipe to connect to the service with" };
		Option<std::string> shmName{ this, "--shm-name", "pm-intro-shm", "Shared memory to connect to the service with" };
		Option<std::string> etwSessionName{ this, "--etw-session-name", "pm-child-etw-session", "ETW session name when lauching service as child" };
		Flag svcAsChild{ this, "--svc-as-child", "Launch service as child console app" };
		Flag traceExceptions{ this, "--trace-exceptions", "Add stack trace to all thrown exceptions (including SEH exceptions)" };
		Flag enableDiagnostic{ this, "--enable-diagnostic", "Enable debug diagnostic layer forwarding (duplicates exiisting log entries)" };
		Flag filesWorking{ this, "--files-working", "Use the working directory for file storage" };
		Flag waitForDebugger{ this, "--wait-for-debugger", "On entry wait for debugger to be attached, then break" };

	private: Group gl_{ this, "Logging", "Customize logging for this tool"}; public:
		Option<log::Level> logLevel{ this, "--log-level", log::Level::Error, "Severity to log at", logLevelTf_ };
		Option<log::Level> logTraceLevel{ this, "--log-trace-level", log::Level::Error, "Severity to print stacktrace at", logLevelTf_ };
		Option<std::string> logDenyList{ this, "--log-deny-list", "", "Path to log deny list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logAllowList{ this, "--log-allow-list", "", "Path to log allow list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logFolder{ this, "--log-folder", "", "Path to directory in which to store log files", CLI::ExistingDirectory };
		Option<std::string> logSvcPipe{ this, "--log-svc-pipe", ::pmon::gid::defaultLogPipeBaseName, "Base name of pipe to use when connecting to service IPC log" };
		Flag logSvcPipeEnable{ this, "--log-svc-pipe-enable", "Enable pipe connection to service IPC log stream" };
		Flag logMiddlewareCopy{ this, "--log-middleware-copy", "Copy log entries from middleware channel to this client" };
		Option<std::vector<log::V>> logVerboseModules{ this, "--log-verbose-modules", {}, "Verbose logging modules to enable", logVmodTf_ };

	private: Group gu_{ this, "CEF UI", "Options to pass thru to the CEF UI system" }; public:
		Option<std::vector<std::pair<std::string, std::string>>> uiOptions{ this, "--ui-option", {}, "Parameterized options to pass to UI process (omit --p2c- prefix)" };
		Option<std::vector<std::string>> uiFlags{ this, "--ui-flag", {}, "Parameterized options to pass to UI process (omit --p2c- prefix)" };

	private: Group gi_{ this, "Internal", "Internal options, do not supply manually"}; public:
		Option<std::string> middlewareDllPath{ this, "--middleware-dll-path", "", "Override middleware DLL path discovery with custom path" };

	Subcommand subcCapture{ this, "capture", "Perform headless capture of frame event data to .csv" }; public:
	private: Group gcaps_{ this, "Standard", "Standard options for the capture subcommand" }; public:
		Option<uint32_t> capTargetPid{ this, "--target-pid", {}, "PID of the process to track" };
		Option<std::string> capTargetName{ this, "--target-name", {}, "Main module name of the process to track" };
		Option<double> capDuration{ this, "--duration", 10., "How long to capture for in seconds" };
		Option<uint32_t> capTelemetryPeriod{ this, "--telemetry-period", 100, "Time between GPU/CPU telemetry samples in ms" };
		Option<std::string> capOutput{ this, "--output", {}, "Name of the output CSV file, optionally with absolute or relative path" };
		Option<std::vector<std::string>> capMetrics{ this, "--metrics", {}, "List of metrics to capture as columns in the output CSV file" };

	Subcommand subcList{ this, "list", "List entities for use with PresentMon SDK/headless CLI" }; public:
	private: Group glists_{ this, "Standard", "Standard options for the list subcommand" }; public:
		Flag listMetrics{ this, "--metrics,-m", "Output a list of available metrics" };
		Flag listMetricsStats{ this, "--stats,-s", "Output a list of available stats for each metric" };
		Flag listDevices{ this, "--devices,-d", "Output a list of available graphics adapters" };
		Flag listFilterFrame{ this, "--filter-frame,-f", "Filter to only metrics available for use with frame event capture" };
		Flag listFilterDynamic{ this, "--filter-dynamic,-y", "Filter to only metrics available for use with dynamic polling" };
		Option<std::string> listSearch{ this, "--search", {}, "Substring to filter metric results on (case-insensitive)" };
	

		static constexpr const char* description = "PresentMon performance overlay and trace capture application";
		static constexpr const char* name = "PresentMon.exe";

	private:
		MutualExclusion exclCapTgt_{ capTargetPid, capTargetName };
		MutualExclusion exclListFilter_{ listFilterFrame, listFilterDynamic };
		MutualExclusion exclLogList_{ logDenyList, logAllowList };
		Dependency inclSvcSes_{ etwSessionName, svcAsChild };
		Dependency inclListFrame_{ listFilterFrame, listMetrics };
		Dependency inclListDyna_{ listFilterDynamic, listMetrics };
		Dependency inclListSearch_{ listSearch, listMetrics };
		Dependency inclListMetricsStats_{ listMetricsStats, listMetrics };
	};
}