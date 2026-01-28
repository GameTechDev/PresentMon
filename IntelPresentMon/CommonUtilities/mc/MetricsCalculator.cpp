// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"
#include "../IntelPresentMon/PresentMonUtils/StreamFormat.h"
#include "../Math.h"

// Layout: internal helpers -> entry points -> metric assembly -> exported helpers

namespace pmon::util::metrics
{
    // ============================================================================
    // 1) Internal helpers (file-local)
    // ============================================================================
    namespace
    {
        // ---- Swap-chain state application ----
        void ApplyStateDeltas(
            SwapChainCoreState& chainState,
            const ComputedMetrics::StateDeltas& d)
        {
            // If we consumed pending input from a dropped frame, clear all
            // "not displayed" input caches.
            if (d.shouldResetInputTimes)
            {
                chainState.lastReceivedNotDisplayedAllInputTime = 0;
                chainState.lastReceivedNotDisplayedMouseClickTime = 0;
                chainState.lastReceivedNotDisplayedAppProviderInputTime = 0;
                chainState.lastReceivedNotDisplayedPclSimStart = 0;
                chainState.lastReceivedNotDisplayedPclInputTime = 0;
            }

            // Dropped-frame input for all-input latency
            if (d.lastReceivedNotDisplayedAllInputTime)
            {
                chainState.lastReceivedNotDisplayedAllInputTime =
                    *d.lastReceivedNotDisplayedAllInputTime;
            }

            // Dropped-frame mouse click
            if (d.lastReceivedNotDisplayedMouseClickTime)
            {
                chainState.lastReceivedNotDisplayedMouseClickTime =
                    *d.lastReceivedNotDisplayedMouseClickTime;
            }

            // Dropped-frame app-provider input
            if (d.lastReceivedNotDisplayedAppProviderInputTime)
            {
                chainState.lastReceivedNotDisplayedAppProviderInputTime =
                    *d.lastReceivedNotDisplayedAppProviderInputTime;
            }

            // Dropped-frame PC Latency sim start/input
            if (d.newLastReceivedPclSimStart)
            {
                chainState.lastReceivedNotDisplayedPclSimStart =
                    *d.newLastReceivedPclSimStart;
            }

            // Accumulated PC latency input→frame-start time
            if (d.newAccumulatedInput2FrameStart)
            {
                chainState.accumulatedInput2FrameStartTime =
                    *d.newAccumulatedInput2FrameStart;
            }

            // Running EMA of PC latency input to frame-start time
            if (d.newInput2FrameStartEma)
            {
                chainState.Input2FrameStartTimeEma =
                    *d.newInput2FrameStartEma;
            }
        }
        
        double ComputeCPUStartTimeMs(
            const QpcConverter& qpc,
            const uint64_t& CPUStartTimeQpc)
        {
            const auto startQpc = qpc.GetSessionStartTimestamp();
            return (startQpc != 0 && CPUStartTimeQpc != 0)
                ? qpc.DeltaSignedMilliSeconds(startQpc, CPUStartTimeQpc)
                : 0.0;
        }

        double ComputeFpsFromMs(double ms)
        {
            return ms > 0.0 ? 1000.0 / ms : 0.0;
        }

    }

    // 2) Public entry points
    // ============================================================================
    std::vector<ComputedMetrics> ComputeMetricsForPresent(
        const QpcConverter& qpc,
        FrameData& present,
        FrameData* nextDisplayed,
        SwapChainCoreState& chainState,
        MetricsVersion version)
    {
        std::vector<ComputedMetrics> results;

        const auto displayCount = present.displayed.Size();
        const bool isDisplayed = present.finalState == PresentResult::Presented && displayCount > 0;

        // Case 1: not displayed, return single not-displayed metrics
        if (!isDisplayed || displayCount == 0) {
            const uint64_t screenTime = 0;
            const uint64_t nextScreenTime = 0;
            const bool isDisplayed = false;

            // Legacy-equivalent attribution: compute displayIndex/appIndex and derive isAppFrame.
            const auto indexing = DisplayIndexing::Calculate(present, nextDisplayed);
            const size_t displayIndex = indexing.startIndex; // Case 1 => 0
            const size_t appIndex = indexing.appIndex;

            const bool isAppFrame = (displayIndex == appIndex);
            const FrameType frameType = (displayCount > 0)
                ? present.displayed[displayIndex].first
                : FrameType::NotSet;

            auto metrics = ComputeFrameMetrics(
                qpc,
                present,
                screenTime,
                nextScreenTime,
                isDisplayed,
                isAppFrame,
                frameType,
                chainState);

            ApplyStateDeltas(chainState, metrics.stateDeltas);

            results.push_back(std::move(metrics));

            chainState.UpdateAfterPresent(present);

            return results;
        }

        // V1: displayed presents are computed immediately (no look-ahead / no postponing).
        // Emit exactly one row per present (legacy V1 behavior).
        if (version == MetricsVersion::V1) {
            const size_t displayIndex = 0;
            uint64_t screenTime = present.displayed[displayIndex].second;
            uint64_t nextScreenTime = 0;

            AdjustScreenTimeForCollapsedPresentNV(
                present,
                nextDisplayed,
                chainState.lastDisplayedFlipDelay,
                chainState.lastDisplayedScreenTime,
                screenTime,
                nextScreenTime,
                version);
            
            // This is so msDisplayedTime comes back as 0 instead of garbage for V1 single-row output.
            // TODO: Better option is to have display metrics be optional. Update metrics struct accordingly.
            nextScreenTime = screenTime;
            const auto indexing = DisplayIndexing::Calculate(present, nullptr);
            const bool isAppFrame = (displayIndex == indexing.appIndex);
            const bool isDisplayedInstance = isDisplayed && screenTime != 0;
            const FrameType frameType = isDisplayedInstance ? present.displayed[displayIndex].first : FrameType::NotSet;

            auto metrics = ComputeFrameMetrics(
                qpc,
                present,
                screenTime,
                nextScreenTime,
                isDisplayedInstance,
                isAppFrame,
                frameType,
                chainState);

            ApplyStateDeltas(chainState, metrics.stateDeltas);
            results.push_back(std::move(metrics));

            chainState.UpdateAfterPresent(present);
            return results;
        }

        // There is at least one displayed frame to process
        const auto indexing = DisplayIndexing::Calculate(present, nextDisplayed);

        // Determine if we should update the swap chain based on nextDisplayed
        const bool shouldUpdateSwapChain = (nextDisplayed != nullptr);

        for (size_t displayIndex = indexing.startIndex; displayIndex < indexing.endIndex; ++displayIndex) {
            uint64_t screenTime = present.displayed[displayIndex].second;
            uint64_t nextScreenTime = 0;

            if (displayIndex + 1 < displayCount) {
                // Next display instance of the same present
                nextScreenTime = present.displayed[displayIndex + 1].second;
            }
            else if (nextDisplayed != nullptr && !nextDisplayed->displayed.Empty()) {
                // First display of the *next* presented frame
                nextScreenTime = nextDisplayed->displayed[0].second;
            }
            else {
                break;  // No next screen time available
            }

            AdjustScreenTimeForCollapsedPresentNV(present, nextDisplayed, 0, 0, screenTime, nextScreenTime, version);

            const bool isAppFrame = (displayIndex == indexing.appIndex);
            const bool isDisplayedInstance = isDisplayed && screenTime != 0 && nextScreenTime != 0;
            const FrameType frameType = isDisplayedInstance ? present.displayed[displayIndex].first : FrameType::NotSet;

            auto metrics = ComputeFrameMetrics(
                qpc,
                present,
                screenTime,
                nextScreenTime,
                isDisplayedInstance,
                isAppFrame,
                frameType,
                chainState);

            ApplyStateDeltas(chainState, metrics.stateDeltas);

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

    // ============================================================================
    // 3) Metric assembly helpers (ComputeFrameMetrics)
    // ============================================================================
    ComputedMetrics ComputeFrameMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        bool isDisplayed,
        bool isAppFrame,
        FrameType frameType,
        const SwapChainCoreState& chain)
    {

        ComputedMetrics result{};
        FrameMetrics& metrics = result.metrics;

        metrics.frameType = frameType;

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

        CalculateCpuGpuMetrics(
            qpc,
            chain,
            present, 
            isAppFrame, 
            metrics);

        CalculateAnimationMetrics(
            qpc,
            chain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime,
            metrics);

        CalculateInputLatencyMetrics(
            qpc,
            chain,
            present,
            isDisplayed,
            isAppFrame,
            metrics,
            result.stateDeltas);

        metrics.msPcLatency = CalculatePcLatency(
            qpc,
            chain,
            present,
            isDisplayed,
            screenTime,
            result.stateDeltas);

        CalculateInstrumentedMetrics(
            qpc,
            chain,
            present,
            isDisplayed,
            isAppFrame,
            screenTime,
            metrics);

        metrics.cpuStartQpc = CalculateCPUStart(chain, present);
        metrics.cpuStartMs = ComputeCPUStartTimeMs(qpc, metrics.cpuStartQpc);

        metrics.fpsPresent = ComputeFpsFromMs(metrics.msBetweenPresents);
        metrics.fpsDisplay = ComputeFpsFromMs(metrics.msBetweenDisplayChange);
        metrics.fpsApplication = ComputeFpsFromMs(metrics.msCPUTime);

        return result;
    }

    // ============================================================================
    // 4) Exported helper definitions (declared in MetricsCalculator.h)
    // ============================================================================
    uint64_t CalculateCPUStart(
        const SwapChainCoreState& chainState,
        const FrameData& present)
    {
        uint64_t cpuStart = 0;
        if (chainState.lastAppPresent.has_value()) {
            const auto& lastAppPresent = chainState.lastAppPresent.value();
            if (lastAppPresent.appPropagatedPresentStartTime != 0) {
                cpuStart = lastAppPresent.appPropagatedPresentStartTime +
                    lastAppPresent.appPropagatedTimeInPresent;
            }
            else {
                cpuStart = lastAppPresent.presentStartTime +
                    lastAppPresent.timeInPresent;
            }
        }
        else {
            cpuStart = chainState.lastPresent.has_value() ?
                chainState.lastPresent->presentStartTime + chainState.lastPresent->timeInPresent : 0;
        }
        return cpuStart;
    }

    // Helper: Calculate simulation start time (for animation error)
    uint64_t CalculateAnimationErrorSimStartTime(
        const SwapChainCoreState& chainState,
        const FrameData& present,
        AnimationErrorSource source)
    {
        uint64_t simStartTime = 0;
        if (source == AnimationErrorSource::CpuStart) {
            simStartTime = CalculateCPUStart(chainState, present);
        }
        else if (source == AnimationErrorSource::AppProvider) {
            simStartTime = present.appSimStartTime;
        }
        else if (source == AnimationErrorSource::PCLatency) {
            simStartTime = present.pclSimStartTime;
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
