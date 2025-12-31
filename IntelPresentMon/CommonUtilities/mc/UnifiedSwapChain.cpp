// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsTypes.h"
#include "UnifiedSwapChain.h"
#include "../PresentData/PresentMonTraceConsumer.hpp"

#include <algorithm>


namespace pmon::util::metrics
{
    uint64_t UnifiedSwapChain::GetLastPresentQpc() const
    {
        return swapChain.lastPresent.has_value() ? swapChain.lastPresent->presentStartTime : 0;
    }

    bool UnifiedSwapChain::IsPrunableBefore(uint64_t minTimestampQpc) const
    {
        auto const last = GetLastPresentQpc();
        return last != 0 && last < minTimestampQpc;
    }

    void UnifiedSwapChain::SanitizeDisplayedRepeatedPresents(FrameData& present)
    {
        // Port of OutputThread.cpp::ReportMetrics() �Remove Repeated flips� pre-pass,
        // but applied to FrameData (so we don�t mutate PresentEvent).
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
        UnifiedSwapChain::Enqueue(FrameData present, MetricsVersion version)
    {
        SanitizeDisplayedRepeatedPresents(present);

        std::vector<ReadyItem> out;

        // V1: FIFO (no buffering / no look-ahead). Every present is ready immediately.
        if (version == MetricsVersion::V1) {
            waitingDisplayed.reset();
            blocked.clear();
            out.push_back(ReadyItem{ std::move(present), nullptr, nullptr });
            return out;
        }

        // Seed baseline
        if (!swapChain.lastPresent.has_value()) {
            SeedFromFirstPresent(std::move(present));
            return out;
        }

        const bool isDisplayed =
            (present.getFinalState() == PresentResult::Presented) &&
            (present.getDisplayedCount() > 0);

        if (isDisplayed) {
            // 1) Finalize previously waiting displayed (if any), pointing at swapchain-owned next displayed.
            if (waitingDisplayed.has_value()) {
                FrameData prev = std::move(*waitingDisplayed);
                waitingDisplayed = std::move(present);

                out.push_back(ReadyItem{ std::move(prev), nullptr, &*waitingDisplayed });
            }
            else {
                // First displayed: becomes the waitingDisplayed_.
                waitingDisplayed = std::move(present);
            }

            // 2) Release blocked not-displayed frames (owned, no look-ahead).
            while (!blocked.empty()) {
                out.push_back(ReadyItem{ std::move(blocked.front()), nullptr, nullptr });
                blocked.pop_front();
            }

            // 3) Current displayed is ready (all-but-last); provide a pointer so NV adjustments persist.
            out.push_back(ReadyItem{ FrameData{}, &*waitingDisplayed, nullptr });
            return out;
        }

        // Not displayed
        if (waitingDisplayed.has_value()) {
            blocked.push_back(std::move(present));
            return out; // nothing ready yet
        }

        out.push_back(ReadyItem{ std::move(present), nullptr, nullptr });
        return out;
    }
}
