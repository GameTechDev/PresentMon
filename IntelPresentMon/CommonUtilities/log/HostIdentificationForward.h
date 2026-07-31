#pragma once
#include "IdentificationTable.h"
#include <string>

namespace pmon::util::log
{
	// Must run in the host module that owns pTable (/MT heap). Middleware invokes these
	// via function pointers when linking identification tables across module boundaries.
	inline void ForwardAddThreadToHostIdTable(IdentificationTable* pTable, uint32_t tid, uint32_t pid,
		const char* name, size_t nameLen) noexcept
	{
		if (!pTable) {
			return;
		}
		try {
			pTable->AddThread_(tid, pid, std::string{ name ? name : "", nameLen });
		}
		catch (...) {
		}
	}

	inline void ForwardAddProcessToHostIdTable(IdentificationTable* pTable, uint32_t pid,
		const char* name, size_t nameLen) noexcept
	{
		if (!pTable) {
			return;
		}
		try {
			pTable->AddProcess_(pid, std::string{ name ? name : "", nameLen });
		}
		catch (...) {
		}
	}
}
