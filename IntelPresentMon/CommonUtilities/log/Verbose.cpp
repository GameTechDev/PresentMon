#include "Verbose.h"
#include "../str/String.h"
#include "../ref/WrapReflect.h"

namespace pmon::util::log
{
	std::string GetVerboseModuleName(V mod)
	{
		return std::string{ reflect::enum_name<V, "Unknown">(mod) };
	}

	std::map<std::string, V> GetVerboseModuleMapNarrow()
	{
		using namespace pmon::util::str;
		std::map<std::string, V> map;
		for (int n = 0; n <= (int)V::Count; n++) {
			const auto lvl = V(n);
			auto key = ToLower(GetVerboseModuleName(lvl));
		}
		return map;
	}
}