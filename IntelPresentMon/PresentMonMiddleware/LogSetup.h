#pragma once
#include <Memory>
#include <filesystem>
#include "../CommonUtilities/log/IChannel.h"
#include "../CommonUtilities/log/CopyDriver.h"
#include "../PresentMonAPI2/PresentMonDiagnostics.h"

namespace pmon::util::log
{
	void SetupCopyChannel(IChannel* pCopyTargetChannel,
		HostLogEntryForwardFn forwardFn = nullptr) noexcept;
	// drop cross-module copy bridge before destroying middleware log channel
	void SeverCopyLoggingBridge() noexcept;
	void SetupODSChannel(Level logLevel, Level stackTraceLevel, bool exceptionTrace) noexcept;
	void SetupDiagnosticChannel(const PM_DIAGNOSTIC_CONFIGURATION* pConfig) noexcept;
	void SetupFileChannel(std::filesystem::path path, Level logLevel, Level stackTraceLevel,
		bool exceptionTrace) noexcept;
	// attach / detach middleware-local output drivers on the current channel, creating a
	// base channel if none exists; never replaces the channel (copy link and diagnostic
	// driver are left intact); empty filePath detaches the file driver
	void SetLocalOutputs(const std::filesystem::path& filePath, bool debuggerOutput);
	struct LocalLogOutputs
	{
		std::filesystem::path filePath;
		bool debuggerOutput = false;
	};
	LocalLogOutputs GetLocalOutputs();
	std::shared_ptr<class DiagnosticDriver> GetDiagnostics();
}