// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AnimationErrorTracker.h"
#include "MetricsCalculator.h"
#include "MetricsTypes.h"
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
    void AnimationErrorTracker::SyncPreviousAnchorAnimationTimeMs(double publishedAnimationTimeMs)
    {
        if (!hasAnchor_ || IsMissingFrameMetricValue(publishedAnimationTimeMs)) {
            return;
        }
        previousAnchor_.animationTimeMs = publishedAnimationTimeMs;
    }
    bool AnimationErrorTracker::IsTransition(const AppAnchor& anchor) const
    {
        return hasAnchor_ && previousAnchor_.source != anchor.source;
    }
    AnimationDisplayContext AnimationErrorTracker::MakeTransitionTimelineOrigin(const AppAnchor& anchor)
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
        const AppAnchor& closingAnchor,
        const std::vector<ReadyDisplayRow>& intervalRows)
    {
        std::vector<AnimationDisplayContext> contexts(intervalRows.size());
        if (!hasAnchor_ || closingAnchor.simStartTime == 0 || intervalRows.empty()) {
            SeedAnchor(closingAnchor);
            return contexts;
        }
        if (IsTransition(closingAnchor)) {
            if (!contexts.empty()) {
                contexts.back() = MakeTransitionTimelineOrigin(closingAnchor);
            }
            return contexts;
        }
        return CloseSameSourceInterval_(qpc, closingAnchor, intervalRows);
    }
    std::vector<AnimationDisplayContext> AnimationErrorTracker::CloseSameSourceInterval_(
        const QpcConverter& qpc,
        const AppAnchor& closingAnchor,
        const std::vector<ReadyDisplayRow>& intervalRows)
    {
        std::vector<AnimationDisplayContext> contexts(intervalRows.size());
        if (closingAnchor.simStartTime <= previousAnchor_.simStartTime) {
            AppAnchor reseededAnchor = closingAnchor;
            reseededAnchor.animationTimeMs = previousAnchor_.animationTimeMs;
            previousAnchor_ = reseededAnchor;
            return contexts;
        }
        const double simStepTicks =
            double(closingAnchor.simStartTime - previousAnchor_.simStartTime) / double(intervalRows.size());
        const double simStepMs = simStepTicks * qpc.GetMilliSecondsPerTick();
        for (size_t i = 0; i < intervalRows.size(); ++i) {
            const auto& row = intervalRows[i];
            const double displayStepMs =
                qpc.DeltaUnsignedMilliSeconds(row.previousDisplayedScreenTime, row.screenTime);
            const double animationTimeMs = previousAnchor_.animationTimeMs + (simStepMs * double(i + 1));
            contexts[i].msAnimationError = simStepMs - displayStepMs;
            contexts[i].msAnimationTime = animationTimeMs;
            contexts[i].source = closingAnchor.source;
            contexts[i].resolvedSimStartTime =
                uint64_t(double(previousAnchor_.simStartTime) + (simStepTicks * double(i + 1)));
            contexts[i].firstSimStartTime = firstSimStartTime_;
            contexts[i].hasResolvedSimStart = true;
        }
        AppAnchor nextAnchor = closingAnchor;
        nextAnchor.animationTimeMs =
            previousAnchor_.animationTimeMs + (simStepMs * double(intervalRows.size()));
        previousAnchor_ = nextAnchor;
        return contexts;
    }
    AnimationErrorTracker::AppAnchor AnimationErrorTracker::ResolveAnchor(
        const SwapChainCoreState& chainState,
        const FrameData& present,
        size_t displayIndex,
        const FrameData* ingestPreviousPresent) const
    {
        AppAnchor anchor{};
        anchor.present = present;
        anchor.displayIndex = displayIndex;
        anchor.screenTime = present.displayed[displayIndex].second;
        anchor.frameType = present.displayed[displayIndex].first;
        anchor.source = ResolveSource_(present);
        anchor.simStartTime = ResolveSimStart_(
            chainState,
            present,
            anchor.source,
            ingestPreviousPresent);
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
    uint64_t AnimationErrorTracker::ResolveSimStart_(
        const SwapChainCoreState& chainState,
        const FrameData& present,
        AnimationErrorSource source,
        const FrameData* ingestPreviousPresent)
    {
        if (source == AnimationErrorSource::AppProvider) {
            return present.appSimStartTime;
        }
        if (source == AnimationErrorSource::PCLatency) {
            return present.pclSimStartTime;
        }
        return CalculateCPUStart(chainState, present, ingestPreviousPresent);
    }
}
