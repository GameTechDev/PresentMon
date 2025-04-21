// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT

#include "PresentMonTraceConsumer.hpp"
#include "NvidiaTraceConsumer.hpp"

NVTraceConsumer::NVTraceConsumer()
{
    // Nothing to do
}

NVTraceConsumer::~NVTraceConsumer()
{
    // Nothing to do
}

void NVTraceConsumer::HandleNvidiaDisplayDriverEvent(EVENT_RECORD* const pEventRecord, PMTraceConsumer* const pmConsumer)
{
    enum {
        FlipRequest = 1
    };

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case FlipRequest: {
        auto alloc = pmConsumer->mMetadata.GetEventData<uint64_t>(pEventRecord, L"alloc");
        auto vidPnSourceId = pmConsumer->mMetadata.GetEventData<uint32_t>(pEventRecord, L"vidPnSourceId");
        auto& lastFlipTime = mLastFlipTimeByHead[vidPnSourceId];
        auto ts = pmConsumer->mMetadata.GetEventData<uint64_t>(pEventRecord, L"ts");
        auto token = pmConsumer->mMetadata.GetEventData<uint32_t>(pEventRecord, L"token");
        uint64_t proposedFlipTime = 0;
        uint64_t delay = 0;

        if (token == mLastFlipToken) {
            return;
        }
        mLastFlipToken = token;

        if (alloc == 0) {
            assert(!delay);
            assert(ts);
            // proposedFlipTime in number of ticks
            proposedFlipTime = ts;
            auto t1 = *(uint64_t*)&hdr.TimeStamp;
            if (proposedFlipTime >= t1) {
                delay = proposedFlipTime - t1;
            }

            if (proposedFlipTime && lastFlipTime && (proposedFlipTime < lastFlipTime)) {
                delay = lastFlipTime - proposedFlipTime;
                proposedFlipTime = lastFlipTime;
            }
        }

        lastFlipTime = proposedFlipTime;

        {
            NvFlipRequest flipRequest;
            flipRequest.FlipDelay = delay;
            flipRequest.FlipToken = token;
            mNvFlipRequestByThreadId.emplace(hdr.ThreadId, flipRequest);
        }
        break;
    }
    default:
        break;
    }
}

void NVTraceConsumer::ApplyFlipDelay(PresentEvent* present, uint32_t threadId)
{
    auto flipIter = mNvFlipRequestByThreadId.find(threadId);
    if (flipIter != mNvFlipRequestByThreadId.end()) {
        present->FlipDelay = flipIter->second.FlipDelay;
        present->FlipToken = flipIter->second.FlipToken;
        // Clear the map (we want the whole map cleared, not just the element with the thread)
        mNvFlipRequestByThreadId.clear();
    }
}