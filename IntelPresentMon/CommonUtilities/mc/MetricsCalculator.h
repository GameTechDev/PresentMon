// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include "../qpc.h"
#include "MetricsTypes.h"
#include "SwapChainState.h"

namespace pmon::util::metrics
{
    // Result of metric calculation for one display index
    struct ComputedMetrics {
        FrameMetrics metrics;

        // State changes to apply to SwapChain
        struct StateDeltas {
            std::optional<double> newEmaInput2FrameStart;
            std::optional<double> newAccumulatedInput2FrameStart;
            std::optional<uint64_t> newLastReceivedPclSimStart;
            std::optional<uint64_t> newLastReceivedPclInputTime;
            std::optional<uint64_t> lastReceivedNotDisplayedAllInputTime;
            std::optional<uint64_t> lastReceivedNotDisplayedMouseClickTime;
            std::optional<uint64_t> lastReceivedNotDisplayedAppProviderInputTime;
            bool shouldResetInputTimes = false;
        } stateDeltas;
    };

    // Index calculation helper
    struct DisplayIndexing {
        size_t startIndex;      // First display index to process
        size_t endIndex;        // One past last index
        size_t appIndex;        // Index of app frame (or SIZE_MAX if none)
        bool hasNextDisplayed;

        static DisplayIndexing Calculate(
            const FrameData& present,
            const FrameData* nextDisplayed);
    };

    std::vector<ComputedMetrics> ComputeMetricsForPresent(
        const QpcConverter& qpc,
        const FrameData& present,
        FrameData* nextDisplayed,
        SwapChainCoreState& chainState);

    // === Pure Calculation Functions ===

    ComputedMetrics ComputeFrameMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        bool isDisplayed,
        bool isAppFrame,
        const SwapChainCoreState& chain);

    // Helper: Calculate CPU start time
    uint64_t CalculateCPUStart(
        const SwapChainCoreState& chainState,
        const FrameData& present);

    // Helper: Calculate simulation start time (for animation error)
    uint64_t CalculateSimStartTime(
        const SwapChainCoreState& chainState,
        const FrameData& present,
        AnimationErrorSource source);

    // Helper: Calculate animation time
    double CalculateAnimationTime(
        const QpcConverter& qpc,
        uint64_t firstAppSimStartTime,
        uint64_t currentSimTime);
}