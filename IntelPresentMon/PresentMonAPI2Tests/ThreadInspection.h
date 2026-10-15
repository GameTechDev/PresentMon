#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/win/Handle.h"
#include <tlhelp32.h>
#include <cstdint>
#include <string_view>

namespace pmon::tests
{
	// opens the thread in process pid whose description (SetThreadDescription) matches name
	// returns an empty handle if no such thread is found
	inline util::win::Handle FindThreadByName(uint32_t pid, std::wstring_view name)
	{
		util::win::Handle snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0) };
		if (!snapshot || snapshot.Get() == INVALID_HANDLE_VALUE) {
			return {};
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
			return 0;
		}
		return (uint64_t)cycles;
	}
}
