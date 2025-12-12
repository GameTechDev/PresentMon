#pragma once
#include "../Interprocess/source/Interprocess.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include <unordered_map>
#include <ranges>

namespace pmon::svc
{
    namespace vi = std::views;
    namespace rn = std::ranges;
	using ipc::FrameData;

	namespace
	{
        FrameData FrameDataFromPresentEvent(const PresentEvent& present)
        {
            using DisplayedEntry = std::pair<FrameType, uint64_t>;
            using DisplayedVector = boost::container::static_vector<DisplayedEntry, 10>;

            return FrameData{
                // Timing data
                .presentStartTime = present.PresentStartTime,
                .readyTime = present.ReadyTime,
                .timeInPresent = present.TimeInPresent,
                .gpuStartTime = present.GPUStartTime,
                .gpuDuration = present.GPUDuration,
                .gpuVideoDuration = present.GPUVideoDuration,

                // XeSS-FG propagated timing
                .appPropagatedPresentStartTime = present.AppPropagatedPresentStartTime,
                .appPropagatedTimeInPresent = present.AppPropagatedTimeInPresent,
                .appPropagatedGPUStartTime = present.AppPropagatedGPUStartTime,
                .appPropagatedReadyTime = present.AppPropagatedReadyTime,
                .appPropagatedGPUDuration = present.AppPropagatedGPUDuration,
                .appPropagatedGPUVideoDuration = present.AppPropagatedGPUVideoDuration,

                // Instrumented timestamps
                .appSimStartTime = present.AppSimStartTime,
                .appSleepStartTime = present.AppSleepStartTime,
                .appSleepEndTime = present.AppSleepEndTime,
                .appRenderSubmitStartTime = present.AppRenderSubmitStartTime,
                .appRenderSubmitEndTime = present.AppRenderSubmitEndTime,
                .appPresentStartTime = present.AppPresentStartTime,
                .appPresentEndTime = present.AppPresentEndTime,
                .appInputSample = present.AppInputSample,

                // Input device timestamps
                .inputTime = present.InputTime,
                .mouseClickTime = present.MouseClickTime,

                // Displayed history (no explicit bounds check; assumed to fit)
                .displayed = DisplayedVector{
                    present.Displayed.begin(),
                    present.Displayed.end()
                },

                // PC Latency data
                .pclSimStartTime = present.PclSimStartTime,
                .pclInputPingTime = present.PclInputPingTime,
                .flipDelay = present.FlipDelay,
                .flipToken = present.FlipToken,

                // Metadata
                .finalState = present.FinalState,
                .processId = present.ProcessId,
                .threadId = present.ThreadId,
                .swapChainAddress = present.SwapChainAddress,
                .frameId = present.FrameId,
                .appFrameId = present.AppFrameId,
            };
        }
	}

	class FrameBroadcaster
	{
	public:
        using Segment = ipc::OwnedDataSegment<ipc::FrameDataStore>;
        FrameBroadcaster(ipc::ServiceComms& comms) : comms_{ comms } {}
		std::shared_ptr<Segment> RegisterTarget(uint32_t pid)
		{
            std::lock_guard lk{ mtx_ };
			// collect garbage first so it doesn't accumulate in the map
			std::erase_if(segments_, [](auto&&pr){return pr.second.expired();});

			// ipc makes the segment hosting the store, track weakly here (strongly in client session ctx)
			auto pSegment = comms_.MakeFrameDataSegment(pid);
			segments_[pid] = pSegment;
			return pSegment;
		}
		void Broadcast(const PresentEvent& present)
		{
            std::lock_guard lk{ mtx_ };
			if (auto i = segments_.find(present.ProcessId); i != segments_.end()) {
				if (auto pSegment = i->second.lock()) {
					pSegment->GetStore().frameData.Push(FrameDataFromPresentEvent(present));
				}
				else {
					// segment expired, clean it up
					segments_.erase(i);
				}
			}
		}
		std::vector<uint32_t> GetPids() const
		{
            std::lock_guard lk{ mtx_ };
            return segments_ | vi::filter([](auto&& p) {return !p.second.expired(); }) |
                vi::keys | rn::to<std::vector>();
		}
        const ipc::ShmNamer& GetNamer() const
        {
            return comms_.GetNamer();
        }
	private:
		// data
		ipc::ServiceComms& comms_;
		std::unordered_map<uint32_t, std::weak_ptr<Segment>> segments_;
        mutable std::mutex mtx_;
	};
}