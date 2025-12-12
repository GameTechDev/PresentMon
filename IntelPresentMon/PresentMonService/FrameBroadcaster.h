#pragma once
#include "../Interprocess/source/Interprocess.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../CommonUtilities/win/Utilities.h"
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
            auto pSegment = comms_.CreateOrGetFrameDataSegment(pid);
            auto& store = pSegment->GetStore();
            // initialize name/pid statics on new store segment creation
            if (!store.bookkeeping.staticInitComplete) {
                store.bookkeeping.staticInitComplete = true;
                store.statics.processId = pid;
                try {
                    store.statics.applicationName = util::win::GetExecutableModulePath(
                        util::win::OpenProcess(pid)
                    ).filename().string().c_str();
                }
                catch (...) {
                    // if we reach here a race condition has occurred where the target has exited
                    // so we will mark this in the bookkeeping
                    pmlog_warn("Process exited right as it was being initialized").pmwatch(pid);
                    store.bookkeeping.targetExited = true;
                }
            }
            return pSegment;
		}
		void Broadcast(const PresentEvent& present)
		{
            std::lock_guard lk{ mtx_ };
			if (auto pSegment = comms_.GetFrameDataSegment(present.ProcessId)) {
                pSegment->GetStore().frameData.Push(FrameDataFromPresentEvent(present));
			}
		}
		std::vector<uint32_t> GetPids() const
		{
            std::lock_guard lk{ mtx_ };
            return comms_.GetFramePids();
		}
        const ipc::ShmNamer& GetNamer() const
        {
            return comms_.GetNamer();
        }
	private:
		// data
		ipc::ServiceComms& comms_;
        mutable std::mutex mtx_;
	};
}