// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"

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
        // Helper dedicated to computing msUntilDisplayed, matching legacy ReportMetricsHelper behavior.

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


        // ---- CPU/GPU metrics ----
        double ComputeMsCpuBusy(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            const auto cpuStart = CalculateCPUStart(swapChain, present);
            if (cpuStart == 0) {
                return 0.0;
            }

            if (present.appPropagatedPresentStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.appPropagatedPresentStartTime);
            }
            if (present.presentStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.presentStartTime);
            }
            return 0.0;
        }


        double ComputeMsCpuWait(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            if (present.appPropagatedTimeInPresent != 0) {
                return qpc.DurationMilliSeconds(present.appPropagatedTimeInPresent);
            }
            if (present.timeInPresent != 0) {
                return qpc.DurationMilliSeconds(present.timeInPresent);
            }
            return 0.0;
        }


        double ComputeMsGpuLatency(
            const QpcConverter& qpc,
            const SwapChainCoreState& swapChain,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            const auto cpuStart = CalculateCPUStart(swapChain, present);
            if (cpuStart == 0) {
                return 0.0;
            }

            if (present.appPropagatedGPUStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.appPropagatedGPUStartTime);
            }
            if (present.gpuStartTime != 0) {
                return qpc.DeltaUnsignedMilliSeconds(cpuStart, present.gpuStartTime);
            }
            return 0.0;
        }


        double ComputeMsGpuBusy(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            //out.msGPUBusy = std::nullopt;
            double msGPUBusy = 0.0;
            if (isAppPresent) {
                if (present.appPropagatedGPUDuration != 0) {
                    msGPUBusy = qpc.DurationMilliSeconds(present.appPropagatedGPUDuration);
                }
                else if (present.gpuDuration != 0) {
                    msGPUBusy = qpc.DurationMilliSeconds(present.gpuDuration);
                }
            }
            return msGPUBusy;
        }

        double ComputeMsVideoBusy(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            if (!isAppPresent) {
                return 0.0;
            }

            if (present.appPropagatedGPUVideoDuration != 0) {
                return qpc.DurationMilliSeconds(present.appPropagatedGPUVideoDuration);
            }
            if (present.gpuVideoDuration != 0) {
                return qpc.DurationMilliSeconds(present.gpuVideoDuration);
            }
            return 0.0;
        }


        double ComputeMsGpuDuration (
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            //msGPUDuration = std::nullopt;
            double msGPUDuration = 0.0;
            if (isAppPresent) {
                if (present.appPropagatedGPUStartTime != 0 || present.appPropagatedReadyTime != 0) {
                    msGPUDuration = qpc.DeltaUnsignedMilliSeconds(present.appPropagatedGPUStartTime, present.appPropagatedReadyTime);
                }
                else if (present.gpuStartTime != 0 || present.readyTime != 0) {
                    msGPUDuration = qpc.DeltaUnsignedMilliSeconds(present.gpuStartTime, present.readyTime);
                }
            }
            return msGPUDuration;
        }

        double ComputeMsGpuWait(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppPresent)
        {
            return std::max(0.0,
                ComputeMsGpuDuration(qpc, present, isAppPresent) -
                ComputeMsGpuBusy(qpc, present, isAppPresent));
        }


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
                    !present.displayed.empty()) {

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
                } else {
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
                } else {
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
                } else {
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

        // ---- Instrumented metrics ----
        std::optional<double> ComputeInstrumentedLatency(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime)
        {
            if (!isDisplayed || !isAppFrame) {
                return std::nullopt;
            }

            auto instrumentedStartTime = present.appSleepEndTime != 0 ?
                present.appSleepEndTime : present.appSimStartTime;

            if (instrumentedStartTime == 0) {
                // No instrumented start time: nothing to compute.
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(instrumentedStartTime, screenTime);
        }

        std::optional<double> ComputeInstrumentedRenderLatency(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime)
        {
            if (!isDisplayed || !isAppFrame) {
                return std::nullopt;
            }

            if (present.appRenderSubmitStartTime == 0) {
                // No app provided render submit start time: nothing to compute.
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(present.appRenderSubmitStartTime, screenTime);
        }

        std::optional<double> ComputeInstrumentedSleep(const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame,
            uint64_t screenTime)
        {
            if (!isAppFrame) {
                return std::nullopt;
            }

            if (present.appSleepStartTime == 0 || present.appSleepEndTime == 0) {
                // No app provided sleep times: nothing to compute.
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(present.appSleepStartTime, present.appSleepEndTime);
        }

        std::optional<double> ComputeInstrumentedGpuLatency(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isDisplayed,
            bool isAppFrame)
        {
            if (!isAppFrame) {
                return std::nullopt;
            }

            auto instrumentedStartTime = present.appSleepEndTime != 0 ?
                present.appSleepEndTime : present.appSimStartTime;

            if (instrumentedStartTime == 0) {
                // No provider sleep end or sim start time: nothing to compute.
                return std::nullopt;
            }

            if (present.gpuStartTime == 0) {
                // No GPU start time: nothing to compute.
                return std::nullopt;
            }

            return qpc.DeltaUnsignedMilliSeconds(instrumentedStartTime, present.gpuStartTime);
        }

        // ---- Simulation metrics ----
        std::optional<double> ComputeMsBetweenSimulationStarts(
            const QpcConverter& qpc,
            const SwapChainCoreState& chain,
            const FrameData& present,
            bool isAppFrame)
        {
            if (!isAppFrame) {
                return std::nullopt;
            }

            // The current sim start time is only dependent on the current frame's simulation start times.
            // Preference is PCL, then App.
            uint64_t currentSimStartTime = 0;
            if (present.pclSimStartTime != 0) {
                currentSimStartTime = present.pclSimStartTime;
            }
            else if (present.appSimStartTime != 0) {
                currentSimStartTime = present.appSimStartTime;
            }
            if (chain.lastSimStartTime != 0 && currentSimStartTime != 0 &&
                currentSimStartTime > chain.lastSimStartTime) {
                return qpc.DeltaUnsignedMilliSeconds(
                    chain.lastSimStartTime,
                    currentSimStartTime);
            }
            else {
                return std::nullopt;
            }
        }

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
    }


    // ============================================================================
    // 2) Public entry points
    // ============================================================================
    DisplayIndexing DisplayIndexing::Calculate(
        const FrameData& present,
        const FrameData* nextDisplayed)
    {
        DisplayIndexing result{};

        // Get display count
        auto displayCount = present.displayed.size();  // ConsoleAdapter/PresentSnapshot method

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

    std::vector<ComputedMetrics> ComputeMetricsForPresent(
        const QpcConverter& qpc,
        FrameData& present,
        FrameData* nextDisplayed,
        SwapChainCoreState& chainState,
        MetricsVersion version)
    {
        std::vector<ComputedMetrics> results;

        const auto displayCount = present.displayed.size();
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
            else if (nextDisplayed != nullptr && !nextDisplayed->displayed.empty()) {
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

        metrics.screenTimeQpc = screenTime;
    }

    void CalculateCpuGpuMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& chainState,
        const FrameData& present,
        bool isAppFrame,
        FrameMetrics& metrics)
    {
        metrics.msCPUBusy = ComputeMsCpuBusy(qpc, chainState, present, isAppFrame);
        metrics.msCPUWait = ComputeMsCpuWait(qpc, present, isAppFrame);
        metrics.msGPULatency = ComputeMsGpuLatency(qpc, chainState, present, isAppFrame);

        metrics.msGPUBusy = ComputeMsGpuBusy(qpc, present, isAppFrame);
        metrics.msVideoBusy = ComputeMsVideoBusy(qpc, present, isAppFrame);
        metrics.msGPUWait = ComputeMsGpuWait(qpc, present, isAppFrame);
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

    void CalculateInputLatencyMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& swapChain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        FrameMetrics& metrics,
        ComputedMetrics::StateDeltas& stateDeltas)
    {
        metrics.msClickToPhotonLatency = ComputeClickToPhotonLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            metrics.screenTimeQpc,
            stateDeltas);

        metrics.msAllInputPhotonLatency = ComputeAllInputToPhotonLatency(
            qpc,
            swapChain, 
            present, 
            isDisplayed, 
            isAppFrame, 
            metrics.screenTimeQpc,
            stateDeltas);

        metrics.msInstrumentedInputTime = ComputeInstrumentedInputToPhotonLatency(
            qpc,
            swapChain,
            present,
            isDisplayed,
            isAppFrame,
            metrics.screenTimeQpc,
            stateDeltas);
    }

    std::optional<double> CalculatePcLatency(
        const QpcConverter& qpc,
        const SwapChainCoreState& chain,
        const FrameData& present,
        bool isDisplayed,
        uint64_t screenTime,
        ComputedMetrics::StateDeltas& stateDeltas)
    {
        if (!isDisplayed) {
            if (present.pclSimStartTime != 0) {
                if (present.pclInputPingTime != 0) {
                    // This frame was dropped but we have valid pc latency input and simulation start
                    // times. Calculate the initial input to sim start time
                    stateDeltas.newAccumulatedInput2FrameStart = qpc.DeltaUnsignedMilliSeconds(
                        present.pclInputPingTime,
                        present.pclSimStartTime);
                }
                else if (chain.accumulatedInput2FrameStartTime != 0.f) {
                    // This frame was also dropped and there is no pc latency input time. However, since we have
                    // accumulated time this means we have a pending input that has had multiple dropped frames
                    // and has not yet hit the screen. Calculate the time between the last not displayed sim start and
                    // this sim start and add it to our accumulated total
                    stateDeltas.newAccumulatedInput2FrameStart = chain.accumulatedInput2FrameStartTime +
                        qpc.DeltaUnsignedMilliSeconds(
                            chain.lastReceivedNotDisplayedPclSimStart,
                            present.pclSimStartTime);
                }
                stateDeltas.newLastReceivedPclSimStart = present.pclSimStartTime;
            }
            return std::nullopt;
        }

        // Check to see if we have a valid PC Latency sim start time
        if (present.pclSimStartTime != 0) {
            if (present.pclInputPingTime != 0) {
                // Both the pclSimStartTime and pclInputPingTime are valid, use them to update
                // the Input to Frame Start EMA. Store in state deltas for later application.

                stateDeltas.newInput2FrameStartEma = pmon::util::CalculateEma(
                    chain.Input2FrameStartTimeEma,
                    qpc.DeltaUnsignedMilliSeconds(present.pclInputPingTime, present.pclSimStartTime),
                    0.1);

                // Defensively clear the tracking variables for when we have a dropped frame with a pc latency input
                stateDeltas.newAccumulatedInput2FrameStart = 0.0;
                stateDeltas.newLastReceivedPclSimStart = 0;
            }
            else {
                if (chain.accumulatedInput2FrameStartTime != 0.0) {
                    // This frame was displayed but we don't have a pc latency input time. However, there is accumulated time
                    // so there is a pending input that will now hit the screen. Add in the time from the last not
                    // displayed pc simulation start to this frame's pc simulation start.
                    stateDeltas.newAccumulatedInput2FrameStart = chain.accumulatedInput2FrameStartTime +
                        qpc.DeltaUnsignedMilliSeconds(
                            chain.lastReceivedNotDisplayedPclSimStart,
                            present.pclSimStartTime);

                    stateDeltas.newInput2FrameStartEma = pmon::util::CalculateEma(
                        chain.Input2FrameStartTimeEma,
                        *stateDeltas.newAccumulatedInput2FrameStart,
                        0.1);

                    // Reset the tracking variables for when we have a dropped frame with a pc latency input
                    stateDeltas.newAccumulatedInput2FrameStart = 0.0;
                    stateDeltas.newLastReceivedPclSimStart = 0;
                }
            }
        }

        auto simStartTime = present.pclSimStartTime != 0 ? present.pclSimStartTime : chain.lastSimStartTime;
        double input2FrameStartEma = stateDeltas.newInput2FrameStartEma.has_value() ?
            stateDeltas.newInput2FrameStartEma.value() : chain.Input2FrameStartTimeEma;
        if (input2FrameStartEma != 0.0 && simStartTime != 0) {
            return input2FrameStartEma + qpc.DeltaSignedMilliSeconds(simStartTime, screenTime);
        }
        else {
            return std::nullopt;
        }
    }

    void CalculateInstrumentedMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& chain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        uint64_t screenTime,
        FrameMetrics& metrics) {

        metrics.msInstrumentedLatency = ComputeInstrumentedLatency(
            qpc,
            present,
            isDisplayed,
            isAppFrame,
            screenTime);

        metrics.msInstrumentedRenderLatency = ComputeInstrumentedRenderLatency(
            qpc,
            present,
            isDisplayed,
            isAppFrame,
            screenTime);

        metrics.msInstrumentedSleep = ComputeInstrumentedSleep(
            qpc,
            present,
            isDisplayed,
            isAppFrame,
            screenTime);

        metrics.msInstrumentedGpuLatency = ComputeInstrumentedGpuLatency(
            qpc,
            present,
            isDisplayed,
            isAppFrame);

        metrics.msBetweenSimStarts = ComputeMsBetweenSimulationStarts(
            qpc,
            chain,
            present,
            isAppFrame);
    }

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