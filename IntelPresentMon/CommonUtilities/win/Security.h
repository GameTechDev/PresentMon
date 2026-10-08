#pragma once
#include "WinAPI.h"
#include "../Memory.h"
#include <string>

namespace pmon::util::win
{
	UniqueLocalPtr<void> MakeSecurityDescriptor(const std::string& desc);
	std::string GetCurrentProcessUserSidString();
}