#pragma once
#include "../CommonUtilities/cli/CliFramework.h"
#include "../CommonUtilities/log/Level.h"
#include "../CommonUtilities/ref/StaticReflection.h"
#include <format>

namespace clio
{
	enum class Mode
	{
		Introspection,
		DynamicQuery,
		FrameQuery,
		CsvFrameQuery,
		CheckMetric,
		MetricList,
		WrapperStaticQuery,
		AddGpuMetric,
		LogDemo,
		DiagnosticsDemo,
		PlaybackFrameQuery,
		PlaybackDynamicQuery,
		IntrospectAllDynamicOptions,
		MultiClient,
		EtlLogger,
		PacedPlayback,
		Count,
	};

	using namespace pmon::util;
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
	private: Group gm_{ this, "Mode", "Choose sample mode" }; public:
		Option<Mode> mode{ this, "--mode", Mode::Count, "Which demonstration mode to run", CLI::CheckedTransformer{ ref::GenerateEnumKeyMap<Mode>(), CLI::ignore_case } };
		Option<int> submode{ this, "--submode", 0, "Which submode option to run for the given mode" };
	private: Group gc_{ this, "Connection", "Control client connection" }; public:
		Option<std::string> controlPipe{ this, "--control-pipe", "", "Name of the named pipe to use for the client-service control channel" };
		Option<std::string> introNsm{ this, "--intro-nsm", "", "Name of the NSM used for introspection data" };
		Option<std::string> middlewareDllPath{ this, "--middleware-dll-path", "", "Override middleware DLL path discovery with custom path" };
	private: Group gs_{ this, "Sampling", "Control sampling / targeting behavior" }; public:
		Option<double> metricOffset{ this, "--metric-offset", 1064., "Offset from top for frame data. Used in --dynamic-query-sample" };
		Option<double> windowSize{ this, "--window-size", 1000., "Window size used for metrics calculation. Used in --dynamic-query-sample" };
		Option<unsigned int> processId{ this, "--process-id", 0, "Process Id to use for polling or frame data capture" };
		Option<std::string> processName{ this, "--process-name", "", "Name of process to use for polling or frame data capture" };
		Option<std::string> metric{ this, "--metric", "", "PM_METRIC, ex. PM_METRIC_PRESENTED_FPS" };
		Option<unsigned int> telemetryPeriodMs{ this, "--telemetry-period-ms", {}, "Telemetry period in milliseconds" };
		Option<unsigned int> etwFlushPeriodMs{ this, "--etw-flush-period-ms", {}, "ETW manual flush period in milliseconds" };
		Option<double> runTime{ this, "--run-time", 10., "How long to capture for, in seconds" };
		Option<double> runStart{ this, "--run-start", 1., "How many seconds to delay before beginning run" };
		Option<double> pollPeriod{ this, "--poll-period", 0.1, "Period in seconds for polling the API query" };
		Option<std::string> outputPath{ this, "--output-path", {}, "Full path to output to" };
	private: Group gl_{ this, "Logging", "Control logging behavior" }; public:
		Option<log::Level> logLevel{ this, "--log-level", log::Level::Error, "Severity to log at", CLI::CheckedTransformer{ log::GetLevelMapNarrow(), CLI::ignore_case } };
		Option<log::Level> logTraceLevel{ this, "--log-trace-level", log::Level::Error, "Severity to print stacktrace at", CLI::CheckedTransformer{ log::GetLevelMapNarrow(), CLI::ignore_case } };
		Flag logTraceExceptions{ this, "--log-trace-exceptions", "Add stack trace to all thrown exceptions (including SEH exceptions)" };
		Option<std::string> logDenyList{ this, "--log-deny-list", "", "Path to log deny list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logAllowList{ this, "--log-allow-list", "", "Path to log allow list (with trace overrides)", CLI::ExistingFile };
		Option<std::string> logFolder{ this, "--log-folder", "", "Folder to create log file(s) in" };
		Flag logNamePid{ this, "--log-name-pid", "Append PID to the log file name" };
	private: Group gv_{ this, "Service", "Control service options" }; public:
		Flag servicePacePlayback{ this, "--service-pace-playback", "Pace ETL playback on the service" };
		Option<std::string> serviceEtlPath{ this, "--service-etl-path", "", "Path of the ETL file to pass to the service for playback" };
	private: Group gt_{ this, "Testing", "Control testing support options" }; public:
		Flag testExpectError{ this, "--test-expect-error", "Indicates to test modes that fail state is being tested" };

		static constexpr const char* description = "Minimal Sample Client for Intel PresentMon service";
		static constexpr const char* name = "SampleClient.exe";

	private:
		MutualExclusion logListExclusion_{ logDenyList, logAllowList };
		Mandatory mandatoryMode_{ mode };
	};
}