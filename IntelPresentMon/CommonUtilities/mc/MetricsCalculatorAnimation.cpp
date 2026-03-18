// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../Math.h"

namespace pmon::util::metrics
{
    namespace
    {
        struct AnimationSourceContext
        {
            AnimationErrorSource effectiveSource = AnimationErrorSource::CpuStart;
            uint64_t currentSimStart = 0;
            bool isTransitionFrame = false;
        };

        AnimationSourceContext ResolveAnimationSourceContext(
            const SwapChainCoreState& chain,
            const FrameData& present)
        {
            AnimationSourceContext context{};
            context.effectiveSource = chain.animationErrorSource;

            switch (chain.animationErrorSource) {
            case AnimationErrorSource::AppProvider:
                break;

            case AnimationErrorSource::PCLatency:
                if (present.appSimStartTime != 0) {
                    context.effectiveSource = AnimationErrorSource::AppProvider;
                    context.isTransitionFrame = true;
                }
                break;

            case AnimationErrorSource::CpuStart:
                if (present.appSimStartTime != 0) {
                    context.effectiveSource = AnimationErrorSource::AppProvider;
                    context.isTransitionFrame = true;
                }
                else if (present.pclSimStartTime != 0) {
                    context.effectiveSource = AnimationErrorSource::PCLatency;
                    context.isTransitionFrame = true;
                }
                break;
            }

            context.currentSimStart = CalculateAnimationErrorSimStartTime(
                chain,
                present,
                context.effectiveSource);

            return context;
        }

        // ---- Animation metrics ----
        double ComputeAnimationError(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime)
        {
            if (!isDisplayed || !isAppFrame) {
                return MissingFrameMetricValue();
            }

            const auto sourceContext = ResolveAnimationSourceContext(chain, present);

            if (sourceContext.isTransitionFrame) {
                return MissingFrameMetricValue();
            }

            if (sourceContext.currentSimStart == 0 ||
                chain.lastDisplayedSimStartTime == 0 ||
                sourceContext.currentSimStart <= chain.lastDisplayedSimStartTime ||
                chain.lastDisplayedAppScreenTime == 0) {
                return MissingFrameMetricValue();
            }

            double simElapsed = qpc.DeltaUnsignedMilliSeconds(
                chain.lastDisplayedSimStartTime,
                sourceContext.currentSimStart);
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(chain.lastDisplayedAppScreenTime, screenTime);

            if (simElapsed == 0.0 || displayElapsed == 0.0) {
                return MissingFrameMetricValue();
            }

            return simElapsed - displayElapsed;
        }


        double ComputeAnimationTime(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame)
        {
            if (!isDisplayed || !isAppFrame) {
                return MissingFrameMetricValue();
            }

            const auto sourceContext = ResolveAnimationSourceContext(chain, present);

            if (sourceContext.isTransitionFrame) {
                return 0.0;
            }

            if (sourceContext.effectiveSource != AnimationErrorSource::CpuStart &&
                sourceContext.currentSimStart == 0) {
                return MissingFrameMetricValue();
            }

            if (sourceContext.currentSimStart == 0) {
                return 0.0;
            }

            return CalculateAnimationTime(qpc, chain.firstAppSimStartTime, sourceContext.currentSimStart);
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
        out.frameId = present.frameId;
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
