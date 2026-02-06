// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"

namespace pmon::util::metrics {

    FrameData FrameData::CopyFrameData(const std::shared_ptr<PresentEvent>& p)
    {
        if (p) {
            return CopyFrameData(*p);
        }
        pmlog_error("Tried to copy frame data from empty PresentEvent ptr");
        return {};
    }

    FrameData FrameData::CopyFrameData(const PresentEvent& p)
    {
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
