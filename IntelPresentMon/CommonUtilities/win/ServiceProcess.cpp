#include "ServiceProcess.h"
#include "Handle.h"
#include "HrError.h"
#include "../log/Log.h"
#include "../str/String.h"
#include <shellapi.h>
#include <vector>

namespace pmon::util::win
{
	namespace
	{
		// SC_HANDLE must be closed with CloseServiceHandle, not CloseHandle.
		struct ScHandle_
		{
			SC_HANDLE handle = nullptr;
			explicit ScHandle_(SC_HANDLE h) noexcept : handle{ h } {}
			~ScHandle_()
			{
				if (handle) {
					CloseServiceHandle(handle);
				}
			}
			ScHandle_(const ScHandle_&) = delete;
			ScHandle_& operator=(const ScHandle_&) = delete;
			explicit operator bool() const noexcept { return handle != nullptr; }
			SC_HANDLE Get() const noexcept { return handle; }
		};

		struct LocalWstrArray_
		{
			wchar_t** argv = nullptr;
			explicit LocalWstrArray_(wchar_t** p) noexcept : argv{ p } {}
			~LocalWstrArray_()
			{
				if (argv) {
					::LocalFree(argv);
				}
			}
			LocalWstrArray_(const LocalWstrArray_&) = delete;
			LocalWstrArray_& operator=(const LocalWstrArray_&) = delete;
		};
	}

	bool ServiceStartNameIsLocalSystem(const std::wstring& serviceStartName) noexcept
	{
		const auto normalized = str::ToLower(serviceStartName);
		return normalized == L"localsystem" ||
			normalized == L"nt authority\\localsystem";
	}

	std::wstring ServiceExecutablePathFromCommandLine(const std::wstring& commandLine)
	{
		if (commandLine.empty()) {
			return {};
		}
		int argc = 0;
		LocalWstrArray_ argv{ ::CommandLineToArgvW(commandLine.c_str(), &argc) };
		if (!argv.argv || argc < 1 || !argv.argv[0] || argv.argv[0][0] == L'\0') {
			return {};
		}
		return argv.argv[0];
	}

	std::optional<WindowsServiceVerificationInfo> TryGetWindowsServiceVerificationInfo(
		const std::wstring& serviceName) noexcept
	{
		try {
			const ScHandle_ hScm{ OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT) };
			if (!hScm) {
				pmlog_warn("OpenSCManager failed for service verification query").hr();
				return std::nullopt;
			}
			const ScHandle_ hService{
				OpenServiceW(hScm.Get(), serviceName.c_str(),
					SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG) };
			if (!hService) {
				const auto err = GetLastError();
				const auto serviceNameNarrow = str::ToNarrow(serviceName);
				if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
					pmlog_dbg("Windows service not registered").pmwatch(serviceNameNarrow);
				}
				else {
					pmlog_warn("OpenService failed for service verification query").pmwatch(serviceNameNarrow).hr();
				}
				return std::nullopt;
			}

			SERVICE_STATUS_PROCESS status{
				.dwServiceType = 0,
				.dwCurrentState = 0,
				.dwControlsAccepted = 0,
				.dwWin32ExitCode = 0,
				.dwServiceSpecificExitCode = 0,
				.dwCheckPoint = 0,
				.dwWaitHint = 0,
				.dwProcessId = 0,
				.dwServiceFlags = 0,
			};
			DWORD bytesNeeded = 0;
			if (!QueryServiceStatusEx(hService.Get(), SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded)) {
				pmlog_warn("QueryServiceStatusEx failed").pmwatch(str::ToNarrow(serviceName)).hr();
				return std::nullopt;
			}
			if (status.dwCurrentState != SERVICE_RUNNING || status.dwProcessId == 0) {
				pmlog_warn("Windows service is not running")
					.pmwatch(str::ToNarrow(serviceName))
					.pmwatch(status.dwCurrentState)
					.pmwatch(status.dwProcessId);
				return std::nullopt;
			}

			DWORD configBytesNeeded = 0;
			QueryServiceConfigW(hService.Get(), nullptr, 0, &configBytesNeeded);
			const DWORD configErr = GetLastError();
			if (configErr != ERROR_INSUFFICIENT_BUFFER || configBytesNeeded == 0) {
				pmlog_warn("QueryServiceConfigW size query failed").pmwatch(str::ToNarrow(serviceName)).hr();
				return std::nullopt;
			}
			std::vector<BYTE> configBuffer(configBytesNeeded);
			auto* pConfig = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(configBuffer.data());
			if (!QueryServiceConfigW(hService.Get(), pConfig, configBytesNeeded,
				&configBytesNeeded)) {
				pmlog_warn("QueryServiceConfigW failed").pmwatch(str::ToNarrow(serviceName)).hr();
				return std::nullopt;
			}

			std::wstring binaryPath;
			if (pConfig->lpBinaryPathName) {
				binaryPath = ServiceExecutablePathFromCommandLine(pConfig->lpBinaryPathName);
			}
			std::wstring serviceStartName;
			if (pConfig->lpServiceStartName) {
				serviceStartName = pConfig->lpServiceStartName;
			}

			return WindowsServiceVerificationInfo{
				.processId = status.dwProcessId,
				.currentState = status.dwCurrentState,
				.binaryPath = std::move(binaryPath),
				.serviceStartName = std::move(serviceStartName),
			};
		}
		catch (...) {
			pmlog_warn(ReportException("TryGetWindowsServiceVerificationInfo failed"));
			return std::nullopt;
		}
	}

	std::optional<WindowsServiceProcessInfo> TryGetWindowsServiceProcessInfo(const std::wstring& serviceName) noexcept
	{
		try {
			const ScHandle_ hScm{ OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT) };
			if (!hScm) {
				pmlog_warn("OpenSCManager failed for service process query").hr();
				return std::nullopt;
			}
			const ScHandle_ hService{
				OpenServiceW(hScm.Get(), serviceName.c_str(), SERVICE_QUERY_STATUS) };
			if (!hService) {
				const auto err = GetLastError();
				const auto serviceNameNarrow = str::ToNarrow(serviceName);
				if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
					pmlog_dbg("Windows service not registered").pmwatch(serviceNameNarrow);
				}
				else {
					pmlog_warn("OpenService failed for service process query").pmwatch(serviceNameNarrow).hr();
				}
				return std::nullopt;
			}
			SERVICE_STATUS_PROCESS status{
				.dwServiceType = 0,
				.dwCurrentState = 0,
				.dwControlsAccepted = 0,
				.dwWin32ExitCode = 0,
				.dwServiceSpecificExitCode =  0,
				.dwCheckPoint = 0,
				.dwWaitHint = 0,
				.dwProcessId = 0,
				.dwServiceFlags = 0,
			};
			DWORD bytesNeeded = 0;
			if (!QueryServiceStatusEx(hService.Get(), SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded)) {
				pmlog_warn("QueryServiceStatusEx failed").pmwatch(str::ToNarrow(serviceName)).hr();
				return std::nullopt;
			}
			if (status.dwCurrentState != SERVICE_RUNNING || status.dwProcessId == 0) {
				pmlog_warn("Windows service is not running")
					.pmwatch(str::ToNarrow(serviceName))
					.pmwatch(status.dwCurrentState)
					.pmwatch(status.dwProcessId);
				return std::nullopt;
			}
			return WindowsServiceProcessInfo{
				.processId = status.dwProcessId,
				.currentState = status.dwCurrentState,
			};
		}
		catch (...) {
			pmlog_warn(ReportException("TryGetWindowsServiceProcessInfo failed"));
			return std::nullopt;
		}
	}

	bool TryGetNamedPipeServerProcessId(HANDLE hPipe, uint32_t& serverProcessId) noexcept
	{
		ULONG pid = 0;
		if (!GetNamedPipeServerProcessId(hPipe, &pid)) {
			return false;
		}
		if (pid == 0) {
			return false;
		}
		serverProcessId = (uint32_t)pid;
		return true;
	}

	bool IsProcessRunningAsLocalSystem(uint32_t processId) noexcept
	{
		try {
			const Handle hProc{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId) };
			if (!hProc) {
				pmlog_warn("OpenProcess failed for LocalSystem check").pmwatch(processId).hr();
				return false;
			}
			Handle hToken;
			if (!OpenProcessToken(hProc.Get(), TOKEN_QUERY, hToken.ClearAndGetAddressOf())) {
				pmlog_warn("OpenProcessToken failed for LocalSystem check").pmwatch(processId).hr();
				return false;
			}
			DWORD size = 0;
			GetTokenInformation(hToken.Get(), TokenUser, nullptr, 0, &size);
			std::vector<BYTE> buffer(size);
			if (!GetTokenInformation(hToken.Get(), TokenUser, buffer.data(), size, &size)) {
				pmlog_warn("GetTokenInformation failed for LocalSystem check").pmwatch(processId).hr();
				return false;
			}
			const auto* pUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
			return IsWellKnownSid(pUser->User.Sid, WinLocalSystemSid) == TRUE;
		}
		catch (...) {
			pmlog_warn(ReportException("IsProcessRunningAsLocalSystem failed"));
			return false;
		}
	}
}
