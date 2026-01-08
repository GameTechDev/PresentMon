#include "Level.h"
#include "../str/String.h"
#include "../ref/WrapReflect.h"

using namespace std::literals;

namespace pmon::util::log
{
	std::string GetLevelName(Level lv) noexcept
	{
		return std::string{ reflect::enum_name(lv) };
	}

	std::map<std::string, Level> GetLevelMapNarrow() noexcept
	{
		using namespace pmon::util::str;
		std::map<std::string, Level> map;
		for (int n = 0; n < (int)Level::EndOfEnumKeys; n++) {
			const auto lvl = Level(n);
			auto key = ToLower(GetLevelName(lvl));
			if (!key.starts_with("level")) {
				map[std::move(key)] = lvl;
			}
		}
		return map;
	}
}