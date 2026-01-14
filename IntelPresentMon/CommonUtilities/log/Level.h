#pragma once
#include <string>
#include <map>

namespace pmon::util::log
{
	enum class Level : uint16_t
	{
		None = 0,
		Fatal = 10,
		Error = 20,
		Warning = 30,
		Info = 40,
		Performance = 50,
		Debug = 60,
		Verbose = 70,
		Verbose2 = 80,
		EndOfEnumKeys,
	};

	std::string GetLevelName(Level);
	std::map<std::string, Level> GetLevelMapNarrow();
}
