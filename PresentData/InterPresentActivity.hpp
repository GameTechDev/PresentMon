// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct PresentEvent;

namespace InterPresentActivityUtil
{
    struct QpcInterval
    {
        uint64_t startQpc = 0;
        uint64_t endQpc = 0;
    };

    // Returns whether any portion of [startQpc, endQpc] overlaps [windowOldest, windowNewest] and,
    // if so, the clipped span. Used so frame metrics only include work that falls inside the
    // inter-present window being closed out.
    bool ClipIntervalToWindow(uint64_t startQpc, uint64_t endQpc, uint64_t windowOldest, uint64_t windowNewest,
        uint64_t& clipStart, uint64_t& clipEnd);

    // Total QPC length after merging overlapping intervals. This is the "how long was the GPU/CPU
    // pipeline busy with at least one of these activities" measure (union of busy time).
    uint64_t MergeBusyQpc(std::vector<QpcInterval> intervals);

    // Sum of interval lengths without merging. When many activities overlap, this can exceed
    // MergeBusyQpc; both are exposed so consumers can report stacked effort vs. wall-clock busy.
    uint64_t SumBusyQpc(const std::vector<QpcInterval>& intervals);
}

// Correlates ETW "activity" events (work that spans time between presents) with the presenting
// frame that should own the metrics. Inter-present windows are bounded by interPresentCloseQpc
// values from PMTraceConsumer (GPU present queue complete or MMIO flip), not DXGI Present_Start.
// The trace consumer feeds start/stop from providers such as D3D12 CreatePipelineStateObject;
// CompleteFrame runs when a PresentEvent is finalized and writes per-frame stats onto that event.
class InterPresentActivity final
{
public:
    // One enumerator per inter-present activity family traced from ETW. Extend Kind and wire the
    // provider in PMTraceConsumer when adding new activity types.
    enum class Kind : uint8_t
    {
        D3D12PsoCompile = 0,
        Count
    };

    // Per-frame attribution for a single Kind, stored on PresentEvent::InterPresentStats.
    struct FrameStats
    {
        // Activities whose start fell in this frame's inter-present window (at most once per
        // logical ETW activity while it remains in flight). Long-running work adds busy time on
        // every overlapping frame without incrementing this again.
        uint64_t activityCount = 0;
        // Union of busy QPC attributed to this frame (overlapping work counted once).
        uint64_t busyQpc = 0;
        // Sum of attributed interval lengths (overlapping work counted separately).
        uint64_t summedBusyQpc = 0;
    };

    // Records that an ETW activity began. State is kept in-flight until stop.
    void OnActivityStart(Kind kind, uint32_t processId, const uint8_t activityId[16], uint64_t startQpc);
    // Records that an ETW activity ended. Contributes any busy span after the last present to the
    // open bucket for the next frame.
    void OnActivityStop(Kind kind, uint32_t processId, const uint8_t activityId[16], uint64_t stopQpc);
    // Closes the inter-present window for this PresentEvent. interPresentCloseQpc is the QPC
    // time on the ETW event that finalizes this present in PMTraceConsumer (same hook as
    // GpuTrace::CompleteFrame): DXGK present queue complete or MMIO flip, not DXGI Present_Start.
    void CompleteFrame(PresentEvent* pEvent, uint64_t interPresentCloseQpc);
    // Drops all per-process attribution state (e.g. when the trace session resets).
    void Reset() { processStateByPid_.clear(); }

private:
    // Identity that pairs ETW start/stop for one logical activity instance.
    struct ActivityKey
    {
        // Activity family (matches Kind channel and PresentEvent stats slot).
        Kind kind = Kind::D3D12PsoCompile;
        // Process that emitted the ETW events; must match the presenting frame's PID for credit.
        uint32_t processId = 0;
        // ETW ActivityId bytes; pairs start and stop for the same compile (etc.).
        uint8_t activityId[16]{};

        bool operator==(const ActivityKey& other) const noexcept;
    };

    // Hash for ActivityKey in the per-kind tracked-activity map.
    struct ActivityKeyHash
    {
        size_t operator()(const ActivityKey& key) const noexcept;
    };

    // One ETW activity from start until stop.
    struct TrackedActivity
    {
        uint64_t startQpc = 0;
        // Whether activityCount was already attributed for this ETW activity instance.
        bool activityCountAttributed = false;
    };

    // Per-kind accumulation for one process between frame completions.
    struct KindChannel
    {
        // Busy spans from activities that fully stopped since the last frame; clipped to the
        // inter-present window when the stop arrived.
        std::vector<InterPresentActivityUtil::QpcInterval> completedIntervals;
        // Starts counted toward activityCount for completed (stop-before-present) work in the
        // current open bucket.
        uint64_t completedStartCountInOpenBucket = 0;
        std::unordered_map<ActivityKey, TrackedActivity, ActivityKeyHash> tracked;
    };

    struct ProcessState
    {
        // interPresentCloseQpc from the previous CompleteFrame for this PID; start of the open
        // inter-present window for the next frame.
        uint64_t lastInterPresentCloseQpc = 0;
        // Separate accumulation per Kind so new activity families do not share buckets. Indexed
        // by Kind key integral value
        std::array<KindChannel, (size_t)Kind::Count> channels{};
    };

    // Builds a map/set key from ETW fields carried on start and stop.
    static ActivityKey MakeKey_(Kind kind, uint32_t processId, const uint8_t activityId[16]);
    // Per-PID frame boundary and kind channels (inserts default state if needed).
    ProcessState& GetProcessState_(uint32_t processId);

    // Attribution state keyed by process; created on first event or frame for that PID.
    std::unordered_map<uint32_t, ProcessState> processStateByPid_;
};
