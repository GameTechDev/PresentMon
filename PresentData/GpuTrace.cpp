// Copyright (C) 2021-2022 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTraceConsumer.hpp"

namespace {

void DebugPrintAccumulatedGpuTime(uint32_t processId, uint64_t accumulatedTime, uint64_t startTime, uint64_t endTime)
{
    auto addedTime = endTime - startTime;

    if (addedTime > 0ull) {
        printf("                             Accumulated GPU time ProcessId=%u: ", processId);
        PrintTimeDelta(accumulatedTime);
        printf(" + [");
        PrintTime(startTime);
        printf(", ");
        PrintTime(endTime);
        printf("] = ");
        PrintTimeDelta(accumulatedTime + addedTime);
        printf("\n");
    }
}

}

uint32_t GpuTrace::LookupPacketTraceProcessId(PacketTrace* packetTrace) const
{
    for (auto const& pr : mProcessFrameInfo) {
        if (packetTrace == &pr.second.mVideoEngines ||
            packetTrace == &pr.second.mOtherEngines) {
            return pr.first;
        }
    }
    return 0;
}

void GpuTrace::PrintRunningContexts() const
{
    for (auto const& pr : mContexts) {
        auto hContext = pr.first;
        auto const& context = pr.second;
        auto const& node = *context.mNode;

        if (node.mQueueCount > 0) {
            printf("                             hContext=0x%llx [", hContext);

            for (uint32_t i = 0; i < node.mQueueCount; ++i) {
                auto queueIdx = (node.mQueueIndex + i) % (uint32_t) node.mQueue.size();
                auto const& entry = node.mQueue[queueIdx];

                if (i > 0) {
                    printf("\n                                                          ");
                }

                printf(" SequenceId=%u", entry.mSequenceId);
                if (entry.mPacketTrace == nullptr) {
                    if (entry.mCompleted) {
                        printf(" DONE");
                    } else {
                        printf(" WAIT");
                    }
                } else {
                    printf(" ProcessId=%u", LookupPacketTraceProcessId(entry.mPacketTrace));
                }
            }

            printf(" ]\n");
        }
    }
}

GpuTrace::GpuTrace(PMTraceConsumer* pmConsumer)
    : mPMConsumer(pmConsumer)
{
}

void GpuTrace::RegisterDevice(uint64_t hDevice, uint64_t pDxgAdapter)
{
    // Sometimes there are duplicate start events
    DebugAssert(mDevices.find(hDevice) == mDevices.end() || mDevices.find(hDevice)->second == pDxgAdapter);

    mDevices.emplace(hDevice, pDxgAdapter);
}

void GpuTrace::UnregisterDevice(uint64_t hDevice)
{
    // Sometimes there are duplicate stop events so it's ok if it's already removed
    mDevices.erase(hDevice);
}

void GpuTrace::RegisterContext(uint64_t hContext, uint64_t hDevice, uint32_t nodeOrdinal, uint32_t processId)
{
    auto deviceIter = mDevices.find(hDevice);
    if (deviceIter == mDevices.end()) {
        DebugAssert(false);
        return;
    }
    auto pDxgAdapter = deviceIter->second;
    auto node = &mNodes[pDxgAdapter].emplace(nodeOrdinal, Node{}).first->second;

    // Sometimes there are duplicate start events, make sure that they say the same thing
    DebugAssert(mContexts.find(hContext) == mContexts.end() || mContexts.find(hContext)->second.mNode == node);

    auto context = &mContexts.emplace(hContext, Context()).first->second;
    context->mPacketTrace = nullptr;
    context->mNode = node;
    context->mParentContext = 0;
    context->mIsParentContext = false;
    context->mIsHwQueue = false;

    if (processId != 0) {
        SetContextProcessId(context, processId);
    }
}

// HwQueue's are similar to Contexts, but are used with HWS nodes and use the
// following pattern:
//     Context_Start hContext=C hDevice=D NodeOrdinal=N
//     HwQueue_Start hContext=C hHwQueue=0x0 ParentDxgHwQueue=Q1
//     HwQueue_Start hContext=C hHwQueue=H1  ParentDxgHwQueue=Q2
//     HwQueue_Start hContext=C hHwQueue=H2  ParentDxgHwQueue=Q3
//     ...
//     DxgKrnl_QueuePacket_Start hContext=Q2 SubmitSequence=S ...
//     ...
//     DxgKrnl_QueuePacket_Stop hContext=Q2 SubmitSequence=S
//     ...
//     HwQueue_Stop hContext=C hHwQueue=0x0 ParentDxgHwQueue=Q3
//     HwQueue_Stop hContext=C hHwQueue=0x0 ParentDxgHwQueue=Q2
//     HwQueue_Stop hContext=C hHwQueue=0x0 ParentDxgHwQueue=Q1
//     Context_Stop hContext=C
void GpuTrace::RegisterHwQueueContext(uint64_t hContext, uint64_t parentDxgHwQueue)
{
    DebugAssert(mContexts.find(hContext)         != mContexts.end());
    DebugAssert(mContexts.find(parentDxgHwQueue) == mContexts.end());

    // Look up the context C, which we're calling the parent context.  We
    // should already have seen a Context_Start event.
    auto ii = mContexts.find(hContext);
    if (ii == mContexts.end()) {
        return;
    }
    auto parentContext = &ii->second;

    DebugAssert(parentContext->mParentContext == 0);
    DebugAssert(parentContext->mIsHwQueue == false);
    parentContext->mIsParentContext = true;

    // Create a new context for the HWQueue.  Even though they map the same
    // device engine, HWQueues need their own context so that tracked sequence
    // IDs are ordered.
    //
    // If there are two HwQueues that map to the same engine, it's not clear
    // whether or not queue packets running at the same time are really running
    // simultaneous, but since we're counting any duration where at least one
    // node is running it doesn't matter.

    Node* node = new Node();
    node->mQueueIndex = 0;
    node->mQueueCount = 0;
    node->mIsVideo = parentContext->mNode->mIsVideo;

    auto hwQueueContext = &mContexts.emplace(parentDxgHwQueue, Context()).first->second;
    hwQueueContext->mPacketTrace = parentContext->mPacketTrace;
    hwQueueContext->mNode = node;
    hwQueueContext->mParentContext = hContext;
    hwQueueContext->mIsParentContext = false;
    hwQueueContext->mIsHwQueue = true;
}

void GpuTrace::UnregisterContext(uint64_t hContext)
{
    // Sometimes there are duplicate stop events so it's ok if it's already
    // removed
    auto ii = mContexts.find(hContext);
    auto ie = mContexts.end();
    if (ii != ie) {
        auto isParentContext = ii->second.mIsParentContext;
        mContexts.erase(ii);

        if (isParentContext) {
            for (ii = mContexts.begin(); ii != ie; ) {
                auto const& context = ii->second;
                if (context.mParentContext == hContext) {
                    DebugAssert(context.mIsHwQueue);
                    delete context.mNode;
                    ii = mContexts.erase(ii);
                } else {
                    ++ii;
                }
            }
        }
    }
}

void GpuTrace::SetEngineType(uint64_t pDxgAdapter, uint32_t nodeOrdinal, Microsoft_Windows_DxgKrnl::DXGK_ENGINE engineType)
{
    // Node should already be created (DxgKrnl::Context_Start comes
    // first) but just to be sure...
    auto node = &mNodes[pDxgAdapter].emplace(nodeOrdinal, Node{}).first->second;

    if (engineType == Microsoft_Windows_DxgKrnl::DXGK_ENGINE::VIDEO_DECODE ||
        engineType == Microsoft_Windows_DxgKrnl::DXGK_ENGINE::VIDEO_ENCODE ||
        engineType == Microsoft_Windows_DxgKrnl::DXGK_ENGINE::VIDEO_PROCESSING) {
        node->mIsVideo = true;
    }
}

void GpuTrace::SetContextProcessId(Context* context, uint32_t processId)
{
    auto p = mProcessFrameInfo.emplace(processId, ProcessFrameInfo{});

    if (mPMConsumer->mTrackGPUVideo && context->mNode->mIsVideo) {
        context->mPacketTrace = &p.first->second.mVideoEngines;
    } else {
        context->mPacketTrace = &p.first->second.mOtherEngines;
    }
}

void GpuTrace::StartPacket(PacketTrace* packetTrace, uint64_t timestamp) const
{
    packetTrace->mRunningPacketCount += 1;
    if (packetTrace->mRunningPacketCount == 1) {
        packetTrace->mRunningPacketStartTime = timestamp;
        if (packetTrace->mFirstPacketTime == 0) {
            packetTrace->mFirstPacketTime = timestamp;

            if (IsVerboseTraceEnabled()) {
                printf("                             GPU: pid=%u frame's first work\n", LookupPacketTraceProcessId(packetTrace));
            }
        }
    }
}

void GpuTrace::CompletePacket(PacketTrace* packetTrace, uint64_t timestamp) const
{
    if (IsVerboseTraceEnabled()) {
        DebugPrintAccumulatedGpuTime(LookupPacketTraceProcessId(packetTrace),
                                     packetTrace->mAccumulatedPacketTime,
                                     packetTrace->mRunningPacketStartTime,
                                     timestamp);
    }

    auto accumulatedTime = timestamp - packetTrace->mRunningPacketStartTime;

    packetTrace->mLastPacketTime = timestamp;
    packetTrace->mAccumulatedPacketTime += accumulatedTime;
    packetTrace->mRunningPacketStartTime = 0;
}

void GpuTrace::EnqueueWork(Context* context, uint32_t sequenceId, uint64_t timestamp, bool isWaitPacket)
{
    auto packetTrace = context->mPacketTrace;
    auto node = context->mNode;

    // A very rare (never observed) race exists where packetTrace can still be
    // nullptr here.  The context must have been created and this packet must
    // have been submitted to the queue before the capture started.
    //
    // In this case, we have to ignore the DMA packet otherwise the node and
    // process tracking will become out of sync.
    if (packetTrace == nullptr) {
        return;
    }

    uint32_t queueSize = (uint32_t) node->mQueue.size();
    if (node->mQueueCount == queueSize) {
        // If the queue is too small, enlarge it by 16 entries at a time.  I only expect this to
        // happen for the first packet observed on this node, which will result in sizing the queue
        // from 0 to 16.
        //
        // However, there are other cases where the queue entries seem to grow unexpectedly.  e.g.,
        // this seems to always happen when an application closes.  So, we place a reasonable
        // maximum limit on this to prevent unexpectedly growing the queues arbitrarily.
        if (node->mQueueCount >= 16*10) {
            return;
        }

        auto queueIndex = queueSize == 0
            ? 0
            : (node->mQueueIndex + node->mQueueCount) % queueSize;

        Node::EnqueuedPacket empty{};
        node->mQueue.insert(node->mQueue.begin() + queueIndex, 16, empty);
        queueSize += 16;
    }

    // Enqueue the packet.
    //
    // Wait packets aren't counted as GPU work, but we still need to enqueue
    // them so they block future work.  We encode wait packets by setting their
    // packetTrace to null.  This saves some memory as we don't need the
    // packetTrace pointer for wait packets.
    if (isWaitPacket) {
        packetTrace = nullptr;
    }

    auto queueIndex = (node->mQueueIndex + node->mQueueCount) % queueSize;
    auto entry = &node->mQueue[queueIndex];
    entry->mPacketTrace = packetTrace;
    entry->mSequenceId = sequenceId;
    entry->mCompleted = false;
    node->mQueueCount += 1;

    // If the queue was empty, the packet starts running right away, otherwise
    // it is just enqueued and will start running after all previous packets
    // complete.
    if (packetTrace != nullptr && node->mQueueCount == 1) {
        StartPacket(packetTrace, timestamp);
    }

    if (IsVerboseTraceEnabled()) {
        PrintRunningContexts();
    }
}

bool GpuTrace::CompleteWork(Context* context, uint32_t sequenceId, uint64_t timestamp)
{
    auto node = context->mNode;

    // It's possible to miss DmaPacket events during realtime analysis, so try
    // to handle it gracefully here.
    //
    // If we get a DmaPacket_Info event for a packet that we didn't get a
    // DmaPacket_Start event for (or that we ignored because we didn't know the
    // process yet) then sequenceId will be smaller than expected.  If this
    // happens, we ignore the DmaPacket_Info event which means that, if there
    // was idle time before the missing DmaPacket_Start event,
    // mAccumulatedPacketTime will be too large.
    //
    // measured: ----------------  -------     ---------------------
    //                                            [---   [--
    // actual:   [-----]  [-----]  [-----]     [-----]-----]-------]
    //           ^     ^  x     ^  ^     ^        x  ^   ^
    //           s1    i1 s2    i2 s3    i3       s2 i1  s3
    if (context->mPacketTrace == nullptr || node->mQueueCount == 0) {
        return false;
    }

    auto runningSequenceId = node->mQueue[node->mQueueIndex].mSequenceId;
    if (sequenceId < runningSequenceId) {
        return false;
    }

    // If we get a DmaPacket_Start event with no corresponding DmaPacket_Info,
    // then sequenceId will be larger than expected.  If this happens, we search
    // through the queue for a match and if no match was found then we ignore
    // this event (we missed both the DmaPacket_Start and DmaPacket_Info for
    // the packet).  In this case, both the missing packet's execution time as
    // well as any idle time afterwards will be associated with the previous
    // packet.
    //
    // If a match is found, then we don't know when the pre-match packets ended
    // (nor when the matched packet started).  We treat this case as if the
    // first packet with a missed DmaPacket_Info ran the whole time, and all
    // other packets up to the match executed with zero time.  Any idle time
    // during this range is ignored, and the correct association of gpu work to
    // process will not be correct (unless all these contexts come from the
    // same process).
    //
    // measured: -------  ----------------     ---------------------
    //                                            [---   [--
    // actual:   [-----]  [-----]  [-----]     [-----]-----]-------]
    //           ^     ^  ^     x  ^     ^        ^  ^     x
    //           s1    i1 s2    i2 s3    i3       s2 i1    i2
    if (sequenceId > runningSequenceId) {
        for (uint32_t missingCount = 1; ; ++missingCount) {
            if (missingCount == node->mQueueCount) {
                return false;
            }

            uint32_t queueIndex = (node->mQueueIndex + missingCount) % (uint32_t) node->mQueue.size();
            auto entry = &node->mQueue[queueIndex];
            if (entry->mSequenceId == sequenceId) {

                // On some 3000-series NVIDIA cards using hardware scheduling, we sometimes get
                // QueuePacket_Stop events for monitored fence packets out of order (too early).
                // This is NOT due to missed events, and any previous render packets should still be
                // considered running/enqueued.  So, we flag these packets as completed so that it
                // is immediately completed once it reaches the front of the queue.
                if (entry->mPacketTrace == nullptr) {
                    entry->mCompleted = true;
                    return true;
                }

                // Otherwise, move current packet into this slot
                *entry = node->mQueue[node->mQueueIndex];
                node->mQueueIndex = queueIndex;
                node->mQueueCount -= missingCount;
                break;
            }
        }
    }

    // If this was the process' last executing packet, accumulate the execution
    // duration into the process' count.
    auto entry = &node->mQueue[node->mQueueIndex];
    if (entry->mPacketTrace != nullptr) {
        entry->mPacketTrace->mRunningPacketCount -= 1;
        if (entry->mPacketTrace->mRunningPacketCount == 0) {
            CompletePacket(entry->mPacketTrace, timestamp);
        }
    }

    // Pop the completed packet from the queue, and start the next one.
    for (;;) {
        node->mQueueIndex = (node->mQueueIndex + 1) % (uint32_t) node->mQueue.size();
        node->mQueueCount -= 1;
        if (node->mQueueCount == 0) {
            break;
        }

        entry = &node->mQueue[node->mQueueIndex];
        if (entry->mPacketTrace != nullptr) {
            StartPacket(entry->mPacketTrace, timestamp);
            break;
        }

        if (!entry->mCompleted) {
            break;
        }
    }

    return true;
}

void GpuTrace::EnqueueQueuePacket(uint64_t hContext, uint32_t sequenceId, uint32_t processId, uint64_t timestamp, bool isWaitPacket)
{
    auto contextIter = mContexts.find(hContext);
    if (contextIter != mContexts.end()) {
        auto context = &contextIter->second;

        // Ensure that the process id is registered with this context, for
        // cases where the context was created before the capture was started
        // so we didn't see a Context_Start event.
        if (context->mPacketTrace == nullptr) {
            SetContextProcessId(context, processId);
        }

        // Use queue packet duration as a proxy for dma duration for cases we
        // don't get dma events for (HWS).
        if (context->mIsHwQueue) {
            EnqueueWork(context, sequenceId, timestamp, isWaitPacket);
        }
    }
}

void GpuTrace::CompleteQueuePacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp)
{
    auto contextIter = mContexts.find(hContext);
    if (contextIter != mContexts.end()) {
        auto context = &contextIter->second;

        // Use queue packet duration as a proxy for dma duration for cases we
        // don't get dma events for (HWS).
        if (context->mIsHwQueue) {
            auto trackedWorkWasCompleted = CompleteWork(context, sequenceId, timestamp);

            #pragma warning(suppress: 4127) // conditional expression is constant in release build
            if (IsVerboseTraceEnabled() && trackedWorkWasCompleted) {
                PrintRunningContexts();
            }
        }
    }
}

void GpuTrace::EnqueueDmaPacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp)
{
    // Lookup the context.  This can fail sometimes e.g. if parsing the
    // beginning of an ETL file where we can get packet events before the
    // context mapping.
    auto ii = mContexts.find(hContext);
    if (ii == mContexts.end()) {
        return;
    }
    auto context = &ii->second;

    // Should not see any dma packets on a HwQueue
    DebugAssert(!context->mIsHwQueue);

    // Start tracking the work
    bool isWaitPacket = false;
    EnqueueWork(context, sequenceId, timestamp, isWaitPacket);
}

void GpuTrace::CompleteDmaPacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp)
{
    // Lookup the context.  This can fail sometimes e.g. if parsing the
    // beginning of an ETL file where we can get packet events before the
    // context mapping.
    auto ii = mContexts.find(hContext);
    if (ii == mContexts.end()) {
        return;
    }
    auto context = &ii->second;

    // Should not see any dma packets on a HwQueue
    DebugAssert(!context->mIsHwQueue);

    // Stop tracking the work
    CompleteWork(context, sequenceId, timestamp);
}

void GpuTrace::CompleteFrame(PresentEvent* pEvent, uint64_t timestamp)
{
    // There are a few different events that can be used to complete the GPU
    // trace for each frame, e.g. QueuePacket_Stop for a present packet, or
    // MMIOFlipMultiPlaneOverlay_Info, and they can occur in any order so we
    // only apply the first one we see.
    if (pEvent->GpuFrameCompleted) {
        return;
    }

    VerboseTraceBeforeModifyingPresent(pEvent);
    pEvent->GpuFrameCompleted = true;

    auto ii = mProcessFrameInfo.find(pEvent->ProcessId);
    if (ii != mProcessFrameInfo.end()) {
        auto frameInfo = &ii->second;
        auto packetTrace = &frameInfo->mOtherEngines;
        auto videoTrace = &frameInfo->mVideoEngines;

        // Update GPUStartTime/ReadyTime/GPUDuration if any DMA packets were
        // observed.
        if (packetTrace->mFirstPacketTime != 0) {
            pEvent->GPUStartTime = packetTrace->mFirstPacketTime;
            pEvent->ReadyTime = packetTrace->mLastPacketTime;
            pEvent->GPUDuration = packetTrace->mAccumulatedPacketTime;
            packetTrace->mFirstPacketTime = 0;
            packetTrace->mLastPacketTime = 0;
            packetTrace->mAccumulatedPacketTime = 0;
        }

        pEvent->GPUVideoDuration = videoTrace->mAccumulatedPacketTime;
        videoTrace->mFirstPacketTime = 0;
        videoTrace->mLastPacketTime = 0;
        videoTrace->mAccumulatedPacketTime = 0;

        if (IsVerboseTraceEnabled()) {
            printf("                             GPU: pid=%u completing frame\n", pEvent->ProcessId);
        }

        // There are some cases where the QueuePacket_Stop timestamp is before
        // the previous dma packet completes.  e.g., this seems to be typical
        // of DWM present packets.  In these cases, instead of loosing track of
        // the previous dma work, we split it at this time and assign portions
        // to both frames.  Note this is incorrect, as the dma's full cost
        // should be fully attributed to the previous frame.
        if (packetTrace->mRunningPacketCount > 0) {

            pEvent->ReadyTime = timestamp;

            auto accumulatedTime = timestamp - packetTrace->mRunningPacketStartTime;
            if (accumulatedTime > 0) {
                if (IsVerboseTraceEnabled()) {
                    DebugPrintAccumulatedGpuTime(pEvent->ProcessId,
                                                 pEvent->GPUDuration,
                                                 packetTrace->mRunningPacketStartTime,
                                                 timestamp);
                    printf("                             GPU: work still running; splitting and considering as new work for next frame\n");
                }

                pEvent->GPUDuration += accumulatedTime;
                packetTrace->mFirstPacketTime = timestamp;
                packetTrace->mRunningPacketStartTime = timestamp;
            }
        }
        if (videoTrace->mRunningPacketCount > 0) {
            pEvent->GPUVideoDuration += timestamp - videoTrace->mRunningPacketStartTime;
            videoTrace->mFirstPacketTime = timestamp;
            videoTrace->mRunningPacketStartTime = timestamp;
        }
    }

    // If we did not track any GPU work, use the frame completion time as the
    // ready time.
    if (pEvent->ReadyTime == 0) {
        pEvent->ReadyTime = timestamp;
    }
}
