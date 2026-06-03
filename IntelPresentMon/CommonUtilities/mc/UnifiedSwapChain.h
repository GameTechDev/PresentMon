// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <vector>

#include "../qpc.h"
#include "AnimationErrorTracker.h"
#include "DisplayFrameQueue.h"
#include "MetricsCalculator.h"
#include "SwapChainState.h"

namespace pmon::util::metrics
{
    struct ProcessPresentRow
    {
        FrameData present;
        ComputedMetrics computed;
    };

    struct UnifiedSwapChain
    {
        SwapChainCoreState swapChain;

        // Frame-generation pipeline entry point: seed first present, then Ingest, Apply, Publish.
        // Console, middleware, and API paths all use this entry point.
        std::vector<ProcessPresentRow> ProcessPresent(
            const QpcConverter& qpc,
            FrameData present);

        // Ingest only (display queue + animation tracker). Does not apply publish policy.
        // Unit tests may call this with ComputeMetricsForReadyDisplayRow to apply without publishing.
        std::vector<ReadyDisplayRow> EnqueueReadyDisplayRows(
            const QpcConverter& qpc,
            FrameData present);

        uint64_t GetLastPresentQpc() const;
        bool IsPrunableBefore(uint64_t minTimestampQpc) const;

        // Applied before both the V1 (ComputeMetricsForPresent) and V2 (EnqueueReadyDisplayRows)
        // paths so that a single Repeated entry adjacent to an Application is collapsed as in the
        // original OutputThread "Remove Repeated flips" pre-pass.
        static void SanitizeDisplayedRepeatedPresents(FrameData& present);

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
        std::vector<ProcessPresentRow> ApplyReadyDisplayRows(
            const QpcConverter& qpc,
            std::vector<ReadyDisplayRow>& ready);

        AnimationErrorTracker animationTracker;
        DisplayFrameQueue displayQueue;
    };
}
