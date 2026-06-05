// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DisplayFrameQueue.h"
#include "MetricsCalculatorInternal.h"
#include <iterator>
#include <utility>
namespace pmon::util::metrics
{
    namespace
    {
        uint64_t CurrentSimTimeForAnimationTime_(
            AnimationErrorSource source,
            const SwapChainCoreState& chain,
            const FrameData& present)
        {
            if (source == AnimationErrorSource::AppProvider) {
                return present.appSimStartTime;
            }
            if (source == AnimationErrorSource::PCLatency) {
                return present.pclSimStartTime;
            }
            return CalculateCPUStart(chain, present);
        }
        void MoveAllToReady_(
            std::vector<ReadyDisplayRow>& from,
            std::vector<ReadyDisplayRow>& ready)
        {
            ready.insert(
                ready.end(),
                std::make_move_iterator(from.begin()),
                std::make_move_iterator(from.end()));
            from.clear();
        }
        void ReleaseLastHeldRowLookahead_(
            std::vector<ReadyDisplayRow>& holdBucket,
            uint64_t nextScreenTime,
            std::vector<ReadyDisplayRow>& ready)
        {
            if (holdBucket.empty()) {
                return;
            }
            holdBucket.back().nextScreenTime = nextScreenTime;
            ready.push_back(std::move(holdBucket.back()));
            holdBucket.pop_back();
        }
    }

    // Ingest is called for every present and is responsible for expanding presents into display instances,
    // attaching animation contexts from the AnimationErrorTracker, and buffering until the publish policy allows
    // rows to be released. The general flow is: if not displayed, buffer if there's an anchor or emit immediately if not;
    // if displayed and not an app anchor, buffer until the next app anchor; if displayed and an app anchor,
    // close the prior interval with this anchor and buffer until lookahead information is available to release.
    std::vector<ReadyDisplayRow> DisplayFrameQueue::Ingest(
        const QpcConverter& qpc,
        FrameData present,
        AnimationErrorTracker& animation,
        SwapChainCoreState& chain)
    {
        const FrameData* ingestPreviousPresent =
            lastIngestedPresent_.has_value() ? &lastIngestedPresent_.value() : nullptr;

        // Ingest may move `present` (not-displayed path); keep a copy for lastIngestedPresent_
        // after processing while ingestPreviousPresent still refers to the prior ingest above.
        FrameData presentForIngestHistory = present;

        std::vector<ReadyDisplayRow> ready;
        if (!IsDisplayed_(present)) {
            IngestNotDisplayedPresent_(std::move(present), animation, ready);
            MarkPresentUpdateRows_(ready);
            lastIngestedPresent_ = std::move(presentForIngestHistory);
            return ready;
        }
        const size_t displayCount = present.displayed.Size();
        for (size_t displayIndex = 0; displayIndex < displayCount; ++displayIndex) {
            ReadyDisplayRow row = BuildDisplayInstanceRow_(present, displayIndex, chain);
            ApplyNvV2Adjustment_(row);
            OnNextDisplayedFrame_(row.screenTime, ready);
            if (!openIntervalBeforeClosingApp_.empty()) {
                openIntervalBeforeClosingApp_.back().nextScreenTime = row.screenTime;
            }
            AcceptDisplayOrder_(row);
            if (!row.isAppFrame) {
                IngestGeneratedDisplayInstance_(std::move(row), animation, ready);
            }
            else {
                IngestAppAnchorDisplayInstance_(
                    qpc,
                    std::move(row),
                    animation,
                    chain,
                    ingestPreviousPresent,
                    ready);
            }
        }
        MarkPresentUpdateRows_(ready);
        lastIngestedPresent_ = std::move(presentForIngestHistory);
        return ready;
    }

    // NoteSeedPresent is used to track the most recent present that has been ingested,
    // regardless of whether it's displayed or not. This is used by the AnimationErrorTracker
    // to resolve anchors for app anchor rows that are ingested before the next app anchor
    // closes the interval, as well as for generated rows that are ingested before any app
    // anchor has been seen.
    void DisplayFrameQueue::NoteSeedPresent(const FrameData& seedPresent)
    {
        lastIngestedPresent_ = seedPresent;
    }
    void DisplayFrameQueue::Clear()
    {
        lastAcceptedScreenTime_ = 0;
        lastAcceptedPresentStartTime_ = 0;
        lastAcceptedFlipDelay_ = 0;
        openIntervalBeforeClosingApp_.clear();
        closedIntervalAwaitingLookahead_.clear();
        preFirstAppAnchorAwaitingLookahead_.clear();
        timelineOriginAwaitingLookahead_.reset();
        blockedNotDisplayedPresents_.clear();
        lastIngestedPresent_.reset();
    }

    // BuildDisplayInstanceRow_ takes the incoming present, display
    // index and chain state to start building up a display ready row
    ReadyDisplayRow DisplayFrameQueue::BuildDisplayInstanceRow_(
        const FrameData& present,
        size_t displayIndex,
        const SwapChainCoreState& chain) const
    {
        const size_t displayCount = present.displayed.Size();
        const auto frameType = present.displayed[displayIndex].first;
        const auto screenTime = present.displayed[displayIndex].second;
        const auto nextScreenTime = (displayIndex + 1 < displayCount)
            ? present.displayed[displayIndex + 1].second
            : screenTime;
        const auto previousScreenTime = lastAcceptedScreenTime_ != 0
            ? lastAcceptedScreenTime_
            : chain.lastDisplayedScreenTime;
        ReadyDisplayRow row{};
        row.present = present;
        row.displayIndex = displayIndex;
        row.previousDisplayedScreenTime = previousScreenTime;
        row.screenTime = screenTime;
        // Note that nextScreenTime will be equal to screenTime if the
        // display count is 1
        row.nextScreenTime = nextScreenTime;
        row.isDisplayed = true;
        row.isAppFrame = IsAppAnchor_(frameType);
        row.frameType = frameType;
        row.updateSwapChainAfterRow = false;
        return row;
    }
    void DisplayFrameQueue::AcceptDisplayOrder_(ReadyDisplayRow& row)
    {
        lastAcceptedScreenTime_ = row.screenTime;
        lastAcceptedPresentStartTime_ = row.present.presentStartTime;
        lastAcceptedFlipDelay_ = row.present.flipDelay;
    }
    bool DisplayFrameQueue::DisplayTimingComplete_(const ReadyDisplayRow& row)
    {
        return row.nextScreenTime != row.screenTime;
    }
    void DisplayFrameQueue::EmitOrHoldTimelineOriginUntilDisplayTimingComplete_(
        ReadyDisplayRow row,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (DisplayTimingComplete_(row)) {
            ready.push_back(std::move(row));
        }
        else {
            timelineOriginAwaitingLookahead_ = std::move(row);
        }
    }
    void DisplayFrameQueue::EmitOrHoldUntilDisplayTimingComplete_(
        ReadyDisplayRow row,
        std::vector<ReadyDisplayRow>& awaitingLookaheadBucket,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (DisplayTimingComplete_(row)) {
            ready.push_back(std::move(row));
        }
        else {
            awaitingLookaheadBucket.push_back(std::move(row));
        }
    }

    // OnNextDisplayedFrame_ is invoked for each displayed instance during ingest. It uses the new
    // instance's screen time as nextScreenTime for rows that were held until display timing was known,
    // then moves them to ready.
    void DisplayFrameQueue::OnNextDisplayedFrame_(
        uint64_t nextScreenTime,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!closedIntervalAwaitingLookahead_.empty()) {
            closedIntervalAwaitingLookahead_.back().nextScreenTime = nextScreenTime;
            MoveAllToReady_(closedIntervalAwaitingLookahead_, ready);
        }

        if (timelineOriginAwaitingLookahead_.has_value()) {
            timelineOriginAwaitingLookahead_->nextScreenTime = nextScreenTime;
            ready.push_back(std::move(*timelineOriginAwaitingLookahead_));
            timelineOriginAwaitingLookahead_.reset();
        }
    }
    // -------------------------------------------------------------------------
    // Not-displayed presents
    // -------------------------------------------------------------------------
    void DisplayFrameQueue::IngestNotDisplayedPresent_(
        FrameData present,
        AnimationErrorTracker& animation,
        std::vector<ReadyDisplayRow>& ready)
    {
        // If there's an app anchor, hold not-displayed presents until the next app anchor closes the interval,
        // at which point they'll be released with animation metrics. Otherwise, they can be emitted immediately
        // with missing animation metrics. This handles cases where we receive generated frames without a prior app anchor,
        // which is expected to be common when the provider starts after the app has already started presenting.
        if (animation.HasAnchor()) {
            blockedNotDisplayedPresents_.push_back(std::move(present));
        }
        else {
            ready.push_back(MakeNotDisplayedRow_(std::move(present)));
        }
    }
    // -------------------------------------------------------------------------
    // Generated display instances
    // -------------------------------------------------------------------------
    void DisplayFrameQueue::IngestGeneratedDisplayInstance_(
        ReadyDisplayRow row,
        AnimationErrorTracker& animation,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!animation.HasAnchor()) {
            // Design: trace start without prior app anchor.
            ReleaseLastHeldRowLookahead_(preFirstAppAnchorAwaitingLookahead_, row.screenTime, ready);
            EmitOrHoldUntilDisplayTimingComplete_(
                std::move(row),
                preFirstAppAnchorAwaitingLookahead_,
                ready);
            return;
        }
        // Design: hold generated rows until the next app anchor closes the interval.
        openIntervalBeforeClosingApp_.push_back(std::move(row));
    }
    // -------------------------------------------------------------------------
    // App anchor display instances (design: App Anchor, Closed Interval)
    // -------------------------------------------------------------------------
    void DisplayFrameQueue::IngestAppAnchorDisplayInstance_(
        const QpcConverter& qpc,
        ReadyDisplayRow row,
        AnimationErrorTracker& animation,
        SwapChainCoreState& chain,
        const FrameData* ingestPreviousPresent,
        std::vector<ReadyDisplayRow>& ready)
    {
        const auto anchor = animation.ResolveAnchor(
            chain,
            row.present,
            row.displayIndex,
            ingestPreviousPresent);
        if (!animation.HasAnchor()) {
            ReleaseLastHeldRowLookahead_(preFirstAppAnchorAwaitingLookahead_, row.screenTime, ready);
            PublishFirstTimelineOrigin_(qpc, std::move(row), animation, chain, anchor, ready);
            openIntervalBeforeClosingApp_.clear();
            return;
        }
        if (animation.IsTransition(anchor)) {
            PublishSourceTransition_(
                qpc,
                std::move(row),
                animation,
                chain,
                ingestPreviousPresent,
                ready);
            return;
        }
        std::vector<ReadyDisplayRow> closedInterval = std::move(openIntervalBeforeClosingApp_);
        closedInterval.push_back(std::move(row));
        CloseIntervalAndHoldForLookahead_(qpc, animation, anchor, std::move(closedInterval));
        openIntervalBeforeClosingApp_.clear();
    }
    void DisplayFrameQueue::PublishFirstTimelineOrigin_(
        const QpcConverter& qpc,
        ReadyDisplayRow row,
        AnimationErrorTracker& animation,
        const SwapChainCoreState& chain,
        const AnimationErrorTracker::AppAnchor& anchor,
        std::vector<ReadyDisplayRow>& ready)
    {
        animation.SeedAnchor(anchor);
        row.animation.msAnimationError = MissingFrameMetricValue();
        row.animation.msAnimationTime = CalculateAnimationTime(
            qpc,
            chain.firstAppSimStartTime,
            CurrentSimTimeForAnimationTime_(anchor.source, chain, row.present));
        row.animation.source = anchor.source;
        row.animation.resolvedSimStartTime = anchor.simStartTime;
        row.animation.firstSimStartTime = chain.firstAppSimStartTime != 0
            ? chain.firstAppSimStartTime
            : qpc.GetSessionStartTimestamp();
        row.animation.hasResolvedSimStart = anchor.simStartTime != 0;
        animation.SyncPreviousAnchorAnimationTimeMs(row.animation.msAnimationTime);
        EmitOrHoldTimelineOriginUntilDisplayTimingComplete_(std::move(row), ready);
    }
    void DisplayFrameQueue::CloseIntervalAndHoldForLookahead_(
        const QpcConverter& qpc,
        AnimationErrorTracker& animation,
        const AnimationErrorTracker::AppAnchor& closingAnchor,
        std::vector<ReadyDisplayRow> intervalRows)
    {
        const auto contexts = animation.CloseInterval(qpc, closingAnchor, intervalRows);
        for (size_t i = 0; i < intervalRows.size(); ++i) {
            intervalRows[i].animation = contexts[i];
        }
        while (!blockedNotDisplayedPresents_.empty()) {
            closedIntervalAwaitingLookahead_.push_back(
                MakeNotDisplayedRow_(std::move(blockedNotDisplayedPresents_.front())));
            blockedNotDisplayedPresents_.pop_front();
        }
        closedIntervalAwaitingLookahead_.insert(
            closedIntervalAwaitingLookahead_.end(),
            std::make_move_iterator(intervalRows.begin()),
            std::make_move_iterator(intervalRows.end()));
    }
    void DisplayFrameQueue::PublishSourceTransition_(
        const QpcConverter& qpc,
        ReadyDisplayRow transitionAppRow,
        AnimationErrorTracker& animation,
        const SwapChainCoreState& chain,
        const FrameData* ingestPreviousPresent,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!openIntervalBeforeClosingApp_.empty()) {
            openIntervalBeforeClosingApp_.back().nextScreenTime = transitionAppRow.screenTime;
        }
        MoveAllToReady_(openIntervalBeforeClosingApp_, ready);
        while (!blockedNotDisplayedPresents_.empty()) {
            ready.push_back(MakeNotDisplayedRow_(std::move(blockedNotDisplayedPresents_.front())));
            blockedNotDisplayedPresents_.pop_front();
        }
        const auto anchor = animation.ResolveAnchor(
            chain,
            transitionAppRow.present,
            transitionAppRow.displayIndex,
            ingestPreviousPresent);
        std::vector<ReadyDisplayRow> transitionInterval;
        transitionInterval.push_back(std::move(transitionAppRow));
        const auto contexts = animation.CloseInterval(qpc, anchor, transitionInterval);
        transitionInterval[0].animation = contexts[0];
        EmitOrHoldTimelineOriginUntilDisplayTimingComplete_(
            std::move(transitionInterval[0]),
            ready);
        openIntervalBeforeClosingApp_.clear();
    }
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------
    bool DisplayFrameQueue::IsAppAnchor_(FrameType frameType)
    {
        return frameType == FrameType::Application || frameType == FrameType::NotSet;
    }
    bool DisplayFrameQueue::IsDisplayed_(const FrameData& present)
    {
        return present.finalState == PresentResult::Presented && !present.displayed.Empty();
    }
    ReadyDisplayRow DisplayFrameQueue::MakeNotDisplayedRow_(FrameData present)
    {
        ReadyDisplayRow row{};
        row.present = std::move(present);
        row.isDisplayed = false;
        row.isAppFrame = true;
        row.updateSwapChainAfterRow = true;
        row.frameType = FrameType::NotSet;
        return row;
    }
    void DisplayFrameQueue::MarkPresentUpdateRows_(std::vector<ReadyDisplayRow>& rows)
    {
        for (size_t i = 0; i < rows.size(); ++i) {
            const bool isLastRowForPresent =
                i + 1 == rows.size() ||
                rows[i + 1].present.presentStartTime != rows[i].present.presentStartTime;
            rows[i].updateSwapChainAfterRow = isLastRowForPresent;
        }
    }
    void DisplayFrameQueue::ApplyNvV2Adjustment_(ReadyDisplayRow& row) const
    {
        if (lastAcceptedPresentStartTime_ == row.present.presentStartTime ||
            row.present.displayed.Empty()) {
            return;
        }
        FrameData previous{};
        previous.flipDelay = lastAcceptedFlipDelay_;
        FrameData adjustedNext = row.present;
        adjustedNext.displayed.Clear();
        adjustedNext.displayed.PushBack({ row.frameType, row.screenTime });
        uint64_t previousScreenTime = lastAcceptedScreenTime_;
        uint64_t adjustedScreenTime = row.screenTime;
        AdjustScreenTimeForCollapsedPresentNV(
            previous,
            &adjustedNext,
            0,
            0,
            previousScreenTime,
            adjustedScreenTime,
            MetricsVersion::V2);
        row.screenTime = adjustedScreenTime;
        row.present.flipDelay = adjustedNext.flipDelay;
        row.present.displayed[row.displayIndex].second = adjustedNext.displayed[0].second;
        if (row.nextScreenTime < row.screenTime) {
            row.nextScreenTime = row.screenTime;
        }
    }
}
