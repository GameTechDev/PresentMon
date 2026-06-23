// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include "InterPresentActivity.hpp"
#include "PresentMonTraceConsumer.hpp"

#include <algorithm>
#include <cstring>

namespace InterPresentActivityUtil
{
    bool ClipIntervalToWindow(uint64_t startQpc, uint64_t endQpc, uint64_t windowOldest, uint64_t windowNewest,
        uint64_t& clipStart, uint64_t& clipEnd)
    {
        if (endQpc <= windowOldest || startQpc >= windowNewest) {
            return false;
        }
        clipStart = std::max(startQpc, windowOldest);
        clipEnd = std::min(endQpc, windowNewest);
        return clipEnd > clipStart;
    }

    uint64_t MergeBusyQpc(std::vector<QpcInterval> intervals)
    {
        if (intervals.empty()) {
            return 0;
        }
        std::sort(intervals.begin(), intervals.end(), [](const QpcInterval& a, const QpcInterval& b) {
            return a.startQpc < b.startQpc;
        });
        uint64_t mergedBusyQpc = 0;
        uint64_t curStart = intervals.front().startQpc;
        uint64_t curEnd = intervals.front().endQpc;
        for (size_t i = 1; i < intervals.size(); ++i) {
            const auto& iv = intervals[i];
            if (iv.startQpc <= curEnd) {
                if (iv.endQpc > curEnd) {
                    curEnd = iv.endQpc;
                }
            }
            else {
                mergedBusyQpc += curEnd - curStart;
                curStart = iv.startQpc;
                curEnd = iv.endQpc;
            }
        }
        mergedBusyQpc += curEnd - curStart;
        return mergedBusyQpc;
    }

    uint64_t SumBusyQpc(const std::vector<QpcInterval>& intervals)
    {
        uint64_t summedBusyQpc = 0;
        for (const auto& iv : intervals) {
            if (iv.endQpc > iv.startQpc) {
                summedBusyQpc += iv.endQpc - iv.startQpc;
            }
        }
        return summedBusyQpc;
    }
}

namespace
{
    using QpcInterval = InterPresentActivityUtil::QpcInterval;

    void AppendInterval_(std::vector<QpcInterval>& intervals, uint64_t startQpc, uint64_t endQpc)
    {
        if (endQpc > startQpc) {
            intervals.push_back(QpcInterval{ startQpc, endQpc });
        }
    }
}

bool InterPresentActivity::ActivityKey::operator==(const ActivityKey& other) const noexcept
{
    return kind == other.kind &&
        processId == other.processId &&
        std::memcmp(activityId, other.activityId, sizeof(activityId)) == 0;
}

size_t InterPresentActivity::ActivityKeyHash::operator()(const ActivityKey& key) const noexcept
{
    size_t h = std::hash<uint32_t>{}((uint32_t)key.kind);
    h = h * 31u + std::hash<uint32_t>{}(key.processId);
    for (size_t i = 0; i < sizeof(key.activityId); ++i) {
        h = h * 31u + (size_t)key.activityId[i];
    }
    return h;
}

InterPresentActivity::ActivityKey InterPresentActivity::MakeKey_(Kind kind, uint32_t processId, const uint8_t activityId[16])
{
    ActivityKey key{};
    key.kind = kind;
    key.processId = processId;
    std::memcpy(key.activityId, activityId, sizeof(key.activityId));
    return key;
}

InterPresentActivity::ProcessState& InterPresentActivity::GetProcessState_(uint32_t processId)
{
    return processStateByPid_[processId];
}

void InterPresentActivity::OnActivityStart(Kind kind, uint32_t processId, const uint8_t activityId[16], uint64_t startQpc)
{
    const ActivityKey key = MakeKey_(kind, processId, activityId);
    auto& channel = GetProcessState_(processId).channels[(size_t)kind];
    const auto existing = channel.tracked.find(key);
    if (existing != channel.tracked.end()) {
        pmlog_warn("Inter-present activity start while same activity id is already in flight")
            .pmwatch(processId)
            .pmwatch((uint32_t)kind)
            .pmwatch(startQpc)
            .pmwatch(existing->second.startQpc);
    }
    channel.tracked[key] = TrackedActivity{ startQpc, false };
}

void InterPresentActivity::OnActivityStop(Kind kind, uint32_t processId, const uint8_t activityId[16], uint64_t stopQpc)
{
    const ActivityKey key = MakeKey_(kind, processId, activityId);
    auto& state = GetProcessState_(processId);
    auto& channel = state.channels[(size_t)kind];
    const auto it = channel.tracked.find(key);
    if (it == channel.tracked.end()) {
        pmlog_warn("Inter-present activity stop without matching in-flight start")
            .pmwatch(processId)
            .pmwatch((uint32_t)kind)
            .pmwatch(stopQpc);
        return;
    }

    const uint64_t startQpc = it->second.startQpc;
    channel.tracked.erase(it);

    if (stopQpc <= startQpc) {
        pmlog_warn("Inter-present activity stop timestamp is not after start")
            .pmwatch(processId)
            .pmwatch((uint32_t)kind)
            .pmwatch(startQpc)
            .pmwatch(stopQpc);
        return;
    }

    // Attribute the finished span to the current open inter-present bucket (since
    // lastInterPresentCloseQpc through stop).
    const uint64_t frameStart = state.lastInterPresentCloseQpc;
    uint64_t clipStart = 0;
    uint64_t clipEnd = 0;
    if (InterPresentActivityUtil::ClipIntervalToWindow(startQpc, stopQpc, frameStart, stopQpc, clipStart, clipEnd)) {
        AppendInterval_(channel.completedIntervals, clipStart, clipEnd);
        if (startQpc >= frameStart && startQpc <= stopQpc) {
            ++channel.completedStartCountInOpenBucket;
        }
    }
}

void InterPresentActivity::CompleteFrame(PresentEvent* pEvent, uint64_t interPresentCloseQpc)
{
    if (pEvent == nullptr) {
        pmlog_warn("Inter-present CompleteFrame called with null PresentEvent");
        return;
    }
    if (pEvent->InterPresentFrameCompleted) {
        pmlog_warn("Inter-present frame stats already completed for this PresentEvent")
            .pmwatch(pEvent->ProcessId)
            .pmwatch(interPresentCloseQpc);
        return;
    }

    pEvent->InterPresentFrameCompleted = true;

    auto& state = GetProcessState_(pEvent->ProcessId);
    const uint64_t frameStart = state.lastInterPresentCloseQpc;
    const uint64_t framePeriodQpc = interPresentCloseQpc > frameStart ? interPresentCloseQpc - frameStart : 0;
    pEvent->InterPresentFramePeriodQpc = framePeriodQpc;

    for (size_t kindIndex = 0; kindIndex < (size_t)Kind::Count; ++kindIndex) {
        auto& channel = state.channels[kindIndex];
        std::vector<QpcInterval> busyIntervals = channel.completedIntervals;
        uint64_t activityCount = channel.completedStartCountInOpenBucket;

        // Still-running activities contribute busy time for this frame's window. Work that began
        // before the window opened credits the full frame period each overlap; activityCount is
        // attributed at most once per ETW activity (when its start falls in that frame's window).
        for (auto& [key, tracked] : channel.tracked) {
            if (key.processId != pEvent->ProcessId) {
                pmlog_warn("In-flight inter-present activity key process id does not match presenting frame")
                    .pmwatch(key.processId)
                    .pmwatch(pEvent->ProcessId)
                    .pmwatch((uint32_t)key.kind);
                continue;
            }
            if (tracked.startQpc >= interPresentCloseQpc) {
                continue;
            }

            if (tracked.startQpc < frameStart) {
                if (framePeriodQpc > 0) {
                    AppendInterval_(busyIntervals, frameStart, interPresentCloseQpc);
                }
            }
            else {
                AppendInterval_(busyIntervals, tracked.startQpc, interPresentCloseQpc);
            }

            if (!tracked.activityCountAttributed &&
                tracked.startQpc >= frameStart &&
                tracked.startQpc < interPresentCloseQpc) {
                ++activityCount;
                tracked.activityCountAttributed = true;
            }
        }

        const uint64_t summedBusyQpc = InterPresentActivityUtil::SumBusyQpc(busyIntervals);
        const uint64_t mergedBusyQpc = InterPresentActivityUtil::MergeBusyQpc(std::move(busyIntervals));
        pEvent->InterPresentStats[kindIndex] = { activityCount, mergedBusyQpc, summedBusyQpc };

        channel.completedIntervals.clear();
        channel.completedStartCountInOpenBucket = 0;
    }

    state.lastInterPresentCloseQpc = interPresentCloseQpc;
}
