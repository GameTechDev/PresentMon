#pragma once
#include "WinAPI.h"
#include "../Memory.h"
#include <cstdint>
#include <string>

namespace pmon::util::win
{
	UniqueLocalPtr<void> MakeSecurityDescriptor(const std::string& desc);
	// string form (S-1-...) of the user SID in the primary token of process pid
	std::string GetProcessUserSid(uint32_t pid);
}