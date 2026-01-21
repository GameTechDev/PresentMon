// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"
#include "../Math.h"

namespace pmon::util::metrics
{
    namespace
    {
        double ComputePresentStartTimeMs(
            const QpcConverter& qpc,
            const FrameData& present)
        {
            const auto startQpc = qpc.GetSessionStartTimestamp();
            return startQpc != 0 && present.presentStartTime != 0
                ? qpc.DeltaSignedMilliSeconds(startQpc, present.presentStartTime)
                : 0.0;
        }
    }

    void CalculateBasePresentMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        FrameMetrics& out)
    {
        out.timeInSeconds = present.presentStartTime;
        out.presentStartQpc = present.presentStartTime;
        out.presentStartMs = ComputePresentStartTimeMs(qpc, present);

        // Calculate the delta from the previous present (if one exists)
        // to the current present
        if (swapChain.lastPresent.has_value()) {
            out.msBetweenPresents = qpc.DeltaUnsignedMilliSeconds(
                swapChain.lastPresent->presentStartTime,
                present.presentStartTime);
        } else {
            out.msBetweenPresents = 0.0;
        }

        out.msInPresentApi = qpc.DurationMilliSeconds(present.timeInPresent);
        out.msUntilRenderStart = qpc.DeltaSignedMilliSeconds(
            present.presentStartTime,
            present.gpuStartTime);
        out.msUntilRenderComplete = qpc.DeltaSignedMilliSeconds(
            present.presentStartTime,
            present.readyTime);
        out.msGpuDuration = qpc.DurationMilliSeconds(present.gpuDuration);
        out.msVideoDuration = qpc.DurationMilliSeconds(present.gpuVideoDuration);
        out.msSinceInput = (present.inputTime == 0)
            ? 0.0
            : qpc.DurationMilliSeconds(present.presentStartTime - present.inputTime);

        // Copy metadata
        out.swapChainAddress = present.swapChainAddress;
        out.runtime = present.runtime;
        out.syncInterval = present.syncInterval;
        out.presentFlags = present.presentFlags;
        out.allowsTearing = present.supportsTearing;
        out.presentMode = present.presentMode;
    }
}
