// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "DisplayFrameQueue.h"
#include "MetricsCalculatorInternal.h"

#include <iterator>

namespace pmon::util::metrics
{
    std::vector<ReadyDisplayRow> DisplayFrameQueue::Enqueue(
        const QpcConverter& qpc,
        FrameData present,
        AnimationErrorTracker& animation,
        SwapChainCoreState& chain)
    {
        std::vector<ReadyDisplayRow> rows;

        if (!IsDisplayed_(present)) {
            if (animation.HasAnchor()) {
                blockedNotDisplayed_.push_back(std::move(present));
            }
            else {
                rows.push_back(MakeNotDisplayedRow_(std::move(present)));
            }
            return rows;
        }

        const size_t displayCount = present.displayed.Size();
        for (size_t i = 0; i < displayCount; ++i) {
            const auto frameType = present.displayed[i].first;
            const auto screenTime = present.displayed[i].second;
            const auto nextScreenTime = (i + 1 < displayCount) ? present.displayed[i + 1].second : screenTime;
            const auto previousScreenTime = lastAcceptedScreenTime_ != 0 ? lastAcceptedScreenTime_ : chain.lastDisplayedScreenTime;

            ReadyDisplayRow row{};
            row.present = present;
            row.displayIndex = i;
            row.previousDisplayedScreenTime = previousScreenTime;
            row.screenTime = screenTime;
            row.nextScreenTime = nextScreenTime;
            row.isDisplayed = true;
            row.isAppFrame = IsAppAnchor_(frameType);
            row.frameType = frameType;
            row.updateSwapChainAfterRow = false;

            ApplyNvV2Adjustment_(row);

            ReleaseClosedInterval_(rows, row.screenTime);
            ReleaseTimelineOriginHold_(rows, row.screenTime);

            if (!pendingIntervalRows_.empty()) {
                pendingIntervalRows_.back().nextScreenTime = row.screenTime;
            }

            lastAcceptedScreenTime_ = row.screenTime;
            lastAcceptedPresentStartTime_ = present.presentStartTime;
            lastAcceptedFlipDelay_ = row.present.flipDelay;

            if (!row.isAppFrame) {
                if (animation.HasAnchor()) {
                    pendingIntervalRows_.push_back(std::move(row));
                }
                else {
                    ReleasePreAnchorHold_(rows, row.screenTime);
                    if (row.nextScreenTime != row.screenTime) {
                        rows.push_back(std::move(row));
                    }
                    else {
                        preAnchorPending_.push_back(std::move(row));
                    }
                }
                continue;
            }

            auto anchor = animation.ResolveAnchor(row.present, row.displayIndex);
            if (!animation.HasAnchor()) {
                ReleasePreAnchorHold_(rows, row.screenTime);
                animation.SeedAnchor(anchor);
                row.animation.msAnimationError = MissingFrameMetricValue();
                row.animation.msAnimationTime = 0.0;
                row.animation.source = anchor.source;
                row.animation.resolvedSimStartTime = anchor.simStartTime;
                row.animation.firstSimStartTime = anchor.simStartTime;
                row.animation.hasResolvedSimStart = anchor.simStartTime != 0;
                if (row.nextScreenTime != row.screenTime) {
                    rows.push_back(std::move(row));
                }
                else {
                    timelineOriginPending_.push_back(std::move(row));
                }
                pendingIntervalRows_.clear();
                continue;
            }

            std::vector<ReadyDisplayRow> intervalRows;
            if (!animation.IsTransition(anchor)) {
                intervalRows = std::move(pendingIntervalRows_);
            }
            else {
                if (!pendingIntervalRows_.empty()) {
                    pendingIntervalRows_.back().nextScreenTime = row.screenTime;
                }
                rows.insert(
                    rows.end(),
                    std::make_move_iterator(pendingIntervalRows_.begin()),
                    std::make_move_iterator(pendingIntervalRows_.end()));
                pendingIntervalRows_.clear();

                while (!blockedNotDisplayed_.empty()) {
                    rows.push_back(MakeNotDisplayedRow_(std::move(blockedNotDisplayed_.front())));
                    blockedNotDisplayed_.pop_front();
                }

                intervalRows.push_back(row);
                auto contexts = animation.CloseInterval(qpc, anchor, intervalRows);
                intervalRows[0].animation = contexts[0];

                if (intervalRows[0].nextScreenTime != intervalRows[0].screenTime) {
                    rows.push_back(std::move(intervalRows[0]));
                }
                else {
                    timelineOriginPending_.push_back(std::move(intervalRows[0]));
                }
                pendingIntervalRows_.clear();
                continue;
            }
            intervalRows.push_back(row);

            auto contexts = animation.CloseInterval(qpc, anchor, intervalRows);
            for (size_t rowIndex = 0; rowIndex < intervalRows.size(); ++rowIndex) {
                intervalRows[rowIndex].animation = contexts[rowIndex];
            }

            while (!blockedNotDisplayed_.empty()) {
                closedIntervalRows_.push_back(MakeNotDisplayedRow_(std::move(blockedNotDisplayed_.front())));
                blockedNotDisplayed_.pop_front();
            }
            closedIntervalRows_.insert(
                closedIntervalRows_.end(),
                std::make_move_iterator(intervalRows.begin()),
                std::make_move_iterator(intervalRows.end()));
            pendingIntervalRows_.clear();
        }

        MarkPresentUpdateRows_(rows);
        return rows;
    }

    void DisplayFrameQueue::Clear()
    {
        lastAcceptedScreenTime_ = 0;
        lastAcceptedPresentStartTime_ = 0;
        lastAcceptedFlipDelay_ = 0;
        closedIntervalRows_.clear();
        pendingIntervalRows_.clear();
        preAnchorPending_.clear();
        timelineOriginPending_.clear();
        blockedNotDisplayed_.clear();
    }

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

    void DisplayFrameQueue::ReleaseClosedInterval_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime)
    {
        if (closedIntervalRows_.empty()) {
            return;
        }

        closedIntervalRows_.back().nextScreenTime = nextScreenTime;
        rows.insert(
            rows.end(),
            std::make_move_iterator(closedIntervalRows_.begin()),
            std::make_move_iterator(closedIntervalRows_.end()));
        closedIntervalRows_.clear();
    }

    void DisplayFrameQueue::ReleasePreAnchorHold_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime)
    {
        if (preAnchorPending_.empty()) {
            return;
        }

        preAnchorPending_.back().nextScreenTime = nextScreenTime;
        rows.push_back(std::move(preAnchorPending_.back()));
        preAnchorPending_.pop_back();
    }

    void DisplayFrameQueue::ReleaseTimelineOriginHold_(std::vector<ReadyDisplayRow>& rows, uint64_t nextScreenTime)
    {
        if (timelineOriginPending_.empty()) {
            return;
        }

        timelineOriginPending_.back().nextScreenTime = nextScreenTime;
        rows.push_back(std::move(timelineOriginPending_.back()));
        timelineOriginPending_.pop_back();
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
