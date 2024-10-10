#include "LogSetup.h"
#include <chrono>
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Channel.h"
#include "../CommonUtilities/log/MsvcDebugDriver.h"
#include "../CommonUtilities/log/BasicFileDriver.h"
#include "../CommonUtilities/log/StdioDriver.h"
#include "../CommonUtilities/log/TextFormatter.h"
#include "../CommonUtilities/log/SimpleFileStrategy.h"
#include "../CommonUtilities/log/LinePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolver.h"
#include "../CommonUtilities/log/ChannelFlusher.h"
#include "../CommonUtilities/log/NamedPipeMarshallSender.h"
#include "../CommonUtilities/log/MarshallDriver.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/Exception.h"
#include "CliOptions.h"
#include "Registry.h"

namespace pmon::util::log
{
	namespace
	{
		std::shared_ptr<IChannel> MakeChannel()
		{
			GlobalPolicy::Get().SetSubsystem(Subsystem::Server);
			// channel (use custom deleter to ensure deletion in this module's heap)
			auto pChannel = std::shared_ptr<IChannel>{ new Channel{}, [](Channel* p) { delete p; } };
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
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
			pChannel->AttachComponent(std::make_shared<StdioDriver>(pFormatter), "drv:std");
			// flusher
			pChannel->AttachComponent(std::make_shared<ChannelFlusher>(pChannel), "obj:fsh");

			return pChannel;
		}
	}

	// this is injected into to util::log namespace and hooks into that system
	std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeChannel);
	}
}

namespace logsetup
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::log;

	void ConfigureLogging(bool asApp) noexcept
	{
		try {
			// shortcuts for command line and registry
			const auto& opt = clio::Options::Get();
			const auto& reg = Reg::Get();
			// get the channel
			auto pChannel = GetDefaultChannel();
			// configure logging based on command line
			if (opt.logLevel || reg.logLevel.Exists()) {
				GlobalPolicy::Get().SetLogLevel(opt.logLevel ? *opt.logLevel : reg.logLevel);
			}
			if (!opt.enableStdioLog) {
				pChannel->AttachComponent({}, "drv:std");
			}
			if (!opt.enableDebuggerLog) {
				pChannel->AttachComponent({}, "drv:dbg");
			}
			if (opt.logDir || reg.logDir.Exists()) {
				const auto dir = opt.logDir ? *opt.logDir : reg.logDir;
				const std::chrono::zoned_time now{ std::chrono::current_zone(), std::chrono::system_clock::now() };
				auto fullPath = std::format("{0}\\pmsvc-log-{1:%y}{1:%m}{1:%d}-{1:%H}{1:%M}{1:%OS}.txt", dir, now);
				pChannel->AttachComponent(std::make_shared<BasicFileDriver>( std::make_shared<TextFormatter>(),
					std::make_shared<SimpleFileStrategy>(fullPath)), "drv:file");
			}
			// setup ipc logging connection for clients
			if (!opt.disableIpcLog) {
				try {
					auto pSender = std::make_shared<NamedPipeMarshallSender>(*opt.logPipeName,
						asApp ? pipe::SecurityMode::Child : pipe::SecurityMode::Service);
					log::IdentificationTable::RegisterSink(pSender);
					auto pDriver = std::make_shared<log::MarshallDriver>(pSender);
					pChannel->AttachComponent(std::move(pDriver));
				}
				catch (...) {
					pmlog_panic_(ReportException());
				}
			}
		}
		catch (...) {
			pmlog_panic_("Failed configuring log in server");
		}
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