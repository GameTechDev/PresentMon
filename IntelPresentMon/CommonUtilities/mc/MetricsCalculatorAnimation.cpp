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
