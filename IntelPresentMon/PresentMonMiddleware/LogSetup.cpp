#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/Channel.h"
#include "../CommonUtilities/log/MsvcDebugDriver.h"
#include "../CommonUtilities/log/BasicFileDriver.h"
#include "../CommonUtilities/log/TextFormatter.h"
#include "../CommonUtilities/log/SimpleFileStrategy.h"
#include "../CommonUtilities/log/LinePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolvePolicy.h"
#include "../CommonUtilities/log/ErrorCodeResolver.h"
#include "../CommonUtilities/win/HrErrorCodeProvider.h"
#include "../PresentMonAPIWrapperCommon/PmErrorCodeProvider.h"
#include "../CommonUtilities/log/CopyDriver.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "LogSetup.h"
#include <memory>


namespace pmon::util::log
{
	namespace
	{
		// cache reference to diagnostics system here for C-Api interaction and attachment to new channels
		std::shared_ptr<DiagnosticDriver> pDiagnostics_;
		std::mutex diagnosticsMtx_;

		std::shared_ptr<ErrorCodeResolvePolicy> MakeErrorCodePolicy_()
		{
			// error resolver
			auto pErrorResolver = std::make_shared<ErrorCodeResolver>();
			pErrorResolver->AddProvider(std::make_unique<win::HrErrorCodeProvider>());
			pErrorResolver->AddProvider(std::make_unique<pmapi::PmErrorCodeProvider>());
			// error resolving policy
			auto pErrPolicy = std::make_shared<ErrorCodeResolvePolicy>();
			pErrPolicy->SetResolver(std::move(pErrorResolver));
			return pErrPolicy;
		}

		// creates a copy channel to copy entries to a channel in the same process (useful when the channel
		// is in a different module/heap)
		std::shared_ptr<IChannel> MakeCopyChannel_(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>();
				// error resolver
				pChannel->AttachComponent(MakeErrorCodePolicy_());
				// make and add the line-tracking policy
				pChannel->AttachComponent(std::make_shared<LinePolicy>());
				// configure drivers
				pChannel->AttachComponent(std::make_shared<CopyDriver>(std::move(pCopyTargetChannel)));
				return pChannel;
			}
			catch (...) {
				return {};
			}
		}
		// make a channel that only outputs to WinDbg with OutputDebugString
		std::shared_ptr<IChannel> MakeODSChannel_() noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>();
				// error resolver
				pChannel->AttachComponent(MakeErrorCodePolicy_());
				// configure drivers
				// make the formatter and file strategy
				const auto pFormatter = std::make_shared<TextFormatter>();
				pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
				return pChannel;
			}
			catch (...) {
				return {};
			}
		}
		// create a channel that outputs to files in the specified directory
		std::shared_ptr<IChannel> MakeFileChannel_(std::filesystem::path path) noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>();
				// error resolver
				pChannel->AttachComponent(MakeErrorCodePolicy_());
				// make the formatter and file strategy
				const auto pFormatter = std::make_shared<TextFormatter>();
				const auto pFileStrategy = std::make_shared<SimpleFileStrategy>(std::move(path));
				pChannel->AttachComponent(std::make_shared<BasicFileDriver>(pFormatter, pFileStrategy), "drv:file");
				// also add debugger output, just in case (it's low overhead)
				pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(pFormatter), "drv:dbg");
				return pChannel;
			}
			catch (...) {
				return {};
			}
		}
		// create a channel that outputs via the diagnostic layer
		std::shared_ptr<IChannel> MakeDiagnosticChannel_(std::shared_ptr<DiagnosticDriver> pDiag) noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>();
				// error resolver
				pChannel->AttachComponent(MakeErrorCodePolicy_());
				// make and add the line-tracking policy
				pChannel->AttachComponent(std::make_shared<LinePolicy>());
				// connect diagnostic driver to new channel
				pChannel->AttachComponent(pDiag, "drv:diag");
				return pChannel;
			}
			catch (...) {
				return {};
			}
		}
		// creates a null channel and configures logging to be disabled as much as possible
		std::shared_ptr<IChannel> MakeNullChannel_()
		{
			return {};
		}
	}

	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeNullChannel_);
	}

	void SetupCopyChannel(std::shared_ptr<IChannel> pCopyTargetChannel) noexcept
	{
		// reset logging level when channel is explicitly requested
		GlobalPolicy::Get().SetLogLevelDefault();
		GlobalPolicy::Get().SetTraceLevelDefault();
		InjectDefaultChannel(MakeCopyChannel_(std::move(pCopyTargetChannel)));
	}

	void SetupODSChannel(Level logLevel, Level stackTraceLevel, bool exceptionTrace) noexcept
	{
		GlobalPolicy::Get().SetLogLevel(logLevel);
		GlobalPolicy::Get().SetTraceLevel(stackTraceLevel);
		GlobalPolicy::Get().SetExceptionTrace(exceptionTrace);
		InjectDefaultChannel(MakeODSChannel_());
	}

	void SetupDiagnosticChannel(const PM_DIAGNOSTIC_CONFIGURATION* pConfig) noexcept
	{
		// create / replace diagnostic driver
		std::lock_guard lk{ diagnosticsMtx_ };
		try {
			pDiagnostics_ = std::make_shared<log::DiagnosticDriver>(pConfig);
			// attach to existing channel if present
			InjectDefaultChannel(MakeDiagnosticChannel_(pDiagnostics_));
			// set global logging policy based on the configuration
			GlobalPolicy::Get().SetLogLevel((Level)pConfig->filterLevel);
			if (!pConfig->enableTrace) {
				GlobalPolicy::Get().SetTraceLevel(Level::None);
			}
		}
		catch (...) {
			pmlog_error(ReportException("Failed to create diagnostic driver"));
		}
	}

	void SetupFileChannel(std::filesystem::path path, Level logLevel, Level stackTraceLevel,
		bool exceptionTrace) noexcept
	{
		GlobalPolicy::Get().SetLogLevel(logLevel);
		GlobalPolicy::Get().SetTraceLevel(stackTraceLevel);
		GlobalPolicy::Get().SetExceptionTrace(exceptionTrace);
		InjectDefaultChannel(MakeFileChannel_(std::move(path)));
	}

	std::shared_ptr<class DiagnosticDriver> GetDiagnostics()
	{
		std::lock_guard lk{ diagnosticsMtx_ };
		return pDiagnostics_;
	}
}