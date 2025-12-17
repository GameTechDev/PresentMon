// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <vector>

#include "SwapChainState.h"
#include "MetricsCalculator.h" // ComputeMetricsForPresent()

namespace pmon::util::metrics
{
    // Phase 1: shared unified swapchain state machine (parallel to legacy UpdateChain()).
    //
    // - Owns sequencing (pending until next displayed) using FrameData snapshots.
    // - Advances SwapChainCoreState by calling ComputeMetricsForPresent() and discarding results.
    // - Does NOT do any CSV/console/middleware output (callers can ignore it).
    struct UnifiedSwapChain
    {
        SwapChainCoreState core;

        // Seed without needing a QPC converter (needed for console GetPresentProcessInfo() early-return).
        void Seed(FrameData present);

        // Feed every present into the unified swapchain (callers may ignore results).
        void OnPresent(const QpcConverter& qpc, FrameData present);

    private:
        // Matches the console pre-pass that removes Application<->Repeated flip pairs.
        static void SanitizeDisplayedRepeats(FrameData& present);
    };
}
