// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace pmon::util::metrics
{
    struct PsoCompileQpcInterval
    {
        uint64_t startQpc = 0;
        uint64_t endQpc = 0;
    };

    struct PsoCompileCompletedSample
    {
        double durationMs = 0.;
        uint64_t eventCompleteQpc = 0;
    };

    struct PsoCompileSliceAggregate
    {
        double compileCountHz = 0.;
        double compileTimeMsPerSecond = 0.;
        double compileBusyPercent = 0.;
    };

    inline double PsoCompileWindowSeconds(uint64_t windowQpc, double qpcPeriodSeconds)
    {
        if (windowQpc == 0 || qpcPeriodSeconds <= 0.) {
            return 0.;
        }
        return double(windowQpc) * qpcPeriodSeconds;
    }

    inline uint64_t PsoCompileDurationMsToQpc(double durationMs, double qpcPeriodSeconds)
    {
        if (durationMs <= 0. || qpcPeriodSeconds <= 0.) {
            return 0;
        }
        return (uint64_t)(durationMs / (qpcPeriodSeconds * 1000.0));
    }

    inline double PsoCompileQpcToDurationMs(uint64_t durationQpc, double qpcPeriodSeconds)
    {
        if (durationQpc == 0 || qpcPeriodSeconds <= 0.) {
            return 0.;
        }
        return double(durationQpc) * qpcPeriodSeconds * 1000.0;
    }

    inline bool PsoCompileClipToWindow(uint64_t startQpc, uint64_t endQpc, uint64_t windowOldest, uint64_t windowNewest,
        uint64_t& clipStart, uint64_t& clipEnd)
    {
        if (endQpc <= windowOldest || startQpc >= windowNewest) {
            return false;
        }
        clipStart = std::max(startQpc, windowOldest);
        clipEnd = std::min(endQpc, windowNewest);
        return clipEnd > clipStart;
    }

    inline double PsoCompileCountRate(uint64_t compileCount, uint64_t windowQpc, double qpcPeriodSeconds)
    {
        const double windowSeconds = PsoCompileWindowSeconds(windowQpc, qpcPeriodSeconds);
        if (windowSeconds <= 0.) {
            return 0.;
        }
        return double(compileCount) / windowSeconds;
    }

    inline double PsoCompileTimeRateMsPerSecond(double compileDurationMsSum, uint64_t windowQpc, double qpcPeriodSeconds)
    {
        const double windowSeconds = PsoCompileWindowSeconds(windowQpc, qpcPeriodSeconds);
        if (windowSeconds <= 0.) {
            return 0.;
        }
        return compileDurationMsSum / windowSeconds;
    }

    inline uint64_t MergePsoCompileBusyQpc(std::vector<PsoCompileQpcInterval> intervals)
    {
        if (intervals.empty()) {
            return 0;
        }
        std::sort(intervals.begin(), intervals.end(), [](const PsoCompileQpcInterval& a, const PsoCompileQpcInterval& b) {
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

    inline double PsoCompileBusyPercent(uint64_t mergedBusyQpc, uint64_t windowQpc)
    {
        if (windowQpc == 0) {
            return 0.;
        }
        return 100. * double(mergedBusyQpc) / double(windowQpc);
    }

    inline PsoCompileSliceAggregate AggregatePsoCompileSlice(
        std::span<const PsoCompileCompletedSample> samples,
        uint64_t sliceStartQpc,
        uint64_t sliceEndQpc,
        double qpcPeriodSeconds)
    {
        PsoCompileSliceAggregate result{};
        if (sliceEndQpc <= sliceStartQpc) {
            return result;
        }
        const uint64_t windowQpc = sliceEndQpc - sliceStartQpc;
        uint64_t compileCount = 0;
        double compileDurationMsSum = 0.;
        std::vector<PsoCompileQpcInterval> busyIntervals;
        busyIntervals.reserve(samples.size());

        for (const auto& sample : samples) {
            const uint64_t endQpc = sample.eventCompleteQpc;
            const uint64_t durationQpc = PsoCompileDurationMsToQpc(sample.durationMs, qpcPeriodSeconds);
            const uint64_t startQpc = endQpc >= durationQpc ? endQpc - durationQpc : 0;
            if (startQpc >= sliceStartQpc && startQpc <= sliceEndQpc) {
                ++compileCount;
            }
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            if (PsoCompileClipToWindow(startQpc, endQpc, sliceStartQpc, sliceEndQpc, clipStart, clipEnd)) {
                compileDurationMsSum += PsoCompileQpcToDurationMs(clipEnd - clipStart, qpcPeriodSeconds);
                busyIntervals.push_back(PsoCompileQpcInterval{ clipStart, clipEnd });
            }
        }

        const uint64_t mergedBusyQpc = MergePsoCompileBusyQpc(std::move(busyIntervals));
        result.compileCountHz = PsoCompileCountRate(compileCount, windowQpc, qpcPeriodSeconds);
        result.compileTimeMsPerSecond = PsoCompileTimeRateMsPerSecond(compileDurationMsSum, windowQpc, qpcPeriodSeconds);
        result.compileBusyPercent = PsoCompileBusyPercent(mergedBusyQpc, windowQpc);
        return result;
    }
}
