#include "DynamicQuery.h"
#include "FrameMetricsSource.h"
#include "QueryValidation.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/Interprocess.h"
#include "../CommonUtilities/Hash.h"
#include "../CommonUtilities/log/Log.h"
#include <unordered_map>
#include <algorithm>

using namespace pmon;
using namespace mid;

namespace
{
	struct TelemetryBindingKey_
	{
		uint32_t deviceId;
		PM_METRIC metric;
		uint32_t arrayIndex;

		bool operator==(const TelemetryBindingKey_& other) const noexcept
		{
			return deviceId == other.deviceId &&
				metric == other.metric &&
				arrayIndex == other.arrayIndex;
		}
	};
}

namespace std
{
	template<>
	struct hash<TelemetryBindingKey_>
	{
		size_t operator()(const TelemetryBindingKey_& key) const noexcept
		{
			const size_t h0 = std::hash<uint32_t>{}(key.deviceId);
			const size_t h1 = std::hash<uint32_t>{}((uint32_t)key.metric);
			const size_t h2 = std::hash<uint32_t>{}(key.arrayIndex);
			return pmon::util::hash::HashCombine(pmon::util::hash::HashCombine(h0, h1), h2);
		}
	};
}

PM_DYNAMIC_QUERY::PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, double windowSizeMs,
	double windowOffsetMs, double qpcPeriodSeconds, ipc::MiddlewareComms& comms, pmon::mid::Middleware& middleware)
	:
	windowOffsetMs_{ windowOffsetMs },
	qpcPeriodSeconds_{ qpcPeriodSeconds },
	windowSizeQpc_{ int64_t((windowSizeMs / 1000.) / qpcPeriodSeconds) },
	windowOffsetQpc_{ int64_t((windowOffsetMs / 1000.) / qpcPeriodSeconds) }
{
	const auto* introBase = comms.GetIntrospectionRoot();
	pmapi::intro::Root introRoot{ introBase, [](const PM_INTROSPECTION_ROOT*) {} };
	pmon::mid::ValidateQueryElements(qels, PM_METRIC_TYPE_DYNAMIC, introRoot, comms);

	std::unordered_map<TelemetryBindingKey_, MetricBinding*> telemetryBindings;
	MetricBinding* frameBinding = nullptr;

	size_t blobCursor = 0;
	for (auto& qel : qels) {
		MetricBinding* binding = nullptr;
		const auto metricView = introRoot.FindMetric(qel.metric);
		const auto metricType = metricView.GetType();
		const bool isStaticMetric = metricType == PM_METRIC_TYPE_STATIC;
		if (isStaticMetric) {
			auto bindingPtr = MakeStaticMetricBinding(qel, middleware);
			binding = bindingPtr.get();
			ringMetricPtrs_.push_back(std::move(bindingPtr));
		}
		else if (qel.deviceId == ipc::kUniversalDeviceId) {
			binding = frameBinding;
			if (!binding) {
				auto bindingPtr = MakeFrameMetricBinding(qel);
				binding = bindingPtr.get();
				frameBinding = bindingPtr.get();
				ringMetricPtrs_.push_back(std::move(bindingPtr));
			}
		}
		else {
			const TelemetryBindingKey_ key{
				.deviceId = qel.deviceId,
				.metric = qel.metric,
				.arrayIndex = qel.arrayIndex,
			};
			if (auto it = telemetryBindings.find(key); it != telemetryBindings.end()) {
				binding = it->second;
			}
			else {
				auto bindingPtr = MakeTelemetryMetricBinding(qel, introRoot);
				binding = bindingPtr.get();
				ringMetricPtrs_.push_back(std::move(bindingPtr));
				telemetryBindings.emplace(key, binding);
			}
		}

		qel.dataOffset = blobCursor;
		binding->AddMetricStat(qel, introRoot);
		blobCursor = qel.dataOffset + qel.dataSize;
	}

	for (auto& binding : ringMetricPtrs_) {
		binding->Finalize();
	}

	hasFrameMetrics_ = frameBinding != nullptr;

	// make sure blob sizes are multiple of 16 bytes for blob array alignment purposes
	blobSize_ = util::PadToAlignment(blobCursor, 16u);
}

size_t PM_DYNAMIC_QUERY::GetBlobSize() const
{
	return blobSize_;
}

bool PM_DYNAMIC_QUERY::HasFrameMetrics() const
{
	return hasFrameMetrics_;
}

DynamicQueryWindow PM_DYNAMIC_QUERY::GenerateQueryWindow_(int64_t nowTimestamp) const
{
	const auto newest = nowTimestamp - windowOffsetQpc_;
	const auto oldest = newest - windowSizeQpc_;
	return { .oldest = uint64_t(oldest), .newest = uint64_t(newest)};
}

uint32_t PM_DYNAMIC_QUERY::Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
	uint64_t nowTimestamp, FrameMetricsSource* frameSource, uint32_t processId, uint32_t maxSwapChains) const
{
	if (pBlobBase == nullptr || maxSwapChains == 0) {
		return 0;
	}

	const auto window = GenerateQueryWindow_(nowTimestamp);

	// Validate any carry-over gap from the previous poll before polling this window.
	if (useIntegrityTracking_ && frameSource != nullptr) {
		for (auto& entry : swapToIntegrityState_) {
			auto& tracking = entry.second;
			// No gap pending for this swap chain.
			if (!tracking.lastPresentQpcOlderThanWindow.has_value()) {
				continue;
			}

			const SwapChainState* pSwapChain = frameSource->FindSwapChainState(entry.first);
			// Skip validation until frame data is available.
			if (pSwapChain == nullptr || pSwapChain->Empty()) {
				continue;
			}

			const uint64_t intervalStart = *tracking.lastPresentQpcOlderThanWindow;
			const uint64_t intervalEnd = tracking.windowNewestEdgeQpc;
			// Degenerate range: clear stale tracking state.
			if (intervalStart >= intervalEnd) {
				tracking.lastPresentQpcOlderThanWindow = std::nullopt;
				tracking.windowNewestEdgeQpc = 0;
				pmlog_warn("degenerate range").pmwatch(intervalStart).pmwatch(intervalEnd);
				continue;
			}

			// newest frame in swap tells us whether we can retire current gap
			const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
			// search for first (oldest) frame in swap AFTER the gap start, to validate violation(s)
			const uint64_t searchStart = intervalStart + 1u;
			const size_t firstLateIndex = pSwapChain->LowerBoundIndex(searchStart);

			// Defer judgement until the capture has advanced beyond the old window edge.
			if (latestPresentQpc <= intervalEnd) {
				// New frame(s) inside the old gap are a violation; move the start up to shrink the gap.
				if (latestPresentQpc > intervalStart && firstLateIndex < pSwapChain->Size()) {
					const uint64_t firstLateQpc = pSwapChain->At(firstLateIndex).presentStartQpc;
					const size_t lateFrameCount = pSwapChain->CountInTimestampRange(searchStart, latestPresentQpc);
					if (lateFrameCount > 0 && firstLateQpc <= latestPresentQpc) {
						// Additional offset needed to place window edge on the first late-arriving frame.
						const double captureGapMs = double(intervalEnd - firstLateQpc) * qpcPeriodSeconds_ * 1000.0;
						pmlog_warn("Dynamic query stats window integrity violation detected")
							.pmwatch(processId)
							.pmwatch(windowOffsetMs_)
							.pmwatch(captureGapMs)
							.pmwatch(lateFrameCount)
							.diag();
					}
				}
				if (latestPresentQpc > intervalStart) {
					tracking.lastPresentQpcOlderThanWindow = latestPresentQpc;
				}

				// Keep extending the pending gap to the newest edge of the current window.
				tracking.windowNewestEdgeQpc = std::max(tracking.windowNewestEdgeQpc, window.newest);
				continue;
			}

			// A frame past the old window edge exists, so the previous gap can be judged definitively.
			if (firstLateIndex < pSwapChain->Size()) {
				const uint64_t firstLateQpc = pSwapChain->At(firstLateIndex).presentStartQpc;
				if (firstLateQpc <= intervalEnd) {
					const size_t lateFrameCount = pSwapChain->CountInTimestampRange(searchStart, intervalEnd);
					if (lateFrameCount > 0) {
						// Additional offset needed to place window edge on the first late-arriving frame.
						const double captureGapMs = double(intervalEnd - firstLateQpc) * qpcPeriodSeconds_ * 1000.0;
						pmlog_warn("Dynamic query stats window integrity violation detected")
							.pmwatch(processId)
							.pmwatch(windowOffsetMs_)
							.pmwatch(captureGapMs)
							.pmwatch(lateFrameCount)
							.diag();
					}
				}
			}

			// Previous window check is complete for this swap chain.
			tracking.lastPresentQpcOlderThanWindow = std::nullopt;
			tracking.windowNewestEdgeQpc = 0;
		}
	}

	std::vector<uint64_t> swapChainAddresses;
	if (frameSource != nullptr) {
		swapChainAddresses = frameSource->GetSwapChainAddressesInTimestampRange(window.oldest, window.newest);
	}

	auto pollOnce = [&](const SwapChainState* pSwapChain, uint64_t swapChainAddress, uint8_t* pBlob) {
		if (useIntegrityTracking_ && pSwapChain != nullptr && !pSwapChain->Empty()) {
			// Track the latest known present and whether the window extends beyond it.
			const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
			auto& tracking = swapToIntegrityState_[swapChainAddress];
			if (latestPresentQpc < window.newest) {
				// Start or refresh pending validation for the open gap [latest+1, window.newest].
				tracking.lastPresentQpcOlderThanWindow = latestPresentQpc;
				tracking.windowNewestEdgeQpc = window.newest;
			}
			else {
				// No gap remains when we already have data at or beyond the window edge.
				tracking.lastPresentQpcOlderThanWindow = std::nullopt;
				tracking.windowNewestEdgeQpc = 0;
			}
		}
		for (auto& pRing : ringMetricPtrs_) {
			pRing->Poll(window, pBlob, comms, pSwapChain, processId);
		}
	};

	if (swapChainAddresses.empty()) {
		pollOnce(nullptr, 0, pBlobBase);
		return 1;
	}

	const uint32_t swapChainsToPoll = (uint32_t)std::min<size_t>(swapChainAddresses.size(), maxSwapChains);
	for (uint32_t i = 0; i < swapChainsToPoll; ++i) {
		const uint64_t swapChainAddress = swapChainAddresses[i];
		const SwapChainState* pSwapChain = frameSource != nullptr
			? frameSource->FindSwapChainState(swapChainAddress)
			: nullptr;
		pollOnce(pSwapChain, swapChainAddress, pBlobBase);
		pBlobBase += blobSize_;
	}

	return swapChainsToPoll;
}
