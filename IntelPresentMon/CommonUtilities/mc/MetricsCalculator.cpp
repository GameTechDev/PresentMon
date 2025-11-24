// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics
{
    DisplayIndexing DisplayIndexing::Calculate(
        const FrameData& present,
        const FrameData* nextDisplayed)
    {
        DisplayIndexing result{};

        // Get display count
        auto displayCount = present.getDisplayedCount();  // ConsoleAdapter/PresentSnapshot method

        // Check if displayed
        bool displayed = present.getFinalState() == PresentResult::Presented && displayCount > 0;

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
                auto frameType = present.getDisplayedFrameType(i);
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

    ComputedMetrics ComputeFrameMetrics(
        const QpcCalculator& qpc,
        const FrameData& present,
        const FrameData* nextDisplayed,
        const SwapChainCoreState& chain)
    {

        ComputedMetrics result{};

        return result;
    }

    // Helper: Calculate CPU start time
    uint64_t CalculateCPUStart(
        const SwapChainCoreState& chainState,
        const FrameData& present)
    {
        uint64_t cpuStart = 0;
        if (chainState.lastAppPresent.has_value()) {
            const auto& lastAppPresent = chainState.lastAppPresent.value();
            if (lastAppPresent.getAppPropagatedPresentStartTime() != 0) {
                cpuStart = lastAppPresent.getAppPropagatedPresentStartTime() +
                    lastAppPresent.getAppPropagatedTimeInPresent();
            }
            else {
                cpuStart = lastAppPresent.getPresentStartTime() +
                    lastAppPresent.getTimeInPresent();
            }
        }
        else {
            cpuStart = chainState.lastPresent.has_value() ?
                chainState.lastPresent->getPresentStartTime() + chainState.lastPresent->getTimeInPresent() : 0;
        }
        return cpuStart;
    }

    // Helper: Calculate simulation start time (for animation error)
    uint64_t CalculateSimStartTime(
        const SwapChainCoreState& chainState,
        const FrameData& present,
        AnimationErrorSource source)
    {
        uint64_t simStartTime = 0;
        if (source == AnimationErrorSource::CpuStart) {
            simStartTime = CalculateCPUStart(chainState, present);
        }
        else if (source == AnimationErrorSource::AppProvider) {
            simStartTime = present.getAppSimStartTime();
            if (simStartTime == 0) {
                // Fallback to CPU start
                simStartTime = CalculateCPUStart(chainState, present);
            }
        }
        else if (source == AnimationErrorSource::PCLatency) {
            simStartTime = present.getPclSimStartTime();
            if (simStartTime == 0) {
                // Fallback to CPU start
                simStartTime = CalculateCPUStart(chainState, present);
            }
        }
        return simStartTime;
    }

    // Helper: Calculate animation time
    double CalculateAnimationTime(
        const QpcCalculator& qpc,
        uint64_t firstAppSimStartTime,
        uint64_t currentSimTime)
    {
        double animationTime = 0.0;
        uint64_t firstSimStartTime = firstAppSimStartTime != 0 ? firstAppSimStartTime : qpc.GetStartTimestamp();
        if (currentSimTime > firstSimStartTime) {
            animationTime = qpc.TimestampDeltaToMilliSeconds(firstSimStartTime, currentSimTime);
        }
        return animationTime;
    }
}