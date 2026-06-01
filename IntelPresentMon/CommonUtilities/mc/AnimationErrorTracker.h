// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <vector>

#include "../qpc.h"
#include "MetricsTypes.h"

namespace pmon::util::metrics
{
    class AnimationErrorTracker
    {
    public:
        struct AppAnchor
        {
            FrameData present;
            size_t displayIndex = 0;
            uint64_t screenTime = 0;
            FrameType frameType = FrameType::NotSet;
            AnimationErrorSource source = AnimationErrorSource::CpuStart;
            uint64_t simStartTime = 0;
            double animationTimeMs = 0.0;
        };

        bool HasAnchor() const;
        bool SeedAnchor(const AppAnchor& anchor);
        bool IsTransition(const AppAnchor& anchor) const;
        AnimationDisplayContext MakeTransitionContext(const AppAnchor& anchor);
        std::vector<AnimationDisplayContext> CloseInterval(
            const QpcConverter& qpc,
            const AppAnchor& anchor,
            const std::vector<ReadyDisplayRow>& intervalRows);

        AppAnchor ResolveAnchor(const FrameData& present, size_t displayIndex) const;

    private:
        static AnimationErrorSource ResolveSource_(const FrameData& present);
        static uint64_t ResolveSimStart_(const FrameData& present, AnimationErrorSource source);

        bool hasAnchor_ = false;
        AppAnchor previousAnchor_{};
        uint64_t firstSimStartTime_ = 0;
    };
}
