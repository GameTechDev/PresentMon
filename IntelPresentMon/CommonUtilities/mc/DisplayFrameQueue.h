// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <deque>
#include <optional>
#include <vector>
#include "../qpc.h"
#include "AnimationErrorTracker.h"
#include "MetricsTypes.h"
#include "SwapChainState.h"
namespace pmon::util::metrics
{
    // Display queue (design: "Display Queue" component).
    // Ingest expands presents into display instances, buffers until publish policy
    // allows release, and attaches animation results from AnimationErrorTracker.
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
        // --- Display instance construction
        ReadyDisplayRow BuildDisplayInstanceRow_(
            const FrameData& present,
            size_t displayIndex,
            const SwapChainCoreState& chain) const;
        void ApplyNvV2Adjustment_(ReadyDisplayRow& row) const;
        void AcceptDisplayOrder_(ReadyDisplayRow& row);
        // --- Release rule (design: Release Rule) ---
        void CompleteRowsWaitingForDisplayLookahead_(
            uint64_t nextScreenTime,
            std::vector<ReadyDisplayRow>& ready);
        // --- Ingest routing
        void IngestNotDisplayedPresent_(
            FrameData present,
            AnimationErrorTracker& animation,
            std::vector<ReadyDisplayRow>& ready);
        void IngestGeneratedDisplayInstance_(
            ReadyDisplayRow row,
            AnimationErrorTracker& animation,
            std::vector<ReadyDisplayRow>& ready);
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
            std::vector<ReadyDisplayRow> intervalRows);
        void PublishSourceTransition_(
            const QpcConverter& qpc,
            ReadyDisplayRow transitionAppRow,
            AnimationErrorTracker& animation,
            const SwapChainCoreState& chain,
            const FrameData* ingestPreviousPresent,
            std::vector<ReadyDisplayRow>& ready);
        static bool IsAppAnchor_(FrameType frameType);
        static bool IsDisplayed_(const FrameData& present);
        static bool DisplayTimingComplete_(const ReadyDisplayRow& row);
        static ReadyDisplayRow MakeNotDisplayedRow_(FrameData present);
        static void MarkPresentUpdateRows_(std::vector<ReadyDisplayRow>& rows);
        uint64_t lastAcceptedScreenTime_ = 0;
        uint64_t lastAcceptedPresentStartTime_ = 0;
        uint64_t lastAcceptedFlipDelay_ = 0;
        // Generated rows after an app anchor, until the next app anchor closes the interval.
        std::vector<ReadyDisplayRow> openAppToAppIntervalRows_;
        // Closed interval rows waiting for the displayed frame after the closing app anchor.
        std::vector<ReadyDisplayRow> closedIntervalAwaitingDisplayLookahead_;
        // Trace start before first app anchor; animation metrics not set.
        std::vector<ReadyDisplayRow> preFirstAppAnchorRowsAwaitingDisplayLookahead_;
        // First app anchor (or source-transition origin) waiting for nextScreenTime.
        std::optional<ReadyDisplayRow> timelineOriginAwaitingDisplayLookahead_;
        // Not-displayed presents held while an app anchor exists.
        std::deque<FrameData> notDisplayedPresentsHeldForIntervalRelease_;
        // Last present that entered Ingest (including held rows). Used for anchor CpuStart
        // when swap chain lastPresent has not advanced yet.
        std::optional<FrameData> lastIngestedPresent_;
    };
}
