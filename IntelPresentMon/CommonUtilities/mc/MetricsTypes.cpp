// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics {

    FrameData FrameData::CopyFrameData(const PmNsmPresentEvent& p) {
        FrameData frame{};

        frame.runtime = p.Runtime;
        frame.presentMode = p.PresentMode;

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
        frame.appSimEndTime = p.AppSimEndTime;
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
        frame.displayed.Reserve(p.DisplayedCount);
        for (size_t i = 0; i < p.DisplayedCount; ++i) {
            frame.displayed.EmplaceBack(
                p.Displayed_FrameType[i],
                p.Displayed_ScreenTime[i]);
        }

        frame.swapChainAddress = p.SwapChainAddress;
        frame.syncInterval = p.SyncInterval;
        frame.presentFlags = p.PresentFlags;

        frame.finalState = p.FinalState;
        frame.supportsTearing = p.SupportsTearing;
        frame.frameId = p.FrameId;
        frame.processId = p.ProcessId;
        frame.threadId = p.ThreadId;
        frame.appFrameId = p.AppFrameId;
        frame.pclFrameId = p.PclFrameId;

        return frame;
    }

    FrameData FrameData::CopyFrameData(const std::shared_ptr<PresentEvent>& p) {
        FrameData frame{};

        frame.runtime = p->Runtime;
        frame.presentMode = p->PresentMode;
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
        frame.appSimEndTime = p->AppSimEndTime;
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

        frame.displayed.Assign(p->Displayed.begin(), p->Displayed.end());

        frame.swapChainAddress = p->SwapChainAddress;
        frame.syncInterval = p->SyncInterval;
        frame.presentFlags = p->PresentFlags;

        frame.finalState = p->FinalState;
        frame.supportsTearing = p->SupportsTearing;
        frame.frameId = p->FrameId;
        frame.processId = p->ProcessId;
        frame.threadId = p->ThreadId;
        frame.appFrameId = p->AppFrameId;
        frame.pclFrameId = p->PclFrameId;

        return frame;
    }

    FrameData FrameData::CopyFrameData(const PresentEvent& p) {
        FrameData frame{};

        frame.runtime = p.Runtime;
        frame.presentMode = p.PresentMode;
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
        frame.appSimEndTime = p.AppSimEndTime;
        frame.appRenderSubmitStartTime = p.AppRenderSubmitStartTime;
        frame.appRenderSubmitEndTime = p.AppRenderSubmitEndTime;
        frame.appPresentStartTime = p.AppPresentStartTime;
        frame.appPresentEndTime = p.AppPresentEndTime;
        frame.appInputSample = p.AppInputSample;

        frame.inputTime = p.InputTime;
        frame.mouseClickTime = p.MouseClickTime;

        frame.pclSimStartTime = p.PclSimStartTime;
        frame.pclInputPingTime = p.PclInputPingTime;
        frame.flipDelay = p.FlipDelay;
        frame.flipToken = p.FlipToken;

        frame.displayed.Assign(p.Displayed.begin(), p.Displayed.end());

        frame.swapChainAddress = p.SwapChainAddress;
        frame.syncInterval = p.SyncInterval;
        frame.presentFlags = p.PresentFlags;

        frame.finalState = p.FinalState;
        frame.supportsTearing = p.SupportsTearing;
        frame.frameId = p.FrameId;
        frame.processId = p.ProcessId;
        frame.threadId = p.ThreadId;
        frame.appFrameId = p.AppFrameId;
        frame.pclFrameId = p.PclFrameId;

        return frame;
    }
}
