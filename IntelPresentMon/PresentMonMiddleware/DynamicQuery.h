#pragma once
#include <vector>
#include <bitset>
#include <map>
#include <span>
#include <optional>
#include <memory>
#include "RingMetricBinding.h"
#include "DynamicQueryWindow.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../ControlLib/CpuTelemetryInfo.h"
#include "../ControlLib/PresentMonPowerTelemetry.h"


namespace pmon::mid
{
	class Middleware;
	class RingMetricBinding;
}

namespace pmon::ipc
{
	class MiddlewareComms;
	class TelemetryMap;
}

namespace pmon::mid::todo
{
	struct PM_DYNAMIC_QUERY
	{
	public:
		PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, ipc::MiddlewareComms& comms);
		size_t GetBlobSize() const;
		void Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms, std::optional<uint32_t> pid, uint64_t nowTimestamp) const;

	private:
		// functions
		DynamicQueryWindow GenerateQueryWindow_(uint64_t nowTimestamp) const;
		// data
		std::vector<std::unique_ptr<RingMetricBinding>> ringMetricPtrs_;
		size_t blobSize_;
		// window parameters; these could theoretically be independent of query but current API couples them
		double windowSizeMs_ = 0;
		double metricOffsetMs_ = 0.;
	};
}

// TODO: legacy: to be deleted
struct PM_DYNAMIC_QUERY
{
	std::vector<PM_QUERY_ELEMENT> elements;
	size_t GetBlobSize() const
	{
		return elements.back().dataOffset + elements.back().dataSize;
	}
	// Data used to track what should be accumulated
	bool accumFpsData = false;
	std::bitset<static_cast<size_t>(GpuTelemetryCapBits::gpu_telemetry_count)> accumGpuBits;
	std::bitset<static_cast<size_t>(CpuTelemetryCapBits::cpu_telemetry_count)> accumCpuBits;
	// Data used to calculate the requested metrics
	double windowSizeMs = 0;
	double metricOffsetMs = 0.;
	size_t queryCacheSize = 0;
	std::optional<uint32_t> cachedGpuInfoIndex;
};

