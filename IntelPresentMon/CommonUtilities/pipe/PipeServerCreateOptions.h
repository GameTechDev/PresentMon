#pragma once
#include "../win/WinAPI.h"

namespace pmon::util::pipe
{
	struct PipeServerCreateOptions
	{
		uint32_t maxInstances = PIPE_UNLIMITED_INSTANCES;
		bool serviceControlHardening = false;
	};
}
