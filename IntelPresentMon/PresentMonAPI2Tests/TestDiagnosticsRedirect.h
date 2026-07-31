#pragma once
#include "../CommonUtilities/test/CrtDiagnosticsRedirect.h"

namespace pmon::test
{
	inline constexpr const char* agentDiagnosticsFolder_ = pmon::util::test::kAgentDiagnosticsFolderRel;

	inline void InstallTestDiagnosticsRedirect() noexcept
	{
		pmon::util::test::MaybeInstallCrtAssertRedirect();
	}

	inline void ReapplyTestDiagnosticsRedirectIfEnabled() noexcept
	{
		pmon::util::test::ReapplyCrtAssertRedirectIfEnabled();
	}

	inline bool WasCriticalCrtReportRecorded() noexcept
	{
		return pmon::util::test::WasCriticalCrtReportRecorded();
	}

	inline void ClearCriticalCrtReportRecorded() noexcept
	{
		pmon::util::test::ClearCriticalCrtReportRecorded();
	}

	inline const char* CriticalCrtReportLogPath() noexcept
	{
		return pmon::util::test::GetCrtReportLogPath();
	}
}
