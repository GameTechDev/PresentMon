#include "Memory.h"
#include "win/WinAPI.h"
#include "log/Log.h"

namespace pmon::util
{
	void LocalFree(void* p) noexcept
	{
		if (::LocalFree(p) != NULL) {
			pmlog_warn("Failed to free memory via LocalFree");
		}
	}
}