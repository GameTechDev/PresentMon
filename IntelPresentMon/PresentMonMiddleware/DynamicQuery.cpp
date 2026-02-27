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
#include <string>
#include <format>
#include <sstream>

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

static std::string BuildPollSnapshotCsv_(const FrameMetricsSource::PollSnapshotData& snapshots)
{
	std::ostringstream os;
	os << "source,swap_chain_address,frame_id,present_qpc,display_qpc\n";

	for (const auto& ipcSnapshot : snapshots.ipcStoreSnapshots) {
		os << "ipc_store,"
			<< ipcSnapshot.swapChainAddress << ","
			<< ipcSnapshot.snapshot.frameId << ","
			<< ipcSnapshot.snapshot.presentQpc << ","
			<< ipcSnapshot.snapshot.displayQpc << "\n";
	}

	for (const auto& swapSnapshots : snapshots.swapChainSnapshots) {
		for (const auto& frameSnapshot : swapSnapshots.snapshots) {
			os << "swap_chain_frame_metrics,"
				<< swapSnapshots.swapChainAddress << ","
				<< frameSnapshot.frameId << ","
				<< frameSnapshot.presentQpc << ","
				<< frameSnapshot.displayQpc << "\n";
		}
	}

	return os.str();
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
		if (!cpuFrameTimeAvgOffset_.has_value() &&
			qel.metric == PM_METRIC_CPU_FRAME_TIME &&
			qel.stat == PM_STAT_AVG) {
			cpuFrameTimeAvgOffset_ = size_t(qel.dataOffset);
		}
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

bool PM_DYNAMIC_QUERY::HasZeroCpuFrameTimeAverage_(const uint8_t* pBlobBase) const
{
	if (pBlobBase == nullptr || !cpuFrameTimeAvgOffset_.has_value()) {
		return false;
	}

	const auto* pValue = reinterpret_cast<const double*>(pBlobBase + *cpuFrameTimeAvgOffset_);
	return *pValue == 0.0;
}

uint32_t PM_DYNAMIC_QUERY::Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
	uint64_t nowTimestamp, FrameMetricsSource* frameSource, uint32_t processId, uint32_t maxSwapChains) const
{
	if (pBlobBase == nullptr || maxSwapChains == 0) {
		return 0;
	}

	const auto window = GenerateQueryWindow_(nowTimestamp);

	// Validate any pending gaps from previous polls before polling this window.
	if (useIntegrityTracking_ && frameSource != nullptr) {
		for (auto it = swapToIntegrityState_.begin(); it != swapToIntegrityState_.end();) {
			auto& tracking = it->second;

			const SwapChainState* pSwapChain = frameSource->FindSwapChainState(it->first);
			// Skip validation until frame data is available.
			if (pSwapChain == nullptr || pSwapChain->Empty()) {
				++it;
				continue;
			}

			const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
			auto& gap = tracking.pendingGap;
			const uint64_t intervalStart = gap.lastPresentQpcOlderThanWindow;
			const uint64_t intervalEnd = gap.pollWindow.newest;
			bool retireGap = latestPresentQpc > intervalEnd;

			// Degenerate range: retire stale tracking state.
			if (intervalStart >= intervalEnd) {
				pmlog_warn("degenerate range").pmwatch(intervalStart).pmwatch(intervalEnd);
				retireGap = true;
			}
			else {
				const uint64_t searchStart = intervalStart + 1u;
				const uint64_t validationEnd = std::min(intervalEnd, latestPresentQpc);
				if (validationEnd >= searchStart) {
					const size_t lateFrameCountRaw = pSwapChain->CountInTimestampRange(searchStart, validationEnd);
					if (lateFrameCountRaw > gap.observedViolationFrameCount) {
						const size_t firstLateIndex = pSwapChain->LowerBoundIndex(searchStart);
						if (firstLateIndex < pSwapChain->Size()) {
							const uint64_t firstLateQpc = pSwapChain->At(firstLateIndex).presentStartQpc;
							if (firstLateQpc <= validationEnd) {
								const size_t lastLateExclusive = pSwapChain->UpperBoundIndex(validationEnd);
								if (lastLateExclusive != 0u) {
									const uint64_t newestViolatingPresentQpc = pSwapChain->At(lastLateExclusive - 1u).presentStartQpc;
									const bool hasNewViolatingFrame = !tracking.newestLoggedViolationPresentQpc.has_value() ||
										newestViolatingPresentQpc > *tracking.newestLoggedViolationPresentQpc;
									gap.observedViolationFrameCount = static_cast<uint32_t>(lateFrameCountRaw);
									if (hasNewViolatingFrame) {
										// Additional offset needed to place window edge on the first late-arriving frame.
										const double captureGapMs = double(intervalEnd - firstLateQpc) * qpcPeriodSeconds_ * 1000.0;
										const double elapsedSinceWindowPollMs = double(int64_t(nowTimestamp) - int64_t(gap.pollTimestampQpc)) * qpcPeriodSeconds_ * 1000.0;
										const uint32_t lateFrameCount = static_cast<uint32_t>(lateFrameCountRaw);
										std::string violatingFramesText;
										violatingFramesText.reserve(static_cast<size_t>(lateFrameCount) * 128u);
										uint32_t violatingFrameLineNumber = 1u;
										pSwapChain->ForEachInTimestampRange(searchStart, validationEnd,
											[&](const util::metrics::FrameMetrics& frame) {
												const auto qpcDeltaMs = [&](uint64_t referenceQpc, uint64_t sampleQpc) {
													return double(int64_t(referenceQpc) - int64_t(sampleQpc)) * qpcPeriodSeconds_ * 1000.0;
												};
												violatingFramesText += "\n    ";
												violatingFramesText += std::to_string(violatingFrameLineNumber++);
												violatingFramesText += ") [" + std::to_string(frame.frameId);
												violatingFramesText += "] ";
												violatingFramesText += std::format("pres_then={:.3f}, ", qpcDeltaMs(gap.pollTimestampQpc, frame.presentStartQpc));
												violatingFramesText += "disp_then=";
												if (frame.screenTimeQpc == 0u) {
													violatingFramesText += "NA";
												}
												else {
													violatingFramesText += std::format("{:.3f}", qpcDeltaMs(gap.pollTimestampQpc, frame.screenTimeQpc));
												}
											});
										tracking.newestLoggedViolationPresentQpc = newestViolatingPresentQpc;
										++gap.loggedViolationCount;
										pmlog_warn("Dynamic query stats window integrity violation detected")
											.pmwatch(processId)
											.pmwatch(windowOffsetMs_)
											.pmwatch(captureGapMs)
											.pmwatch(elapsedSinceWindowPollMs)
											.pmwatch(lateFrameCount)
											.pmwatch(gap.loggedViolationCount)
											.pmwatch(violatingFramesText)
											.diag();
									}
								}
							}
						}
					}
				}
			}

			if (retireGap) {
				it = swapToIntegrityState_.erase(it);
			}
			else {
				++it;
			}
		}
	}

	std::vector<uint64_t> swapChainAddresses;
	if (frameSource != nullptr) {
		swapChainAddresses = frameSource->GetSwapChainAddressesInTimestampRange(window.oldest, window.newest);
	}

	std::optional<FrameMetricsSource::PollSnapshotData> pollSnapshots;
	if (enableSnapshotDumpOnZeroCpuFrameTimeAverage_ && frameSource != nullptr && cpuFrameTimeAvgOffset_.has_value()) {
		pollSnapshots = frameSource->CapturePollSnapshotData();
	}

	std::vector<uint64_t> zeroCpuFrameTimeAvgSwapChains;
	zeroCpuFrameTimeAvgSwapChains.reserve(4u);

	auto dumpSnapshotsIfNeeded = [&]() {
		if (!pollSnapshots || zeroCpuFrameTimeAvgSwapChains.empty()) {
			return;
		}

		pmlog_warn("Dynamic query detected zero CPU frame time average and dumped poll snapshots")
			.pmwatch(processId)
			.pmwatch(nowTimestamp)
			.pmwatch(zeroCpuFrameTimeAvgSwapChains.size())
			.pmwatch(pollSnapshots->ipcStoreSnapshots.size())
			.pmwatch(pollSnapshots->swapChainSnapshots.size())
			.watch("poll_snapshot_csv", BuildPollSnapshotCsv_(*pollSnapshots))
			.diag();
	};

	auto pollOnce = [&](const SwapChainState* pSwapChain, uint64_t swapChainAddress, uint8_t* pBlob) {
		if (useIntegrityTracking_ && pSwapChain != nullptr && !pSwapChain->Empty()) {
			// Persist a new pending gap for this poll when the window extends beyond latest known data.
			const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
			if (latestPresentQpc < window.newest) {
				swapToIntegrityState_.try_emplace(swapChainAddress, IntegrityTrackingState_{
					.pendingGap = PendingIntegrityGap_{
						.lastPresentQpcOlderThanWindow = latestPresentQpc,
						.pollWindow = window,
						.pollTimestampQpc = nowTimestamp,
					},
				});
			}
		}
		for (auto& pRing : ringMetricPtrs_) {
			pRing->Poll(window, pBlob, comms, pSwapChain, processId);
		}
		if (enableSnapshotDumpOnZeroCpuFrameTimeAverage_ && cpuFrameTimeAvgOffset_.has_value() &&
			HasZeroCpuFrameTimeAverage_(pBlob)) {
			zeroCpuFrameTimeAvgSwapChains.push_back(swapChainAddress);
		}
	};

	if (swapChainAddresses.empty()) {
		pollOnce(nullptr, 0, pBlobBase);
		dumpSnapshotsIfNeeded();
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

	dumpSnapshotsIfNeeded();

	return swapChainsToPoll;
}
