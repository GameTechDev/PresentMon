#pragma once
#include <vector>
#include <bitset>
#include <map>
#include <span>
#include <optional>
#include <memory>
#include <unordered_map>
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
	bool HasFrameMetrics() const;
	uint32_t Poll(uint8_t* pBlobBase, pmon::ipc::MiddlewareComms& comms,
		uint64_t nowTimestamp, pmon::mid::FrameMetricsSource* frameSource, uint32_t processId, uint32_t maxSwapChains) const;

private:
	struct PendingIntegrityGap_
	{
		uint64_t lastPresentQpcOlderThanWindow = 0;
		pmon::mid::DynamicQueryWindow pollWindow;
		uint64_t pollTimestampQpc = 0;
		uint32_t observedViolationFrameCount = 0;
		uint32_t loggedViolationCount = 0;
	};

	struct IntegrityTrackingState_
	{
		std::optional<PendingIntegrityGap_> pendingGap;
		std::optional<uint64_t> newestLoggedViolationPresentQpc;
	};

	// functions
	pmon::mid::DynamicQueryWindow GenerateQueryWindow_(int64_t nowTimestamp) const;
	// data
	std::vector<std::unique_ptr<pmon::mid::MetricBinding>> ringMetricPtrs_;
	size_t blobSize_;
	bool hasFrameMetrics_ = false;
	// window parameters; these could theoretically be independent of query but current API couples them
	double windowOffsetMs_ = 0.0;
	double qpcPeriodSeconds_ = 0.0;
	int64_t windowSizeQpc_ = 0;
	int64_t windowOffsetQpc_ = 0;
	// window integrity validation data
	bool useIntegrityTracking_ = true;
	mutable std::unordered_map<uint64_t, IntegrityTrackingState_> swapToIntegrityState_;
};

