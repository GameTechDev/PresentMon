// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"

#include "../PresentData/InterPresentActivity.hpp"
#include "../PresentData/PresentMonTraceConsumer.hpp"

#include <cstring>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace InterPresentActivityTests
{
    namespace
    {
        void SetActivityId_(uint8_t activityId[16], uint8_t byte)
        {
            std::memset(activityId, 0, 16);
            activityId[0] = byte;
        }

        const InterPresentActivity::FrameStats& PsoStats_(const PresentEvent& frame)
        {
            return frame.InterPresentStats[(size_t)InterPresentActivity::Kind::D3D12PsoCompile];
        }
    }

    TEST_CLASS(InterPresentActivityTests)
    {
    public:
        TEST_METHOD(MergeBusyQpcOverlappingIntervals)
        {
            std::vector<InterPresentActivityUtil::QpcInterval> intervals{
                { 100, 200 },
                { 150, 250 },
            };
            const uint64_t merged = InterPresentActivityUtil::MergeBusyQpc(std::move(intervals));
            Assert::AreEqual((uint64_t)150, merged);
        }

        TEST_METHOD(MergeBusyQpcDisjointIntervals)
        {
            std::vector<InterPresentActivityUtil::QpcInterval> intervals{
                { 0, 100 },
                { 200, 250 },
            };
            const uint64_t merged = InterPresentActivityUtil::MergeBusyQpc(std::move(intervals));
            Assert::AreEqual((uint64_t)150, merged);
        }

        TEST_METHOD(ClipIntervalToWindow)
        {
            uint64_t clipStart = 0;
            uint64_t clipEnd = 0;
            const bool clipped = InterPresentActivityUtil::ClipIntervalToWindow(
                50, 250, 100, 200, clipStart, clipEnd);
            Assert::IsTrue(clipped);
            Assert::AreEqual((uint64_t)100, clipStart);
            Assert::AreEqual((uint64_t)200, clipEnd);
        }

        TEST_METHOD(BeforeFrameInFlightCreditsFullFramePeriod)
        {
            InterPresentActivity trace;
            PresentEvent anchor{};
            anchor.ProcessId = 42;

            trace.CompleteFrame(&anchor, 5000);

            uint8_t activityId[16]{};
            SetActivityId_(activityId, 1);
            trace.OnActivityStart(
                InterPresentActivity::Kind::D3D12PsoCompile, 42, activityId, 1000);

            PresentEvent frame{};
            frame.ProcessId = 42;
            trace.CompleteFrame(&frame, 8000);

            Assert::AreEqual((uint64_t)0, PsoStats_(frame).activityCount);
            Assert::AreEqual((uint64_t)3000, PsoStats_(frame).busyQpc);
            Assert::AreEqual((uint64_t)3000, PsoStats_(frame).summedBusyQpc);
            Assert::AreEqual((uint64_t)3000, frame.InterPresentFramePeriodQpc);

            trace.OnActivityStop(
                InterPresentActivity::Kind::D3D12PsoCompile, 42, activityId, 9000);
        }

        TEST_METHOD(InWindowInFlightCreditsPartialOverlap)
        {
            InterPresentActivity trace;
            PresentEvent anchor{};
            anchor.ProcessId = 7;
            trace.CompleteFrame(&anchor, 1000);

            uint8_t activityId[16]{};
            SetActivityId_(activityId, 2);
            trace.OnActivityStart(
                InterPresentActivity::Kind::D3D12PsoCompile, 7, activityId, 1500);

            PresentEvent frame{};
            frame.ProcessId = 7;
            trace.CompleteFrame(&frame, 2000);

            Assert::AreEqual((uint64_t)1, PsoStats_(frame).activityCount);
            Assert::AreEqual((uint64_t)500, PsoStats_(frame).busyQpc);
        }

        TEST_METHOD(CompletedCompileClipsToFrameWindow)
        {
            InterPresentActivity trace;
            PresentEvent anchor{};
            anchor.ProcessId = 3;
            trace.CompleteFrame(&anchor, 1000);

            uint8_t activityId[16]{};
            SetActivityId_(activityId, 3);
            trace.OnActivityStart(
                InterPresentActivity::Kind::D3D12PsoCompile, 3, activityId, 1100);
            trace.OnActivityStop(
                InterPresentActivity::Kind::D3D12PsoCompile, 3, activityId, 1900);

            PresentEvent frame{};
            frame.ProcessId = 3;
            trace.CompleteFrame(&frame, 2000);

            Assert::AreEqual((uint64_t)1, PsoStats_(frame).activityCount);
            Assert::AreEqual((uint64_t)800, PsoStats_(frame).busyQpc);
            Assert::AreEqual((uint64_t)800, PsoStats_(frame).summedBusyQpc);
        }

        TEST_METHOD(ConcurrentInFlightSummedBusyExceedsMerged)
        {
            InterPresentActivity trace;
            PresentEvent anchor{};
            anchor.ProcessId = 9;
            trace.CompleteFrame(&anchor, 1000);

            for (uint8_t i = 0; i < 10; ++i) {
                uint8_t activityId[16]{};
                SetActivityId_(activityId, i);
                trace.OnActivityStart(
                    InterPresentActivity::Kind::D3D12PsoCompile, 9, activityId, 1100);
            }

            PresentEvent frame{};
            frame.ProcessId = 9;
            trace.CompleteFrame(&frame, 2000);

            Assert::AreEqual((uint64_t)10, PsoStats_(frame).activityCount);
            Assert::AreEqual((uint64_t)900, PsoStats_(frame).busyQpc);
            Assert::AreEqual((uint64_t)9000, PsoStats_(frame).summedBusyQpc);
        }

        TEST_METHOD(MultiFrameInFlightCreditsBusyEachFrame)
        {
            InterPresentActivity trace;
            PresentEvent anchor{};
            anchor.ProcessId = 42;
            trace.CompleteFrame(&anchor, 5000);

            uint8_t activityId[16]{};
            SetActivityId_(activityId, 4);
            trace.OnActivityStart(
                InterPresentActivity::Kind::D3D12PsoCompile, 42, activityId, 1000);

            PresentEvent frame1{};
            frame1.ProcessId = 42;
            trace.CompleteFrame(&frame1, 8000);
            Assert::AreEqual((uint64_t)0, PsoStats_(frame1).activityCount);
            Assert::AreEqual((uint64_t)3000, PsoStats_(frame1).busyQpc);

            PresentEvent frame2{};
            frame2.ProcessId = 42;
            trace.CompleteFrame(&frame2, 10000);
            Assert::AreEqual((uint64_t)0, PsoStats_(frame2).activityCount);
            Assert::AreEqual((uint64_t)2000, PsoStats_(frame2).busyQpc);

            trace.OnActivityStop(
                InterPresentActivity::Kind::D3D12PsoCompile, 42, activityId, 10500);
        }
    };
}
