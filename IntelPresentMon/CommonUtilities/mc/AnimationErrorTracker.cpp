// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AnimationErrorTracker.h"

namespace pmon::util::metrics
{
    bool AnimationErrorTracker::HasAnchor() const
    {
        return hasAnchor_;
    }

    bool AnimationErrorTracker::SeedAnchor(const AppAnchor& anchor)
    {
        if (anchor.simStartTime == 0) {
            return false;
        }

        previousAnchor_ = anchor;
        previousAnchor_.animationTimeMs = 0.0;
        firstSimStartTime_ = anchor.simStartTime;
        hasAnchor_ = true;
        return true;
    }

    bool AnimationErrorTracker::IsTransition(const AppAnchor& anchor) const
    {
        return hasAnchor_ && previousAnchor_.source != anchor.source;
    }

    AnimationDisplayContext AnimationErrorTracker::MakeTransitionContext(const AppAnchor& anchor)
    {
        AnimationDisplayContext context{};
        context.msAnimationError = MissingFrameMetricValue();
        context.msAnimationTime = 0.0;
        context.source = anchor.source;
        context.resolvedSimStartTime = anchor.simStartTime;
        context.firstSimStartTime = anchor.simStartTime;
        context.hasResolvedSimStart = anchor.simStartTime != 0;

        SeedAnchor(anchor);
        return context;
    }

    std::vector<AnimationDisplayContext> AnimationErrorTracker::CloseInterval(
        const QpcConverter& qpc,
        const AppAnchor& anchor,
        const std::vector<ReadyDisplayRow>& intervalRows)
    {
        std::vector<AnimationDisplayContext> contexts(intervalRows.size());
        if (!hasAnchor_ || anchor.simStartTime == 0 || intervalRows.empty()) {
            SeedAnchor(anchor);
            return contexts;
        }

        if (IsTransition(anchor)) {
            if (!contexts.empty()) {
                contexts.back() = MakeTransitionContext(anchor);
            }
            return contexts;
        }

        if (anchor.simStartTime <= previousAnchor_.simStartTime) {
            AppAnchor nextAnchor = anchor;
            nextAnchor.animationTimeMs = previousAnchor_.animationTimeMs;
            previousAnchor_ = nextAnchor;
            return contexts;
        }

        const double simStepTicks = double(anchor.simStartTime - previousAnchor_.simStartTime) / double(intervalRows.size());
        const double simStepMs = simStepTicks * qpc.GetMilliSecondsPerTick();

        for (size_t i = 0; i < intervalRows.size(); ++i) {
            const auto& row = intervalRows[i];
            const double displayStepMs = qpc.DeltaUnsignedMilliSeconds(row.previousDisplayedScreenTime, row.screenTime);
            const double animationTimeMs = previousAnchor_.animationTimeMs + (simStepMs * double(i + 1));

            contexts[i].msAnimationError = simStepMs - displayStepMs;
            contexts[i].msAnimationTime = animationTimeMs;
            contexts[i].source = anchor.source;
            contexts[i].resolvedSimStartTime = uint64_t(double(previousAnchor_.simStartTime) + (simStepTicks * double(i + 1)));
            contexts[i].firstSimStartTime = firstSimStartTime_;
            contexts[i].hasResolvedSimStart = true;
        }

        AppAnchor nextAnchor = anchor;
        nextAnchor.animationTimeMs = previousAnchor_.animationTimeMs + (simStepMs * double(intervalRows.size()));
        previousAnchor_ = nextAnchor;
        return contexts;
    }

    AnimationErrorTracker::AppAnchor AnimationErrorTracker::ResolveAnchor(
        const FrameData& present,
        size_t displayIndex) const
    {
        AppAnchor anchor{};
        anchor.present = present;
        anchor.displayIndex = displayIndex;
        anchor.screenTime = present.displayed[displayIndex].second;
        anchor.frameType = present.displayed[displayIndex].first;
        anchor.source = ResolveSource_(present);
        anchor.simStartTime = ResolveSimStart_(present, anchor.source);
        return anchor;
    }

    AnimationErrorSource AnimationErrorTracker::ResolveSource_(const FrameData& present)
    {
        if (present.appSimStartTime != 0) {
            return AnimationErrorSource::AppProvider;
        }
        if (present.pclSimStartTime != 0) {
            return AnimationErrorSource::PCLatency;
        }
        return AnimationErrorSource::CpuStart;
    }

    uint64_t AnimationErrorTracker::ResolveSimStart_(const FrameData& present, AnimationErrorSource source)
    {
        if (source == AnimationErrorSource::AppProvider) {
            return present.appSimStartTime;
        }
        if (source == AnimationErrorSource::PCLatency) {
            return present.pclSimStartTime;
        }
        if (present.appPropagatedPresentStartTime != 0) {
            return present.appPropagatedPresentStartTime + present.appPropagatedTimeInPresent;
        }
        return present.presentStartTime + present.timeInPresent;
    }
}
