// Copyright (C) 2021-2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>
#include <unordered_map>

#include "etw/Microsoft_Windows_DxgKrnl.h"

struct PresentEvent;
struct PMTraceConsumer;

class GpuTrace {
    // PacketTrace is the execution information for each process' frame.
    struct PacketTrace {
        uint64_t mFirstPacketTime;         // QPC when the first packet started for the current frame
        uint64_t mLastPacketTime;          // QPC when the last packet completed for the current frame
        uint64_t mAccumulatedPacketTime;   // QPC duration while at least one packet was running during the current frame
        uint64_t mRunningPacketStartTime;  // QPC when the oldest, currently-running packet started on any node
        uint32_t mRunningPacketCount;      // Number of currently-running packets on any node
    };

    // Node is information about a particular GPU parallel node, including any
    // packets currently running/queued to it.  For implementations with
    // hardware scheduling enabled, there is one node per HWQueue many of which
    // may map to the same device engine.
    struct Node {
        struct EnqueuedPacket {
            PacketTrace* mPacketTrace;      // Frame trace for this packet
            uint32_t mSequenceId;           // Sequence ID for this packet
            bool mCompleted;                // Flag to signal that the packet completed out-of-order
        };
        std::vector<EnqueuedPacket> mQueue; // Ring buffer of current enqueued packets
        uint32_t mQueueIndex;               // Index into mQueue for currently-running packet
        uint32_t mQueueCount;               // Number of enqueued packets
        bool mIsVideo;
    };

    // Context is a process' gpu context, mapping a PacketTrace to a
    // particular Node.
    struct Context {
        PacketTrace* mPacketTrace;
        Node* mNode;
        uint64_t mParentContext;
        bool mIsParentContext;
        bool mIsHwQueue;
    };

    // State for tracking GPU execution per-frame, per-process
    struct ProcessFrameInfo {
        // Depending on mTrackGPUVideo, we may track video engines separately
        PacketTrace mVideoEngines;
        PacketTrace mOtherEngines;
    };

    std::unordered_map<uint64_t, std::unordered_map<uint32_t, Node> > mNodes;   // pDxgAdapter -> NodeOrdinal -> Node
    std::unordered_map<uint64_t, uint64_t> mDevices;                            // hDevice -> pDxgAdapter
    std::unordered_map<uint64_t, Context> mContexts;                            // hContext -> Context
    std::unordered_map<uint32_t, ProcessFrameInfo> mProcessFrameInfo;           // ProcessID -> ProcessFrameInfo
    std::unordered_map<uint64_t, uint32_t> mPagingSequenceIds;                  // SequenceID -> ProcessID

    // The parent trace consumer
    PMTraceConsumer* mPMConsumer;

    void SetContextProcessId(Context* context, uint32_t processId);

    void StartPacket(PacketTrace* packetTrace, uint64_t timestamp) const;
    void CompletePacket(PacketTrace* packetTrace, uint64_t timestamp) const;

    void EnqueueWork(Context* context, uint32_t sequenceId, uint64_t timestamp, bool isWaitPacket);
    bool CompleteWork(Context* context, uint32_t sequenceId, uint64_t timestamp);

    uint32_t LookupPacketTraceProcessId(PacketTrace* packetTrace) const;
    void PrintRunningContexts() const;

public:
    explicit GpuTrace(PMTraceConsumer* pmConsumer);

    void RegisterDevice(uint64_t hDevice, uint64_t pDxgAdapter);
    void UnregisterDevice(uint64_t hDevice);

    void RegisterContext(uint64_t hContext, uint64_t hDevice, uint32_t nodeOrdinal, uint32_t processId);
    void RegisterHwQueueContext(uint64_t hContext, uint64_t parentDxgHwQueue);
    void UnregisterContext(uint64_t hContext);

    void SetEngineType(uint64_t pDxgAdapter, uint32_t nodeOrdinal, Microsoft_Windows_DxgKrnl::DXGK_ENGINE engineType);

    void EnqueueQueuePacket(uint64_t hContext, uint32_t sequenceId, uint32_t processId, uint64_t timestamp, bool isWaitPacket);
    void CompleteQueuePacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp);

    void EnqueueDmaPacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp);
    void CompleteDmaPacket(uint64_t hContext, uint32_t sequenceId, uint64_t timestamp);

    void CompleteFrame(PresentEvent* pEvent, uint64_t timestamp);
};
