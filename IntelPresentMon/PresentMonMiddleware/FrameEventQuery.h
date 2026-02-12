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

namespace pmon::mid
{
	class Middleware;
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
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements, pmon::mid::Middleware& middleware,
		pmon::ipc::MiddlewareComms& comms, const pmapi::intro::Root& introRoot);
	~PM_FRAME_QUERY();
	void GatherToBlob(uint8_t* pBlobBytes, uint32_t processId,
		const pmon::util::metrics::FrameMetrics& frameMetrics) const;
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
		PM_DATA_TYPE gatherType = PM_DATA_TYPE_VOID;
		uint32_t blobOffset;
		uint32_t dataSize = 0;
		// offset into FrameMetrics struct from metric calculator
		uint32_t frameMetricsOffset = 0;
		uint32_t deviceId = 0;
		uint32_t arrayIdx = 0;
		// indicates whether the source data is gatherType or optional<gatherType>
		bool isOptional = false;
		bool isStatic = false;
	};
	// functions
	static GatherCommand_ MapQueryElementToFrameGatherCommand_(const PM_QUERY_ELEMENT& q, size_t blobCursor, PM_DATA_TYPE frameType);
	void GatherFromFrameMetrics_(const GatherCommand_& cmd, uint8_t* pBlobBytes,
		const pmon::util::metrics::FrameMetrics& frameMetrics) const;
	void GatherFromStatic_(const GatherCommand_& cmd, uint8_t* pBlobBytes, uint32_t processId) const;
	void GatherFromTelemetry_(const GatherCommand_& cmd, uint8_t* pBlobBytes, int64_t searchQpc,
		const pmon::ipc::TelemetryMap& teleMap) const;
	// data
	pmon::mid::Middleware& middleware_;
	const pmon::ipc::MiddlewareComms& comms_;
	std::vector<GatherCommand_> gatherCommands_;
	size_t blobSize_ = 0;
	size_t nextFrameSerial_ = 0;
};

namespace pmon::mid
{
	using ::PM_FRAME_QUERY;
}
