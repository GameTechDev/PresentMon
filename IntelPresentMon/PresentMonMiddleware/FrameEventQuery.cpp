// Copyright (C) 2017-2024 Intel Corporation
#include "FrameEventQuery.h"
#include "Middleware.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/Memory.h"
#include "../CommonUtilities/Qpc.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"
#include "QueryValidation.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

using namespace util;

PM_FRAME_QUERY::PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements, pmon::mid::Middleware& middleware,
	ipc::MiddlewareComms& comms, const pmapi::intro::Root& introRoot)
	:
	middleware_{ middleware },
	comms_{ comms }
{
	pmon::mid::ValidateQueryElements(queryElements, PM_METRIC_TYPE_FRAME_EVENT, introRoot, comms_);

	size_t blobCursor = 0;
	gatherCommands_.reserve(queryElements.size());

	for (auto& q : queryElements) {
		const auto metricView = introRoot.FindMetric(q.metric);
		const auto metricType = metricView.GetType();
		const bool isStaticMetric = metricType == PM_METRIC_TYPE_STATIC;

		const auto frameType = metricView.GetDataTypeInfo().GetFrameType();
		const auto frameTypeSize = ipc::intro::GetDataTypeSize(frameType);

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
		else if (q.deviceId == ipc::kUniversalDeviceId) {
			cmd = MapQueryElementToFrameGatherCommand_(q, blobCursor, frameType);
			cmd.dataSize = (uint32_t)frameTypeSize;
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

PM_FRAME_QUERY::GatherCommand_ PM_FRAME_QUERY::MapQueryElementToFrameGatherCommand_(const PM_QUERY_ELEMENT& q, size_t blobByteCursor, PM_DATA_TYPE frameType)
{
	GatherCommand_ cmd{
		.metricId = q.metric,
		.gatherType = frameType,
		.blobOffset = uint32_t(blobByteCursor),
		.frameMetricsOffset = std::numeric_limits<uint32_t>::max(),
		.deviceId = q.deviceId,
		.arrayIdx = q.arrayIndex,
	};

	const bool mapped = util::DispatchEnumValue<PM_METRIC, int(PM_METRIC_COUNT)>(
		q.metric,
		[&]<PM_METRIC Metric>() -> bool {
			if constexpr (util::metrics::HasFrameMetricMember<Metric>) {
				constexpr auto memberPtr = util::metrics::FrameMetricMember<Metric>::member;
				using MemberType = typename util::MemberPointerInfo<decltype(memberPtr)>::MemberType;
				static const uint32_t memberOffset = uint32_t(util::MemberPointerOffset(memberPtr));
				cmd.frameMetricsOffset = memberOffset;
				cmd.isOptional = util::IsStdOptional<MemberType>;
				return true;
			}
			return false;
		},
		false);
	assert(mapped);
	assert(cmd.frameMetricsOffset != std::numeric_limits<uint32_t>::max());
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
