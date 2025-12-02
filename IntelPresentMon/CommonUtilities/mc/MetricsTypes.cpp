// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics {

    FrameData FrameData::CopyFrameData(const PmNsmPresentEvent& p) {
        FrameData frame{};

        frame.presentStartTime = p.PresentStartTime;
        frame.readyTime = p.ReadyTime;
        frame.timeInPresent = p.TimeInPresent;
        frame.gpuStartTime = p.GPUStartTime;
        frame.gpuDuration = p.GPUDuration;
        frame.gpuVideoDuration = p.GPUVideoDuration;

        frame.appPropagatedPresentStartTime = p.AppPropagatedPresentStartTime;
        frame.appPropagatedTimeInPresent = p.AppPropagatedTimeInPresent;
        frame.appPropagatedGPUStartTime = p.AppPropagatedGPUStartTime;
        frame.appPropagatedReadyTime = p.AppPropagatedReadyTime;
        frame.appPropagatedGPUDuration = p.AppPropagatedGPUDuration;
        frame.appPropagatedGPUVideoDuration = p.AppPropagatedGPUVideoDuration;

        frame.appSleepStartTime = p.AppSleepStartTime;
        frame.appSleepEndTime = p.AppSleepEndTime;
        frame.appSimStartTime = p.AppSimStartTime;
        frame.appSleepEndTime = p.AppSleepEndTime;
        frame.appRenderSubmitStartTime = p.AppRenderSubmitStartTime;
        frame.appRenderSubmitEndTime = p.AppRenderSubmitEndTime;
        frame.appPresentStartTime = p.AppPresentStartTime;
        frame.appPresentEndTime = p.AppPresentEndTime;
        frame.appInputSample = { p.AppInputTime, p.AppInputType };

        frame.inputTime = p.InputTime;
        frame.mouseClickTime = p.MouseClickTime;

        frame.pclSimStartTime = p.PclSimStartTime;
        frame.pclInputPingTime = p.PclInputPingTime;
        frame.flipDelay = p.FlipDelay;
        frame.flipToken = p.FlipToken;

        // Normalize parallel arrays to vector<DisplayEntry>
        frame.displayed.reserve(p.DisplayedCount);
        for (size_t i = 0; i < p.DisplayedCount; ++i) {
            frame.displayed.push_back({
                p.Displayed_FrameType[i],
                p.Displayed_ScreenTime[i]
                });
        }

        frame.finalState = p.FinalState;
        frame.swapChainAddress = p.SwapChainAddress;
        frame.frameId = p.FrameId;
        frame.processId = p.ProcessId;
        frame.threadId = p.ThreadId;
        frame.appFrameId = p.AppFrameId;

        return frame;
    }

    FrameData FrameData::CopyFrameData(const std::shared_ptr<PresentEvent>& p) {
        FrameData frame{};

        frame.presentStartTime = p->PresentStartTime;
        frame.readyTime = p->ReadyTime;
        frame.timeInPresent = p->TimeInPresent;
        frame.gpuStartTime = p->GPUStartTime;
        frame.gpuDuration = p->GPUDuration;
        frame.gpuVideoDuration = p->GPUVideoDuration;

        frame.appPropagatedPresentStartTime = p->AppPropagatedPresentStartTime;
        frame.appPropagatedTimeInPresent = p->AppPropagatedTimeInPresent;
        frame.appPropagatedGPUStartTime = p->AppPropagatedGPUStartTime;
        frame.appPropagatedReadyTime = p->AppPropagatedReadyTime;
        frame.appPropagatedGPUDuration = p->AppPropagatedGPUDuration;
        frame.appPropagatedGPUVideoDuration = p->AppPropagatedGPUVideoDuration;

        frame.appSleepStartTime = p->AppSleepStartTime;
        frame.appSleepEndTime = p->AppSleepEndTime;
        frame.appSimStartTime = p->AppSimStartTime;
        frame.appSleepEndTime = p->AppSleepEndTime;
        frame.appRenderSubmitStartTime = p->AppRenderSubmitStartTime;
        frame.appRenderSubmitEndTime = p->AppRenderSubmitEndTime;
        frame.appPresentStartTime = p->AppPresentStartTime;
        frame.appPresentEndTime = p->AppPresentEndTime;
        frame.appInputSample = p->AppInputSample;

        frame.inputTime = p->InputTime;
        frame.mouseClickTime = p->MouseClickTime;

        frame.pclSimStartTime = p->PclSimStartTime;
        frame.pclInputPingTime = p->PclInputPingTime;
        frame.flipDelay = p->FlipDelay;
        frame.flipToken = p->FlipToken;

        frame.displayed = p->Displayed;

        frame.finalState = p->FinalState;
        frame.swapChainAddress = p->SwapChainAddress;
        frame.frameId = p->FrameId;
        frame.processId = p->ProcessId;
        frame.threadId = p->ThreadId;
        frame.appFrameId = p->AppFrameId;

        return frame;
    }
}