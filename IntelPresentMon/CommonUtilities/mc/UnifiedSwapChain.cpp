// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"
#include "UnifiedSwapChain.h"
#include "../PresentData/PresentMonTraceConsumer.hpp"


namespace pmon::util::metrics
{
    void UnifiedSwapChain::SanitizeDisplayedRepeats(FrameData& present)
    {
        // Port of OutputThread.cpp::ReportMetrics() “Remove Repeated flips” pre-pass,
        // but applied to FrameData (so we don’t mutate PresentEvent).
        auto& d = present.displayed;
        for (size_t i = 0; i + 1 < d.size(); ) {
            const auto a = d[i].first;
            const auto b = d[i + 1].first;

            const bool app_then_rep = (a == FrameType::Application && b == FrameType::Repeated);
            const bool rep_then_app = (a == FrameType::Repeated && b == FrameType::Application);

            if (app_then_rep) {
                d.erase(d.begin() + i + 1);
            }
            else if (rep_then_app) {
                d.erase(d.begin() + i);
            }
            else {
                ++i;
            }
        }
    }

    void UnifiedSwapChain::Seed(FrameData present)
    {
        SanitizeDisplayedRepeats(present);

        // Mirror console baseline behavior:
        // first present just seeds history (no pending pipeline).
        core.pendingPresents.clear();
        core.UpdateAfterPresent(present);
    }

    void UnifiedSwapChain::OnPresent(const QpcConverter& qpc, FrameData present)
    {
        SanitizeDisplayedRepeats(present);

        // If unseeded, seed immediately and return.
        // (Must not require qpc for the seed path.)
        if (!core.lastPresent.has_value()) {
            core.pendingPresents.clear();
            core.UpdateAfterPresent(present);
            return;
        }

        const bool isDisplayed =
            (present.getFinalState() == PresentResult::Presented) &&
            (present.getDisplayedCount() > 0);

        // Match the console “pending until next displayed” rule:
        // - a displayed present blocks subsequent not-displayed presents until the next displayed arrives.
        if (isDisplayed) {
            // 1) Flush pending frames using this displayed present as nextDisplayed.
            //    (This finalizes the previously postponed last-display instance.)
            if (!core.pendingPresents.empty()) {
                FrameData nextDisplayed = present; // ComputeMetricsForPresent needs a non-const pointer
                for (const auto& blocked : core.pendingPresents) {
                    (void)ComputeMetricsForPresent(qpc, blocked, &nextDisplayed, core);
                }
            }

            // 2) Process this present with nextDisplayed=nullptr:
            //    - applies state deltas for all display instances it can resolve
            //    - DOES NOT call UpdateAfterPresent() yet (by design)
            (void)ComputeMetricsForPresent(qpc, present, nullptr, core);

            // 3) This present becomes the new “waiting for next displayed”
            core.pendingPresents.clear();
            core.pendingPresents.push_back(present);
            return;
        }

        // Not displayed:
        // - If nothing is waiting, process immediately (this path calls UpdateAfterPresent()).
        // - Otherwise, queue behind the waiting displayed present.
        if (core.pendingPresents.empty()) {
            (void)ComputeMetricsForPresent(qpc, present, nullptr, core);
        }
        else {
            core.pendingPresents.push_back(present);
        }
    }
}