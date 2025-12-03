// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"

namespace pmon::util::metrics
{
    namespace
    {
        // Helper dedicated to computing msUntilDisplayed, matching legacy ReportMetricsHelper behavior.
        void ComputeMsUntilDisplayed(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime,
            FrameMetrics& out)
        {
            if (isDisplayed) {
                out.msUntilDisplayed = qpc.DeltaUnsignedMilliSeconds(
                    present.getPresentStartTime(),
                    screenTime);
            }
            else {
                out.msUntilDisplayed = 0.0;
            }
        }

        // Helper dedicated to computing msBetweenDisplayChange, matching legacy ReportMetricsHelper behavior.
        void ComputeMsBetweenDisplayChange(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            bool isDisplayed,
            uint64_t screenTime,
            FrameMetrics& out)
        {
            if (isDisplayed) {
                out.msBetweenDisplayChange = qpc.DeltaUnsignedMilliSeconds(
                    chain.lastDisplayedScreenTime,
                    screenTime);
            }
            else {
                out.msBetweenDisplayChange = 0.0;
            }
        }

        // Helper dedicated to computing msDisplayedTime, matching legacy ReportMetricsHelper behavior.
        void ComputeMsDisplayedTime(
            const QpcConverter& qpc,
            bool isDisplayed,
            uint64_t screenTime,
            uint64_t nextScreenTime,
            FrameMetrics& out)
        {
            if (isDisplayed) {
                out.msDisplayedTime = qpc.DeltaUnsignedMilliSeconds(
                    screenTime,
                    nextScreenTime);
            }
            else {
                out.msDisplayedTime = 0.0;
            }
        }
        
        void ComputeMsFlipDelay(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            FrameMetrics& out)
        {
            if (isDisplayed && present.getFlipDelay() != 0) {
                out.msFlipDelay = qpc.DurationMilliSeconds(present.getFlipDelay());
            }
            else {
                out.msFlipDelay = 0.0;
            }
        }

        void ComputeMsDisplayLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime,
            FrameMetrics& out)
        {
            const auto cpuStart = CalculateCPUStart(swapChain, present);
            if (isDisplayed && cpuStart != 0) {
                out.msDisplayLatency = qpc.DeltaUnsignedMilliSeconds(cpuStart, screenTime);
            } else {
                out.msDisplayLatency = 0.0;
            }
        }

        void ComputeMsReadyTimeToDisplayLatency(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            uint64_t screenTime,
            FrameMetrics& out)
        {
            if (isDisplayed && present.getReadyTime() != 0) {
                out.msReadyTimeToDisplayLatency = qpc.DeltaUnsignedMilliSeconds(present.getReadyTime(), screenTime);
            }
            else {
                out.msReadyTimeToDisplayLatency = 0.0;
            }
        }

        void AdjustScreenTimeForCollapsedPresentNV(
            const FrameData& present,
            FrameData* nextDisplayedPresent,
            uint64_t& screenTime,
            uint64_t& nextScreenTime)
        {
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
    }

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

    std::vector<ComputedMetrics> ComputeMetricsForPresent(
        const QpcConverter& qpc,
        const FrameData& present,
        FrameData* nextDisplayed,
        SwapChainCoreState& chainState)
    {
        std::vector<ComputedMetrics> results;

        const auto displayCount = present.getDisplayedCount();
        const bool isDisplayed = present.getFinalState() == PresentResult::Presented && displayCount > 0;

        // Case 1: not displayed, return single not-displayed metrics
        if (!isDisplayed || displayCount == 0) {
            const uint64_t screenTime = 0;
            const uint64_t nextScreenTime = 0;
            const bool isDisplayed = false;
            const bool isAppFrame = false;

            auto metrics = ComputeFrameMetrics(
                qpc,
                present,
                screenTime,
                nextScreenTime,
                isDisplayed,
                isAppFrame,
                chainState);
            results.push_back(std::move(metrics));
            
            // TODO - Check and remove this comment ->
            // Matches old ReportMetricsHelper: UpdateChain is called for not-displayed frames.
            chainState.UpdateAfterPresent(present);

            return results;
        }

        // There is at least one displayed frame to process
        const auto indexing = DisplayIndexing::Calculate(present, nextDisplayed);

        // Determine if we should update the swap chain based on nextDisplayed
        const bool shouldUpdateSwapChain = (nextDisplayed != nullptr);

        for (size_t displayIndex = indexing.startIndex; displayIndex < indexing.endIndex; ++displayIndex) {
            uint64_t screenTime = present.getDisplayedScreenTime(displayIndex);
            uint64_t nextScreenTime = 0;

            if (displayIndex + 1 < displayCount) {
                // Next display instance of the same present
                nextScreenTime = present.getDisplayedScreenTime(displayIndex + 1);
            }
            else if (nextDisplayed != nullptr && nextDisplayed->getDisplayedCount() > 0) {
                // First display of the *next* presented frame
                nextScreenTime = nextDisplayed->getDisplayedScreenTime(0);
            }
            else {
                break;  // No next screen time available
            }

            AdjustScreenTimeForCollapsedPresentNV(present, nextDisplayed, screenTime, nextScreenTime);

            const bool isAppFrame = (displayIndex == indexing.appIndex);

            auto metrics = ComputeFrameMetrics(
                qpc,
                present,
                screenTime,
                nextScreenTime,
                isDisplayed,
                isAppFrame,
                chainState);
            results.push_back(std::move(metrics));
        }

        // Matches old ReportMetricsHelper:
        // - Case 2 (no nextDisplayed): no UpdateChain yet.
        // - Case 3 (has nextDisplayed): this is the call that finally updates the chain.
        if (shouldUpdateSwapChain) {
            chainState.UpdateAfterPresent(present);
        }

        return results;
    }

    void CalculateBasePresentMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        FrameMetrics& out)
    {
        out.timeInSeconds = present.presentStartTime;

        // Calculate the delta from the previous present (if one exists)
        // to the current present
        if (swapChain.lastPresent.has_value()) {
            out.msBetweenPresents = qpc.DeltaUnsignedMilliSeconds(
                swapChain.lastPresent->getPresentStartTime(),
                present.getPresentStartTime());
        } else {
            out.msBetweenPresents = 0.0;
        }

        out.msInPresentApi = qpc.DurationMilliSeconds(present.getTimeInPresent());
        out.msUntilRenderComplete = qpc.DeltaSignedMilliSeconds(
            present.getPresentStartTime(),
            present.getReadyTime());
    }

    void CalculateDisplayMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        bool isDisplayed,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        FrameMetrics& metrics) {

        // msUntilDisplayed depends only on whether this display instance is displayed and its screen time
        ComputeMsUntilDisplayed(
            qpc,
            present,
            isDisplayed,
            screenTime,
            metrics);

        // msBetweenDisplayChange depends on previous displayed screen time and the current screen time
        ComputeMsBetweenDisplayChange(
            qpc,
            swapChain,
            isDisplayed,
            screenTime,
            metrics);

        // msDisplayedTime depends on the current screen time and the next screen time
        ComputeMsDisplayedTime(
            qpc,
            isDisplayed,
            screenTime,
            nextScreenTime,
            metrics);

        // msFlipDelay depends if the current present has a flip delay
        ComputeMsFlipDelay(
            qpc,
            present,
            isDisplayed,
            metrics);
        
        // msDisplayLatency is the cpu start time to the current screen time
        ComputeMsDisplayLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            screenTime,
            metrics);

        // msReadyTimeToDisplayLatency is ready time to the current screen time
        ComputeMsReadyTimeToDisplayLatency(
            qpc,
            present,
            isDisplayed,
            screenTime,
            metrics);

        // screenTimeQpc is simply the current screen time
        metrics.screenTimeQpc = screenTime;
    }

    ComputedMetrics ComputeFrameMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        bool isDisplayed,
        bool isAppFrame,
        const SwapChainCoreState& chain)
    {

        ComputedMetrics result{};
        FrameMetrics& metrics = result.metrics;

        CalculateBasePresentMetrics(
            qpc,
            present,
            chain,
            metrics);

        CalculateDisplayMetrics(
            qpc,
            present,
            chain,
            isDisplayed,
            screenTime,
            nextScreenTime,
            metrics);

        metrics.cpuStartQpc = CalculateCPUStart(chain, present);

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
        const QpcConverter& qpc,
        uint64_t firstAppSimStartTime,
        uint64_t currentSimTime)
    {
        double animationTime = 0.0;
        uint64_t firstSimStartTime = firstAppSimStartTime != 0 ? firstAppSimStartTime : qpc.GetSessionStartTimestamp();
        if (currentSimTime > firstSimStartTime) {
            animationTime = qpc.DeltaUnsignedMilliSeconds(firstSimStartTime, currentSimTime);
        }
        return animationTime;
    }
}