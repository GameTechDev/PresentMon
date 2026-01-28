#include "DynamicQuery.h"
#include "FrameMetricsSource.h"
#include "QueryValidation.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/Interprocess.h"
#include "../CommonUtilities/Hash.h"
#include <unordered_map>
#include <algorithm>
#include <limits>

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

	// make sure blob sizes are multiple of 16 bytes for blob array alignment purposes
	blobSize_ = util::PadToAlignment(blobCursor, 16u);
}

size_t PM_DYNAMIC_QUERY::GetBlobSize() const
{
	return blobSize_;
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
	std::vector<uint64_t> swapChainAddresses;
	if (frameSource != nullptr) {
		swapChainAddresses = frameSource->GetSwapChainAddressesInTimestampRange(window.oldest, window.newest);
	}

	auto pollOnce = [&](const SwapChainState* pSwapChain, uint8_t* pBlob) {
		for (auto& pRing : ringMetricPtrs_) {
			pRing->Poll(window, pBlob, comms, pSwapChain, processId);
		}
	};

	if (swapChainAddresses.empty()) {
		pollOnce(nullptr, pBlobBase);
		return 1;
	}

	const uint32_t swapChainsToPoll = (uint32_t)std::min<size_t>(swapChainAddresses.size(), maxSwapChains);
	for (uint32_t i = 0; i < swapChainsToPoll; ++i) {
		const SwapChainState* pSwapChain = frameSource != nullptr
			? frameSource->FindSwapChainState(swapChainAddresses[i])
			: nullptr;
		pollOnce(pSwapChain, pBlobBase);
		pBlobBase += blobSize_;
	}

	return swapChainsToPoll;
}
