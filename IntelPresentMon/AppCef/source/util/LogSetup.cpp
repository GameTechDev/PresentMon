#include <CommonUtilities/win/WinAPI.h>
#include <CommonUtilities/win/HrErrorCodeProvider.h>
#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/log/Channel.h>
#include <CommonUtilities/log/MsvcDebugDriver.h>
#include <CommonUtilities/log/BasicFileDriver.h>
#include <CommonUtilities/log/MarshallDriver.h>
#include <CommonUtilities/log/NamedPipeMarshallSender.h>
#include <CommonUtilities/log/TextFormatter.h>
#include <CommonUtilities/log/SimpleFileStrategy.h>
#include <CommonUtilities/log/LinePolicy.h>
#include <CommonUtilities/log/LineTable.h>
#include <CommonUtilities/log/ErrorCodeResolvePolicy.h>
#include <CommonUtilities/log/ErrorCodeResolver.h>
#include <CommonUtilities/log/NamedPipeMarshallReceiver.h>
#include <CommonUtilities/log/EntryMarshallInjector.h>
#include <CommonUtilities/log/PanicLogger.h>
#include <CommonUtilities/mt/Thread.h>
#include <CommonUtilities/str/String.h>
#include <Core/source/infra/util/FolderResolver.h>
#include "CliOptions.h"
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
			// make the channel instance
			auto pChannel = std::make_shared<Channel>();
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
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
			const auto pFileStrategy = std::make_shared<SimpleFileStrategy>(std::format("pmui-init-log-{}.txt", GetCurrentProcessId()));
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

namespace p2c::client::util
{
	using namespace pmon::util;
	using namespace pmon::util::log;

	_invalid_parameter_handler pOriginalInvalidParameterHandler_ = nullptr;

	namespace {
		// TODO: factor this out as it is useful everywhere
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

			auto&& pol = GlobalPolicy::Get();
			// set the global policy settings
			if (opt.logLevel) {
				pol.SetLogLevel(*opt.logLevel);
			}
			if (opt.logVerboseModules) {
				for (auto mod : *opt.logVerboseModules) {
					pol.ActivateVerboseModule(mod);
				}
			}
			if (opt.logTraceLevel) {
				pol.SetTraceLevel(*opt.logTraceLevel);
			}
			if (opt.traceExceptions) {
				pol.SetExceptionTrace(*opt.traceExceptions);
				pol.SetSehTracing(*opt.traceExceptions);
			}
			if (opt.logDenyList) {
				pmquell(LineTable::IngestList(*opt.logDenyList, true));
			}
			else if (opt.logAllowList) {
				pmquell(LineTable::IngestList(*opt.logAllowList, false));
			}
			// setup ipc logging server connection for parent process (graph flowing towards kproc)
			if (opt.logPipeName) {
				try {
					auto pSender = std::make_shared<log::NamedPipeMarshallSender>(*opt.logPipeName);
					log::IdentificationTable::RegisterSink(pSender);
					auto pDriver = std::make_shared<log::MarshallDriver>(pSender);
					pChan->AttachComponent(std::move(pDriver));
					// block here waiting for the parent process to connect
					if (!pSender->WaitForConnection(1500ms)) {
						pmlog_warn("Parent process did not connect to ipc logging pipe before timeout (it might have connected later)");
					}
					else {
						// remove independent output drivers (only copy to main process via IPC)
						pChan->AttachComponent({}, "drv:dbg");
						pChan->AttachComponent({}, "drv:file");
					}
				}
				catch (...) {
					pmlog_error(ReportException("log server setup"));
				}
			}
			else if (!opt.cefType || *opt.cefType == "render") {
				pmlog_warn("log pipe name not specified");
			}
		}
		catch (...) {
			pmlog_error(ReportException("general log configuration"));
		}
	}

	// TODO: factor this logic out because it is used by kproc and appcef
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
		FlushEntryPoint();
	}
}