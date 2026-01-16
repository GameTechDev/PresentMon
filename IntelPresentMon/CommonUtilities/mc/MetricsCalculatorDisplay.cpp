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
        // ---- Display metrics ----
        double ComputeMsUntilDisplayed(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime)
        {
            return isDisplayed
                ? qpc.DeltaUnsignedMilliSeconds(present.presentStartTime, screenTime)
                : 0.0;
        }


        // Helper dedicated to computing msBetweenDisplayChange, matching legacy ReportMetricsHelper behavior.
        double ComputeMsBetweenDisplayChange(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            bool isDisplayed,
            uint64_t screenTime)
        {
            return isDisplayed
                ? qpc.DeltaUnsignedMilliSeconds(chain.lastDisplayedScreenTime, screenTime)
                : 0.0;
        }


        // Helper dedicated to computing msDisplayedTime, matching legacy ReportMetricsHelper behavior.
        double ComputeMsDisplayedTime(
            const QpcConverter& qpc,
            bool isDisplayed,
            uint64_t screenTime,
            uint64_t nextScreenTime)
        {
            return isDisplayed
                ? qpc.DeltaUnsignedMilliSeconds(screenTime, nextScreenTime)
                : 0.0;
        }

        
        std::optional<double> ComputeMsFlipDelay(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed)
        {
            if (isDisplayed && present.flipDelay != 0) {
                return qpc.DurationMilliSeconds(present.flipDelay);
            }
            return std::nullopt;
        }


        double ComputeMsDisplayLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime)
        {
            const auto cpuStart = CalculateCPUStart(swapChain, present);
            return (isDisplayed && cpuStart != 0)
                ? qpc.DeltaUnsignedMilliSeconds(cpuStart, screenTime)
                : 0.0;
        }


        std::optional<double> ComputeMsReadyTimeToDisplayLatency(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime)
        {
            if (isDisplayed && present.readyTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(present.readyTime, screenTime);
            }
            return std::nullopt;
        }


    }

    // ---- NV collapsed/runt correction ----
    void AdjustScreenTimeForCollapsedPresentNV(
        FrameData& present,
        FrameData* nextDisplayedPresent,
        const uint64_t& lastDisplayedFlipDelay,
        const uint64_t& lastDisplayedScreenTime,
        uint64_t& screenTime,
        uint64_t& nextScreenTime,
        MetricsVersion version)
    {
        if (version == MetricsVersion::V1) {
            // NV1 collapsed/runt correction: legacy V1 adjusts the *current* present using the
            // previous displayed state when the last displayed screen time (adjusted by flip delay)
            // is greater than this present's screen time.
            if (lastDisplayedFlipDelay > 0 &&
                lastDisplayedScreenTime > screenTime &&
                !present.displayed.Empty()) {

                const uint64_t diff = lastDisplayedScreenTime - screenTime;
                present.flipDelay += diff;
                present.displayed[0].second = lastDisplayedScreenTime;
                screenTime = present.displayed[0].second;
            }
            return;
        }

        // nextDisplayedPresent should always be non-null for NV GPU.
        if (present.flipDelay && screenTime > nextScreenTime && nextDisplayedPresent) {
            // If screenTime that is adjusted by flipDelay is larger than nextScreenTime,
            // it implies this present is a collapsed present, or a runt frame.
            // So we adjust the screenTime and flipDelay of nextDisplayedPresent,
            // effectively making nextScreenTime equals to screenTime.

            nextDisplayedPresent->flipDelay += (screenTime - nextScreenTime);
            nextScreenTime = screenTime;
            nextDisplayedPresent->displayed[0].second = nextScreenTime;
        }
    }


    DisplayIndexing DisplayIndexing::Calculate(
        const FrameData& present,
        const FrameData* nextDisplayed)
    {
        DisplayIndexing result{};

        // Get display count
        auto displayCount = present.displayed.Size();  // ConsoleAdapter/PresentSnapshot method

        // Check if displayed
        bool displayed = present.finalState == PresentResult::Presented && displayCount > 0;

        // hasNextDisplayed
        result.hasNextDisplayed = (nextDisplayed != nullptr);

        // Figure out range to process based on three cases:
        // Case 1: Not displayed → empty range [0, 0)
        // Case 2: Displayed, no next → process [0..N-2], postpone N-1 → range [0, N-1)
        // Case 3: Displayed, with next → process postponed [N-1] → range [N-1, N)

        if (!displayed || displayCount == 0) {
            // Case 1: Not displayed
            result.startIndex = 0;
            result.endIndex = 0;  // Empty range
        }
        else if (nextDisplayed == nullptr) {
            // Case 2: Postpone last display
            result.startIndex = 0;
            result.endIndex = displayCount - 1;  // One past [N-2] = [N-1] (excludes last!)
        }
        else {
            // Case 3: Process postponed last display
            result.startIndex = displayCount - 1;
            result.endIndex = displayCount;  // One past [N-1] = [N]
        }

        // appIndex - find first NotSet or Application frame
        // Search from startIndex through ALL displays (not just the processing range)
        result.appIndex = std::numeric_limits<size_t>::max();
        if (displayCount > 0) {
            for (size_t i = result.startIndex; i < displayCount; ++i) {
                auto frameType = present.displayed[i].first;
                if (frameType == FrameType::NotSet || frameType == FrameType::Application) {
                    result.appIndex = i;
                    break;
                }
            }
        }
        else {
            result.appIndex = 0;
        }
        return result;
    }


    void CalculateDisplayMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        bool isDisplayed,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        FrameMetrics& metrics)
    {
        metrics.msUntilDisplayed = ComputeMsUntilDisplayed(qpc, present, isDisplayed, screenTime);
        metrics.msBetweenDisplayChange = ComputeMsBetweenDisplayChange(qpc, swapChain, isDisplayed, screenTime);
        metrics.msDisplayedTime = ComputeMsDisplayedTime(qpc, isDisplayed, screenTime, nextScreenTime);
        metrics.msFlipDelay = ComputeMsFlipDelay(qpc, present, isDisplayed);
        metrics.msDisplayLatency = ComputeMsDisplayLatency(qpc, swapChain, present, isDisplayed, screenTime);
        metrics.msReadyTimeToDisplayLatency = ComputeMsReadyTimeToDisplayLatency(qpc, present, isDisplayed, screenTime);
        metrics.isDroppedFrame = !isDisplayed;
        metrics.screenTimeQpc = screenTime;
    }
}
