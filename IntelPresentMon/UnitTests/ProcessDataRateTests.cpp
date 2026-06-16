// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/metrics/ProcessDataRate.h"
#include "CppUnitTest.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ProcessDataRateTests
{
    TEST_CLASS(ProcessDataRateTests)
    {
    public:
        TEST_METHOD(CountRateUsesWindowDuration)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t windowQpc = 10'000'000;
            Assert::AreEqual(2.0, pmon::util::metrics::PsoCompileCountRate(2, windowQpc, qpcPeriod), 0.0001);
        }

        TEST_METHOD(TimeRateSumsCompileMillisecondsPerSecond)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t windowQpc = 10'000'000;
            Assert::AreEqual(50.0, pmon::util::metrics::PsoCompileTimeRateMsPerSecond(50.0, windowQpc, qpcPeriod), 0.0001);
        }

        TEST_METHOD(ZeroWindowReturnsZeroRates)
        {
            Assert::AreEqual(0.0, pmon::util::metrics::PsoCompileCountRate(5, 0, 1.0), 0.0001);
            Assert::AreEqual(0.0, pmon::util::metrics::PsoCompileTimeRateMsPerSecond(25.0, 0, 1.0), 0.0001);
            Assert::AreEqual(0.0, pmon::util::metrics::PsoCompileBusyPercent(100, 0), 0.0001);
        }

        TEST_METHOD(BusyPercentMergesNonOverlappingIntervals)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::util::metrics::PsoCompileQpcInterval> intervals{
                { 0, 200 },
                { 400, 600 },
            };
            const uint64_t merged = pmon::util::metrics::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(400ull, merged);
            Assert::AreEqual(40.0, pmon::util::metrics::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentMergesOverlappingIntervals)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::util::metrics::PsoCompileQpcInterval> intervals{
                { 100, 600 },
                { 400, 900 },
            };
            const uint64_t merged = pmon::util::metrics::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(800ull, merged);
            Assert::AreEqual(80.0, pmon::util::metrics::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentIdenticalOverlapCountsOnce)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::util::metrics::PsoCompileQpcInterval> intervals{
                { 250, 750 },
                { 250, 750 },
            };
            const uint64_t merged = pmon::util::metrics::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(500ull, merged);
            Assert::AreEqual(50.0, pmon::util::metrics::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentUsesClippedIntervalLength)
        {
            std::vector<pmon::util::metrics::PsoCompileQpcInterval> intervals{
                { 0, 500 },
            };
            const uint64_t merged = pmon::util::metrics::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(500ull, merged);
            Assert::AreEqual(50.0, pmon::util::metrics::PsoCompileBusyPercent(merged, 1000), 0.0001);
        }

        TEST_METHOD(ClipToWindowIncludesPartialOverlap)
        {
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            const bool clipped = pmon::util::metrics::PsoCompileClipToWindow(50, 250, 100, 200, clipStart, clipEnd);
            Assert::IsTrue(clipped);
            Assert::AreEqual(100ull, clipStart);
            Assert::AreEqual(200ull, clipEnd);
        }

        TEST_METHOD(ClipToWindowRejectsNonOverlap)
        {
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            Assert::IsFalse(pmon::util::metrics::PsoCompileClipToWindow(0, 50, 100, 200, clipStart, clipEnd));
            Assert::IsFalse(pmon::util::metrics::PsoCompileClipToWindow(250, 300, 100, 200, clipStart, clipEnd));
        }

        TEST_METHOD(QpcToDurationMsInvertsDurationMsToQpc)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t qpc = pmon::util::metrics::PsoCompileDurationMsToQpc(50.0, qpcPeriod);
            Assert::AreEqual(50.0, pmon::util::metrics::PsoCompileQpcToDurationMs(qpc, qpcPeriod), 0.0001);
        }

        TEST_METHOD(AggregateSliceEmptyReturnsZeros)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const std::vector<pmon::util::metrics::PsoCompileCompletedSample> samples{};
            const auto agg = pmon::util::metrics::AggregatePsoCompileSlice(samples, 0, 10'000'000, qpcPeriod);
            Assert::AreEqual(0.0, agg.compileCountHz, 0.0001);
            Assert::AreEqual(0.0, agg.compileTimeMsPerSecond, 0.0001);
            Assert::AreEqual(0.0, agg.compileBusyPercent, 0.0001);
        }

        TEST_METHOD(AggregateSliceMatchesSingleCompileInWindow)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t sliceStart = 0;
            const uint64_t sliceEnd = 10'000'000;
            const uint64_t durationQpc = 1'000'000;
            const uint64_t endQpc = sliceStart + 5'000'000;
            const double durationMs = pmon::util::metrics::PsoCompileQpcToDurationMs(durationQpc, qpcPeriod);
            const std::vector<pmon::util::metrics::PsoCompileCompletedSample> samples{
                { durationMs, endQpc },
            };
            const auto agg = pmon::util::metrics::AggregatePsoCompileSlice(samples, sliceStart, sliceEnd, qpcPeriod);
            Assert::AreEqual(1.0, agg.compileCountHz, 0.0001);
            Assert::AreEqual(durationMs, agg.compileTimeMsPerSecond, 0.1);
            Assert::AreEqual(10.0, agg.compileBusyPercent, 0.1);
        }
    };
}
