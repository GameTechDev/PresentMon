#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/MsvcDebugDriver.h>
#include <CommonUtilities/log/BasicFileDriver.h>
#include <CommonUtilities/log/StdioDriver.h>
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
#include <PresentMonAPI2/Internal.h>
#include <PresentMonAPI2/PresentMonDiagnostics.h>
#include <Core/source/infra/util/FolderResolver.h>
#include "../cli/CliOptions.h"
#include "LogSetup.h"
#include <memory>
#include <format>
#include <chrono>
#include <filesystem>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			GlobalPolicy::Get().SetSubsystem(Subsystem::IntelPresentmon);
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
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
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
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

			// determine log folder path
			std::filesystem::path logFolder;
			if (opt.logFolder) {
				logFolder = *opt.logFolder;
			}
			else {
				logFolder = infra::util::FolderResolver::Get().Resolve(
					infra::util::FolderResolver::Folder::App, L"logs\\");
			}
			// always enable the logfile for client
			const auto logfilePathClient = std::filesystem::path{ logFolder } / "pmlog.txt";
			pChan->AttachComponent(std::make_shared<BasicFileDriver>(std::make_shared<TextFormatter>(),
				std::make_shared<SimpleFileStrategy>(logfilePathClient)), "drv:file");
			auto&& pol = GlobalPolicy::Get();
			// connect dll channel and id table to exe, get access to global settings in dll
			LoggingSingletons getters;
			if (opt.logMiddlewareCopy) {
				getters = pmLinkLogging_(pChan, []() -> IdentificationTable& {
					return IdentificationTable::Get_(); });
			}
			// set the global policy settings
			if (opt.logLevel) {
				pol.SetLogLevel(*opt.logLevel);
				if (getters) {
					getters.getGlobalPolicy().SetLogLevel(*opt.logLevel);
				}
			}
			if (opt.logVerboseModules) {
				for (auto mod : *opt.logVerboseModules) {
					pol.ActivateVerboseModule(mod);
					if (getters) {
						getters.getGlobalPolicy().ActivateVerboseModule(mod);
					}
				}
			}
			if (opt.logTraceLevel) {
				pol.SetTraceLevel(*opt.logTraceLevel);
				if (getters) {
					getters.getGlobalPolicy().SetTraceLevel(*opt.logTraceLevel);
				}
			}
			if (opt.traceExceptions) {
				pol.SetExceptionTrace(*opt.traceExceptions);
				pol.SetSehTracing(*opt.traceExceptions);
				if (getters) {
					getters.getGlobalPolicy().SetExceptionTrace(*opt.traceExceptions);
					getters.getGlobalPolicy().SetSehTracing(*opt.traceExceptions);
				}
			}
			if (opt.logDenyList) {
				pmquell(LineTable::IngestList(*opt.logDenyList, true));
				if (getters) {
					pmquell(getters.getLineTable().IngestList_(*opt.logDenyList, true));
				}
			}
			else if (opt.logAllowList) {
				pmquell(LineTable::IngestList(*opt.logAllowList, false));
				if (getters) {
					pmquell(getters.getLineTable().IngestList_(*opt.logAllowList, false));
				}
			}
			// enable logfile for middleware if we're not doing the copy connection
			if (!opt.logMiddlewareCopy) {
				if (auto sta = pmSetupFileLogging_((logFolder / "pmlog-mid.txt").string().c_str(),
					(PM_DIAGNOSTIC_LEVEL)pol.GetLogLevel(),
					(PM_DIAGNOSTIC_LEVEL)pol.GetTraceLevel(),
					pol.GetExceptionTrace()
				); sta != PM_STATUS_SUCCESS) {
					pmlog_error("configuring middleware file logging").code(sta).no_trace();
				}
			}
		}
		catch (...) {
			pmlog_error(ReportException());
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