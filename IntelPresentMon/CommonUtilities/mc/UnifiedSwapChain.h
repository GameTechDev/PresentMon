// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <deque>

#include "SwapChainState.h"
#include "MetricsCalculator.h" // ComputeMetricsForPresent()

namespace pmon::util::metrics
{
    struct UnifiedSwapChain
    {
        struct ReadyItem
        {
            FrameData present;
            std::optional<FrameData> nextDisplayed; // populated when flushing pending
        };

        SwapChainCoreState swapChain;

        // Seed without needing a QPC converter (needed for console GetPresentProcessInfo() early-return).
        void SeedFromFirstPresent(FrameData present);

        std::vector<ReadyItem> Enqueue(FrameData present);

    private:
        static void SanitizeDisplayedRepeatedPresents(FrameData& present);
        std::optional<FrameData> waitingDisplayed_;
        std::deque<FrameData> blocked_;
    };
}
