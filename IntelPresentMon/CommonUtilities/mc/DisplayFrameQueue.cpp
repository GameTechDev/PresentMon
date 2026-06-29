// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DisplayFrameQueue.h"
#include "MetricsCalculatorInternal.h"
#include <cassert>
#include <utility>
namespace pmon::util::metrics
{
    // Ingest is called for every present and is responsible for expanding presents into display instances,
    // attaching animation contexts from the AnimationErrorTracker, and buffering until the publish policy allows
    // rows to be released. If frame-type metadata exists, each metadata entry becomes a row even when
    // the present was not displayed. Displayed generated rows wait for an app anchor; displayed app rows
    // close the prior interval and then wait for lookahead before release.
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

        const bool presentIsDisplayed = IsDisplayed_(present);
        const size_t displayCount = present.displayed.Size();

        std::vector<ReadyDisplayRow> ready;
        if (displayCount == 0) {
            IngestNotDisplayedRow_(MakeNotDisplayedRow_(std::move(present)), ready);
            lastIngestedPresent_ = std::move(presentForIngestHistory);
            return ready;
        }

        for (size_t displayIndex = 0; displayIndex < displayCount; ++displayIndex) {
            ReadyDisplayRow row = BuildDisplayInstanceRow_(present, displayIndex, chain);

            if (!presentIsDisplayed) {
                row.isDisplayed = false;
                row.previousDisplayedScreenTime = chain.lastDisplayedScreenTime;
                row.screenTime = 0;
                row.nextScreenTime = 0;
                IngestNotDisplayedRow_(std::move(row), ready);
                continue;
            }

            ApplyNvV2Adjustment_(row);

            // This displayed instance is lookahead for rows that were complete except
            // for nextScreenTime.
            CompleteRowsWaitingForDisplayLookahead_(row.screenTime, ready);

            // Rows in the open app-to-app interval use the next displayed instance
            // for their display duration, even before the interval closes.
            if (!openAppToAppIntervalRows_.empty()) {
                openAppToAppIntervalRows_.back().row.nextScreenTime = row.screenTime;
                openAppToAppIntervalRows_.back().displayTimingComplete = true;
            }
            AcceptDisplayOrder_(row);
            if (!row.isAppFrame) {
                IngestGeneratedDisplayInstance_(std::move(row), ready);
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
        hasObservedAppAnchor_ = false;
        openAppToAppIntervalRows_.clear();
        closedIntervalAwaitingDisplayLookahead_.clear();
        preFirstAppAnchorRowsAwaitingDisplayLookahead_.clear();
        timelineOriginAwaitingDisplayLookahead_.reset();
        notDisplayedRowsHeldForIntervalRelease_.clear();
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

    // Display timing has no meaning for a not-displayed row, so it is trivially
    // complete. Displayed rows with a sibling display entry already have lookahead.
    // The explicit PendingRow flag handles later lookahead, including valid
    // zero-duration cases where nextScreenTime == screenTime.
    bool DisplayFrameQueue::DisplayTimingComplete_(const ReadyDisplayRow& row)
    {
        if (!row.isDisplayed) {
            return true;
        }
        return row.displayIndex + 1 < row.present.displayed.Size();
    }

    // Step 5 gate: the single place publishability (design: Row Readiness) is
    // checked. A row may only leave the queue once its animation context and display
    // timing are both resolved, even when the animation result is intentionally "not set".
    bool DisplayFrameQueue::IsPublishable_(const PendingRow& pending)
    {
        return pending.animationComplete && pending.displayTimingComplete;
    }

    // Step 4: explicit state-update role policy (design: "Displayed present with an
    // app row" / "Displayed present without an app row"). This depends only on the row's own
    // present structure, display index, and frame type, so it gives the same answer
    // regardless of which other rows from this present have already released or are
    // still buffered:
    //   isAppFrame                                       -> the app row represents
    //                                                        the present.
    //   present has no app anchor and this is the final  -> the final display row
    //   display index of the present                        represents the present
    //                                                        (e.g. PresentFrameType_Info
    //                                                        generated-only presents).
    //   otherwise                                         -> this row does not
    //                                                        represent the present.
    DisplayFrameQueue::StateUpdateRole DisplayFrameQueue::ComputeStateUpdateRole_(const ReadyDisplayRow& row)
    {
        if (row.isAppFrame) {
            return StateUpdateRole::AdvancesSwapChainState;
        }
        if (!PresentHasAppAnchor_(row.present) &&
            row.displayIndex + 1 == row.present.displayed.Size()) {
            return StateUpdateRole::AdvancesSwapChainState;
        }
        return StateUpdateRole::None;
    }

    // Step 5: release. Consumes the state-update role assigned in step 4 -- derived
    // from the row's own present/display metadata -- to decide whether
    // MetricsCalculator applies this row's swap-chain state update. Never decided by
    // which rows happened to release together.
    void DisplayFrameQueue::Release_(PendingRow pending, std::vector<ReadyDisplayRow>& ready)
    {
        assert(IsPublishable_(pending) && "DisplayFrameQueue: released a row that is not publishable");
        pending.row.updateSwapChainAfterRow =
            pending.stateUpdateRole == StateUpdateRole::AdvancesSwapChainState;
        ready.push_back(std::move(pending.row));
    }

    // Pending collections can hold rows with mixed readiness (e.g. a closing app
    // anchor still awaiting lookahead alongside rows that just became complete), so
    // this filters explicitly instead of assuming every entry is publishable.
    void DisplayFrameQueue::ReleaseAll_(std::vector<PendingRow>& pending, std::vector<ReadyDisplayRow>& ready)
    {
        std::vector<PendingRow> retained;
        for (auto& row : pending) {
            if (IsPublishable_(row)) {
                Release_(std::move(row), ready);
            }
            else {
                retained.push_back(std::move(row));
            }
        }
        pending = std::move(retained);
    }

    // Step 2: complete display timing. Closed intervals and timeline origins wait for
    // the next displayed frame after the closing/origin app anchor. This method
    // supplies that lookahead and releases those rows. Pre-first-app-anchor rows are
    // handled in their routing branch.
    void DisplayFrameQueue::CompleteRowsWaitingForDisplayLookahead_(
        uint64_t nextScreenTime,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!closedIntervalAwaitingDisplayLookahead_.empty()) {
            closedIntervalAwaitingDisplayLookahead_.back().row.nextScreenTime = nextScreenTime;
            closedIntervalAwaitingDisplayLookahead_.back().displayTimingComplete = true;
            ReleaseAll_(closedIntervalAwaitingDisplayLookahead_, ready);
        }

        if (timelineOriginAwaitingDisplayLookahead_.has_value()) {
            timelineOriginAwaitingDisplayLookahead_->row.nextScreenTime = nextScreenTime;
            timelineOriginAwaitingDisplayLookahead_->displayTimingComplete = true;
            if (IsPublishable_(*timelineOriginAwaitingDisplayLookahead_)) {
                Release_(std::move(*timelineOriginAwaitingDisplayLookahead_), ready);
                timelineOriginAwaitingDisplayLookahead_.reset();
            }
        }
    }

    // -------------------------------------------------------------------------
    // Step 1: ingest routing -- not-displayed rows
    // -------------------------------------------------------------------------
    void DisplayFrameQueue::IngestNotDisplayedRow_(
        ReadyDisplayRow row,
        std::vector<ReadyDisplayRow>& ready)
    {
        // If there's an app anchor, hold not-displayed rows until the next app anchor closes the interval,
        // at which point they'll be released with animation metrics. Otherwise, they can be emitted immediately
        // with missing animation metrics. This handles cases where we receive generated frames without a prior app anchor,
        // which is expected to be common when the provider starts after the app has already started presenting.
        //
        // A not-displayed present can carry several frame-type metadata rows (one row per
        // entry); the same representative-row policy as displayed presents applies (design:
        // "the emitted not-displayed row that represents the present"), so the role is
        // computed the same way: the app-type entry if there is one, otherwise the final
        // metadata entry.
        const auto role = ComputeStateUpdateRole_(row);
        const bool displayTimingComplete = DisplayTimingComplete_(row);
        if (hasObservedAppAnchor_) {
            notDisplayedRowsHeldForIntervalRelease_.push_back(
                PendingRow{ std::move(row), /*animationComplete*/ false, displayTimingComplete, role });
        }
        else {
            Release_(PendingRow{ std::move(row), /*animationComplete*/ true, displayTimingComplete, role }, ready);
        }
    }

    // -------------------------------------------------------------------------
    // Step 1: ingest routing -- generated display instances
    // -------------------------------------------------------------------------
    void DisplayFrameQueue::IngestGeneratedDisplayInstance_(
        ReadyDisplayRow row,
        std::vector<ReadyDisplayRow>& ready)
    {
        if (!hasObservedAppAnchor_) {
            // Design: trace start without prior app anchor.
            if (!preFirstAppAnchorRowsAwaitingDisplayLookahead_.empty()) {
                auto pendingOrigin = std::move(preFirstAppAnchorRowsAwaitingDisplayLookahead_.back());
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.pop_back();
                pendingOrigin.row.nextScreenTime = row.screenTime;
                pendingOrigin.displayTimingComplete = true;
                Release_(std::move(pendingOrigin), ready);
            }
            const auto role = ComputeStateUpdateRole_(row);
            const bool displayTimingComplete = DisplayTimingComplete_(row);
            PendingRow pending{ std::move(row), /*animationComplete*/ true, displayTimingComplete, role };
            if (pending.displayTimingComplete) {
                Release_(std::move(pending), ready);
            }
            else {
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.push_back(std::move(pending));
            }
            return;
        }
        // Design: hold generated rows until the next app anchor closes the interval.
        // Role is assigned now, before the row is released by anything: a generated
        // row from a present that has no app anchor of its own (PresentFrameType_Info
        // generated-only present) still represents that present once it is the final
        // display row, even while it sits inside this open interval.
        const auto role = ComputeStateUpdateRole_(row);
        const bool displayTimingComplete = DisplayTimingComplete_(row);
        openAppToAppIntervalRows_.push_back(
            PendingRow{ std::move(row), /*animationComplete*/ false, displayTimingComplete, role });
    }

    // -------------------------------------------------------------------------
    // Step 3: resolve animation at app anchors (design: App Anchor, Closed Interval)
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
        const bool isFirstObservedAppAnchor = !hasObservedAppAnchor_;
        hasObservedAppAnchor_ = true;

        if (isFirstObservedAppAnchor) {
            if (!preFirstAppAnchorRowsAwaitingDisplayLookahead_.empty()) {
                auto pendingOrigin = std::move(preFirstAppAnchorRowsAwaitingDisplayLookahead_.back());
                preFirstAppAnchorRowsAwaitingDisplayLookahead_.pop_back();
                pendingOrigin.row.nextScreenTime = row.screenTime;
                pendingOrigin.displayTimingComplete = true;
                Release_(std::move(pendingOrigin), ready);
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
        std::vector<PendingRow> closedInterval;
        closedInterval.reserve(openAppToAppIntervalRows_.size() + 1);
        for (auto& pending : openAppToAppIntervalRows_) {
            closedInterval.push_back(std::move(pending));
        }
        const auto role = ComputeStateUpdateRole_(row);
        const bool displayTimingComplete = DisplayTimingComplete_(row);
        closedInterval.push_back(PendingRow{ std::move(row), /*animationComplete*/ false, displayTimingComplete, role });
        ResolveIntervalAndHoldForLookahead_(qpc, animation, anchor, std::move(closedInterval), ready);
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

        const auto role = ComputeStateUpdateRole_(row);
        const bool displayTimingComplete = DisplayTimingComplete_(row);
        PendingRow pending{ std::move(row), /*animationComplete*/ true, displayTimingComplete, role };
        if (pending.displayTimingComplete) {
            Release_(std::move(pending), ready);
        }
        else {
            timelineOriginAwaitingDisplayLookahead_ = std::move(pending);
        }
    }

    // Resolving an interval makes every row in it animationComplete, but rows do not
    // have to release together (design: Closed Interval / Row Readiness). Generated
    // rows already display-timing-complete (they already got nextScreenTime from a
    // sibling or later display instance) publish immediately here. The closing app
    // anchor is usually still missing its own nextScreenTime lookahead and stays
    // buffered until CompleteRowsWaitingForDisplayLookahead_ supplies it.
    void DisplayFrameQueue::ResolveIntervalAndHoldForLookahead_(
        const QpcConverter& qpc,
        AnimationErrorTracker& animation,
        const AnimationErrorTracker::AppAnchor& closingAnchor,
        std::vector<PendingRow> intervalRows,
        std::vector<ReadyDisplayRow>& ready)
    {
        std::vector<ReadyDisplayRow> animationRows;
        animationRows.reserve(intervalRows.size());
        for (const auto& pending : intervalRows) {
            animationRows.push_back(pending.row);
        }

        const auto contexts = animation.ResolveIntervalAndAdvanceAnchor(qpc, closingAnchor, animationRows);
        for (size_t i = 0; i < intervalRows.size(); ++i) {
            intervalRows[i].row.animation = contexts[i];
            intervalRows[i].animationComplete = true;
        }

        std::vector<PendingRow> resolved;
        resolved.reserve(intervalRows.size() + notDisplayedRowsHeldForIntervalRelease_.size());
        while (!notDisplayedRowsHeldForIntervalRelease_.empty()) {
            auto pendingNotDisplayed = std::move(notDisplayedRowsHeldForIntervalRelease_.front());
            notDisplayedRowsHeldForIntervalRelease_.pop_front();
            pendingNotDisplayed.animationComplete = true;
            resolved.push_back(std::move(pendingNotDisplayed));
        }
        for (auto& pending : intervalRows) {
            resolved.push_back(std::move(pending));
        }

        for (auto& pending : resolved) {
            if (IsPublishable_(pending)) {
                Release_(std::move(pending), ready);
            }
            else {
                closedIntervalAwaitingDisplayLookahead_.push_back(std::move(pending));
            }
        }
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
            openAppToAppIntervalRows_.back().row.nextScreenTime = transitionAppRow.screenTime;
            openAppToAppIntervalRows_.back().displayTimingComplete = true;
        }
        // Abandoned by the transition: animation is intentionally not set for these rows.
        // Each row's nextScreenTime is already known (chained from the next display
        // instance as it arrived, or from the transition row above), but route release
        // through ReleaseAll_ rather than asserting so an unexpected gap retains the row
        // instead of publishing it.
        for (auto& pending : openAppToAppIntervalRows_) {
            pending.animationComplete = true;
        }
        ReleaseAll_(openAppToAppIntervalRows_, ready);

        while (!notDisplayedRowsHeldForIntervalRelease_.empty()) {
            auto pendingNotDisplayed = std::move(notDisplayedRowsHeldForIntervalRelease_.front());
            notDisplayedRowsHeldForIntervalRelease_.pop_front();
            pendingNotDisplayed.animationComplete = true;
            Release_(std::move(pendingNotDisplayed), ready);
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
        const auto role = ComputeStateUpdateRole_(transitionInterval[0]);
        const bool displayTimingComplete = DisplayTimingComplete_(transitionInterval[0]);
        PendingRow pendingOrigin{
            std::move(transitionInterval[0]), /*animationComplete*/ true, displayTimingComplete, role };
        if (pendingOrigin.displayTimingComplete) {
            Release_(std::move(pendingOrigin), ready);
        }
        else {
            timelineOriginAwaitingDisplayLookahead_ = std::move(pendingOrigin);
        }
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

    bool DisplayFrameQueue::PresentHasAppAnchor_(const FrameData& present)
    {
        for (size_t i = 0; i < present.displayed.Size(); ++i) {
            if (IsAppAnchor_(present.displayed[i].first)) {
                return true;
            }
        }
        return false;
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
