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
        bool HasTimelineAnchor() const;
        // Start a new animation timeline at anchor. Returns false if the anchor
        // has no resolved simulation start.
        bool TryStartTimelineAtAnchor(const AppAnchor& anchor);
        // Set the current anchor's accumulated animation time after its origin row
        // has been published.
        void SetCurrentAnchorAnimationTimeMs(double publishedAnimationTimeMs);
        bool IsSourceTransition(const AppAnchor& anchor) const;
        // Resolve animation contexts for intervalRows and advance the current
        // timeline anchor according to same-source, transition, and invalid-interval
        // rules.
        std::vector<AnimationDisplayContext> ResolveIntervalAndAdvanceAnchor(
            const QpcConverter& qpc,
            const AppAnchor& closingAnchor,
            const std::vector<ReadyDisplayRow>& intervalRows);
        AppAnchor ResolveAppAnchor(
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
        AnimationDisplayContext StartTransitionTimelineAndBuildOriginContext_(
            const AppAnchor& anchor);
        std::vector<AnimationDisplayContext> ResolveSameSourceIntervalAndAdvanceAnchor_(
            const QpcConverter& qpc,
            const AppAnchor& closingAnchor,
            const std::vector<ReadyDisplayRow>& intervalRows);
        bool hasCurrentAnchor_ = false;
        AppAnchor currentAnchor_{};
        uint64_t timelineFirstSimStartTime_ = 0;
    };
}
