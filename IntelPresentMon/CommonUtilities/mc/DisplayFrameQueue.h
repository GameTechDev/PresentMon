// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <optional>
#include <vector>
#include "../qpc.h"
#include "AnimationErrorTracker.h"
#include "MetricsTypes.h"
#include "SwapChainState.h"
namespace pmon::util::metrics
{
    // Display queue
    //
    // Each display instance moves through this lifecycle:
    //   1. Ingest  - expand FrameData.displayed into one row per display instance.
    //   2. Complete display timing - supply nextScreenTime lookahead for a held row
    //      (CompleteRowsWaitingForDisplayLookahead_); see DisplayTimingComplete_.
    //   3. Resolve animation - close an app-to-app interval at the next app anchor
    //      and attach the result from AnimationErrorTracker; see animationComplete.
    //   4. Derive state-update role - decide, from the row's own present/display
    //      metadata, whether it advances long-lived swap-chain state when applied
    //      (ComputeStateUpdateRole_).
    //   5. Release - emit the row once animationComplete && displayTimingComplete
    //      (IsPublishable_), writing the role into
    //      ReadyDisplayRow::updateSwapChainAfterRow (Release_ / ReleaseAll_).
    class DisplayFrameQueue
    {
    public:
        std::vector<ReadyDisplayRow> Ingest(
            const QpcConverter& qpc,
            FrameData present,
            AnimationErrorTracker& animation,
            SwapChainCoreState& chain);
        void NoteSeedPresent(const FrameData& seedPresent);
        void Clear();
    private:
        // --- Steps 4-5: state-update role and release (design: "Row Readiness") ---
        // Whether a held row's swap-chain state update, if any, advances long-lived
        // swap-chain state when the row is applied. Derived explicitly from the
        // row's own present/display metadata (see ComputeStateUpdateRole_) and
        // consumed by Release_, which is the single place
        // ReadyDisplayRow::updateSwapChainAfterRow is set.
        enum class StateUpdateRole
        {
            None,
            AdvancesSwapChainState,
        };
        // Internal bookkeeping for a display instance held inside the queue. A row is
        // only publishable once animationComplete and displayTimingComplete are both true.
        struct PendingRow
        {
            ReadyDisplayRow row;
            bool animationComplete = false;
            bool displayTimingComplete = false;
            StateUpdateRole stateUpdateRole = StateUpdateRole::None;
        };
        static bool DisplayTimingComplete_(const ReadyDisplayRow& row);
        static bool IsPublishable_(const PendingRow& pending);
        // Explicit state-update role policy (design: "Displayed present with/without an
        // app row"). Computed purely from this row's own present structure, display
        // index, and frame type -- never from which other rows happen to be released
        // in the same Ingest/ProcessPresent call.
        static StateUpdateRole ComputeStateUpdateRole_(const ReadyDisplayRow& row);
        // Single place every release path funnels through: only emits rows whose
        // lifecycle state (design: Release Rule) is publishable, and is where the
        // row's explicit state-update role is written into updateSwapChainAfterRow.
        static void Release_(PendingRow pending, std::vector<ReadyDisplayRow>& ready);
        static void ReleaseAll_(std::vector<PendingRow>& pending, std::vector<ReadyDisplayRow>& ready);

        // --- Step 1: construct the display-instance row ---
        ReadyDisplayRow BuildDisplayInstanceRow_(
            const FrameData& present,
            size_t displayIndex,
            const SwapChainCoreState& chain) const;
        void ApplyNvV2Adjustment_(ReadyDisplayRow& row) const;
        void AcceptDisplayOrder_(ReadyDisplayRow& row);
        // --- Step 2: complete display timing for already-held rows ---
        // Supplies nextScreenTime lookahead to rows held only on displayTimingComplete,
        // then releases the ones that become publishable.
        void CompleteRowsWaitingForDisplayLookahead_(
            uint64_t nextScreenTime,
            std::vector<ReadyDisplayRow>& ready);
        void CompleteOpenTimelineDisplayLookahead_(uint64_t nextScreenTime);
        void IngestNotDisplayedRow_(
            ReadyDisplayRow row,
            std::vector<ReadyDisplayRow>& ready);
        void IngestGeneratedDisplayInstance_(
            ReadyDisplayRow row,
            std::vector<ReadyDisplayRow>& ready);
        // --- Step 3: resolve animation at app anchors ---
        void IngestAppAnchorDisplayInstance_(
            const QpcConverter& qpc,
            ReadyDisplayRow row,
            AnimationErrorTracker& animation,
            SwapChainCoreState& chain,
            const FrameData* ingestPreviousPresent,
            std::vector<ReadyDisplayRow>& ready);
        void PublishFirstTimelineOrigin_(
            const QpcConverter& qpc,
            ReadyDisplayRow row,
            AnimationErrorTracker& animation,
            const SwapChainCoreState& chain,
            const AnimationErrorTracker::AppAnchor& anchor,
            std::vector<ReadyDisplayRow>& ready);
        void ResolveIntervalAndHoldForLookahead_(
            const QpcConverter& qpc,
            AnimationErrorTracker& animation,
            const AnimationErrorTracker::AppAnchor& closingAnchor,
            std::vector<PendingRow> intervalRows,
            std::vector<ReadyDisplayRow>& ready);
        void PublishSourceTransition_(
            const QpcConverter& qpc,
            ReadyDisplayRow transitionAppRow,
            AnimationErrorTracker& animation,
            const SwapChainCoreState& chain,
            const FrameData* ingestPreviousPresent,
            std::vector<ReadyDisplayRow>& ready);
        static bool IsAppAnchor_(FrameType frameType);
        static bool IsDisplayed_(const FrameData& present);
        static bool PresentHasAppAnchor_(const FrameData& present);
        static ReadyDisplayRow MakeNotDisplayedRow_(FrameData present);
        uint64_t lastAcceptedScreenTime_ = 0;
        uint64_t lastAcceptedPresentStartTime_ = 0;
        uint64_t lastAcceptedFlipDelay_ = 0;
        bool hasObservedAppAnchor_ = false;
        // Rows after an app anchor, in ingest order, until the next app anchor closes
        // the interval. Only displayed rows participate in animation resolution.
        std::vector<PendingRow> openTimelineRows_;
        // Closed interval rows waiting for the displayed frame after the closing app
        // anchor. Already animation-complete; held only for displayTimingComplete.
        std::vector<PendingRow> closedIntervalAwaitingDisplayLookahead_;
        // Trace start before first app anchor; animation intentionally complete-but-missing.
        std::vector<PendingRow> preFirstAppAnchorRowsAwaitingDisplayLookahead_;
        // First app anchor (or source-transition origin) waiting for nextScreenTime.
        // Already animation-complete.
        std::optional<PendingRow> timelineOriginAwaitingDisplayLookahead_;
        // Last present that entered Ingest (including held rows). Used for anchor CpuStart
        // when swap chain lastPresent has not advanced yet.
        std::optional<FrameData> lastIngestedPresent_;
    };
}
