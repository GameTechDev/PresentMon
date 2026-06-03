// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsTypes.h"
#include "UnifiedSwapChain.h"
#include <PresentData/PresentMonTraceConsumer.hpp>

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
        // Port of OutputThread.cpp::ReportMetrics() "Remove Repeated flips" pre-pass,
        // but applied to FrameData (so we do not mutate PresentEvent).
        //
        // Only apply when there is exactly one Repeated entry in the sequence.
        // Two or more Repeated entries indicates a frame-generation shape
        // (e.g., AMD AFMF: Repeated, Application, Repeated) where every Repeated
        // is a distinct generated display instance that must reach the queue.
        auto& d = present.displayed;

        size_t repeatedCount = 0;
        for (size_t i = 0; i < d.Size(); ++i) {
            if (d[i].first == FrameType::Repeated) {
                ++repeatedCount;
            }
        }
        if (repeatedCount >= 2) {
            return;
        }

        for (size_t i = 0; i + 1 < d.Size(); ) {
            const auto a = d[i].first;
            const auto b = d[i + 1].first;

            const bool app_then_rep = (a == FrameType::Application && b == FrameType::Repeated);
            const bool rep_then_app = (a == FrameType::Repeated && b == FrameType::Application);

            if (app_then_rep) {
                d.Erase(d.begin() + i + 1);
            }
            else if (rep_then_app) {
                d.Erase(d.begin() + i);
            }
            else {
                ++i;
            }
        }
    }

    std::vector<ProcessPresentRow> UnifiedSwapChain::ProcessPresent(
        const QpcConverter& qpc,
        FrameData present)
    {
        if (!swapChain.lastPresent.has_value()) {
            SanitizeDisplayedRepeatedPresents(present);
            swapChain.UpdateAfterPresent(present);
            displayQueue.NoteSeedPresent(present);
            return {};
        }

        auto ready = EnqueueReadyDisplayRows(qpc, std::move(present));
        return ApplyReadyDisplayRows(qpc, ready);
    }

    std::vector<ProcessPresentRow> UnifiedSwapChain::ApplyReadyDisplayRows(
        const QpcConverter& qpc,
        std::vector<ReadyDisplayRow>& ready)
    {
        std::vector<ProcessPresentRow> applied;
        applied.reserve(ready.size());
        for (auto& row : ready) {
            ProcessPresentRow output{};
            output.present = row.present;
            output.computed = ComputeMetricsForReadyDisplayRow(qpc, row, swapChain);
            applied.push_back(std::move(output));
        }
        return applied;
    }

    std::vector<ReadyDisplayRow> UnifiedSwapChain::EnqueueReadyDisplayRows(
        const QpcConverter& qpc,
        FrameData present)
    {
        SanitizeDisplayedRepeatedPresents(present);
        return displayQueue.Ingest(qpc, std::move(present), animationTracker, swapChain);
    }
}
