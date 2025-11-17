// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <CommonUtilities/mc/QpcCalculator.h>
#include <CommonUtilities/mc/MetricsTypes.h>
#include <CommonUtilities/mc/MetricsCalculator.h>
#include <CommonUtilities/mc/SwapChainCoreState.h>
#include <IntelPresentMon/PresentMonUtils/StreamFormat.h>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace pmon::util::metrics;

namespace MetricsCoreTests
{
    // ============================================================================
    // SECTION 1: Core Types & Foundation
    // ============================================================================

    TEST_CLASS(QpcCalculatorTests)
    {
    public:
        TEST_METHOD(TimestampDeltaToMilliSeconds_BasicConversion)
        {
            // 10MHz QPC frequency (10,000,000 ticks per second)
            QpcCalculator qpc(10'000'000, 0);
            
            // 10,000 ticks = 1 millisecond at 10MHz
            double result = qpc.TimestampDeltaToMilliSeconds(10'000);
            Assert::AreEqual(1.0, result, 0.0001);
        }

        TEST_METHOD(TimestampDeltaToMilliSeconds_ZeroDuration)
        {
            QpcCalculator qpc(10'000'000, 0);
            double result = qpc.TimestampDeltaToMilliSeconds(0);
            Assert::AreEqual(0.0, result);
        }

        TEST_METHOD(TimestampDeltaToMilliSeconds_LargeDuration)
        {
            QpcCalculator qpc(10'000'000, 0);
            
            // 100,000,000 ticks = 10,000 milliseconds at 10MHz
            double result = qpc.TimestampDeltaToMilliSeconds(100'000'000);
            Assert::AreEqual(10'000.0, result, 0.01);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_ForwardTime)
        {
            QpcCalculator qpc(10'000'000, 0);
            
            // Start at 1000, end at 11000 (10,000 ticks = 1ms)
            double result = qpc.TimestampDeltaToUnsignedMilliSeconds(1000, 11'000);
            Assert::AreEqual(1.0, result, 0.0001);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_ZeroDelta)
        {
            QpcCalculator qpc(10'000'000, 0);
            double result = qpc.TimestampDeltaToUnsignedMilliSeconds(5000, 5000);
            Assert::AreEqual(0.0, result);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_TypicalFrameTime)
        {
            // Typical QPC frequency: ~10MHz
            QpcCalculator qpc(10'000'000, 0);
            
            // 16.666ms frame time at 60fps
            uint64_t frameTimeTicks = 166'660;
            double result = qpc.TimestampDeltaToUnsignedMilliSeconds(0, frameTimeTicks);
            Assert::AreEqual(16.666, result, 0.001);
        }

        TEST_METHOD(GetStartTimestamp_ReturnsCorrectValue)
        {
            uint64_t startTime = 123'456'789;
            QpcCalculator qpc(10'000'000, startTime);
            
            Assert::AreEqual(startTime, qpc.GetStartTimestamp());
        }
    };

    TEST_CLASS(PresentSnapshotTests)
    {
    public:
        TEST_METHOD(FromCircularBuffer_CopiesBasicTimingFields)
        {
            // Create a mock NSM present event
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.PresentStartTime = 1000;
            nsmEvent.ReadyTime = 2000;
            nsmEvent.TimeInPresent = 500;
            nsmEvent.GPUStartTime = 1200;
            nsmEvent.GPUDuration = 800;
            nsmEvent.GPUVideoDuration = 300;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(1000ull, snap.presentStartTime);
            Assert::AreEqual(2000ull, snap.readyTime);
            Assert::AreEqual(500ull, snap.timeInPresent);
            Assert::AreEqual(1200ull, snap.gpuStartTime);
            Assert::AreEqual(800ull, snap.gpuDuration);
            Assert::AreEqual(300ull, snap.gpuVideoDuration);
        }

        TEST_METHOD(FromCircularBuffer_CopiesAppPropagatedData)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.AppPropagatedPresentStartTime = 5000;
            nsmEvent.AppPropagatedTimeInPresent = 600;
            nsmEvent.AppPropagatedGPUStartTime = 5200;
            nsmEvent.AppPropagatedReadyTime = 6000;
            nsmEvent.AppPropagatedGPUDuration = 800;
            nsmEvent.AppPropagatedGPUVideoDuration = 200;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(5000ull, snap.appPropagatedPresentStartTime);
            Assert::AreEqual(600ull, snap.appPropagatedTimeInPresent);
            Assert::AreEqual(5200ull, snap.appPropagatedGPUStartTime);
            Assert::AreEqual(6000ull, snap.appPropagatedReadyTime);
            Assert::AreEqual(800ull, snap.appPropagatedGPUDuration);
            Assert::AreEqual(200ull, snap.appPropagatedGPUVideoDuration);
        }

        TEST_METHOD(FromCircularBuffer_CopiesInstrumentedTimestamps)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.AppSimStartTime = 100;
            nsmEvent.AppSleepStartTime = 200;
            nsmEvent.AppSleepEndTime = 250;
            nsmEvent.AppRenderSubmitStartTime = 300;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(100ull, snap.appSimStartTime);
            Assert::AreEqual(200ull, snap.appSleepStartTime);
            Assert::AreEqual(250ull, snap.appSleepEndTime);
            Assert::AreEqual(300ull, snap.appRenderSubmitStartTime);
        }

        TEST_METHOD(FromCircularBuffer_CopiesPcLatencyData)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.PclSimStartTime = 7000;
            nsmEvent.PclInputPingTime = 6500;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(7000ull, snap.pclSimStartTime);
            Assert::AreEqual(6500ull, snap.pclInputPingTime);
        }

        TEST_METHOD(FromCircularBuffer_CopiesInputTimes)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.InputTime = 8000;
            nsmEvent.MouseClickTime = 8050;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(8000ull, snap.inputTime);
            Assert::AreEqual(8050ull, snap.mouseClickTime);
        }

        TEST_METHOD(FromCircularBuffer_NormalizesDisplayArrays)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.DisplayedCount = 2;
            nsmEvent.Displayed_FrameType[0] = FrameType::Application;
            nsmEvent.Displayed_ScreenTime[0] = 9000;
            nsmEvent.Displayed_FrameType[1] = FrameType::Repeated;
            nsmEvent.Displayed_ScreenTime[1] = 9500;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(size_t(2), snap.displayed.size());
            Assert::IsTrue(snap.displayed[0].frameType == FrameType::Application);
            Assert::AreEqual(9000ull, snap.displayed[0].screenTime);
            Assert::IsTrue(snap.displayed[1].frameType == FrameType::Repeated);
            Assert::AreEqual(9500ull, snap.displayed[1].screenTime);
        }

        TEST_METHOD(FromCircularBuffer_HandlesEmptyDisplayArray)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.DisplayedCount = 0;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(size_t(0), snap.displayed.size());
        }

        TEST_METHOD(FromCircularBuffer_CopiesMetadata)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.ProcessId = 1234;
            nsmEvent.ThreadId = 5678;
            nsmEvent.SwapChainAddress = 0xDEADBEEF;
            nsmEvent.FrameId = 42;

            auto snap = PresentSnapshot::FromCircularBuffer(nsmEvent);

            Assert::AreEqual(uint32_t(1234), snap.processId);
            Assert::AreEqual(uint32_t(5678), snap.threadId);
            Assert::AreEqual(uint64_t(0xDEADBEEF), snap.swapChainAddress);
            Assert::AreEqual(uint32_t(42), snap.frameId);
        }
    };

    // ConsoleAdapter tests are skipped in unit tests because they require PresentData
    // which has ETW dependencies. These will be tested during Console integration.
    /*
    TEST_CLASS(ConsoleAdapterTests)
    {
    public:
        TEST_METHOD(Constructor_AcceptsSharedPtr)
        {
            auto event = std::make_shared<PresentEvent>();
            event->PresentStartTime = 1234;

            ConsoleAdapter adapter(event);

            Assert::AreEqual(1234ull, adapter.getPresentStartTime());
        }

        TEST_METHOD(Constructor_AcceptsRawPointer)
        {
            PresentEvent event{};
            event.ReadyTime = 5678;

            ConsoleAdapter adapter(&event);

            Assert::AreEqual(5678ull, adapter.getReadyTime());
        }

        TEST_METHOD(GettersProvideAccessToAllTimingFields)
        {
            auto event = std::make_shared<PresentEvent>();
            event->PresentStartTime = 100;
            event->ReadyTime = 200;
            event->TimeInPresent = 50;
            event->GPUStartTime = 150;
            event->GPUDuration = 75;
            event->GPUVideoDuration = 25;

            ConsoleAdapter adapter(event);

            Assert::AreEqual(100ull, adapter.getPresentStartTime());
            Assert::AreEqual(200ull, adapter.getReadyTime());
            Assert::AreEqual(50ull, adapter.getTimeInPresent());
            Assert::AreEqual(150ull, adapter.getGPUStartTime());
            Assert::AreEqual(75ull, adapter.getGPUDuration());
            Assert::AreEqual(25ull, adapter.getGPUVideoDuration());
        }

        TEST_METHOD(GettersProvideAccessToAppPropagatedData)
        {
            auto event = std::make_shared<PresentEvent>();
            event->AppPropagatedPresentStartTime = 300;
            event->AppPropagatedGPUDuration = 100;

            ConsoleAdapter adapter(event);

            Assert::AreEqual(300ull, adapter.getAppPropagatedPresentStartTime());
            Assert::AreEqual(100ull, adapter.getAppPropagatedGPUDuration());
        }

        TEST_METHOD(GettersProvideAccessToPcLatencyData)
        {
            auto event = std::make_shared<PresentEvent>();
            event->PclSimStartTime = 400;
            event->PclInputPingTime = 350;

            ConsoleAdapter adapter(event);

            Assert::AreEqual(400ull, adapter.getPclSimStartTime());
            Assert::AreEqual(350ull, adapter.getPclInputPingTime());
        }

        TEST_METHOD(GetDisplayedCount_ReturnsVectorSize)
        {
            auto event = std::make_shared<PresentEvent>();
            event->Displayed.push_back({FrameType::Application, 100});
            event->Displayed.push_back({FrameType::Repeated, 200});

            ConsoleAdapter adapter(event);

            Assert::AreEqual(size_t(2), adapter.getDisplayedCount());
        }

        TEST_METHOD(GetDisplayed_ProvidesIndexedAccess)
        {
            auto event = std::make_shared<PresentEvent>();
            event->Displayed.push_back({FrameType::Application, 1000});
            event->Displayed.push_back({FrameType::Repeated, 2000});

            ConsoleAdapter adapter(event);

            Assert::IsTrue(adapter.getDisplayedFrameType(0) == FrameType::Application);
            Assert::AreEqual(1000ull, adapter.getDisplayedScreenTime(0));
            Assert::IsTrue(adapter.getDisplayedFrameType(1) == FrameType::Repeated);
            Assert::AreEqual(2000ull, adapter.getDisplayedScreenTime(1));
        }

        TEST_METHOD(HasAppPropagatedData_ReturnsTrueWhenPresent)
        {
            auto event = std::make_shared<PresentEvent>();
            event->AppPropagatedPresentStartTime = 123;

            ConsoleAdapter adapter(event);

            Assert::IsTrue(adapter.hasAppPropagatedData());
        }

        TEST_METHOD(HasAppPropagatedData_ReturnsFalseWhenZero)
        {
            auto event = std::make_shared<PresentEvent>();
            event->AppPropagatedPresentStartTime = 0;

            ConsoleAdapter adapter(event);

            Assert::IsFalse(adapter.hasAppPropagatedData());
        }

        TEST_METHOD(HasPclSimStartTime_ReturnsTrueWhenPresent)
        {
            auto event = std::make_shared<PresentEvent>();
            event->PclSimStartTime = 456;

            ConsoleAdapter adapter(event);

            Assert::IsTrue(adapter.hasPclSimStartTime());
        }

        TEST_METHOD(HasPclInputPingTime_ReturnsTrueWhenPresent)
        {
            auto event = std::make_shared<PresentEvent>();
            event->PclInputPingTime = 789;

            ConsoleAdapter adapter(event);

            Assert::IsTrue(adapter.hasPclInputPingTime());
        }
    };
    */

    // ============================================================================
    // SECTION 2: SwapChainCoreState
    // ============================================================================

    TEST_CLASS(SwapChainCoreStateTests)
    {
    public:
        // Simple mock type for testing - just needs to be storable
        struct MockPresent {
            uint64_t presentStartTime = 0;
        };

        TEST_METHOD(DefaultConstruction_InitializesTimestampsToZero)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            Assert::AreEqual(0ull, state.lastSimStartTime);
            Assert::AreEqual(0ull, state.lastDisplayedSimStartTime);
            Assert::AreEqual(0ull, state.lastDisplayedScreenTime);
            Assert::AreEqual(0ull, state.firstAppSimStartTime);
        }

        TEST_METHOD(DefaultConstruction_InitializesOptionalPresentToEmpty)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            Assert::IsFalse(state.lastPresent.has_value());
            Assert::IsFalse(state.lastAppPresent.has_value());
        }

        TEST_METHOD(PendingPresents_CanStoreMultipleSharedPtrs)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            auto p1 = std::make_shared<MockPresent>();
            auto p2 = std::make_shared<MockPresent>();
            auto p3 = std::make_shared<MockPresent>();

            state.pendingPresents.push_back(p1);
            state.pendingPresents.push_back(p2);
            state.pendingPresents.push_back(p3);

            Assert::AreEqual(size_t(3), state.pendingPresents.size());
        }

        TEST_METHOD(LastPresent_CanBeAssigned)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;
            auto event = std::make_shared<MockPresent>();
            event->presentStartTime = 12345;

            state.lastPresent = event;

            Assert::IsTrue(state.lastPresent.has_value());
            Assert::AreEqual(12345ull, (*state.lastPresent)->presentStartTime);
        }

        TEST_METHOD(DroppedInputTracking_InitializesToZero)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            Assert::AreEqual(0ull, state.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(0ull, state.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(0ull, state.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(DroppedInputTracking_CanBeUpdated)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            state.lastReceivedNotDisplayedAllInputTime = 1000;
            state.lastReceivedNotDisplayedMouseClickTime = 2000;
            state.lastReceivedNotDisplayedAppProviderInputTime = 3000;

            Assert::AreEqual(1000ull, state.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(2000ull, state.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(3000ull, state.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(PcLatencyAccumulation_InitializesToZero)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime);
        }

        TEST_METHOD(PcLatencyAccumulation_CanAccumulateDroppedFrames)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            // Simulate accumulating 3 dropped frames at 16.666ms each
            state.accumulatedInput2FrameStartTime += 16.666;
            state.accumulatedInput2FrameStartTime += 16.666;
            state.accumulatedInput2FrameStartTime += 16.666;

            Assert::AreEqual(49.998, state.accumulatedInput2FrameStartTime, 0.001);
        }

        TEST_METHOD(AnimationErrorSource_DefaultsToCpuStart)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::CpuStart);
        }

        TEST_METHOD(AnimationErrorSource_CanBeChanged)
        {
            SwapChainCoreState<std::shared_ptr<MockPresent>> state;

            state.animationErrorSource = AnimationErrorSource::PCLatency;
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::PCLatency);

            state.animationErrorSource = AnimationErrorSource::AppProvider;
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider);
        }

        TEST_METHOD(WorksWithPresentSnapshotType)
        {
            // Verify template instantiation works with PresentSnapshot too
            SwapChainCoreState<PresentSnapshot> state;

            PresentSnapshot snap{};
            snap.presentStartTime = 5000;

            state.pendingPresents.push_back(snap);
            state.lastPresent = snap;

            Assert::AreEqual(size_t(1), state.pendingPresents.size());
            Assert::IsTrue(state.lastPresent.has_value());
            Assert::AreEqual(5000ull, state.lastPresent->presentStartTime);
        }
    };
}
