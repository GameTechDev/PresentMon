#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/win/Handle.h"
#include <tlhelp32.h>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

namespace pmon::tests
{
	// opens the thread in process pid whose description (SetThreadDescription) matches name
	// returns an empty handle if no such thread is found; throws if the thread list cannot be captured
	inline util::win::Handle FindThreadByName(uint32_t pid, std::wstring_view name)
	{
		util::win::Handle snapshot;
		// snapshot creation can transiently fail with ERROR_BAD_LENGTH while threads are being created
		for (int attempt = 0; attempt < 20; attempt++) {
			const auto hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (hSnap != INVALID_HANDLE_VALUE) {
				snapshot = util::win::Handle{ hSnap };
				break;
			}
			if (GetLastError() != ERROR_BAD_LENGTH) {
				break;
			}
		}
		if (!snapshot) {
			throw std::runtime_error{ std::format("Failed to snapshot threads, error {}", GetLastError()) };
		}
		THREADENTRY32 entry{ .dwSize = sizeof(THREADENTRY32) };
		for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry)) {
			if (entry.th32OwnerProcessID != pid) {
				continue;
			}
			util::win::Handle hThread{ OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ThreadID) };
			if (!hThread) {
				continue;
			}
			PWSTR pDescription = nullptr;
			if (SUCCEEDED(GetThreadDescription(hThread, &pDescription))) {
				const bool match = pDescription && name == pDescription;
				LocalFree(pDescription);
				if (match) {
					return hThread;
				}
			}
		}
		return {};
	}

	// total cpu cycles the thread has spent executing; does not advance while the thread is blocked
	inline uint64_t GetThreadCycles(const util::win::Handle& hThread)
	{
		ULONG64 cycles = 0;
		if (!QueryThreadCycleTime(hThread, &cycles)) {
			throw std::runtime_error{ std::format("Failed to query thread cycle time, error {}", GetLastError()) };
		}
		return (uint64_t)cycles;
	}

	inline bool IsThreadRunning(const util::win::Handle& hThread)
	{
		DWORD exitCode = 0;
		if (!GetExitCodeThread(hThread, &exitCode)) {
			throw std::runtime_error{ std::format("Failed to query thread exit code, error {}", GetLastError()) };
		}
		return exitCode == STILL_ACTIVE;
	}
}
