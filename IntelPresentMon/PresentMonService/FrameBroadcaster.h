#pragma once
#include "../Interprocess/source/Interprocess.h"
#include "../../PresentData/PresentMonTraceConsumer.hpp"
#include "../CommonUtilities/win/Utilities.h"
#include "../CommonUtilities/str/String.h"
#include <unordered_map>
#include <ranges>

namespace pmon::svc
{
    namespace vi = std::views;
    namespace rn = std::ranges;
	using ipc::FrameData;
    using namespace std::literals;

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
		std::shared_ptr<Segment> RegisterTarget(uint32_t pid, bool isPlayback, bool isBackpressured)
		{
            std::lock_guard lk{ mtx_ };
            auto pSegment = comms_.CreateOrGetFrameDataSegment(pid, isBackpressured);
            auto& store = pSegment->GetStore();
            // just overwrite these every time Register is called, it will always be the same
            store.statics.processId = pid;
            store.bookkeeping.isPlayback = isPlayback;
            // initialize name/pid statics on new store segment creation
            // for playback, this needs to be done when processing 1st process event
            if (!store.bookkeeping.staticInitComplete && !isPlayback) {
                store.bookkeeping.staticInitComplete = true;
                try {
                    store.statics.applicationName = util::win::GetExecutableModulePath(
                        util::win::OpenProcess(pid)
                    ).filename().string().c_str();
                }
                catch (...) {
                    // if we reach here a race condition has occurred where the target has exited
                    // so we will mark this in the bookkeeping
                    pmlog_warn("Process exited right as it was being initialized").pmwatch(pid);
                }
            }
            return pSegment;
		}
        void Broadcast(const PresentEvent& present, std::optional<uint32_t> timeoutMs = {})
		{
            std::lock_guard lk{ mtx_ };
			if (auto pSegment = comms_.GetFrameDataSegment(present.ProcessId)) {
                pSegment->GetStore().frameData.Push(FrameDataFromPresentEvent(present));
			}
		}
        void HandleTargetProcessEvent(const ProcessEvent& targetProcessEvent)
        {
            std::lock_guard lk{ mtx_ };
            if (auto pSegment = comms_.GetFrameDataSegment(targetProcessEvent.ProcessId)) {
                auto& store = pSegment->GetStore();
                if (!store.bookkeeping.staticInitComplete && store.bookkeeping.isPlayback) {
                    store.bookkeeping.staticInitComplete = true;
                    store.statics.applicationName =
                        util::str::ToNarrow(targetProcessEvent.ImageFileName).c_str();
                }
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