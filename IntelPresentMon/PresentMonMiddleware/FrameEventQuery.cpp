// Copyright (C) 2017-2024 Intel Corporation
#include "FrameEventQuery.h"
#include "Middleware.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../PresentMonAPIWrapperCommon/Exception.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Memory.h"
#include "../CommonUtilities/Qpc.h"
#include <algorithm>
#include <cstddef>
#include <limits>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

using namespace util;

PM_FRAME_QUERY::PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements, pmon::mid::Middleware& middleware,
	ipc::MiddlewareComms& comms, const pmapi::intro::Root& introRoot)
	:
	middleware_{ middleware },
	comms_{ comms }
{
	if (queryElements.empty()) {
		pmlog_error("Frame query requires at least one query element").diag();
		throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Empty frame query");
	}

	size_t blobCursor = 0;
	gatherCommands_.reserve(queryElements.size());

	for (auto& q : queryElements) {
		const auto metricView = introRoot.FindMetric(q.metric);
		const auto metricType = metricView.GetType();
		const bool isStaticMetric = metricType == PM_METRIC_TYPE_STATIC;
		if (!pmapi::intro::MetricTypeIsFrameEvent(metricType) && !isStaticMetric) {
			pmlog_error("Non-frame metric used in frame query")
				.pmwatch(metricView.Introspect().GetSymbol()).diag();
			throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Frame query contains non-frame metric");
		}

		if (q.stat != PM_STAT_NONE) {
			pmlog_warn("Frame query stat should be NONE")
				.pmwatch(metricView.Introspect().GetSymbol())
				.pmwatch((int)q.stat).diag();
		}

		const auto frameType = metricView.GetDataTypeInfo().GetFrameType();
		const auto frameTypeSize = ipc::intro::GetDataTypeSize(frameType);
		if (frameTypeSize == 0) {
			pmlog_error("Unsupported frame query data type")
				.pmwatch(metricView.Introspect().GetSymbol()).diag();
			throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unsupported frame query data type");
		}

		if (q.deviceId != ipc::kUniversalDeviceId) {
			try {
				introRoot.FindDevice(q.deviceId);
			}
			catch (const pmapi::LookupException&) {
				pmlog_error(util::ReportException("Failed to find device ID while registering frame query"))
					.diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Invalid device ID");
			}
		}

		const auto deviceMetricInfo = [&]() -> std::optional<pmapi::intro::DeviceMetricInfoView> {
			for (auto info : metricView.GetDeviceMetricInfo()) {
				if (info.GetDevice().GetId() == q.deviceId) {
					return info;
				}
			}
			return std::nullopt;
		}();

		if (!deviceMetricInfo.has_value()) {
			if (!isStaticMetric || q.deviceId != ipc::kSystemDeviceId) {
				pmlog_error("Metric not supported by device in frame query")
					.pmwatch(metricView.Introspect().GetSymbol())
					.pmwatch(q.deviceId).diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Metric not supported by device in frame query");
			}
			if (q.arrayIndex != 0) {
				pmlog_error("Frame query array index out of bounds")
					.pmwatch(metricView.Introspect().GetSymbol())
					.pmwatch(q.arrayIndex)
					.pmwatch(1).diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Frame query array index out of bounds");
			}
		}
		else {
			if (!deviceMetricInfo->IsAvailable()) {
				pmlog_error("Metric not supported by device in frame query")
					.pmwatch(metricView.Introspect().GetSymbol())
					.pmwatch(q.deviceId).diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Metric not supported by device in frame query");
			}

			const auto arraySize = deviceMetricInfo->GetArraySize();
			if (q.arrayIndex >= arraySize) {
				pmlog_error("Frame query array index out of bounds")
					.pmwatch(metricView.Introspect().GetSymbol())
					.pmwatch(q.arrayIndex)
					.pmwatch(arraySize).diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Frame query array index out of bounds");
			}
		}

		const auto alignment = ipc::intro::GetDataTypeAlignment(frameType);
		blobCursor = util::PadToAlignment(blobCursor, alignment);

		GatherCommand_ cmd{};
		cmd.metricId = q.metric;
		cmd.gatherType = frameType;
		cmd.blobOffset = uint32_t(blobCursor);
		cmd.dataSize = (uint32_t)frameTypeSize;
		cmd.deviceId = q.deviceId;
		cmd.arrayIdx = q.arrayIndex;

		if (isStaticMetric) {
			cmd.isStatic = true;
		}
		else if (q.deviceId > 0 && q.deviceId <= ipc::kSystemDeviceId) {
			const auto& teleMap = q.deviceId == ipc::kSystemDeviceId ?
				comms_.GetSystemDataStore().telemetryData :
				comms_.GetGpuDataStore(q.deviceId).telemetryData;

			if (teleMap.ArraySize(q.metric) == 0) {
				pmlog_error("Telemetry ring missing for metric in frame query")
					.pmwatch(metricView.Introspect().GetSymbol())
					.pmwatch(q.deviceId).diag();
				throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Telemetry ring missing for metric in frame query");
			}
		}
		else if (q.deviceId == ipc::kUniversalDeviceId) {
			cmd = MapQueryElementToFrameGatherCommand_(q, blobCursor, metricView);
			cmd.dataSize = (uint32_t)frameTypeSize;
		}
		else {
			pmlog_error("Invalid device id in frame query")
				.pmwatch(q.deviceId).diag();
			throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Invalid device id in frame query");
		}

		q.dataOffset = blobCursor;
		q.dataSize = frameTypeSize;
		blobCursor += frameTypeSize;

		gatherCommands_.push_back(cmd);
	}
	// make sure blob size is a multiple of 16 so that blobs in array always start 16-aligned
	blobSize_ = util::PadToAlignment(blobCursor, 16u);
}

PM_FRAME_QUERY::~PM_FRAME_QUERY() = default;

void PM_FRAME_QUERY::GatherToBlob(uint8_t* pBlobBytes, uint32_t processId, const util::metrics::FrameMetrics& frameMetrics) const
{
	for (auto& cmd : gatherCommands_) {
		if (cmd.isStatic) {
			GatherFromStatic_(cmd, pBlobBytes, processId);
		}
		else if (cmd.deviceId == ipc::kUniversalDeviceId) {
			GatherFromFrameMetrics_(cmd, pBlobBytes, frameMetrics);
		}
		else if (cmd.deviceId == ipc::kSystemDeviceId) {
			GatherFromTelemetry_(cmd, pBlobBytes, frameMetrics.cpuStartQpc, comms_.GetSystemDataStore().telemetryData);
		}
		else if (cmd.deviceId < ipc::kSystemDeviceId) {
			GatherFromTelemetry_(cmd, pBlobBytes, frameMetrics.cpuStartQpc, comms_.GetGpuDataStore(cmd.deviceId).telemetryData);
		}
		else {
			pmlog_error("Bad device ID").pmwatch(cmd.deviceId);
		}
	}
}

size_t PM_FRAME_QUERY::GetBlobSize() const
{
	return blobSize_;
}

PM_FRAME_QUERY::GatherCommand_ PM_FRAME_QUERY::MapQueryElementToFrameGatherCommand_(const PM_QUERY_ELEMENT& q, size_t blobByteCursor, const pmapi::intro::MetricView& metricView)
{
	GatherCommand_ cmd{
		.metricId = q.metric,
		.gatherType = metricView.GetDataTypeInfo().GetFrameType(),
		.blobOffset = uint32_t(blobByteCursor),
		.frameMetricsOffset = std::numeric_limits<uint32_t>::max(),
		.deviceId = q.deviceId,
		.arrayIdx = q.arrayIndex,
	};

	switch (q.metric) {
	case PM_METRIC_ALLOWS_TEARING:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, allowsTearing);
		break;
	case PM_METRIC_PRESENT_RUNTIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, runtime);
		break;
	case PM_METRIC_PRESENT_MODE:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, presentMode);
		break;
	case PM_METRIC_PRESENT_FLAGS:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, presentFlags);
		break;
	case PM_METRIC_SYNC_INTERVAL:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, syncInterval);
		break;
	case PM_METRIC_SWAP_CHAIN_ADDRESS:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, swapChainAddress);
		break;
	case PM_METRIC_PRESENT_START_QPC:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, presentStartQpc);
		break;
	case PM_METRIC_PRESENT_START_TIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, presentStartMs);
		break;
	case PM_METRIC_CPU_START_QPC:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, cpuStartQpc);
		break;
	case PM_METRIC_CPU_START_TIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, cpuStartMs);
		break;
	case PM_METRIC_BETWEEN_PRESENTS:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msBetweenPresents);
		break;
	case PM_METRIC_IN_PRESENT_API:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msInPresentApi);
		break;
	case PM_METRIC_RENDER_PRESENT_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msUntilRenderComplete);
		break;
	case PM_METRIC_BETWEEN_DISPLAY_CHANGE:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msBetweenDisplayChange);
		break;
	case PM_METRIC_UNTIL_DISPLAYED:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msUntilDisplayed);
		break;
	case PM_METRIC_DISPLAYED_TIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msDisplayedTime);
		break;
	case PM_METRIC_DISPLAY_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msDisplayLatency);
		break;
	case PM_METRIC_BETWEEN_SIMULATION_START:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msBetweenSimStarts);
		cmd.isOptional = true;
		break;
	case PM_METRIC_PC_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msPcLatency);
		cmd.isOptional = true;
		break;
	case PM_METRIC_CPU_BUSY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msCPUBusy);
		break;
	case PM_METRIC_CPU_WAIT:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msCPUWait);
		break;
	case PM_METRIC_CPU_FRAME_TIME:
	case PM_METRIC_BETWEEN_APP_START:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msCPUTime);
		break;
	case PM_METRIC_GPU_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msGPULatency);
		break;
	case PM_METRIC_GPU_TIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msGPUTime);
		break;
	case PM_METRIC_GPU_BUSY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msGPUBusy);
		break;
	case PM_METRIC_GPU_WAIT:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msGPUWait);
		break;
	case PM_METRIC_DROPPED_FRAMES:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, isDroppedFrame);
		break;
	case PM_METRIC_ANIMATION_ERROR:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msAnimationError);
		cmd.isOptional = true;
		break;
	case PM_METRIC_ANIMATION_TIME:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msAnimationTime);
		cmd.isOptional = true;
		break;
	case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msClickToPhotonLatency);
		cmd.isOptional = true;
		break;
	case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msAllInputPhotonLatency);
		cmd.isOptional = true;
		break;
	case PM_METRIC_INSTRUMENTED_LATENCY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msInstrumentedLatency);
		cmd.isOptional = true;
		break;
	case PM_METRIC_FLIP_DELAY:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, msFlipDelay);
		cmd.isOptional = true;
		break;
	case PM_METRIC_FRAME_TYPE:
		cmd.frameMetricsOffset = offsetof(util::metrics::FrameMetrics, frameType);
		break;
	default:
		pmlog_error("Unexpected frame metric in frame query")
			.pmwatch(metricView.Introspect().GetSymbol())
			.pmwatch(metricView.IntrospectType().GetSymbol())
			.pmwatch((int)q.metric).diag();
		throw Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unexpected frame metric in frame query");
	}
	if (cmd.frameMetricsOffset == std::numeric_limits<uint32_t>::max()) {
		pmlog_error("Frame metrics offset not set")
			.pmwatch(metricView.Introspect().GetSymbol())
			.pmwatch((int)q.metric);
		throw Except<>("Frame metrics offset not set in command mapping");
	}
	return cmd;
}

void PM_FRAME_QUERY::GatherFromFrameMetrics_(const GatherCommand_& cmd, uint8_t* pBlobBytes,
	const util::metrics::FrameMetrics& frameMetrics) const
{
	const auto pFrameMemberBytes = reinterpret_cast<const uint8_t*>(&frameMetrics) + cmd.frameMetricsOffset;
	// metrics relating to display decay to NaN when frame was dropped/not displayed at all
	if (frameMetrics.isDroppedFrame && (
		cmd.metricId == PM_METRIC_DISPLAYED_TIME ||
		cmd.metricId == PM_METRIC_DISPLAY_LATENCY ||
		cmd.metricId == PM_METRIC_UNTIL_DISPLAYED ||
		cmd.metricId == PM_METRIC_BETWEEN_DISPLAY_CHANGE)) {
		*reinterpret_cast<double*>(pBlobBytes + cmd.blobOffset) =
			std::numeric_limits<double>::quiet_NaN();
		return;
	}
	switch (cmd.gatherType) {
	case PM_DATA_TYPE_UINT64:
		*reinterpret_cast<uint64_t*>(pBlobBytes + cmd.blobOffset) =
			*reinterpret_cast<const uint64_t*>(pFrameMemberBytes);
		break;
	case PM_DATA_TYPE_INT32:
		*reinterpret_cast<int32_t*>(pBlobBytes + cmd.blobOffset) =
			*reinterpret_cast<const int32_t*>(pFrameMemberBytes);
		break;
	case PM_DATA_TYPE_UINT32:
		*reinterpret_cast<uint32_t*>(pBlobBytes + cmd.blobOffset) =
			*reinterpret_cast<const uint32_t*>(pFrameMemberBytes);
		break;
	case PM_DATA_TYPE_DOUBLE:
	{
		auto& blobDouble = *reinterpret_cast<double*>(pBlobBytes + cmd.blobOffset);
		if (!cmd.isOptional) {
			blobDouble = *reinterpret_cast<const double*>(pFrameMemberBytes);
		}
		else {
			auto& optDouble = *reinterpret_cast<const std::optional<double>*>(pFrameMemberBytes);
			if (optDouble) {
				blobDouble = *optDouble;
			}
			else {
				blobDouble = std::numeric_limits<double>::quiet_NaN();
			}
		}
		break;
	}
	case PM_DATA_TYPE_ENUM:
		*reinterpret_cast<int*>(pBlobBytes + cmd.blobOffset) =
			*reinterpret_cast<const int*>(pFrameMemberBytes);
		break;
	case PM_DATA_TYPE_BOOL:
		*reinterpret_cast<bool*>(pBlobBytes + cmd.blobOffset) =
			*reinterpret_cast<const bool*>(pFrameMemberBytes);
		break;
	case PM_DATA_TYPE_STRING:
	case PM_DATA_TYPE_VOID:
		pmlog_error("Unsupported frame data type").pmwatch((int)cmd.gatherType);
		break;
	}
}

void PM_FRAME_QUERY::GatherFromStatic_(const GatherCommand_& cmd, uint8_t* pBlobBytes, uint32_t processId) const
{
	const PM_QUERY_ELEMENT element{
		.metric = cmd.metricId,
		.stat = PM_STAT_NONE,
		.deviceId = cmd.deviceId,
		.arrayIndex = cmd.arrayIdx,
		.dataOffset = (uint64_t)cmd.blobOffset,
		.dataSize = (uint64_t)cmd.dataSize,
	};

	middleware_.PollStaticQuery(element, processId, pBlobBytes + cmd.blobOffset);
}

void PM_FRAME_QUERY::GatherFromTelemetry_(const GatherCommand_& cmd, uint8_t* pBlobBytes, int64_t searchQpc,
	const ipc::TelemetryMap& teleMap) const
{
	const auto ResolveValue = [&, this]<typename T>() -> const T& {
		// resolve ring by metric id + array index
		auto& ring = teleMap.FindRing<T>(cmd.metricId)[cmd.arrayIdx];
		// find nearest sample in ring and return value
		return ring.Nearest(searchQpc).value;
	};

	switch (cmd.gatherType) {
	case PM_DATA_TYPE_UINT64:
		*reinterpret_cast<uint64_t*>(pBlobBytes + cmd.blobOffset) = ResolveValue.operator()<uint64_t>();
		break;
	case PM_DATA_TYPE_DOUBLE:
		*reinterpret_cast<double*>(pBlobBytes + cmd.blobOffset) = ResolveValue.operator()<double>();
		break;
	case PM_DATA_TYPE_ENUM:
		*reinterpret_cast<int*>(pBlobBytes + cmd.blobOffset) = ResolveValue.operator()<int>();
		break;
	case PM_DATA_TYPE_BOOL:
		*reinterpret_cast<bool*>(pBlobBytes + cmd.blobOffset) = ResolveValue.operator()<bool>();
		break;
	case PM_DATA_TYPE_INT32:
	case PM_DATA_TYPE_UINT32:
	case PM_DATA_TYPE_STRING:
	case PM_DATA_TYPE_VOID:
		pmlog_error("Unsupported telemetry data type").pmwatch((int)cmd.gatherType);
		break;
	}
}
