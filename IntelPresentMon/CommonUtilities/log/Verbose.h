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
		kact,
		ipc_sto,
		ipc_ring,
		met_use,
		Count
	};

	std::string GetVerboseModuleName(V mod);
	std::map<std::string, V> GetVerboseModuleMapNarrow();
}
