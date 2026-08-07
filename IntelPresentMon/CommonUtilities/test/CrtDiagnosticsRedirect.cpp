#include "CrtDiagnosticsRedirect.h"

#include "../win/WinAPI.h"
#include <crtdbg.h>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <share.h>
#include <string>

namespace pmon::util::test
{
	namespace
	{
		std::mutex g_reportMtx;
		std::filesystem::path g_reportLogPath;
		std::string g_reportLogPathChars;
		FILE* g_crtReportFile = nullptr;
		bool g_criticalReportRecorded = false;
		bool g_reportHookInstalled = false;
		bool g_enabledCached = false;
		bool g_enabledValue = false;
		bool g_enabledLookupDone = false;

		void OpenCrtReportFile_() noexcept;

		bool IsTruthyEnvValue_(const char* value) noexcept
		{
			if (!value || !value[0]) {
				return false;
			}
			std::string token;
			for (const char* p = value; *p; ++p) {
				const unsigned char ch = static_cast<unsigned char>(*p);
				if (std::isspace(ch)) {
					continue;
				}
				token.push_back(static_cast<char>(std::tolower(ch)));
			}
			if (token.empty()) {
				return false;
			}
			return token != "0" && token != "false" && token != "no" && token != "off";
		}

		bool IsRedirectEnabled_() noexcept
		{
			if (!g_enabledLookupDone) {
				g_enabledLookupDone = true;
				char* value = nullptr;
				size_t len = 0;
				if (_dupenv_s(&value, &len, kCrtAssertRedirectEnvVar) == 0 && value) {
					g_enabledValue = IsTruthyEnvValue_(value);
					free(value);
				}
				else {
					g_enabledValue = false;
				}
				g_enabledCached = true;
			}
			return g_enabledCached && g_enabledValue;
		}

		void AppendReportLineCrtSafe_(const char* prefix, const char* message) noexcept
		{
			if (g_crtReportFile) {
				std::fprintf(g_crtReportFile, "%s pid=%lu tid=%lu %s\n",
					prefix,
					static_cast<unsigned long>(GetCurrentProcessId()),
					static_cast<unsigned long>(GetCurrentThreadId()),
					message ? message : "(null message)");
				std::fflush(g_crtReportFile);
				return;
			}
			try {
				if (g_reportLogPath.empty()) {
					return;
				}
				std::ofstream out{ g_reportLogPath, std::ios::app };
				if (!out) {
					return;
				}
				out << prefix << " pid=" << GetCurrentProcessId() << " tid=" << GetCurrentThreadId()
					<< ' ' << (message ? message : "(null message)") << '\n';
			}
			catch (...) {
			}
		}

		void AppendReportLine_(const char* prefix, const char* message) noexcept
		{
			AppendReportLineCrtSafe_(prefix, message);
			try {
				std::lock_guard lock{ g_reportMtx };
				if (g_reportLogPath.empty()) {
					return;
				}
				std::ofstream out{ g_reportLogPath, std::ios::app };
				if (!out) {
					return;
				}
				out << std::format("{} pid={} tid={} {}\n",
					prefix,
					GetCurrentProcessId(),
					GetCurrentThreadId(),
					message ? message : "(null message)");
			}
			catch (...) {
			}
		}

		int __cdecl CrtReportHook_(int reportType, char* message, int* returnValue)
		{
			if (reportType < 0 || reportType >= _CRT_ERRCNT) {
				if (returnValue) {
					*returnValue = 0;
				}
				return FALSE;
			}
			if (!g_crtReportFile && g_reportLogPath.empty()) {
				OpenCrtReportFile_();
			}
			else if (!g_crtReportFile && !g_reportLogPath.empty()) {
				g_crtReportFile = _fsopen(g_reportLogPath.string().c_str(), "a", _SH_DENYNO);
				if (g_crtReportFile) {
					setvbuf(g_crtReportFile, nullptr, _IONBF, 0);
				}
			}
			const char* prefix = "CRT-REPORT";
			switch (reportType) {
			case _CRT_ASSERT: prefix = "CRT-ASSERT"; break;
			case _CRT_ERROR: prefix = "CRT-ERROR"; break;
			case _CRT_WARN: prefix = "CRT-WARN"; break;
			default: break;
			}
			AppendReportLineCrtSafe_(prefix, message);
			if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR) {
				g_criticalReportRecorded = true;
			}
			if (returnValue) {
				*returnValue = 0;
			}
			return TRUE;
		}

		std::filesystem::path ResolveAgentDiagnosticsFolder_() noexcept
		{
			try {
				wchar_t modulePath[MAX_PATH]{};
				HMODULE selfModule = nullptr;
				if (GetModuleHandleExW(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCWSTR>(&ResolveAgentDiagnosticsFolder_),
					&selfModule) && selfModule) {
					if (GetModuleFileNameW(selfModule, modulePath, MAX_PATH) > 0) {
						return std::filesystem::path{ modulePath }.parent_path() / kAgentDiagnosticsFolderRel;
					}
				}
				wchar_t exePath[MAX_PATH]{};
				if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
					return std::filesystem::path{ exePath }.parent_path() / kAgentDiagnosticsFolderRel;
				}
			}
			catch (...) {
			}
			return std::filesystem::path{ kAgentDiagnosticsFolderRel };
		}

		void OpenCrtReportFile_() noexcept
		{
			try {
				const auto folder = ResolveAgentDiagnosticsFolder_();
				std::error_code ec;
				std::filesystem::create_directories(folder, ec);
				g_reportLogPath = folder / std::format("crt-reports-{}.txt", GetCurrentProcessId());
				g_reportLogPathChars = g_reportLogPath.string();
				if (g_crtReportFile) {
					fclose(g_crtReportFile);
					g_crtReportFile = nullptr;
				}
				g_crtReportFile = _fsopen(g_reportLogPath.string().c_str(), "a", _SH_DENYNO);
				if (g_crtReportFile) {
					setvbuf(g_crtReportFile, nullptr, _IONBF, 0);
				}
				else {
					std::ofstream touch{ g_reportLogPath, std::ios::app };
				}
			}
			catch (...) {
			}
		}

		void ApplyCrtReportPolicy_() noexcept
		{
			SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
			_set_error_mode(_OUT_TO_STDERR);
			_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

			if (g_reportHookInstalled) {
				if (_CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, CrtReportHook_) != 0) {
					g_reportHookInstalled = false;
				}
			}
			if (_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtReportHook_) != 0) {
				g_reportHookInstalled = true;
			}

			for (int reportType : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT }) {
				_CrtSetReportMode(reportType, _CRTDBG_MODE_FILE);
				if (g_crtReportFile) {
					_CrtSetReportFile(reportType, g_crtReportFile);
				}
				else {
					_CrtSetReportFile(reportType, _CRTDBG_FILE_STDERR);
				}
			}
		}

		void InstallIfEnabled_() noexcept
		{
			if (!IsRedirectEnabled_()) {
				return;
			}
			OpenCrtReportFile_();
			ApplyCrtReportPolicy_();
			AppendReportLineCrtSafe_("CRT-SETUP", "CRT assert redirect installed for this module");
		}
	}

	bool IsCrtAssertRedirectEnabled() noexcept
	{
		return IsRedirectEnabled_();
	}

	void MaybeInstallCrtAssertRedirect() noexcept
	{
		InstallIfEnabled_();
	}

	void ReapplyCrtAssertRedirectIfEnabled() noexcept
	{
		InstallIfEnabled_();
	}

	bool WasCriticalCrtReportRecorded() noexcept
	{
		return g_criticalReportRecorded;
	}

	void ClearCriticalCrtReportRecorded() noexcept
	{
		g_criticalReportRecorded = false;
	}

	const char* GetCrtReportLogPath() noexcept
	{
		return g_reportLogPathChars.empty() ? "" : g_reportLogPathChars.c_str();
	}
}
