// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <deque>
#include <vector>

#include "../qpc.h"
#include "AnimationErrorTracker.h"
#include "MetricsTypes.h"
#include "SwapChainState.h"

namespace pmon::util::metrics
{
    class DisplayFrameQueue
    {
    public:
        std::vector<ReadyDisplayRow> Enqueue(
            const QpcConverter& qpc,
            FrameData present,
            AnimationErrorTracker& animation,
            SwapChainCoreState& chain);

        void Clear();

    private:
        static bool IsAppAnchor_(FrameType frameType);
        static bool IsDisplayed_(const FrameData& present);
        static ReadyDisplayRow MakeNotDisplayedRow_(FrameData present);
        static void MarkPresentUpdateRows_(std::vector<ReadyDisplayRow>& rows);
        void ReleaseClosedInterval_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime);
        void ReleasePreAnchorHold_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime);
        void ReleaseTimelineOriginHold_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime);
        void ApplyNvV2Adjustment_(ReadyDisplayRow& row) const;

        uint64_t lastAcceptedScreenTime_ = 0;
        uint64_t lastAcceptedPresentStartTime_ = 0;
        uint64_t lastAcceptedFlipDelay_ = 0;
        std::vector<ReadyDisplayRow> closedIntervalRows_;
        std::vector<ReadyDisplayRow> pendingIntervalRows_;
        std::vector<ReadyDisplayRow> preAnchorPending_;
        std::vector<ReadyDisplayRow> timelineOriginPending_;
        std::deque<FrameData> blockedNotDisplayed_;
    };
}
