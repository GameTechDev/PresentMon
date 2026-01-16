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
        // ---- Animation metrics ----
        std::optional<double> ComputeAnimationError(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime)
        {
            if (!isDisplayed || !isAppFrame) {
                return std::nullopt;
            }

            uint64_t currentSimStart = CalculateAnimationErrorSimStartTime(chain, present, chain.animationErrorSource);

            if (currentSimStart == 0 ||
                chain.lastDisplayedSimStartTime == 0 ||
                currentSimStart <= chain.lastDisplayedSimStartTime ||
                chain.lastDisplayedAppScreenTime == 0) {
                return std::nullopt;
            }

            double simElapsed = qpc.DeltaUnsignedMilliSeconds(chain.lastDisplayedSimStartTime, currentSimStart);
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(chain.lastDisplayedAppScreenTime, screenTime);

            if (simElapsed == 0.0 || displayElapsed == 0.0) {
                return std::nullopt;
            }

            return simElapsed - displayElapsed;
        }


        std::optional<double> ComputeAnimationTime(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame)
        {
            if (!isDisplayed || !isAppFrame) {
                return std::nullopt;
            }

            bool isFirstProviderSimTime =
                chain.animationErrorSource == AnimationErrorSource::CpuStart &&
                (present.appSimStartTime != 0 || present.pclSimStartTime != 0);
            if (isFirstProviderSimTime) {
                // Seed only: no animation time yet. UpdateAfterPresent will flip us
                // into AppProvider/PCL and latch firstAppSimStartTime.
                return std::nullopt;
            }

            uint64_t currentSimStart = CalculateAnimationErrorSimStartTime(chain, present, chain.animationErrorSource);
            if (currentSimStart == 0) {
                return std::nullopt;
            }

            return CalculateAnimationTime(qpc, chain.firstAppSimStartTime, currentSimStart);
        }

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

    void CalculateAnimationMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& swapChain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        uint64_t screenTime,
        FrameMetrics& metrics)
    {
        metrics.msAnimationError = ComputeAnimationError(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime);

        metrics.msAnimationTime = ComputeAnimationTime(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame);
    }
}
