// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <vector>
#include "../qpc.h"
#include "MetricsTypes.h"
#include "SwapChainState.h"
namespace pmon::util::metrics
{
    // Animation tracker (design: "Animation Tracker" component).
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
        void SyncPreviousAnchorAnimationTimeMs(double publishedAnimationTimeMs);
        bool IsTransition(const AppAnchor& anchor) const;
        AnimationDisplayContext MakeTransitionTimelineOrigin(const AppAnchor& anchor);
        std::vector<AnimationDisplayContext> CloseInterval(
            const QpcConverter& qpc,
            const AppAnchor& closingAnchor,
            const std::vector<ReadyDisplayRow>& intervalRows);
        AppAnchor ResolveAnchor(
            const SwapChainCoreState& chainState,
            const FrameData& present,
            size_t displayIndex,
            const FrameData* ingestPreviousPresent = nullptr) const;
    private:
        static AnimationErrorSource ResolveSource_(const FrameData& present);
        static uint64_t ResolveSimStart_(
            const SwapChainCoreState& chainState,
            const FrameData& present,
            AnimationErrorSource source,
            const FrameData* ingestPreviousPresent);
        std::vector<AnimationDisplayContext> CloseSameSourceInterval_(
            const QpcConverter& qpc,
            const AppAnchor& closingAnchor,
            const std::vector<ReadyDisplayRow>& intervalRows);
        bool hasAnchor_ = false;
        AppAnchor previousAnchor_{};
        uint64_t firstSimStartTime_ = 0;
    };
}
