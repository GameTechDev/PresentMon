#include "PresentMonAPIVersion.h"
#include "BuildId.h"
#include <cstring>
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::bid
{
	PM_VERSION GetApiVersion() noexcept
	{
		PM_VERSION ver{
			.major = PM_API_VERSION_MAJOR,
			.minor = PM_API_VERSION_MINOR,
			.patch = 0,
			.tag = "",
		};
		strncpy_s(ver.hash, BuildIdShortHash(), _TRUNCATE);
		strncpy_s(ver.config, BuildIdConfig(), _TRUNCATE);
		return ver;
	}
}