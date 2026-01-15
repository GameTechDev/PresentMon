#pragma once
#include <vector>
#include <span>
#include <memory>
#include "../CommonUtilities/mc/MetricsTypes.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmapi::intro
{
	class Root;
}

namespace pmon::ipc
{
	class MiddlewareComms;
	class TelemetryMap;
}

struct PM_FRAME_QUERY
{
public:
	// functions
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements, pmon::ipc::MiddlewareComms& comms);
	~PM_FRAME_QUERY();
	void GatherToBlob(uint8_t* pBlobBytes, const pmon::util::metrics::FrameMetrics& frameMetrics) const;
	size_t GetBlobSize() const;

	PM_FRAME_QUERY(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY& operator=(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY(PM_FRAME_QUERY&&) = delete;
	PM_FRAME_QUERY& operator=(PM_FRAME_QUERY&&) = delete;

private:
	// types
	struct GatherCommand_
	{
		PM_METRIC metricId;
		PM_DATA_TYPE gatherType;
		uint32_t blobOffset;
		// offset into FrameMetrics struct from metric calculator
		uint32_t frameMetricsOffset;
		uint32_t deviceId;
		uint32_t arrayIdx;
		// indicates whether the source data is gatherType or optional<gatherType>
		bool isOptional;
		// for qpc values that need conversion to milliseconds
		double qpcToMs;
	};
	// functions
	static GatherCommand_ MapQueryElementToFrameGatherCommand_(const PM_QUERY_ELEMENT& q, size_t blobCursor);
	static void GatherFromFrameMetrics_(const GatherCommand_& cmd, uint8_t* pBlobBytes, const pmon::util::metrics::FrameMetrics& frameMetrics);
	void GatherFromTelemetry_(const GatherCommand_& cmd, uint8_t* pBlobBytes, int64_t searchQpc,
		const pmon::ipc::TelemetryMap& teleMap) const;
	// data
	const pmon::ipc::MiddlewareComms& comms_;
	std::vector<GatherCommand_> gatherCommands_;
	size_t blobSize_ = 0;
	size_t nextFrameSerial_ = 0;
};

namespace pmon::mid
{
	using ::PM_FRAME_QUERY;
}
