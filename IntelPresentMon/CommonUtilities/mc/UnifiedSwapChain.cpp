// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"
#include "UnifiedSwapChain.h"
#include "../PresentData/PresentMonTraceConsumer.hpp"


namespace pmon::util::metrics
{
    void UnifiedSwapChain::SanitizeDisplayedRepeatedPresents(FrameData& present)
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

    void UnifiedSwapChain::SeedFromFirstPresent(FrameData present)
    {

        // Mirror console baseline behavior:
        // first present just seeds history (no pending pipeline).
        swapChain.pendingPresents.clear();
        swapChain.UpdateAfterPresent(present);
    }

    // UnifiedSwapChain.cpp
    std::vector<UnifiedSwapChain::ReadyItem>
        UnifiedSwapChain::Enqueue(FrameData present)
    {
        SanitizeDisplayedRepeatedPresents(present);

        std::vector<ReadyItem> out;

        // Seed baseline
        if (!swapChain.lastPresent.has_value()) {
            SeedFromFirstPresent(std::move(present));
            return out;
        }

        const bool isDisplayed =
            (present.getFinalState() == PresentResult::Presented) &&
            (present.getDisplayedCount() > 0);

        if (isDisplayed) {
            // 1) Finalize previously waiting displayed
            if (waitingDisplayed_.has_value()) {
                out.push_back(ReadyItem{ std::move(*waitingDisplayed_), present /* nextDisplayed */ });
                waitingDisplayed_.reset();
            }
            
            // 2) Release blocked not-displayed frames
            while (!blocked_.empty()) {
                out.push_back(ReadyItem{ std::move(blocked_.front()), std::nullopt });
                blocked_.pop_front();
            }

            // 3) Current displayed is ready (all-but-last); becomes the new waitingDisplayed
            out.push_back(ReadyItem{ present /* copy */, std::nullopt });
            waitingDisplayed_ = std::move(present);
            return out;
        }

        // Not displayed
        if (waitingDisplayed_.has_value()) {
            blocked_.push_back(std::move(present));
            return out; // nothing ready yet
        }

        out.push_back(ReadyItem{ std::move(present), std::nullopt });
        return out;
    }
}