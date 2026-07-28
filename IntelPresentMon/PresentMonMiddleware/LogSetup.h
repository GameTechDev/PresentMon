#pragma once
#include <Memory>
#include <filesystem>
#include "../CommonUtilities/log/IChannel.h"
#include "../PresentMonAPI2/PresentMonDiagnostics.h"

namespace pmon::util::log
{
	void SetupCopyChannel(IChannel* pCopyTargetChannel) noexcept;
	// drop cross-module copy bridge before destroying middleware log channel
	void SeverCopyLoggingBridge() noexcept;
	void SetupODSChannel(Level logLevel, Level stackTraceLevel, bool exceptionTrace) noexcept;
	void SetupDiagnosticChannel(const PM_DIAGNOSTIC_CONFIGURATION* pConfig) noexcept;
	void SetupFileChannel(std::filesystem::path path, Level logLevel, Level stackTraceLevel,
		bool exceptionTrace) noexcept;
	std::shared_ptr<class DiagnosticDriver> GetDiagnostics();
}