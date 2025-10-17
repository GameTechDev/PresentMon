#pragma once
#include <string>
#include <filesystem>
#include <optional>
#include "WinAPI.h"
#include "Handle.h"

namespace pmon::util::win
{
	// convert WinAPI HRESULT code to human-readable string
	std::string GetErrorDescription(HRESULT hr) noexcept;
	// convert Structured Exception Handling error code to human-readable string
	std::string GetSEHSymbol(DWORD sehCode) noexcept;
	// returns true of the named pipe is available for connection
	bool WaitForNamedPipe(const std::string& fullname, int timeoutMs);
	// open a process
	win::Handle OpenProcess(uint32_t pid, UINT permissions = PROCESS_QUERY_LIMITED_INFORMATION);
	// gets the full path to the current process's module
	std::filesystem::path GetExecutableModulePath();
	// gets the full path to a process's module
	std::filesystem::path GetExecutableModulePath(HANDLE hProc);
	// gets the full path to a process's module
	std::filesystem::path GetExecutableModulePathFromPid(uint32_t pid);
	// checks whether a process is 32-bit or 64-bit
	bool ProcessIs32Bit(HANDLE hProc);
	// convert a guid to string
	std::wstring GuidToString(const GUID& guid);
}