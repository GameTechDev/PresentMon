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
#include <cstdint>

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

static uint64_t GetTargetStartQpc_(ipc::MiddlewareComms& comms, uint32_t processId)
{
	const int64_t startQpcSigned = comms.GetFrameDataStore(processId).bookkeeping.startQpc;
	return startQpcSigned > 0 ? static_cast<uint64_t>(startQpcSigned) : 0u;
}

static std::string BuildElapsedSinceTargetStartText_(uint64_t targetStartQpc, uint64_t nowTimestamp, double qpcPeriodSeconds)
{
	if (targetStartQpc == 0u || nowTimestamp == 0u) {
		return "NA";
	}

	const double elapsedSeconds = double(int64_t(nowTimestamp) - int64_t(targetStartQpc)) * qpcPeriodSeconds;
	return std::format("{:.4f}", elapsedSeconds);
}

static std::string BuildRelativeMillisecondsText_(uint64_t referenceQpc, uint64_t sampleQpc, double qpcPeriodSeconds)
{
	const double deltaMs = double(int64_t(referenceQpc) - int64_t(sampleQpc)) * qpcPeriodSeconds * 1000.0;
	return std::format("{:.4f}", deltaMs);
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

void PM_DYNAMIC_QUERY::ValidatePendingIntegrityWindows_(FrameMetricsSource* frameSource,
	ipc::MiddlewareComms& comms,
	uint32_t processId, uint64_t nowTimestamp) const
{
	if (frameSource == nullptr) {
		return;
	}

	const uint64_t targetStartQpc = GetTargetStartQpc_(comms, processId);
	const std::string elapsedSinceTargetStartSecondsText =
		BuildElapsedSinceTargetStartText_(targetStartQpc, nowTimestamp, qpcPeriodSeconds_);

	for (auto it = swapToIntegrityState_.begin(); it != swapToIntegrityState_.end();) {
		const uint64_t swapChainAddress = it->first;
		auto& tracking = it->second;
		if (tracking.pendingWindows.empty()) {
			it = swapToIntegrityState_.erase(it);
			continue;
		}

		const SwapChainState* pSwapChain = frameSource->FindSwapChainState(swapChainAddress);
		// Skip validation until frame data is available for this swap chain.
		if (pSwapChain == nullptr || pSwapChain->Empty()) {
			++it;
			continue;
		}

		const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
		while (!tracking.pendingWindows.empty() &&
			latestPresentQpc > tracking.pendingWindows.front().pollWindow.newest) {
			const auto pendingWindow = tracking.pendingWindows.front();
			tracking.pendingWindows.pop_front();

			const uint64_t intervalStart = pendingWindow.lastPresentQpcOlderThanWindow;
			const uint64_t intervalEnd = pendingWindow.pollWindow.newest;
			if (intervalStart >= intervalEnd) {
				continue;
			}

			const uint64_t searchStart = intervalStart + 1u;
			const size_t lateFrameCountRaw = pSwapChain->CountInTimestampRange(searchStart, intervalEnd);
			if (lateFrameCountRaw == 0u) {
				continue;
			}

			const size_t firstLateIndex = pSwapChain->LowerBoundIndex(searchStart);
			if (firstLateIndex >= pSwapChain->Size()) {
				continue;
			}

			const uint64_t firstLateQpc = pSwapChain->At(firstLateIndex).presentStartQpc;
			if (firstLateQpc > intervalEnd) {
				continue;
			}

			// Additional offset needed to place window edge on the first late-arriving frame.
			const double captureGapMs = double(intervalEnd - firstLateQpc) * qpcPeriodSeconds_ * 1000.0;
			const double elapsedSinceWindowPollMs = double(int64_t(nowTimestamp) - int64_t(pendingWindow.pollTimestampQpc)) * qpcPeriodSeconds_ * 1000.0;
			const uint32_t lateFrameCount = static_cast<uint32_t>(lateFrameCountRaw);
			std::string violatingFramesText;
			violatingFramesText.reserve(static_cast<size_t>(lateFrameCount) * 128u);
			uint32_t violatingFrameLineNumber = 1u;
			pSwapChain->ForEachInTimestampRange(searchStart, intervalEnd,
				[&](const util::metrics::FrameMetrics& frame) {
					const auto qpcDeltaMs = [&](uint64_t referenceQpc, uint64_t sampleQpc) {
						return double(int64_t(referenceQpc) - int64_t(sampleQpc)) * qpcPeriodSeconds_ * 1000.0;
					};
					violatingFramesText += "\n    ";
					violatingFramesText += std::to_string(violatingFrameLineNumber++);
					violatingFramesText += ") [" + std::to_string(frame.frameId);
					violatingFramesText += "] ";
					violatingFramesText += std::format("pres_then={:.3f}, ", qpcDeltaMs(pendingWindow.pollTimestampQpc, frame.presentStartQpc));
					violatingFramesText += "disp_then=";
					if (frame.screenTimeQpc == 0u) {
						violatingFramesText += "NA";
					}
					else {
						violatingFramesText += std::format("{:.3f}", qpcDeltaMs(pendingWindow.pollTimestampQpc, frame.screenTimeQpc));
					}
				});

			++tracking.loggedViolationCount;
			pmlog_dbg("Dynamic query stats window integrity violation detected")
				.pmwatch(processId)
				.watch("swapChainAddress", reinterpret_cast<void*>(static_cast<uintptr_t>(swapChainAddress)))
				.pmwatch(pendingWindow.windowSequence)
				.pmwatch(windowOffsetMs_)
				.pmwatch(captureGapMs)
				.pmwatch(elapsedSinceWindowPollMs)
				.pmwatch(lateFrameCount)
				.pmwatch(tracking.loggedViolationCount)
				.watch("elapsed_since_target_start_s", elapsedSinceTargetStartSecondsText)
				.pmwatch(violatingFramesText)
				.diag();
		}

		if (tracking.pendingWindows.empty()) {
			it = swapToIntegrityState_.erase(it);
		}
		else {
			++it;
		}
	}
}

uint32_t PM_DYNAMIC_QUERY::Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
	uint64_t nowTimestamp, FrameMetricsSource* frameSource, uint32_t processId, uint32_t maxSwapChains) const
{
	if (pBlobBase == nullptr || maxSwapChains == 0) {
		return 0;
	}

	const auto window = GenerateQueryWindow_(nowTimestamp);
	const bool integrityCheckEnabled = util::log::GlobalPolicy::Get().GetLogLevel() >= util::log::Level::Debug;

	// Validate pending windows from previous polls before polling this window.
	if (integrityCheckEnabled) {
		ValidatePendingIntegrityWindows_(frameSource, comms, processId, nowTimestamp);
	}

	std::vector<uint64_t> swapChainAddresses;
	if (frameSource != nullptr) {
		swapChainAddresses = frameSource->GetSwapChainAddressesInTimestampRange(window.oldest, window.newest);
	}

	const bool snapshotDumpEnabled = util::log::GlobalPolicy::Get().GetLogLevel() >= util::log::Level::Verbose &&
		util::log::GlobalPolicy::VCheck(util::log::V::middleware);

	std::optional<FrameMetricsSource::PollSnapshotData> pollSnapshots;
	if (snapshotDumpEnabled && frameSource != nullptr && cpuFrameTimeAvgOffset_.has_value()) {
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
		if (integrityCheckEnabled && pSwapChain != nullptr && !pSwapChain->Empty()) {
			// Track every poll window that extends beyond known data for this swap chain.
			const uint64_t latestPresentQpc = pSwapChain->At(pSwapChain->Size() - 1u).presentStartQpc;
			if (latestPresentQpc < window.newest) {
				const auto trackingIt = swapToIntegrityState_.try_emplace(swapChainAddress).first;
				auto& tracking = trackingIt->second;
				const uint64_t windowSequence = tracking.nextWindowSequence++;
				tracking.pendingWindows.push_back(PendingIntegrityWindow_{
					.windowSequence = windowSequence,
					.lastPresentQpcOlderThanWindow = latestPresentQpc,
					.pollWindow = window,
					.pollTimestampQpc = nowTimestamp,
				});
				pmlog_verb(util::log::V::middleware)("Dynamic query integrity potential violation window opened")
					.pmwatch(processId)
					.watch("swapChainAddress", reinterpret_cast<void*>(static_cast<uintptr_t>(swapChainAddress)))
					.pmwatch(windowSequence)
					.pmwatch(nowTimestamp)
					.pmwatch(window.oldest)
					.pmwatch(window.newest)
					.pmwatch(latestPresentQpc)
					.pmwatch(tracking.pendingWindows.size())
					.diag();
			}
		}
		for (auto& pRing : ringMetricPtrs_) {
			pRing->Poll(window, pBlob, comms, pSwapChain, processId);
		}
		if (snapshotDumpEnabled && cpuFrameTimeAvgOffset_.has_value() &&
			HasZeroCpuFrameTimeAverage_(pBlob)) {
			zeroCpuFrameTimeAvgSwapChains.push_back(swapChainAddress);
		}
	};

	if (swapChainAddresses.empty()) {
		if (frameSource != nullptr) {
			const uint64_t targetStartQpc = GetTargetStartQpc_(comms, processId);
			const std::string elapsedSinceTargetStartSecondsText =
				BuildElapsedSinceTargetStartText_(targetStartQpc, nowTimestamp, qpcPeriodSeconds_);
			pmlog_dbg("Dynamic query poll found no swap chains in window")
				.watch("queryHandle", std::format("0x{:x}", reinterpret_cast<uintptr_t>(this)))
				.pmwatch(processId)
				.watch("window_oldest_ms_from_now", BuildRelativeMillisecondsText_(nowTimestamp, window.oldest, qpcPeriodSeconds_))
				.watch("window_newest_ms_from_now", BuildRelativeMillisecondsText_(nowTimestamp, window.newest, qpcPeriodSeconds_))
				.watch("elapsed_since_target_start_s", elapsedSinceTargetStartSecondsText)
				.diag();
		}
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
