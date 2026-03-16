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
		uci,
		core_metric,
		core_hotkey,
		core_window,
		etwq,
		kact,
		ipc_sto,
		ipc_ring,
		met_use,
		middleware,
		chrome,
		Count
	};

	std::string GetVerboseModuleName(V mod);
	std::map<std::string, V> GetVerboseModuleMapNarrow();
}
