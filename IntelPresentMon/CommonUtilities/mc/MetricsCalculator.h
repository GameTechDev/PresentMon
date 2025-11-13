// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <optional>
#include "QpcCalculator.h"
#include "MetricsTypes.h"

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

    // Context for single display index computation
    struct FrameComputationContext {
        size_t displayIndex;
        size_t appIndex;           // Index of application frame in display array
        bool displayed;
        uint64_t screenTime;
        uint64_t nextScreenTime;
        uint64_t cpuStart;
        uint64_t simStartTime;
    };

    // Index calculation helper
    struct DisplayIndexing {
        size_t startIndex;      // First display index to process
        size_t endIndex;        // One past last index
        size_t appIndex;        // Index of app frame (or SIZE_MAX if none)
        bool hasNextDisplayed;

        // Template version works with both ConsoleAdapter and PresentSnapshot
        template<typename PresentT>
        static DisplayIndexing Calculate(
            const PresentT& present,
            const PresentT* nextDisplayed);
    };

    // === Pure Calculation Functions ===

    // Main computation kernel (pure function)
    // Template version - works with different input types
    // PresentT: ConsoleAdapter (Strategy A) or PresentSnapshot (Strategy B for Console, always for GUI)
    // ChainT: SwapChainData (Console) or fpsSwapChainData (GUI) - both stable references
    template<typename PresentT, typename ChainT>
    ComputedMetrics ComputeFrameMetrics(
        const QpcCalculator& qpc,
        const PresentT& present,
        const PresentT* nextDisplayed,
        const ChainT& chain,  // Direct reference to stable map entry
        const FrameComputationContext& context);

    // Helper: Calculate CPU start time
    template<typename PresentT, typename ChainT>
    uint64_t CalculateCPUStart(
        const ChainT& chainState,
        const PresentT& present);

    // Helper: Calculate simulation start time (for animation error)
    template<typename PresentT, typename ChainT>
    uint64_t CalculateSimStartTime(
        const ChainT& chainState,
        const PresentT& present,
        AnimationErrorSource source);

    // Helper: Calculate animation time
    double CalculateAnimationTime(
        const QpcCalculator& qpc,
        uint64_t firstAppSimStartTime,
        uint64_t currentSimTime);
}