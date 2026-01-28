#pragma once
#include <vector>
#include <bitset>
#include <map>
#include <span>
#include <optional>
#include <memory>
#include "MetricBinding.h"
#include "DynamicQueryWindow.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../CommonUtilities/Qpc.h"


namespace pmon::mid
{
	class MetricBinding;
	class FrameMetricsSource;
}

namespace pmon::ipc
{
	class MiddlewareComms;
}

struct PM_DYNAMIC_QUERY
{
public:
	PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, double windowSizeMs, double windowOffsetMs,
		double qpcPeriodSeconds, pmon::ipc::MiddlewareComms& comms, pmon::mid::Middleware& middleware);
	size_t GetBlobSize() const;
	uint32_t Poll(uint8_t* pBlobBase, pmon::ipc::MiddlewareComms& comms,
		uint64_t nowTimestamp, pmon::mid::FrameMetricsSource* frameSource, uint32_t processId, uint32_t maxSwapChains) const;

private:
	// functions
	pmon::mid::DynamicQueryWindow GenerateQueryWindow_(int64_t nowTimestamp) const;
	// data
	std::vector<std::unique_ptr<pmon::mid::MetricBinding>> ringMetricPtrs_;
	size_t blobSize_;
	// window parameters; these could theoretically be independent of query but current API couples them
	int64_t windowSizeQpc_ = 0;
	int64_t windowOffsetQpc_ = 0;
};

