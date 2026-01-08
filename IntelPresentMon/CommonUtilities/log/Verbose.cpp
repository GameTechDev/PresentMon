#include "Verbose.h"
#include "../str/String.h"
#include "../ref/WrapReflect.h"

namespace pmon::util::log
{
	std::string GetVerboseModuleName(V mod) noexcept
	{
		return std::string{ reflect::enum_name(mod) };
	}

	std::map<std::string, V> GetVerboseModuleMapNarrow() noexcept
	{
		using namespace pmon::util::str;
		std::map<std::string, V> map;
		for (int n = 0; n <= (int)V::Count; n++) {
			const auto lvl = V(n);
			auto name = ToLower(GetVerboseModuleName(lvl));
			map[name] = lvl;
		}
		return map;
	}
}