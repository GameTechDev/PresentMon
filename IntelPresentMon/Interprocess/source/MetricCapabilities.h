#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <optional>
#include <unordered_map>
#include <string>

namespace pmon::ipc
{
	struct MetricCapabilityEntry
	{
		size_t arraySize = 0;
		PM_METRIC_AVAILABILITY availability = PM_METRIC_AVAILABILITY_UNAVAILABLE;
	};

	// capabilities typically communicated from telemetry providers to be collected in here
	// used to determine which metrics available for queries, which ShmRings need to be
	// allocated elements, etc.
	class MetricCapabilities
	{
	public:
		void Set(PM_METRIC metricId, size_t arraySize,
			PM_METRIC_AVAILABILITY availability = PM_METRIC_AVAILABILITY_AVAILABLE);
		void Merge(const MetricCapabilities& capsToMerge);
		size_t Check(PM_METRIC metricId) const noexcept;
		std::optional<MetricCapabilityEntry> Lookup(PM_METRIC metricId) const noexcept;
		std::string ToString(size_t indentSpaces = 0) const;
		auto begin() const noexcept
		{
			return caps_.begin();
		}
		auto end() const noexcept
		{
			return caps_.end();
		}
	private:
		std::unordered_map<PM_METRIC, MetricCapabilityEntry> caps_;
	};
}
