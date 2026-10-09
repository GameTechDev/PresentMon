#pragma once
#include "WinAPI.h"
#include <optional>
#include <string>

namespace pmon::util::win
{
	struct WindowsServiceProcessInfo
	{
		uint32_t processId = 0;
		DWORD currentState = 0;
	};

	// SCM identity fields used to verify the shared service without opening its process.
	struct WindowsServiceVerificationInfo
	{
		uint32_t processId = 0;
		DWORD currentState = 0;
		// Executable token from lpBinaryPathName, not the raw service command line.
		std::wstring binaryPath;
		std::wstring serviceStartName;
	};

	// Returns nullopt if the service is not registered, not running, or query fails.
	std::optional<WindowsServiceProcessInfo> TryGetWindowsServiceProcessInfo(const std::wstring& serviceName) noexcept;

	// Running service PID, executable path, and logon account (QueryServiceConfig).
	std::optional<WindowsServiceVerificationInfo> TryGetWindowsServiceVerificationInfo(
		const std::wstring& serviceName) noexcept;

	// Built-in LocalSystem only: "LocalSystem" or "NT AUTHORITY\LocalSystem".
	bool ServiceStartNameIsLocalSystem(const std::wstring& serviceStartName) noexcept;

	// First token of a service lpBinaryPathName command line. Empty when unparsable.
	std::wstring ServiceExecutablePathFromCommandLine(const std::wstring& commandLine);

	bool TryGetNamedPipeServerProcessId(HANDLE hPipe, uint32_t& serverProcessId) noexcept;

	bool IsProcessRunningAsLocalSystem(uint32_t processId) noexcept;
}
