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

        std::optional<double> ComputeInstrumentedSleep(
            const QpcConverter& qpc,
            const FrameData& present,
            bool isAppFrame)
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
            isAppFrame);

        metrics.msInstrumentedGpuLatency = ComputeInstrumentedGpuLatency(
            qpc,
            present,
            isAppFrame);

        metrics.msBetweenSimStarts = ComputeMsBetweenSimulationStarts(
            qpc,
            chain,
            present,
            isAppFrame);
    }
}
