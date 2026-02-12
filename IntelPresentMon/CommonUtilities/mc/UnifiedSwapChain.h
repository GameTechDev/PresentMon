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
            FrameData present;                     // owned (used when presentPtr==nullptr)
            FrameData* presentPtr = nullptr;       // points into waitingDisplayed_ (optional)
            FrameData* nextDisplayedPtr = nullptr; // points into waitingDisplayed_ (optional)
        };

        SwapChainCoreState swapChain;

        // Seed without needing a QPC converter (needed for console GetPresentProcessInfo() early-return).
        void SeedFromFirstPresent(FrameData present);

        std::vector<ReadyItem> Enqueue(FrameData present, MetricsVersion version);

        uint64_t GetLastPresentQpc() const;
        bool IsPrunableBefore(uint64_t minTimestampQpc) const;

        // Frame statistics
        float avgCPUDuration = 0.f;
        float avgGPUDuration = 0.f;
        float avgDisplayLatency = 0.f;
        float avgDisplayedTime = 0.f;
        float avgMsUntilDisplayed = 0.f;
        float avgMsBetweenDisplayChange = 0.f;
        double emaInput2FrameStartTime = 0.f;
        double accumulatedInput2FrameStartTime = 0.f;

    private:
        static void SanitizeDisplayedRepeatedPresents(FrameData& present);
        std::optional<FrameData> waitingDisplayed;
        std::deque<FrameData> blocked;
    };
}
