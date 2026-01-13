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

	class FrameBroadcaster
	{
	public:
        using Segment = ipc::OwnedDataSegment<ipc::FrameDataStore>;
        FrameBroadcaster(ipc::ServiceComms& comms) : comms_{ comms } {}
		std::shared_ptr<Segment> RegisterTarget(uint32_t pid, bool isPlayback, bool isBackpressured)
		{
            std::lock_guard lk{ mtx_ };
            const auto startQpc = util::GetCurrentTimestamp();
            auto pSegment = comms_.CreateOrGetFrameDataSegment(pid, isBackpressured);
            auto& store = pSegment->GetStore();
            auto& book = store.bookkeeping;
            // init bookkeeping only once and only here
            if (!book.bookkeepingInitComplete) {
                book.processId = pid; // we can also init this static here
                book.isPlayback = isPlayback;
                book.startQpc = startQpc;
                book.bookkeepingInitComplete = true;
            }
            else { // sanity checks for subsequent opens
                if (book.processId != pid || book.isPlayback != isPlayback ||
                    book.startQpc >= startQpc) {
                    pmlog_error("Mismatch in bookkeeping data")
                        .pmwatch(book.processId).pmwatch(pid)
                        .pmwatch(book.isPlayback).pmwatch(isPlayback)
                        .pmwatch(book.startQpc).pmwatch(startQpc);
                }
            }
            // initialize name/pid statics on new store segment creation
            // for playback, this needs to be deferred to when processing 1st process event
            if (!book.staticInitComplete && !isPlayback) {
                book.staticInitComplete = true;
                try {
                    store.statics.applicationName = util::win::GetExecutableModulePath(
                        util::win::OpenProcess(pid)
                    ).filename().string().c_str();
                }
                catch (...) {
                    pmlog_warn("Process exited right as it was being initialized").pmwatch(pid);
                }
            }
            return pSegment;
		}
        void Broadcast(const PresentEvent& present, std::optional<uint32_t> timeoutMs = {})
		{
            std::lock_guard lk{ mtx_ };
			if (auto pSegment = comms_.GetFrameDataSegment(present.ProcessId)) {
                pSegment->GetStore().frameData.Push(FrameData::CopyFrameData(present));
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
