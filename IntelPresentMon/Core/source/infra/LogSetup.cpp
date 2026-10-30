#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/MsvcDebugDriver.h>
#include <CommonUtilities/log/BasicFileDriver.h>
#include <CommonUtilities/log/StdioDriver.h>
#include <CommonUtilities/log/DiagnosticDriver.h>
#include <CommonUtilities/log/MarshallDriver.h>
#include <CommonUtilities/log/NamedPipeMarshallSender.h>
#include <CommonUtilities/log/TextFormatter.h>
#include <CommonUtilities/log/SimpleFileStrategy.h>
#include <CommonUtilities/log/LinePolicy.h>
#include <CommonUtilities/log/ErrorCodeResolvePolicy.h>
#include <CommonUtilities/log/ErrorCodeResolver.h>
#include <CommonUtilities/mt/Thread.h>
#include <CommonUtilities/log/NamedPipeMarshallReceiver.h>
#include <CommonUtilities/log/EntryMarshallInjector.h>
#include <CommonUtilities/win/WinAPI.h>
#include <CommonUtilities/win/HrErrorCodeProvider.h>
#include <CommonUtilities/str/String.h>
#include <CommonUtilities/log/PanicLogger.h>
#include <PresentMonAPIWrapperCommon/PmErrorCodeProvider.h>
#include <CommonUtilities/log/HostLogEntryForward.h>
#include <CommonUtilities/log/HostIdentificationForward.h>
#include <PresentMonAPI2/Internal.h>
#include <PresentMonAPI2/PresentMonDiagnostics.h>
#include <Core/source/infra/util/FolderResolver.h>
#include "../cli/CliOptions.h"
#include "LogSetup.h"
#include "LoggingConfigurationStore.h"
#include <memory>
#include <format>
#include <chrono>
#include <filesystem>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			GlobalPolicy::Get().SetSubsystem(Subsystem::IntelPresentmon);
			bool synchronousMode = false;
			bool enableDiagnostics = false;
			try {
				synchronousMode = *::p2c::cli::Options::Get().logSynchronous;
				enableDiagnostics = *::p2c::cli::Options::Get().enableDiagnostic;
			}
			catch (...) {}
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{ synchronousMode }, [](Channel* p) { delete p; } };
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			pErrorResolver->AddProvider(std::make_unique<pmapi::PmErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			pChannel->AttachComponent(std::move(pErrPolicy));
			// make and add the line-tracking policy
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			// make the formatter
			const auto pFormatter = std::make_shared<TextFormatter>();
			// attach drivers
			// TODO: test if cwd is writeable, if not write to some other location (temp?)
			if (!enableDiagnostics) {
				pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
			}
			const auto pFileStrategy = std::make_shared<SimpleFileStrategy>("pm-early-log.txt");
			pChannel->AttachComponent(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy), "drv:file");

			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}

namespace p2c
{
	using namespace pmon::util;
	using namespace pmon::util::log;

	_invalid_parameter_handler pOriginalInvalidParameterHandler_ = nullptr;

	namespace {
		LoggingSingletons linkedLogging_;
		// new files are only started when the diagnostic logging folder changes
		struct ActiveDiagnosticLogFiles_
		{
			std::filesystem::path folder;
			DiagnosticLoggingPaths paths;
		};
		std::optional<ActiveDiagnosticLogFiles_> activeDiagnosticLogFiles_;

		// The copy link passes C++ objects across the module boundary and is only valid for
		// identical builds, so it is established solely on explicit --log-middleware-copy;
		// the caller is responsible for ensuring the builds match.
		LoggingSingletons LinkMiddlewareLogging_(IChannel* pChannel)
		{
			return pmLinkLoggingPtrs_(pChannel, IdentificationTable::GetPtr(),
				&ForwardLogEntryToHostChannel,
				&ForwardAddThreadToHostIdTable,
				&ForwardAddProcessToHostIdTable);
		}

		bool DiagnosticOwnsDebugger_() noexcept
		{
			try {
				return *cli::Options::Get().enableDiagnostic;
			}
			catch (...) {
				return false;
			}
		}

		std::optional<std::filesystem::path> QueryMiddlewareLogPath_()
		{
			pmon::api2::MiddlewareLogOutputState state;
			auto sta = pmGetMiddlewareLogOutput_(&state);
			if (sta == PM_STATUS_SUCCESS) {
				return std::filesystem::path{};
			}
			if (sta != PM_STATUS_INSUFFICIENT_BUFFER) {
				return std::nullopt;
			}
			std::vector<wchar_t> pathBuffer((size_t)state.filePathLength + 1);
			state.filePathCapacity = (uint32_t)pathBuffer.size();
			state.pFilePath = pathBuffer.data();
			sta = pmGetMiddlewareLogOutput_(&state);
			if (sta != PM_STATUS_SUCCESS) {
				return std::nullopt;
			}
			return std::filesystem::path{ pathBuffer.data() };
		}

		// Configure middleware logging through the link-free internal interface. When copy
		// linked, middleware entries are forwarded into this channel, so no local outputs.
		// Returns the middleware log file actually in use (empty when none).
		std::filesystem::path ConfigureMiddlewareLogging_(Level logLevel, Level traceLevel,
			bool exceptionTrace, uint64_t verboseModules, const std::filesystem::path& filePath)
		{
			const bool linked = (bool)linkedLogging_;
			const auto localFilePath = linked ? std::filesystem::path{} : filePath;
			// attach outputs before applying policy so the dll's earliest entries are captured
			const auto filePathWide = localFilePath.wstring();
			pmon::api2::MiddlewareLogOutput output;
			// diagnostics own debugger output when enabled
			output.debuggerOutput = (uint8_t)(!linked && !DiagnosticOwnsDebugger_());
			output.pFilePath = filePathWide.empty() ? nullptr : filePathWide.c_str();
			const auto outputSta = pmSetMiddlewareLogOutput_(&output);
			if (outputSta == PM_STATUS_MIDDLEWARE_MISSING_ENDPOINT) {
				// older middleware: the deprecated endpoint replaces its channel, which would
				// sever a copy link, so only use it for plain file logging; warn once and only
				// rebuild the legacy channel when its settings change
				static bool warnedLegacy = false;
				static std::optional<std::tuple<std::filesystem::path, Level, Level, bool>> lastLegacy;
				if (linked || localFilePath.empty()) {
					if (!std::exchange(warnedLegacy, true)) {
						pmlog_warn("Middleware lacks log configuration endpoints; middleware log policy not applied");
					}
					return {};
				}
				const auto legacy = std::tuple{ localFilePath, logLevel, traceLevel, exceptionTrace };
				if (lastLegacy == legacy) {
					return localFilePath;
				}
				if (const auto fileSta = pmSetupFileLogging_(localFilePath.string().c_str(),
					(PM_DIAGNOSTIC_LEVEL)logLevel, (PM_DIAGNOSTIC_LEVEL)traceLevel, exceptionTrace);
					fileSta != PM_STATUS_SUCCESS) {
					pmlog_error("Failed to configure middleware file logging").code(fileSta);
					return {};
				}
				lastLegacy = legacy;
				return localFilePath;
			}
			if (outputSta != PM_STATUS_SUCCESS) {
				pmlog_error("Failed to configure middleware log output").code(outputSta);
			}
			pmon::api2::MiddlewareLogPolicy policy;
			policy.logLevel = (PM_DIAGNOSTIC_LEVEL)logLevel;
			policy.traceLevel = (PM_DIAGNOSTIC_LEVEL)traceLevel;
			policy.exceptionTrace = (uint8_t)exceptionTrace;
			policy.sehTrace = (uint8_t)exceptionTrace;
			policy.verboseModuleBitset = verboseModules;
			if (const auto sta = pmSetMiddlewareLogPolicy_(&policy); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Failed to configure middleware log policy").code(sta);
			}
			return QueryMiddlewareLogPath_().value_or(
				outputSta == PM_STATUS_SUCCESS ? localFilePath : std::filesystem::path{});
		}

		void ConfigureMiddlewareLogFilter_(const std::string& listPath, bool isDenyList)
		{
			const auto listPathWide = std::filesystem::path{ listPath }.wstring();
			pmon::api2::MiddlewareLogFilter filter;
			filter.listMode = isDenyList ? pmon::api2::MiddlewareLogListMode::Deny
				: pmon::api2::MiddlewareLogListMode::Allow;
			filter.pListPath = listPathWide.c_str();
			if (const auto sta = pmSetMiddlewareLogFilter_(&filter); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Failed to configure middleware log filter").code(sta);
			}
		}

		void InvalidParameterHandler_(
			const wchar_t* expression,
			const wchar_t* function,
			const wchar_t* file,
			unsigned int line,
			uintptr_t pReserved)
		{
			// convert to narrow and log the assertion content
			const auto narrowFunction = str::ToNarrow(function);
			const auto narrowFile = str::ToNarrow(file);
			pmlog_from_(Level::Fatal, narrowFile.c_str(), narrowFunction.c_str(), (int)line)
				.note(std::format("CRT assertion: {}", str::ToNarrow(expression)));
			// flush the default channel
			if (auto pChan = GetDefaultChannel()) {
				pChan->Flush();
			}
			// invoke the original handler
			if (pOriginalInvalidParameterHandler_) {
				pOriginalInvalidParameterHandler_(expression, function, file, line, pReserved);
			}
		}
	}

	void ConfigureLogging() noexcept
	{
		try {
			auto pChan = GetDefaultChannel();
			// setup CRT assert handler
			pOriginalInvalidParameterHandler_ = _set_invalid_parameter_handler(InvalidParameterHandler_);
			// shortcut for command line
			const auto& opt = cli::Options::Get();
			const bool enableDiagnostics = *opt.enableDiagnostic;

			if (opt.logDenyList) {
				pmquell(LineTable::IngestList(*opt.logDenyList, true));
			}
			else if (opt.logAllowList) {
				pmquell(LineTable::IngestList(*opt.logAllowList, false));
			}

			// The store (initialized by the caller via InitializeFromDiskAndCli) holds the
			// effective settings. The driver copies its configuration, so initialize it from
			// the effective settings before later runtime policy application.
			auto& store = LoggingConfigurationStore::Get();
			const auto effective = store.GetEffective();
			if (enableDiagnostics) {
				PM_DIAGNOSTIC_CONFIGURATION diagConfig{};
				diagConfig.filterLevel = (PM_DIAGNOSTIC_LEVEL)effective.logLevel;
				diagConfig.outputFlags = PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER;
				diagConfig.enableTimestamp = true;
				diagConfig.enableTrace = effective.traceLevel != Level::None;
				diagConfig.enableLocation = false;
				diagConfig.disableDiagnosticFilter = true;
				diagConfig.enableSynchronousLogging = *opt.logSynchronous;

				// diagnostics own debugger output when enabled, keep non-debugger channels active
				pChan->AttachComponent(std::shared_ptr<IChannelComponent>{}, "drv:dbg");
				pChan->AttachComponent(std::make_shared<DiagnosticDriver>(&diagConfig), "drv:diag");
			}

			// Middleware copy link only on explicit request (identical builds required).
			// Link before applying middleware policy since linking resets it to defaults.
			if (*opt.logMiddlewareCopy) {
				linkedLogging_ = LinkMiddlewareLogging_(pChan.get());
				if (!linkedLogging_) {
					pmlog_warn("Middleware log copy requested but link is unavailable");
				}
			}
			if (opt.logDenyList) {
				ConfigureMiddlewareLogFilter_(*opt.logDenyList, true);
			}
			else if (opt.logAllowList) {
				ConfigureMiddlewareLogFilter_(*opt.logAllowList, false);
			}

			store.ApplyEffectiveToRuntime();
		}
		catch (...) {
			pmlog_error(ReportException());
		}
	}

	DiagnosticLoggingPaths ConfigureDiagnosticLogging(const DiagnosticLoggingConfiguration& config)
	{
		auto folder = config.folder.empty()
			? infra::util::FolderResolver::Get().ResolveLogPath()
			: config.folder;
		// a missing configured folder falls back to the default, matching the UI process
		if (std::error_code ec; !config.folder.empty() && !std::filesystem::is_directory(folder, ec)) {
			pmlog_warn("Configured log folder does not exist; using default log folder")
				.pmwatch(str::ToNarrow(folder.wstring()));
			folder = infra::util::FolderResolver::Get().ResolveLogPath();
		}
		// the default folder may not exist yet (e.g. fresh install); middleware output requires it
		std::error_code mkdirEc; std::filesystem::create_directories(folder, mkdirEc);
		if (!activeDiagnosticLogFiles_ || activeDiagnosticLogFiles_->folder != folder) {
			const auto now = std::chrono::system_clock::now();
			const auto timestamp = [&] {
				try {
					return std::format("{0:%y}{0:%m}{0:%d}-{0:%H}{0:%M}{0:%OS}",
						std::chrono::zoned_time{ std::chrono::current_zone(), now });
				}
				catch (...) {
					return std::format("{0:%y}{0:%m}{0:%d}-{0:%H}{0:%M}{0:%OS}", now);
				}
			}();
			activeDiagnosticLogFiles_ = ActiveDiagnosticLogFiles_{
				.folder = folder,
				.paths = {
					.client = folder / std::format("pmlog-client-{}.txt", timestamp),
					.middleware = folder / std::format("pmlog-middleware-{}.txt", timestamp),
				},
			};
		}
		const auto& paths = activeDiagnosticLogFiles_->paths;
		auto pChannel = GetDefaultChannel();
		if (!pChannel) {
			throw Except<Exception>("Logging channel is unavailable");
		}

		auto& policy = GlobalPolicy::Get();
		policy.SetLogLevel(config.logLevel);
		policy.SetTraceLevel(config.traceLevel);
		policy.SetExceptionTrace(config.exceptionTrace);
		policy.SetSehTracing(config.exceptionTrace);
		policy.StoreVerboseModules(config.verboseModules);

		// attach the client file first so middleware problems cannot leave it without a log
		pChannel->AttachComponent(std::make_shared<BasicFileDriver>(
			std::make_shared<TextFormatter>(),
			std::make_shared<SimpleFileStrategy>(paths.client)), "drv:file");
		const auto middlewarePath = ConfigureMiddlewareLogging_(config.logLevel, config.traceLevel,
			config.exceptionTrace, config.verboseModules, paths.middleware);
		const DiagnosticLoggingPaths activePaths{
			.client = paths.client,
			// when copy linked, middleware entries are forwarded into the client log
			.middleware = linkedLogging_ ? paths.client : middlewarePath,
		};
		pmlog_info(std::format("Diagnostic logging configured: client [{}], middleware [{}]",
			str::ToNarrow(activePaths.client.wstring()),
			activePaths.middleware.empty() ? std::string{ "<none>" } : str::ToNarrow(activePaths.middleware.wstring())));
		return activePaths;
	}

	void FlushDiagnosticLogging()
	{
		// Middleware forwards into the client channel when copy linked, so drain it first.
		if (const auto sta = pmFlushMiddlewareLog_(2000); sta != PM_STATUS_SUCCESS) {
			pmlog_warn("Failed to flush middleware logging without exit semantics").code(sta);
		}
		// bounded: runs on a kernel action thread and must not hang on a stalled driver
		if (auto pChannel = GetDefaultChannel(); pChannel && !pChannel->TryFlushFor(2s)) {
			pmlog_warn("Timed out flushing client log channel");
		}
	}

	void ConfigureHeadlessLogging() noexcept
	{
		class StdioTextFormatter : public ITextFormatter
		{
		public:
			std::string Format(const Entry& e) const override
			{
				try {
					std::ostringstream oss;
					oss << GetLevelName(e.level_) << ": " << e.note_ << "\n";
					if (e.errorCode_) {
						auto& ec = e.errorCode_;
						if (ec.IsResolvedNontrivial()) {
							auto pStrings = ec.GetStrings();
							oss << std::format("  !{} [{}] ({}): {} => {}\n", pStrings->type, ec.AsHex(), pStrings->symbol, pStrings->name, pStrings->description);
						}
						else {
							oss << std::format("  !UNKNOWN [{}]\n", ec.AsHex());
						}
					}
					return oss.str();
				}
				catch (...) {
					pmlog_panic_("Exception in StdioTextFormatter::Format");
					return {};
				}
			}
		};

		try {
			auto pChan = GetDefaultChannel();
			// shortcut for command line
			const auto& opt = cli::Options::Get();
			pChan->AttachComponent(std::make_shared<StdioDriver>(std::make_shared<StdioTextFormatter>()));
			// default to error levels of logging to not spam stderr
			if (!opt.logLevel) {
				log::GlobalPolicy::Get().SetLogLevel(Level::Error);
			}
		}
		catch (...) {
			pmlog_error(ReportException());
		}
	}

	void ConnectToLoggingSourcePipe(const std::string& pipePrefix)
	{
		mt::Thread{ "logconn-" + pipePrefix, [pipePrefix] {
			try {
				const auto fullPipeName = R"(\\.\pipe\)" + pipePrefix;
				// wait maximum 1.5sec for pipe to be created
				if (!pipe::DuplexPipe::WaitForAvailability(fullPipeName, 1500)) {
					pmlog_warn(std::format("Failed to connect to logging source server {} after waiting 1.5s", pipePrefix));
					return;
				}
				// retry connection maximum 3 times, every 16ms
				const int nAttempts = 3;
				for (int i = 0; i < nAttempts; i++) {
					try {
						auto pChan = log::GetDefaultChannel();
						auto pReceiver = std::make_shared<NamedPipeMarshallReceiver>(pipePrefix, log::IdentificationTable::GetPtr());
						auto pInjector = std::make_shared<EntryMarshallInjector>(pChan, std::move(pReceiver));
						pChan->AttachComponent(std::move(pInjector));
						pmlog_info(std::format("Connected to logpipe [{}]", fullPipeName));
						return;
					}
					catch (const pipe::PipeError&) {
						std::this_thread::sleep_for(16ms);
					}
				}
				pmlog_warn(std::format("Failed to connect to logging source server {} after {} attempts", pipePrefix, nAttempts));
			}
			catch (...) {
				pmlog_error(ReportException());
			}
		} }.detach();
	}

	LogChannelManager::LogChannelManager() noexcept
	{
		InstallSehTranslator();
		BootDefaultChannelEager();
	}
	LogChannelManager::~LogChannelManager()
	{
		pmFlushEntryPoint_();
		FlushEntryPoint();
	}
}
