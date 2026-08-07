#pragma once

namespace pmon::util::test
{
	// Relative to process image directory (build\Debug during tests).
	inline constexpr const char* kAgentDiagnosticsFolderRel = "TestLogs\\AgentDiagnostics";

	// When set to a non-empty value other than 0/false/no/off (case-insensitive), CRT asserts
	// and errors are logged to TestLogs\AgentDiagnostics\crt-reports-<pid>.txt instead of
	// showing modal dialogs. Each /MT module must call MaybeInstall in its own image.
	inline constexpr const char* kCrtAssertRedirectEnvVar = "PM_TEST_REDIRECT_CRT_ASSERTS";

	bool IsCrtAssertRedirectEnabled() noexcept;
	void MaybeInstallCrtAssertRedirect() noexcept;
	void ReapplyCrtAssertRedirectIfEnabled() noexcept;
	bool WasCriticalCrtReportRecorded() noexcept;
	void ClearCriticalCrtReportRecorded() noexcept;
	const char* GetCrtReportLogPath() noexcept;
}
