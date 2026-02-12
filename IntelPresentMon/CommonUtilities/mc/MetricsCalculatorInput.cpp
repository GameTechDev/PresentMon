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
        // ---- Input latency metrics ----
        std::optional<double> ComputeClickToPhotonLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime,
            ComputedMetrics::StateDeltas& stateDeltas)
        {
            // Only app frames participate in click-to-photon.
            if (!isAppFrame) {
                return std::nullopt;
            }

            uint64_t inputTime = 0;

            // Case 1: This frame *has* a click.
            if (present.mouseClickTime != 0) {
                inputTime = present.mouseClickTime;

                if (!isDisplayed) {
                    // Not displayed: stash the click for a future displayed frame.
                    stateDeltas.lastReceivedNotDisplayedMouseClickTime = inputTime;
                    return std::nullopt;
                }
                else {
                    stateDeltas.shouldResetInputTimes = true;
                }
            }
            // Case 2: No click on this frame, but frame is displayed: see if we can
            // reuse a pending click from a previously dropped frame.
            else if (isDisplayed && chain.lastReceivedNotDisplayedMouseClickTime != 0) {
                inputTime = chain.lastReceivedNotDisplayedMouseClickTime;
                stateDeltas.shouldResetInputTimes = true;
            }

            // If we still have no inputTime, nothing to compute.
            if (inputTime == 0) {
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(inputTime, screenTime);
        }


        std::optional<double> ComputeAllInputToPhotonLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime,
            ComputedMetrics::StateDeltas& stateDeltas)
        {
            // Only app frames participate in click-to-photon.
            if (!isAppFrame) {
                return std::nullopt;
            }

            uint64_t inputTime = 0;

            // Case 1: This frame *has* a click.
            if (present.inputTime != 0) {
                inputTime = present.inputTime;

                if (!isDisplayed) {
                    // Not displayed: stash the click for a future displayed frame.
                    stateDeltas.lastReceivedNotDisplayedAllInputTime = inputTime;
                    return std::nullopt;
                }
                else {
                    stateDeltas.shouldResetInputTimes = true;
                }
            }
            // Case 2: No click on this frame, but frame is displayed: see if we can
            // reuse a pending click from a previously dropped frame.
            else if (isDisplayed && chain.lastReceivedNotDisplayedAllInputTime != 0) {
                inputTime = chain.lastReceivedNotDisplayedAllInputTime;
                stateDeltas.shouldResetInputTimes = true;
            }

            // If we still have no inputTime, nothing to compute.
            if (inputTime == 0) {
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(inputTime, screenTime);
        }

        std::optional<double> ComputeInstrumentedInputToPhotonLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime,
            ComputedMetrics::StateDeltas& stateDeltas)
        {
            // Only app frames participate in click-to-photon.
            if (!isAppFrame) {
                return std::nullopt;
            }

            uint64_t inputTime = 0;

            // Case 1: This frame *has* a click.
            if (present.appInputSample.first != 0) {
                inputTime = present.appInputSample.first;

                if (!isDisplayed) {
                    // Not displayed: stash the click for a future displayed frame.
                    stateDeltas.lastReceivedNotDisplayedAppProviderInputTime = inputTime;
                    return std::nullopt;
                }
                else {
                    stateDeltas.shouldResetInputTimes = true;
                }
            }
            // Case 2: No click on this frame, but frame is displayed: see if we can
            // reuse a pending click from a previously dropped frame.
            else if (isDisplayed && chain.lastReceivedNotDisplayedAppProviderInputTime != 0) {
                inputTime = chain.lastReceivedNotDisplayedAppProviderInputTime;
                stateDeltas.shouldResetInputTimes = true;
            }

            // If we still have no inputTime, nothing to compute.
            if (inputTime == 0) {
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(inputTime, screenTime);
        }
    }
    
    void CalculateInputLatencyMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& swapChain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        FrameMetrics& metrics,
        ComputedMetrics::StateDeltas& stateDeltas)
    {
        const uint64_t screenTime = metrics.screenTimeQpc;
        metrics.msClickToPhotonLatency = ComputeClickToPhotonLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime,
            stateDeltas);

        metrics.msAllInputPhotonLatency = ComputeAllInputToPhotonLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime,
            stateDeltas);

        metrics.msInstrumentedInputTime = ComputeInstrumentedInputToPhotonLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime,
            stateDeltas);
    }
}
