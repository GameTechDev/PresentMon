#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../PresentMonUtils/StreamFormat.h"
#include <vector>
#include <span>
#include <memory>
#include "FrameTimingData.h"

namespace pmapi::intro
{
	class Root;
}

namespace pmon::mid
{
	class GatherCommand_;
}

struct PM_FRAME_QUERY
{
public:
	// types
	struct Context
	{
		// functions
		Context(uint64_t qpcStart, long long perfCounterFrequency, FrameTimingData& frameTimingData) : qpcStart{ qpcStart },
			performanceCounterPeriodMs{ perfCounterFrequency != 0.f ? 1000.0 / perfCounterFrequency : 0.f },
			frameTimingData{ frameTimingData } {}
		void UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in,
			const PmNsmFrameData* pFrameDataOfNextDisplayed,
			const PmNsmFrameData* pFrameDataofLastPresented,
			const PmNsmFrameData* pFrameDataofLastAppPresented,
			const PmNsmFrameData* pFrameDataOfLastDisplayed,
			const PmNsmFrameData* pFrameDataOfLastAppDisplayed,
			const PmNsmFrameData* pPreviousFrameDataOfLastDisplayed);
		// data
		const PmNsmFrameData* pSourceFrameData = nullptr;
		uint32_t sourceFrameDisplayIndex = 0;
		const double performanceCounterPeriodMs{};
		const uint64_t qpcStart{};
		bool dropped{};
		// Start qpc of the previous frame, displayed or not
		uint64_t cpuStart = 0;
        // Present start qpc of the previous frame, displayed or not
        uint64_t previousPresentStartQpc = 0;
		// Start cpustart qpc of the previously displayed frame
		uint64_t lastDisplayedCpuStart = 0;
		// Screen time qpc of the previously displayed frame.
		uint64_t previousDisplayedQpc = 0;
		// Screen time qpc of the first display in the next displayed PmNsmFrameData
		uint64_t nextDisplayedQpc = 0;
		// Display index to attribute cpu work, gpu work, animation error and
		// input latency
		size_t appIndex = 0;
		// Click time qpc of non displayed frame
		uint64_t lastReceivedNotDisplayedClickQpc = 0;
		// All other input time qpc of non displayed frame
		uint64_t lastReceivedNotDisplayedAllInputTime = 0;
		// QPC of the last PC Latency simulation start
		uint64_t mLastReceivedNotDisplayedPclSimStart = 0;
		// QPC of the last PC Latency pc input
		uint64_t mLastReceivedNotDisplayedPclInputTime = 0;
        FrameTimingData frameTimingData{};

		// Accumlated input to frame start time
		double mAccumulatedInput2FrameStartTime = 0.f;
		// Current input to frame start average
		double avgInput2Fs{};
	};
	// functions
	PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements);
	~PM_FRAME_QUERY();
	void GatherToBlob(Context& ctx, uint8_t* pDestBlob) const;
	size_t GetBlobSize() const;
	std::optional<uint32_t> GetReferencedDevice() const;

	PM_FRAME_QUERY(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY& operator=(const PM_FRAME_QUERY&) = delete;
	PM_FRAME_QUERY(PM_FRAME_QUERY&&) = delete;
	PM_FRAME_QUERY& operator=(PM_FRAME_QUERY&&) = delete;

private:
	// functions
	std::unique_ptr<pmon::mid::GatherCommand_> MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos);
	// data
	std::vector<std::unique_ptr<pmon::mid::GatherCommand_>> gatherCommands_;
	size_t blobSize_ = 0;
	std::optional<uint32_t> referencedDevice_;
};