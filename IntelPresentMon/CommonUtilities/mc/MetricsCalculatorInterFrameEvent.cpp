// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MetricsCalculator.h"
#include "MetricsCalculatorInternal.h"
#include <PresentData/InterPresentActivity.hpp>

namespace pmon::util::metrics
{
    void CalculateInterFrameEventMetrics(
        const QpcConverter& qpc,
        const FrameData& present,
        bool isAppFrame,
        FrameMetrics& metrics)
    {
        if (!isAppFrame) {
            return;
        }

        const uint64_t windowQpc = present.interFrameEventWindowQpc;
        for (size_t kindIndex = 0; kindIndex < (size_t)InterPresentActivity::Kind::Count; ++kindIndex) {
            const auto kind = (InterPresentActivity::Kind)kindIndex;
            const auto& stats = present.interFrameEventStats[kindIndex];
            // Add a case here when extending InterPresentActivity::Kind (and FrameMetrics fields).
            switch (kind) {
            case InterPresentActivity::Kind::D3D12PsoCompile:
                metrics.psoCompileCount = stats.activityCount;
                metrics.msPsoCompileTime = qpc.DurationMilliSeconds(stats.summedBusyQpc);
                metrics.psoCompileBusyPercent = windowQpc == 0
                    ? 0.
                    : 100. * (double)stats.busyQpc / (double)windowQpc;
                break;
            default:
                break;
            }
        }
    }
}
