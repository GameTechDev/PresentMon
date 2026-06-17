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

            // This displayed instance is lookahead for rows that were complete except
            // for nextScreenTime.
            CompleteRowsWaitingForDisplayLookahead_(row.screenTime, ready);

            // Rows in the open app-to-app interval use the next displayed instance
            // for their display duration, even before the interval closes.
            if (!openAppToAppIntervalRows_.empty()) {
                openAppToAppIntervalRows_.back().nextScreenTime = row.screenTime;
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
        openAppToAppIntervalRows_.clear();
        closedIntervalAwaitingDisplayLookahead_.clear();
        preFirstAppAnchorRowsAwaitingDisplayLookahead_.clear();
        timelineOriginAwaitingDisplayLookahead_.reset();
        notDisplayedPresentsHeldForIntervalRelease_.clear();
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

    // Closed intervals and timeline origins wait for the next displayed frame after
    // the closing/origin app anchor. This method supplies that lookahead and releases
    // those rows. Pre-first-app-anchor rows are handled in their routing branch.
    void DisplayFrameQueue::CompleteRowsWaitingForDisplayLookahead_(
        uint64_t nextScreenTime,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!closedIntervalAwaitingDisplayLookahead_.empty()) {
            closedIntervalAwaitingDisplayLookahead_.back().nextScreenTime = nextScreenTime;
            MoveAllToReady_(closedIntervalAwaitingDisplayLookahead_, ready);
        }

        if (timelineOriginAwaitingDisplayLookahead_.has_value()) {
            timelineOriginAwaitingDisplayLookahead_->nextScreenTime = nextScreenTime;
            ready.push_back(std::move(*timelineOriginAwaitingDisplayLookahead_));
            timelineOriginAwaitingDisplayLookahead_.reset();
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
        if (animation.HasTimelineAnchor()) {
            notDisplayedPresentsHeldForIntervalRelease_.push_back(std::move(present));
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
        if (!animation.HasTimelineAnchor()) {
            // Design: trace start without prior app anchor.
            if (!preFirstAppAnchorRowsAwaitingDisplayLookahead_.empty()) {
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.back().nextScreenTime = row.screenTime;
                ready.push_back(std::move(preFirstAppAnchorRowsAwaitingDisplayLookahead_.back()));
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.pop_back();
            }
            if (DisplayTimingComplete_(row)) {
                ready.push_back(std::move(row));
            }
            else {
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.push_back(std::move(row));
            }
            return;
        }
        // Design: hold generated rows until the next app anchor closes the interval.
        openAppToAppIntervalRows_.push_back(std::move(row));
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
        const auto anchor = animation.ResolveAppAnchor(
            chain,
            row.present,
            row.displayIndex,
            ingestPreviousPresent);
        if (!animation.HasTimelineAnchor()) {
            if (!preFirstAppAnchorRowsAwaitingDisplayLookahead_.empty()) {
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.back().nextScreenTime = row.screenTime;
                ready.push_back(std::move(preFirstAppAnchorRowsAwaitingDisplayLookahead_.back()));
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.pop_back();
            }
            PublishFirstTimelineOrigin_(qpc, std::move(row), animation, chain, anchor, ready);
            openAppToAppIntervalRows_.clear();
            return;
        }
        if (animation.IsSourceTransition(anchor)) {
            PublishSourceTransition_(
                qpc,
                std::move(row),
                animation,
                chain,
                ingestPreviousPresent,
                ready);
            return;
        }
        std::vector<ReadyDisplayRow> closedInterval = std::move(openAppToAppIntervalRows_);
        closedInterval.push_back(std::move(row));
        ResolveIntervalAndHoldForLookahead_(qpc, animation, anchor, std::move(closedInterval));
        openAppToAppIntervalRows_.clear();
    }

    void DisplayFrameQueue::PublishFirstTimelineOrigin_(
        const QpcConverter& qpc,
        ReadyDisplayRow row,
        AnimationErrorTracker& animation,
        const SwapChainCoreState& chain,
        const AnimationErrorTracker::AppAnchor& anchor,
        std::vector<ReadyDisplayRow>& ready)
    {
        row.animation.msAnimationError = MissingFrameMetricValue();
        row.animation.source = anchor.source;
        row.animation.resolvedSimStartTime = anchor.simStartTime;
        row.animation.firstSimStartTime = chain.firstAppSimStartTime != 0
            ? chain.firstAppSimStartTime
            : qpc.GetSessionStartTimestamp();
        row.animation.hasResolvedSimStart = anchor.simStartTime != 0;

        if (animation.TryStartTimelineAtAnchor(anchor)) {
            row.animation.msAnimationTime = CalculateAnimationTime(
                qpc,
                chain.firstAppSimStartTime,
                anchor.simStartTime);
            animation.SetCurrentAnchorAnimationTimeMs(row.animation.msAnimationTime);
        }
        else {
            row.animation.msAnimationTime = MissingFrameMetricValue();
            row.animation.hasResolvedSimStart = false;
        }

        if (DisplayTimingComplete_(row)) {
            ready.push_back(std::move(row));
        }
        else {
            timelineOriginAwaitingDisplayLookahead_ = std::move(row);
        }
    }

    void DisplayFrameQueue::ResolveIntervalAndHoldForLookahead_(
        const QpcConverter& qpc,
        AnimationErrorTracker& animation,
        const AnimationErrorTracker::AppAnchor& closingAnchor,
        std::vector<ReadyDisplayRow> intervalRows)
    {
        const auto contexts = animation.ResolveIntervalAndAdvanceAnchor(qpc, closingAnchor, intervalRows);
        for (size_t i = 0; i < intervalRows.size(); ++i) {
            intervalRows[i].animation = contexts[i];
        }
        while (!notDisplayedPresentsHeldForIntervalRelease_.empty()) {
            closedIntervalAwaitingDisplayLookahead_.push_back(
                MakeNotDisplayedRow_(std::move(notDisplayedPresentsHeldForIntervalRelease_.front())));
            notDisplayedPresentsHeldForIntervalRelease_.pop_front();
        }
        closedIntervalAwaitingDisplayLookahead_.insert(
            closedIntervalAwaitingDisplayLookahead_.end(),
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
        if (!openAppToAppIntervalRows_.empty()) {
            openAppToAppIntervalRows_.back().nextScreenTime = transitionAppRow.screenTime;
        }
        MoveAllToReady_(openAppToAppIntervalRows_, ready);
        while (!notDisplayedPresentsHeldForIntervalRelease_.empty()) {
            ready.push_back(MakeNotDisplayedRow_(std::move(notDisplayedPresentsHeldForIntervalRelease_.front())));
            notDisplayedPresentsHeldForIntervalRelease_.pop_front();
        }
        const auto anchor = animation.ResolveAppAnchor(
            chain,
            transitionAppRow.present,
            transitionAppRow.displayIndex,
            ingestPreviousPresent);
        std::vector<ReadyDisplayRow> transitionInterval;
        transitionInterval.push_back(std::move(transitionAppRow));
        const auto contexts = animation.ResolveIntervalAndAdvanceAnchor(qpc, anchor, transitionInterval);
        transitionInterval[0].animation = contexts[0];
        if (DisplayTimingComplete_(transitionInterval[0])) {
            ready.push_back(std::move(transitionInterval[0]));
        }
        else {
            timelineOriginAwaitingDisplayLookahead_ = std::move(transitionInterval[0]);
        }
        openAppToAppIntervalRows_.clear();
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
        // Metrics are calculated for every ready row. Swap-chain history advances
        // once per released present: use the app row when present, otherwise the
        // final row for that present.
        size_t groupBegin = 0;
        while (groupBegin < rows.size()) {
            size_t groupEnd = groupBegin + 1;
            while (groupEnd < rows.size() &&
                rows[groupEnd].present.presentStartTime == rows[groupBegin].present.presentStartTime) {
                ++groupEnd;
            }

            size_t rowToUpdateChain = groupEnd - 1;
            for (size_t i = groupBegin; i < groupEnd; ++i) {
                rows[i].updateSwapChainAfterRow = false;
                if (rows[i].isAppFrame) {
                    rowToUpdateChain = i;
                }
            }
            rows[rowToUpdateChain].updateSwapChainAfterRow = true;

            groupBegin = groupEnd;
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
