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
		std::wstring binaryPath;
		std::wstring serviceStartName;
	};

	// Returns nullopt if the service is not registered, not running, or query fails.
	std::optional<WindowsServiceProcessInfo> TryGetWindowsServiceProcessInfo(const std::wstring& serviceName) noexcept;

	// Running service PID plus configured binary path and logon account (QueryServiceConfig).
	std::optional<WindowsServiceVerificationInfo> TryGetWindowsServiceVerificationInfo(
		const std::wstring& serviceName) noexcept;

	bool ServiceStartNameIsLocalSystem(const std::wstring& serviceStartName) noexcept;

	bool TryGetNamedPipeServerProcessId(HANDLE hPipe, uint32_t& serverProcessId) noexcept;

	bool IsProcessRunningAsLocalSystem(uint32_t processId) noexcept;
}
