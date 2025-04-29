// Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved
// SPDX-License-Identifier: MIT
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <unordered_map>
#include <windows.h>
#include <evntcons.h> // must include after windows.h

struct NvFlipRequest {
    uint64_t FlipDelay;
    uint32_t FlipToken;
};

struct NVTraceConsumer
{
    NVTraceConsumer();
    ~NVTraceConsumer();

    NVTraceConsumer(const NVTraceConsumer&) = delete;
    NVTraceConsumer& operator=(const NVTraceConsumer&) = delete;
    NVTraceConsumer(NVTraceConsumer&&) = delete;
    NVTraceConsumer& operator=(NVTraceConsumer&&) = delete;

    // ThreaId -> NV FlipRequest
    std::unordered_map<uint32_t, NvFlipRequest> mNvFlipRequestByThreadId;
    // vidPnSourceId -> flip qpcTime
    std::unordered_map<uint32_t, uint64_t> mLastFlipTimeByHead;

    void HandleNvidiaDisplayDriverEvent(EVENT_RECORD* const pEventRecord, PMTraceConsumer* const pmConsumer);
    void ApplyFlipDelay(PresentEvent* present, uint32_t threadId);

    uint32_t mLastFlipToken = 0;
};