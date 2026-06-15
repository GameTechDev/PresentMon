// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../PresentMonMiddleware/ProcessDataRate.h"
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
            Assert::AreEqual(2.0, pmon::mid::PsoCompileCountRate(2, windowQpc, qpcPeriod), 0.0001);
        }

        TEST_METHOD(TimeRateSumsCompileMillisecondsPerSecond)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t windowQpc = 10'000'000;
            Assert::AreEqual(50.0, pmon::mid::PsoCompileTimeRateMsPerSecond(50.0, windowQpc, qpcPeriod), 0.0001);
        }

        TEST_METHOD(ZeroWindowReturnsZeroRates)
        {
            Assert::AreEqual(0.0, pmon::mid::PsoCompileCountRate(5, 0, 1.0), 0.0001);
            Assert::AreEqual(0.0, pmon::mid::PsoCompileTimeRateMsPerSecond(25.0, 0, 1.0), 0.0001);
            Assert::AreEqual(0.0, pmon::mid::PsoCompileBusyPercent(100, 0), 0.0001);
        }

        TEST_METHOD(BusyPercentMergesNonOverlappingIntervals)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::mid::PsoCompileQpcInterval> intervals{
                { 0, 200 },
                { 400, 600 },
            };
            const uint64_t merged = pmon::mid::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(400ull, merged);
            Assert::AreEqual(40.0, pmon::mid::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentMergesOverlappingIntervals)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::mid::PsoCompileQpcInterval> intervals{
                { 100, 600 },
                { 400, 900 },
            };
            const uint64_t merged = pmon::mid::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(800ull, merged);
            Assert::AreEqual(80.0, pmon::mid::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentIdenticalOverlapCountsOnce)
        {
            const uint64_t windowQpc = 1000;
            std::vector<pmon::mid::PsoCompileQpcInterval> intervals{
                { 250, 750 },
                { 250, 750 },
            };
            const uint64_t merged = pmon::mid::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(500ull, merged);
            Assert::AreEqual(50.0, pmon::mid::PsoCompileBusyPercent(merged, windowQpc), 0.0001);
        }

        TEST_METHOD(BusyPercentUsesClippedIntervalLength)
        {
            std::vector<pmon::mid::PsoCompileQpcInterval> intervals{
                { 0, 500 },
            };
            const uint64_t merged = pmon::mid::MergePsoCompileBusyQpc(std::move(intervals));
            Assert::AreEqual(500ull, merged);
            Assert::AreEqual(50.0, pmon::mid::PsoCompileBusyPercent(merged, 1000), 0.0001);
        }

        TEST_METHOD(ClipToWindowIncludesPartialOverlap)
        {
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            const bool clipped = pmon::mid::PsoCompileClipToWindow(50, 250, 100, 200, clipStart, clipEnd);
            Assert::IsTrue(clipped);
            Assert::AreEqual(100ull, clipStart);
            Assert::AreEqual(200ull, clipEnd);
        }

        TEST_METHOD(ClipToWindowRejectsNonOverlap)
        {
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            Assert::IsFalse(pmon::mid::PsoCompileClipToWindow(0, 50, 100, 200, clipStart, clipEnd));
            Assert::IsFalse(pmon::mid::PsoCompileClipToWindow(250, 300, 100, 200, clipStart, clipEnd));
        }

        TEST_METHOD(QpcToDurationMsInvertsDurationMsToQpc)
        {
            const double qpcPeriod = 1.0 / 10'000'000.0;
            const uint64_t qpc = pmon::mid::PsoCompileDurationMsToQpc(50.0, qpcPeriod);
            Assert::AreEqual(50.0, pmon::mid::PsoCompileQpcToDurationMs(qpc, qpcPeriod), 0.0001);
        }
    };
}
