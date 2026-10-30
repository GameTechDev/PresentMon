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
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/CopyDriver.h"
#include "../CommonUtilities/log/DiagnosticDriver.h"
#include "../CommonUtilities/log/Log.h"
#include "LogSetup.h"
#include <memory>
#include <mutex>


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
		std::shared_ptr<IChannel> MakeCopyChannel_(IChannel* pCopyTargetChannel,
			HostLogEntryForwardFn forwardFn) noexcept
		{
			try {
				if (!pCopyTargetChannel) {
					return {};
				}
				// channel
				auto pChannel = std::make_shared<Channel>();
				// error resolver
				pChannel->AttachComponent(MakeErrorCodePolicy_());
				// make and add the line-tracking policy
				pChannel->AttachComponent(std::make_shared<LinePolicy>());
				// configure drivers
				pChannel->AttachComponent(std::make_shared<CopyDriver>(pCopyTargetChannel, forwardFn), "drv:copy");
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
		std::shared_ptr<IChannel> MakeDiagnosticChannel_(std::shared_ptr<DiagnosticDriver> pDiag,
			bool synchronousMode) noexcept
		{
			try {
				// channel
				auto pChannel = std::make_shared<Channel>(synchronousMode);
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
		// create a channel with no drivers, used as the base for middleware-local outputs
		std::shared_ptr<IChannel> MakeBaseChannel_()
		{
			auto pChannel = std::make_shared<Channel>();
			pChannel->AttachComponent(MakeErrorCodePolicy_());
			pChannel->AttachComponent(std::make_shared<LinePolicy>());
			return pChannel;
		}
		// state for middleware-local outputs
		std::mutex localOutputMtx_;
		std::weak_ptr<IChannelComponent> pLocalFileDriver_;
		std::filesystem::path localFilePath_;
		// creates a null channel and configures logging to be disabled as much as possible
		// NOTE: by design (external API clients get no logging unless requested), entries
		// emitted before a host configures outputs (link, Setup*, pmSetMiddlewareLogOutput_)
		// are dropped; the kernel configures in the same call that first loads the dll
		std::shared_ptr<IChannel> MakeNullChannel_()
		{
			return {};
		}
	}

	std::shared_ptr<IChannel> GetDefaultChannel() noexcept
	{
		return GetDefaultChannelWithFactory(MakeNullChannel_);
	}

	void SetupCopyChannel(IChannel* pCopyTargetChannel, HostLogEntryForwardFn forwardFn) noexcept
	{
		// reset logging level when channel is explicitly requested
		GlobalPolicy::Get().SetLogLevelDefault();
		GlobalPolicy::Get().SetTraceLevelDefault();
		FlushEntryPoint();
		SeverCopyLoggingBridge();
		InjectDefaultChannel(MakeCopyChannel_(pCopyTargetChannel, forwardFn));
	}

	void SeverCopyLoggingBridge() noexcept
	{
		try {
			if (auto pChan = GetDefaultChannel()) {
				pChan->AttachComponent({}, "drv:copy");
			}
		}
		catch (...) {}
	}

	void SetupODSChannel(Level logLevel, Level stackTraceLevel, bool exceptionTrace) noexcept
	{
		GlobalPolicy::Get().SetLogLevel(logLevel);
		GlobalPolicy::Get().SetTraceLevel(stackTraceLevel);
		GlobalPolicy::Get().SetExceptionTrace(exceptionTrace);
		SeverCopyLoggingBridge();
		InjectDefaultChannel(MakeODSChannel_());
	}

	void SetupDiagnosticChannel(const PM_DIAGNOSTIC_CONFIGURATION* pConfig) noexcept
	{
		// create / replace diagnostic driver
		std::lock_guard lk{ diagnosticsMtx_ };
		try {
			pDiagnostics_ = std::make_shared<log::DiagnosticDriver>(pConfig);
			// attach to existing channel if present
			SeverCopyLoggingBridge();
			InjectDefaultChannel(MakeDiagnosticChannel_(pDiagnostics_, pConfig->enableSynchronousLogging));
			// set global logging policy based on the configuration
			GlobalPolicy::Get().SetLogLevel((Level)pConfig->filterLevel);
			GlobalPolicy::Get().StoreVerboseModules(pConfig->verboseModuleBitset);
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
		SeverCopyLoggingBridge();
		InjectDefaultChannel(MakeFileChannel_(std::move(path)));
	}

	void SetLocalOutputs(const std::filesystem::path& filePath, bool debuggerOutput)
	{
		std::lock_guard lk{ localOutputMtx_ };
		auto pChannel = GetDefaultChannel();
		if (!pChannel) {
			if (filePath.empty() && !debuggerOutput) {
				return;
			}
			pChannel = MakeBaseChannel_();
			InjectDefaultChannel(pChannel);
		}
		if (filePath.empty()) {
			pChannel->AttachComponent({}, "drv:file");
			pLocalFileDriver_.reset();
			localFilePath_.clear();
		}
		else {
			// skip reopening when our driver for this path is still attached
			const auto pCurrent = pChannel->GetComponent("drv:file");
			if (!pCurrent || pCurrent != pLocalFileDriver_.lock() || filePath != localFilePath_) {
				auto pDriver = std::make_shared<BasicFileDriver>(std::make_shared<TextFormatter>(),
					std::make_shared<SimpleFileStrategy>(filePath));
				pChannel->AttachComponent(pDriver, "drv:file");
				pLocalFileDriver_ = pDriver;
				localFilePath_ = filePath;
			}
		}
		if (!debuggerOutput) {
			pChannel->AttachComponent({}, "drv:dbg");
		}
		else if (!pChannel->GetComponent("drv:dbg")) {
			pChannel->AttachComponent(std::make_shared<MsvcDebugDriver>(
				std::make_shared<TextFormatter>()), "drv:dbg");
		}
	}

	LocalLogOutputs GetLocalOutputs()
	{
		std::lock_guard lk{ localOutputMtx_ };
		LocalLogOutputs outputs;
		if (auto pChannel = GetDefaultChannel()) {
			if (const auto pFile = pChannel->GetComponent("drv:file");
				pFile && pFile == pLocalFileDriver_.lock()) {
				outputs.filePath = localFilePath_;
			}
			outputs.debuggerOutput = (bool)pChannel->GetComponent("drv:dbg");
		}
		return outputs;
	}

	std::shared_ptr<class DiagnosticDriver> GetDiagnostics()
	{
		std::lock_guard lk{ diagnosticsMtx_ };
		return pDiagnostics_;
	}
}
