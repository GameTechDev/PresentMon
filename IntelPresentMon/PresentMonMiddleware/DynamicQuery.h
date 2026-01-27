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


namespace pmon::mid
{
	class RingMetricBinding;
}

namespace pmon::ipc
{
	class MiddlewareComms;
}

struct PM_DYNAMIC_QUERY
{
public:
	PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, pmon::ipc::MiddlewareComms& comms);
	size_t GetBlobSize() const;
	void Poll(uint8_t* pBlobBase, pmon::ipc::MiddlewareComms& comms,
		uint64_t nowTimestamp, pmon::mid::FrameMetricsSource* frameSource) const;

private:
	// functions
	pmon::mid::DynamicQueryWindow GenerateQueryWindow_(uint64_t nowTimestamp) const;
	// data
	std::vector<std::unique_ptr<pmon::mid::RingMetricBinding>> ringMetricPtrs_;
	size_t blobSize_;
	// window parameters; these could theoretically be independent of query but current API couples them
	double windowSizeMs_ = 0;
	double metricOffsetMs_ = 0.;
};

