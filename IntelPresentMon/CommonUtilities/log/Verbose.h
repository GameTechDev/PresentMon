#pragma once
#include <string>
#include <map>

namespace pmon::util::log
{
	enum class V
	{
		v8async,
		procwatch,
		tele_gpu,
		core_metric,
		core_hotkey,
		core_window,
		etwq,
		Count
	};

	std::string GetVerboseModuleName(V mod) noexcept;
	std::map<std::string, V> GetVerboseModuleMapNarrow() noexcept;
}