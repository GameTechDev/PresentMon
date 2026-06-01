// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"

#include <PresentData/PresentMonTraceConsumer.hpp>
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

            // Accumulated PC latency input->frame-start time
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

    }

    // 2) Public entry points
    // ============================================================================
    std::vector<ComputedMetrics> ComputeMetricsForPresent(
        const QpcConverter& qpc,
        FrameData& present,
        FrameData* nextDisplayed,
        SwapChainCoreState& chainState)
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
                chainState.lastDisplayedScreenTime,
                screenTime,
                nextScreenTime,
                isDisplayed,
                isAppFrame,
                frameType,
                AnimationDisplayContext{},
                chainState);

            ApplyStateDeltas(chainState, metrics.stateDeltas);

            results.push_back(std::move(metrics));

            chainState.UpdateAfterPresent(present);

            return results;
        }

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
            MetricsVersion::V1);

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
            chainState.lastDisplayedScreenTime,
            screenTime,
            nextScreenTime,
            isDisplayedInstance,
            isAppFrame,
            frameType,
            AnimationDisplayContext{},
            chainState);

        ApplyStateDeltas(chainState, metrics.stateDeltas);
        results.push_back(std::move(metrics));

        chainState.UpdateAfterPresent(present);
        return results;
    }

    ComputedMetrics ComputeMetricsForReadyDisplayRow(
        const QpcConverter& qpc,
        const ReadyDisplayRow& row,
        SwapChainCoreState& chainState)
    {
        auto metrics = ComputeFrameMetrics(
            qpc,
            row.present,
            row.previousDisplayedScreenTime,
            row.screenTime,
            row.nextScreenTime,
            row.isDisplayed,
            row.isAppFrame,
            row.frameType,
            row.animation,
            chainState);

        ApplyStateDeltas(chainState, metrics.stateDeltas);
        if (row.updateSwapChainAfterRow) {
            chainState.UpdateAfterReadyDisplayRow(row);
        }

        return metrics;
    }

    // ============================================================================
    // 3) Metric assembly helpers (ComputeFrameMetrics)
    // ============================================================================
    ComputedMetrics ComputeFrameMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        uint64_t previousDisplayedScreenTime,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        bool isDisplayed,
        bool isAppFrame,
        FrameType frameType,
        const AnimationDisplayContext& animation,
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
            previousDisplayedScreenTime,
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
            animation,
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

}
