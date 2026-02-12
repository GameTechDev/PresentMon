// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "MetricsCalculator.h"

namespace pmon::util::metrics
{
    // Category calculators implemented across MetricsCalculator_*.cpp translation units.
    void CalculateBasePresentMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        FrameMetrics& out);

    void CalculateDisplayMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        const SwapChainCoreState& swapChain,
        bool isDisplayed,
        uint64_t screenTime,
        uint64_t nextScreenTime,
        FrameMetrics& metrics);

    void CalculateCpuGpuMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& chainState,
        const FrameData& present,
        bool isAppFrame,
        FrameMetrics& metrics);

    void CalculateAnimationMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& swapChain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        uint64_t screenTime,
        FrameMetrics& metrics);

    void CalculateInputLatencyMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& swapChain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        FrameMetrics& metrics,
        ComputedMetrics::StateDeltas& stateDeltas);

    std::optional<double> CalculatePcLatency(
        const QpcConverter& qpc,
        const SwapChainCoreState& chain,
        const FrameData& present,
        bool isDisplayed,
        uint64_t screenTime,
        ComputedMetrics::StateDeltas& stateDeltas);

    void CalculateInstrumentedMetrics(
        const QpcConverter& qpc,
        const SwapChainCoreState& chain,
        const FrameData& present,
        bool isDisplayed,
        bool isAppFrame,
        uint64_t screenTime,
        FrameMetrics& metrics);

    // NVIDIA collapsed/runt correction helper used by ComputeMetricsForPresent.
    void AdjustScreenTimeForCollapsedPresentNV(
        FrameData& present,
        FrameData* nextDisplayedPresent,
        const uint64_t& lastDisplayedFlipDelay,
        const uint64_t& lastDisplayedScreenTime,
        uint64_t& screenTime,
        uint64_t& nextScreenTime,
        MetricsVersion version);
}