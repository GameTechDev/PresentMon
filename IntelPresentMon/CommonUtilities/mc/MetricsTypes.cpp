// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics {

    PresentSnapshot PresentSnapshot::FromCircularBuffer(const PmNsmPresentEvent& p) {
        PresentSnapshot snap{};

        snap.presentStartTime = p.PresentStartTime;
        snap.readyTime = p.ReadyTime;
        snap.timeInPresent = p.TimeInPresent;
        snap.gpuStartTime = p.GPUStartTime;
        snap.gpuDuration = p.GPUDuration;
        snap.gpuVideoDuration = p.GPUVideoDuration;

        snap.appPropagatedPresentStartTime = p.AppPropagatedPresentStartTime;
        snap.appPropagatedTimeInPresent = p.AppPropagatedTimeInPresent;
        snap.appPropagatedGPUStartTime = p.AppPropagatedGPUStartTime;
        snap.appPropagatedReadyTime = p.AppPropagatedReadyTime;
        snap.appPropagatedGPUDuration = p.AppPropagatedGPUDuration;
        snap.appPropagatedGPUVideoDuration = p.AppPropagatedGPUVideoDuration;

        snap.appSleepStartTime = p.AppSleepStartTime;
        snap.appSleepEndTime = p.AppSleepEndTime;
        snap.appSimStartTime = p.AppSimStartTime;
        snap.appSleepEndTime = p.AppSleepEndTime;
        snap.appRenderSubmitStartTime = p.AppRenderSubmitStartTime;
        snap.appRenderSubmitEndTime = p.AppRenderSubmitEndTime;
        snap.appPresentStartTime = p.AppPresentStartTime;
        snap.appPresentEndTime = p.AppPresentEndTime;
        snap.appInputSample = { p.AppInputTime, p.AppInputType };

        snap.inputTime = p.InputTime;
        snap.mouseClickTime = p.MouseClickTime;

        snap.pclSimStartTime = p.PclSimStartTime;
        snap.pclInputPingTime = p.PclInputPingTime;
        snap.flipDelay = p.FlipDelay;
        snap.FlipToken = p.FlipToken;

        // Normalize parallel arrays to vector<DisplayEntry>
        snap.displayed.reserve(p.DisplayedCount);
        for (size_t i = 0; i < p.DisplayedCount; ++i) {
            snap.displayed.push_back({
                p.Displayed_FrameType[i],
                p.Displayed_ScreenTime[i]
                });
        }

        snap.finalState = p.FinalState;
        snap.swapChainAddress = p.SwapChainAddress;
        snap.frameId = p.FrameId;
        snap.processId = p.ProcessId;
        snap.threadId = p.ThreadId;
        snap.appFrameId = p.AppFrameId;

        return snap;
    }
}