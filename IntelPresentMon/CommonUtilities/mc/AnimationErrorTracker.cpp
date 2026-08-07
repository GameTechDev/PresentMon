// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AnimationErrorTracker.h"
#include "MetricsCalculator.h"
#include "MetricsTypes.h"
namespace pmon::util::metrics
{
    namespace
    {
        bool IsAppFrameType_(FrameType frameType)
        {
            return frameType == FrameType::Application || frameType == FrameType::NotSet;
        }

        bool HasAppWork_(const FrameData& present)
        {
            if (present.displayed.Empty()) {
                return true;
            }
            for (size_t i = 0; i < present.displayed.Size(); ++i) {
                if (IsAppFrameType_(present.displayed[i].first)) {
                    return true;
                }
            }
            return false;
        }

        uint64_t PresentEndQpc_(const FrameData& present)
        {
            if (present.appPropagatedPresentStartTime != 0) {
                return present.appPropagatedPresentStartTime + present.appPropagatedTimeInPresent;
            }
            return present.presentStartTime + present.timeInPresent;
        }

        uint64_t ResolveCpuStartForAnimation_(
            const SwapChainCoreState& chainState,
            const FrameData* ingestPreviousPresent)
        {
            if (ingestPreviousPresent != nullptr && HasAppWork_(*ingestPreviousPresent)) {
                return PresentEndQpc_(*ingestPreviousPresent);
            }
            if (chainState.lastAppPresent.has_value()) {
                return PresentEndQpc_(chainState.lastAppPresent.value());
            }
            return 0;
        }
    }

    bool AnimationErrorTracker::HasTimelineAnchor() const
    {
        return hasCurrentAnchor_;
    }
    bool AnimationErrorTracker::TryStartTimelineAtAnchor(const AppAnchor& anchor)
    {
        if (anchor.simStartTime == 0) {
            return false;
        }
        currentAnchor_ = anchor;
        currentAnchor_.animationTimeMs = 0.0;
        timelineFirstSimStartTime_ = anchor.simStartTime;
        hasCurrentAnchor_ = true;
        return true;
    }
    void AnimationErrorTracker::SetCurrentAnchorAnimationTimeMs(double publishedAnimationTimeMs)
    {
        if (!hasCurrentAnchor_ || IsMissingFrameMetricValue(publishedAnimationTimeMs)) {
            return;
        }
        currentAnchor_.animationTimeMs = publishedAnimationTimeMs;
    }
    bool AnimationErrorTracker::IsSourceTransition(const AppAnchor& anchor) const
    {
        return hasCurrentAnchor_ && currentAnchor_.source != anchor.source;
    }
    AnimationDisplayContext AnimationErrorTracker::StartTransitionTimelineAndBuildOriginContext_(
        const AppAnchor& anchor)
    {
        AnimationDisplayContext context{};
        context.msAnimationError = MissingFrameMetricValue();
        context.msAnimationTime = 0.0;
        context.source = anchor.source;
        context.resolvedSimStartTime = anchor.simStartTime;
        context.firstSimStartTime = anchor.simStartTime;
        context.hasResolvedSimStart = anchor.simStartTime != 0;
        TryStartTimelineAtAnchor(anchor);
        return context;
    }
    std::vector<AnimationDisplayContext> AnimationErrorTracker::ResolveIntervalAndAdvanceAnchor(
        const QpcConverter& qpc,
        const AppAnchor& closingAnchor,
        const std::vector<ReadyDisplayRow>& intervalRows)
    {
        std::vector<AnimationDisplayContext> contexts(intervalRows.size());
        if (!hasCurrentAnchor_ || closingAnchor.simStartTime == 0 || intervalRows.empty()) {
            TryStartTimelineAtAnchor(closingAnchor);
            return contexts;
        }
        if (IsSourceTransition(closingAnchor)) {
            if (!contexts.empty()) {
                contexts.back() = StartTransitionTimelineAndBuildOriginContext_(closingAnchor);
            }
            return contexts;
        }
        return ResolveSameSourceIntervalAndAdvanceAnchor_(qpc, closingAnchor, intervalRows);
    }
    std::vector<AnimationDisplayContext> AnimationErrorTracker::ResolveSameSourceIntervalAndAdvanceAnchor_(
        const QpcConverter& qpc,
        const AppAnchor& closingAnchor,
        const std::vector<ReadyDisplayRow>& intervalRows)
    {
        std::vector<AnimationDisplayContext> contexts(intervalRows.size());
        if (closingAnchor.simStartTime <= currentAnchor_.simStartTime) {
            AppAnchor reseededAnchor = closingAnchor;
            reseededAnchor.animationTimeMs = currentAnchor_.animationTimeMs;
            currentAnchor_ = reseededAnchor;
            return contexts;
        }
        const double simStepTicks =
            double(closingAnchor.simStartTime - currentAnchor_.simStartTime) / double(intervalRows.size());
        const double simStepMs = simStepTicks * qpc.GetMilliSecondsPerTick();
        for (size_t i = 0; i < intervalRows.size(); ++i) {
            const auto& row = intervalRows[i];
            const double displayStepMs =
                qpc.DeltaUnsignedMilliSeconds(row.previousDisplayedScreenTime, row.screenTime);
            const double animationTimeMs = currentAnchor_.animationTimeMs + (simStepMs * double(i + 1));
            contexts[i].msAnimationError =
                simStepMs == 0.0 || displayStepMs == 0.0
                    ? MissingFrameMetricValue()
                    : simStepMs - displayStepMs;
            contexts[i].msAnimationTime = animationTimeMs;
            contexts[i].source = closingAnchor.source;
            contexts[i].resolvedSimStartTime =
                uint64_t(double(currentAnchor_.simStartTime) + (simStepTicks * double(i + 1)));
            contexts[i].firstSimStartTime = timelineFirstSimStartTime_;
            contexts[i].hasResolvedSimStart = true;
        }
        AppAnchor nextAnchor = closingAnchor;
        nextAnchor.animationTimeMs =
            currentAnchor_.animationTimeMs + (simStepMs * double(intervalRows.size()));
        currentAnchor_ = nextAnchor;
        return contexts;
    }
    AnimationErrorTracker::AppAnchor AnimationErrorTracker::ResolveAppAnchor(
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
        return ResolveCpuStartForAnimation_(chainState, ingestPreviousPresent);
    }
}
