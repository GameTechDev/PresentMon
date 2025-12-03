#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <unordered_map>

namespace pmon::ipc
{
	// capabilities typically communicated from telemetry providers to be collected in here
	// used to determine which metrics available for queries, which ShmRings need to be
	// allocated elements, etc.
	class MetricCapabilities
	{
	public:
		void Set(PM_METRIC metricId, size_t arraySize);
		void Merge(const MetricCapabilities& capsToMerge);
		size_t Check(PM_METRIC metricId) const noexcept;
		auto begin() const noexcept
		{
			return caps_.begin();
		}
		auto end() const noexcept
		{
			return caps_.end();
		}
	private:
		std::unordered_map<PM_METRIC, size_t> caps_;
	};
}