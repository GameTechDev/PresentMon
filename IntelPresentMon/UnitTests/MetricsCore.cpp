// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <CommonUtilities/qpc.h>
#include <CommonUtilities/mc/MetricsTypes.h>
#include <CommonUtilities/mc/MetricsCalculator.h>
#include <CommonUtilities/mc/SwapChainState.h>
#include <IntelPresentMon/PresentMonUtils/StreamFormat.h>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace pmon::util::metrics;
using namespace pmon::util;

namespace MetricsCoreTests
{
    // ============================================================================
    // SECTION 1: Core Types & Foundation
    // ============================================================================

    TEST_CLASS(QpcConverterTests)
    {
    public:
        TEST_METHOD(TimestampDeltaToMilliSeconds_BasicConversion)
        {
            // 10MHz QPC frequency (10,000,000 ticks per second)
            QpcConverter qpc(10'000'000, 0);

            // 10,000 ticks = 1 millisecond at 10MHz
            double result = qpc.DurationMilliSeconds(10'000);
            Assert::AreEqual(1.0, result, 0.0001);
        }

        TEST_METHOD(TimestampDeltaToMilliSeconds_ZeroDuration)
        {
            QpcConverter qpc(10'000'000, 0);
            double result = qpc.DurationMilliSeconds(0);
            Assert::AreEqual(0.0, result);
        }

        TEST_METHOD(TimestampDeltaToMilliSeconds_LargeDuration)
        {
            QpcConverter qpc(10'000'000, 0);

            // 100,000,000 ticks = 10,000 milliseconds at 10MHz
            double result = qpc.DurationMilliSeconds(100'000'000);
            Assert::AreEqual(10'000.0, result, 0.01);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_ForwardTime)
        {
            QpcConverter qpc(10'000'000, 0);

            // Start at 1000, end at 11000 (10,000 ticks = 1ms)
            double result = qpc.DeltaUnsignedMilliSeconds(1000, 11'000);
            Assert::AreEqual(1.0, result, 0.0001);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_ZeroDelta)
        {
            QpcConverter qpc(10'000'000, 0);
            double result = qpc.DeltaUnsignedMilliSeconds(5000, 5000);
            Assert::AreEqual(0.0, result);
        }

        TEST_METHOD(TimestampDeltaToUnsignedMilliSeconds_TypicalFrameTime)
        {
            // Typical QPC frequency: ~10MHz
            QpcConverter qpc(10'000'000, 0);

            // 16.666ms frame time at 60fps
            uint64_t frameTimeTicks = 166'660;
            double result = qpc.DurationMilliSeconds(frameTimeTicks);
            Assert::AreEqual(16.666, result, 0.001);
        }

        TEST_METHOD(GetStartTimestamp_ReturnsCorrectValue)
        {
            uint64_t startTime = 123'456'789;
            QpcConverter qpc(10'000'000, startTime);

            Assert::AreEqual(startTime, qpc.GetSessionStartTimestamp());
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

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(1000ull, frame.presentStartTime);
            Assert::AreEqual(2000ull, frame.readyTime);
            Assert::AreEqual(500ull, frame.timeInPresent);
            Assert::AreEqual(1200ull, frame.gpuStartTime);
            Assert::AreEqual(800ull, frame.gpuDuration);
            Assert::AreEqual(300ull, frame.gpuVideoDuration);
        }

        TEST_METHOD(FromCircularBuffer_CopiesAppPropagatedData)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.AppPropagatedPresentStartTime =5000;
            nsmEvent.AppPropagatedTimeInPresent =600;
            nsmEvent.AppPropagatedGPUStartTime =5200;
            nsmEvent.AppPropagatedReadyTime =6000;
            nsmEvent.AppPropagatedGPUDuration =800;
            nsmEvent.AppPropagatedGPUVideoDuration =200;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(5000ull, frame.appPropagatedPresentStartTime);
            Assert::AreEqual(600ull, frame.appPropagatedTimeInPresent);
            Assert::AreEqual(5200ull, frame.appPropagatedGPUStartTime);
            Assert::AreEqual(6000ull, frame.appPropagatedReadyTime);
            Assert::AreEqual(800ull, frame.appPropagatedGPUDuration);
            Assert::AreEqual(200ull, frame.appPropagatedGPUVideoDuration);
        }

        TEST_METHOD(FromCircularBuffer_CopiesInstrumentedTimestamps)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.AppSimStartTime =100;
            nsmEvent.AppSleepStartTime =200;
            nsmEvent.AppSleepEndTime =250;
            nsmEvent.AppRenderSubmitStartTime =300;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(100ull, frame.appSimStartTime);
            Assert::AreEqual(200ull, frame.appSleepStartTime);
            Assert::AreEqual(250ull, frame.appSleepEndTime);
            Assert::AreEqual(300ull, frame.appRenderSubmitStartTime);
        }

        TEST_METHOD(FromCircularBuffer_CopiesPcLatencyData)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.PclSimStartTime =7000;
            nsmEvent.PclInputPingTime =6500;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(7000ull, frame.pclSimStartTime);
            Assert::AreEqual(6500ull, frame.pclInputPingTime);
        }

        TEST_METHOD(FromCircularBuffer_CopiesInputTimes)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.InputTime =8000;
            nsmEvent.MouseClickTime =8050;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(8000ull, frame.inputTime);
            Assert::AreEqual(8050ull, frame.mouseClickTime);
        }

        TEST_METHOD(FromCircularBuffer_NormalizesDisplayArrays)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.DisplayedCount =2;
            nsmEvent.Displayed_FrameType[0] = FrameType::Application;
            nsmEvent.Displayed_ScreenTime[0] =9000;
            nsmEvent.Displayed_FrameType[1] = FrameType::Repeated;
            nsmEvent.Displayed_ScreenTime[1] =9500;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(size_t(2), frame.displayed.size());
            Assert::IsTrue(frame.displayed[0].first == FrameType::Application);
            Assert::AreEqual(9000ull, frame.displayed[0].second);
            Assert::IsTrue(frame.displayed[1].first == FrameType::Repeated);
            Assert::AreEqual(9500ull, frame.displayed[1].second);
        }

        TEST_METHOD(FromCircularBuffer_HandlesEmptyDisplayArray)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.DisplayedCount = 0;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(size_t(0), frame.displayed.size());
        }

        TEST_METHOD(FromCircularBuffer_CopiesMetadata)
        {
            PmNsmPresentEvent nsmEvent{};
            nsmEvent.ProcessId =1234;
            nsmEvent.ThreadId =5678;
            nsmEvent.SwapChainAddress =0xDEADBEEF;
            nsmEvent.FrameId =42;

            auto frame = FrameData::CopyFrameData(nsmEvent);

            Assert::AreEqual(uint32_t(1234), frame.processId);
            Assert::AreEqual(uint32_t(5678), frame.threadId);
            Assert::AreEqual(uint64_t(0xDEADBEEF), frame.swapChainAddress);
            Assert::AreEqual(uint32_t(42), frame.frameId);
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
            SwapChainCoreState swapChain;

            Assert::AreEqual(0ull, swapChain.lastSimStartTime);
            Assert::AreEqual(0ull, swapChain.lastDisplayedSimStartTime);
            Assert::AreEqual(0ull, swapChain.lastDisplayedScreenTime);
            Assert::AreEqual(0ull, swapChain.firstAppSimStartTime);
        }

        TEST_METHOD(DefaultConstruction_InitializesOptionalPresentToEmpty)
        {
            SwapChainCoreState swapChain;

            Assert::IsFalse(swapChain.lastPresent.has_value());
            Assert::IsFalse(swapChain.lastAppPresent.has_value());
        }

        TEST_METHOD(PendingPresents_CanStoreMultiplePendingPresents)
        {
            SwapChainCoreState swapChain;

            FrameData p1{};
            FrameData p2{};
            FrameData p3{};

            swapChain.pendingPresents.push_back(p1);
            swapChain.pendingPresents.push_back(p2);
            swapChain.pendingPresents.push_back(p3);

            Assert::AreEqual(size_t(3), swapChain.pendingPresents.size());
        }

        TEST_METHOD(LastPresent_CanBeAssigned)
        {
            SwapChainCoreState swapChain;
            FrameData p1{};
            p1.presentStartTime = 12345;
            swapChain.lastPresent = p1;

            Assert::IsTrue(swapChain.lastPresent.has_value());
            Assert::AreEqual(12345ull, swapChain.lastPresent.value().presentStartTime);
        }

        TEST_METHOD(DroppedInputTracking_InitializesToZero)
        {
            SwapChainCoreState swapChain;

            Assert::AreEqual(0ull, swapChain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(0ull, swapChain.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(0ull, swapChain.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(DroppedInputTracking_CanBeUpdated)
        {
            SwapChainCoreState swapChain;

            swapChain.lastReceivedNotDisplayedAllInputTime = 1000;
            swapChain.lastReceivedNotDisplayedMouseClickTime = 2000;
            swapChain.lastReceivedNotDisplayedAppProviderInputTime = 3000;

            Assert::AreEqual(1000ull, swapChain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(2000ull, swapChain.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(3000ull, swapChain.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(PcLatencyAccumulation_InitializesToZero)
        {
            SwapChainCoreState swapChain;

            Assert::AreEqual(0.0, swapChain.accumulatedInput2FrameStartTime);
        }

        TEST_METHOD(PcLatencyAccumulation_CanAccumulateTime)
        {
            SwapChainCoreState swapChain;

            // Simulate accumulating 3 dropped frames at 16.666ms each
            swapChain.accumulatedInput2FrameStartTime += 16.666;
            swapChain.accumulatedInput2FrameStartTime += 16.666;
            swapChain.accumulatedInput2FrameStartTime += 16.666;

            Assert::AreEqual(49.998, swapChain.accumulatedInput2FrameStartTime, 0.001);
        }

        TEST_METHOD(AnimationErrorSource_DefaultsToCpuStart)
        {
            SwapChainCoreState swapChain;

            Assert::IsTrue(swapChain.animationErrorSource == AnimationErrorSource::CpuStart);
        }

        TEST_METHOD(AnimationErrorSource_CanBeChanged)
        {
            SwapChainCoreState swapChain;

            swapChain.animationErrorSource = AnimationErrorSource::PCLatency;
            Assert::IsTrue(swapChain.animationErrorSource == AnimationErrorSource::PCLatency);

            swapChain.animationErrorSource = AnimationErrorSource::AppProvider;
            Assert::IsTrue(swapChain.animationErrorSource == AnimationErrorSource::AppProvider);
        }
    };

    // ============================================================================
    // SECTION 2: DisplayIndexing Calculator
    // ============================================================================

    TEST_CLASS(DisplayIndexingTests)
    {
    public:
        TEST_METHOD(Calculate_NoDisplayedFrames_ReturnsEmptyRange)
        {
            FrameData present{};
            // No displayed frames
            present.displayed.clear();

            auto result = DisplayIndexing::Calculate(present, nullptr);

            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);
            Assert::AreEqual(size_t(0), result.appIndex);  // No displays → appIndex = 0
            Assert::IsFalse(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_SingleDisplay_NoNext_Postponed)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Single display with no next = postponed (empty range)
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);  // Empty! Postponed
            Assert::AreEqual(size_t(0), result.appIndex);  // Would be 0 if processed
            Assert::IsFalse(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_MultipleDisplays_NoNext_PostponeLast)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.displayed.push_back({ FrameType::Repeated, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);  // Excludes last!
            Assert::AreEqual(size_t(0), result.appIndex);  // App frame at index 0
            Assert::IsFalse(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_MultipleDisplays_WithNext_ProcessPostponed)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.displayed.push_back({ FrameType::Repeated, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            present.setFinalState(PresentResult::Presented);

            FrameData next{};
            next.displayed.push_back({ FrameType::Application, 4000 });

            auto result = DisplayIndexing::Calculate(present, &next);

            // Process only postponed last display [2]
            Assert::AreEqual(size_t(2), result.startIndex);
            Assert::AreEqual(size_t(3), result.endIndex);
            Assert::AreEqual(SIZE_MAX, result.appIndex);  // No app frame at [2], it's Repeated
            Assert::IsTrue(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_NotDisplayed_ReturnsEmptyRange)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.displayed.push_back({ FrameType::Repeated, 2000 });
            // Don't set finalState = Presented, so displayed = false

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Not displayed → empty range
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);
            Assert::AreEqual(size_t(0), result.appIndex);  // Fallback when displayCount > 0 but not displayed
            Assert::IsFalse(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_FindsAppFrameIndex_Displayed)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Repeated, 1000 });
            present.displayed.push_back({ FrameType::Application, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);
            Assert::AreEqual(size_t(1), result.appIndex);  // App at index 1
        }

        TEST_METHOD(Calculate_FindsAppFrameIndex_NotDisplayed)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Repeated, 1000 });
            present.displayed.push_back({ FrameType::Application, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            // Not displayed

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Not displayed → empty range
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);
        }

        TEST_METHOD(Calculate_AllRepeatedFrames_AppIndexInvalid)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Repeated, 1000 });
            present.displayed.push_back({ FrameType::Repeated, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);
            Assert::AreEqual(SIZE_MAX, result.appIndex);  // No app frame found
        }

        TEST_METHOD(Calculate_MultipleAppFrames_FindsFirst)
        {
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.displayed.push_back({ FrameType::Application, 2000 });
            present.displayed.push_back({ FrameType::Repeated, 3000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);
            Assert::AreEqual(size_t(0), result.appIndex);  // First app frame
        }

        TEST_METHOD(Calculate_WorksWithFrameData)
        {
            // Verify template works with FrameData
            FrameData present{};
            present.displayed.push_back({ FrameType::Application, 1000 });
            present.setFinalState(PresentResult::Presented);

            auto result = DisplayIndexing::Calculate(present, nullptr);

            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex); // Postponed [0], nothing processed
            Assert::IsTrue(result.appIndex == 0);
        }
    };

    // ============================================================================
    // SECTION 3: Helper Functions
    // ============================================================================

    TEST_CLASS(CalculateCPUStartTests)
    {
    public:
        TEST_METHOD(UsesAppPropagatedWhenAvailable)
        {
            // Setup: swapchain with lastAppPresent that has AppPropagated data
            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.appPropagatedPresentStartTime = 1000;
            lastApp.appPropagatedTimeInPresent = 50;
            swapChain.lastAppPresent = lastApp;  // std::optional assignment

            FrameData current{};
            current.presentStartTime = 2000;

            auto result = CalculateCPUStart(swapChain, current);

            // Should use appPropagated: 1000 + 50 = 1050
            Assert::AreEqual(1050ull, result);
        }

        TEST_METHOD(FallsBackToRegularPresentStart)
        {
            // Setup: swapchain with lastAppPresent but NO appPropagated data
            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.appPropagatedPresentStartTime = 0;  // No propagated data
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};

            auto result = CalculateCPUStart(swapChain, current);

            // Should use regular: 1000 + 50 = 1050
            Assert::AreEqual(1050ull, result);
        }

        TEST_METHOD(UsesLastPresentWhenNoAppPresent)
        {
            // Setup: swapchain with lastPresent but NO lastAppPresent
            SwapChainCoreState swapChain{};
            // lastAppPresent is std::nullopt by default

            FrameData lastPresent{};
            lastPresent.presentStartTime = 1000;
            lastPresent.timeInPresent = 50;
            swapChain.lastPresent = lastPresent;

            FrameData current{};
            current.timeInPresent = 30;

            auto result = CalculateCPUStart(swapChain, current);

            // Should use lastPresents values: 1000 + 50 (last presents start time and the 
            // time it spent in that present). This would equal the last presents
            // stop time which is the earliest the application can start the next frame.
            Assert::AreEqual(1050ull, result);
        }

        TEST_METHOD(ReturnsZeroWhenNoHistory)
        {
            // Setup: empty chain (both optionals are std::nullopt)
            SwapChainCoreState swapChain{};

            FrameData current{};
            current.presentStartTime = 2000;

            auto result = CalculateCPUStart(swapChain, current);

            // Should return 0 when no history
            Assert::AreEqual(0ull, result);
        }
    };

    TEST_CLASS(CalculateSimStartTimeTests)
    {
    public:
        TEST_METHOD(UsesCpuStartSource)
        {
            QpcConverter qpc{ 10000000, 0 };  // 10 MHz for easy math

            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};
            current.appSimStartTime = 5000;  // Has appSim, but source is CpuStart

            auto result = CalculateSimStartTime(swapChain, current, AnimationErrorSource::CpuStart);

            // Should use CPU start calculation: 1000 + 50 = 1050
            Assert::AreEqual(1050ull, result);
        }

        TEST_METHOD(UsesAppProviderSource)
        {
            QpcConverter qpc{ 10000000, 0 };

            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};
            current.appSimStartTime = 5000;

            auto result = CalculateSimStartTime(swapChain, current, AnimationErrorSource::AppProvider);

            // Should use appSimStartTime
            Assert::AreEqual(5000ull, result);
        }

        TEST_METHOD(UsesPCLatencySource)
        {
            QpcConverter qpc{ 10000000, 0 };

            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};
            current.pclSimStartTime = 6000;

            auto result = CalculateSimStartTime(swapChain, current, AnimationErrorSource::PCLatency);

            // Should use pclSimStartTime
            Assert::AreEqual(6000ull, result);
        }

        TEST_METHOD(AppProviderFallsBackToCpuStartWhenZero)
        {
            QpcConverter qpc{ 10000000, 0 };

            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};
            current.appSimStartTime = 0;  // Not available

            auto result = CalculateSimStartTime(swapChain, current, AnimationErrorSource::AppProvider);

            // Should fall back to CPU start: 1000 + 50 = 1050
            Assert::AreEqual(1050ull, result);
        }

        TEST_METHOD(PCLatencyFallsBackToCpuStartWhenZero)
        {
            QpcConverter qpc{ 10000000, 0 };

            SwapChainCoreState swapChain{};
            FrameData lastApp{};
            lastApp.presentStartTime = 1000;
            lastApp.timeInPresent = 50;
            swapChain.lastAppPresent = lastApp;

            FrameData current{};
            current.pclSimStartTime = 0;  // Not available

            auto result = CalculateSimStartTime(swapChain, current, AnimationErrorSource::PCLatency);

            // Should fall back to CPU start: 1000 + 50 = 1050
            Assert::AreEqual(1050ull, result);
        }
    };

    TEST_CLASS(CalculateAnimationTimeTests)
    {
    public:
        TEST_METHOD(ComputesRelativeTime)
        {
            QpcConverter qpc{ 10000000, 0 };  // 10 MHz QPC frequency

            uint64_t firstSimStart = 1000;
            uint64_t currentSimStart = 1500;  // 500 ticks later

            auto result = CalculateAnimationTime(qpc, firstSimStart, currentSimStart);

            // 500 ticks at 10 MHz = 0.05 ms
            Assert::AreEqual(0.05, result, 0.001);
        }

        TEST_METHOD(HandlesZeroFirst)
        {
            QpcConverter qpc{ 10000000, 0 };

            uint64_t firstSimStart = 0;  // Not initialized yet
            uint64_t currentSimStart = 1500;

            auto result = CalculateAnimationTime(qpc, firstSimStart, currentSimStart);

            // When first is 0, should return 0
            Assert::AreEqual(0.0, result, 0.001);
        }

        TEST_METHOD(HandlesSameTimestamp)
        {
            QpcConverter qpc{ 10000000, 0 };

            uint64_t firstSimStart = 1000;
            uint64_t currentSimStart = 1000;  // Same as first

            auto result = CalculateAnimationTime(qpc, firstSimStart, currentSimStart);

            // Same timestamp = 0 ms elapsed
            Assert::AreEqual(0.0, result, 0.001);
        }

        TEST_METHOD(HandlesLargeTimespan)
        {
            QpcConverter qpc{ 10000000, 0 };  // 10 MHz

            uint64_t firstSimStart = 1000;
            uint64_t currentSimStart = 1000 + (10000000 * 5);  // +5 seconds in ticks

            auto result = CalculateAnimationTime(qpc, firstSimStart, currentSimStart);

            // 5 seconds = 5000 ms
            Assert::AreEqual(5000.0, result, 0.1);
        }

        TEST_METHOD(HandlesBackwardsTime)
        {
            QpcConverter qpc{ 10000000, 0 };

            uint64_t firstSimStart = 2000;
            uint64_t currentSimStart = 1000;  // Earlier than first (unusual but possible)

            auto result = CalculateAnimationTime(qpc, firstSimStart, currentSimStart);

            // Should handle gracefully - returns negative or 0 depending on implementation
            // This tests error handling
            Assert::IsTrue(result <= 0.0);
        }
    };

    // TEST HELPERS FOR METRICS UNIFICATION
    // ============================================================================

    using namespace pmon::util::metrics;

    // Simple helper to construct FrameData for metrics tests.
    static FrameData MakeFrame(
        PresentResult finalState,
        uint64_t presentStartTime,
        uint64_t timeInPresent,
        uint64_t readyTime,
        const std::vector<std::pair<FrameType, uint64_t>>& displayed,
        uint64_t appSimStartTime = 0,
        uint64_t pclSimStartTime = 0,
        uint64_t flipDelay = 0)
    {
        FrameData f{};
        f.presentStartTime = presentStartTime;
        f.timeInPresent = timeInPresent;
        f.readyTime = readyTime;
        f.displayed = displayed;
        f.appSimStartTime = appSimStartTime;
        f.pclSimStartTime = pclSimStartTime;
        f.flipDelay = flipDelay;
        f.finalState = finalState;
        return f;
    }

    TEST_CLASS(ComputeMetricsForPresentTests)
    {
    public:
        TEST_METHOD(ComputeMetricsForPresent_NotDisplayed_NoDisplays_ProducesSingleMetricsAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 10'000, 500, 10'500, {}); // Presented but no displays => not displayed path
            auto metrics = ComputeMetricsForPresent(qpc, frame, nullptr, chain);

            Assert::AreEqual(size_t(1), metrics.size(), L"Should produce exactly one metrics entry.");
            Assert::IsTrue(chain.lastPresent.has_value(), L"Chain should be updated for not displayed.");
            Assert::IsTrue(chain.lastAppPresent.has_value(), L"Not displayed frames become lastAppPresent.");
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(ComputeMetricsForPresent_NotDisplayed_WithDisplaysButNotPresented_ProducesSingleMetricsAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Simulate a frame with 'displayed' entries but finalState != Presented (treat as not displayed).
            auto frame = MakeFrame(static_cast<PresentResult>(9999), 1'000, 100, 1'200,
                                   { { FrameType::Application, 2'000 } });

            auto metrics = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), metrics.size());
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime, L"Not displayed path should not update displayed screen time.");
        }

        TEST_METHOD(ComputeMetricsForPresent_DisplayedNoNext_SingleDisplay_PostponedChainNotUpdated)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 5'000, 200, 5'500,
                                   { { FrameType::Application, 6'000 } });

            auto metrics = ComputeMetricsForPresent(qpc, frame, nullptr, chain);

            Assert::AreEqual(size_t(0), metrics.size(), L"Single display is postponed => zero metrics now.");
            Assert::IsFalse(chain.lastPresent.has_value(), L"Chain should NOT be updated yet.");
            Assert::IsFalse(chain.lastAppPresent.has_value());
        }

        TEST_METHOD(ComputeMetricsForPresent_DisplayedNoNext_MultipleDisplays_ProcessesAllButLast)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 10'000, 300, 10'800,
                                   {
                                       { FrameType::Application, 11'000 },
                                       { FrameType::Repeated,    11'500 },
                                       { FrameType::Repeated,    12'000 } // postponed
                                   });

            auto metrics = ComputeMetricsForPresent(qpc, frame, nullptr, chain);

            Assert::AreEqual(size_t(2), metrics.size(), L"Should process all but last display.");
            Assert::IsFalse(chain.lastPresent.has_value());
            Assert::IsFalse(chain.lastAppPresent.has_value());
        }

        TEST_METHOD(ComputeMetricsForPresent_DisplayedWithNext_ProcessesPostponedLastAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 10'000, 300, 10'800,
                                   {
                                       { FrameType::Application, 11'000 },
                                       { FrameType::Repeated,    11'500 },
                                       { FrameType::Repeated,    12'000 }
                                   },
                                   0, 0, 777);

            auto nextDisplayed = MakeFrame(PresentResult::Presented, 13'000, 250, 13'600,
                                           { { FrameType::Application, 14'000 } });

            // First call without nextDisplayed: postpone last
            auto preMetrics = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), preMetrics.size());
            Assert::IsFalse(chain.lastPresent.has_value());

            // Second call with nextDisplayed: process postponed last + update chain
            auto postMetrics = ComputeMetricsForPresent(qpc, frame, &nextDisplayed, chain);
            Assert::AreEqual(size_t(1), postMetrics.size(), L"Should process only the postponed last display this time.");
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::AreEqual(uint64_t(12'000), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(777), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(ComputeMetricsForPresent_DisplayedWithNext_LastDisplayIsRepeated_DoesNotUpdateLastAppPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Previous app present for fallback usage.
            FrameData prevApp = MakeFrame(PresentResult::Presented, 2'000, 100, 2'300,
                                          { { FrameType::Application, 2'800 } });
            chain.lastAppPresent = prevApp;

            auto frame = MakeFrame(PresentResult::Presented, 4'000, 120, 4'300,
                                   {
                                       { FrameType::Application, 4'500 },
                                       { FrameType::Repeated,    4'900 } // last (Repeated)
                                   });

            auto nextDisplayed = MakeFrame(PresentResult::Presented, 5'000, 110, 5'250,
                                           { { FrameType::Application, 5'600 } });

            auto metrics = ComputeMetricsForPresent(qpc, frame, &nextDisplayed, chain);
            Assert::AreEqual(size_t(1), metrics.size());

            Assert::IsTrue(chain.lastPresent.has_value());
            // lastAppPresent should remain previous since last display was Repeated
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(2'000), chain.lastAppPresent->presentStartTime);
        }
    };

    TEST_CLASS(UpdateAfterPresentAnimationErrorSourceTests)
    {
    public:
        TEST_METHOD(UpdateAfterPresent_AnimationSource_AppProvider_UpdatesSimStartAndFirstAppSim)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::AppProvider;

            auto frame = MakeFrame(PresentResult::Presented, 1'000, 50, 1'200,
                                   { { FrameType::Application, 1'500 } },
                                   10'000 /* appSimStartTime */);

            chain.UpdateAfterPresent(frame);

            Assert::AreEqual(uint64_t(10'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(10'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(1'500), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_PCLatency_UpdatesSimStartAndFirstAppSim)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::PCLatency;

            auto frame = MakeFrame(PresentResult::Presented, 2'000, 40, 2'300,
                                   { { FrameType::Application, 2'700 } },
                                   0 /* appSimStartTime */, 12'345 /* pclSimStart */);

            chain.UpdateAfterPresent(frame);

            Assert::AreEqual(uint64_t(12'345), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(12'345), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(2'700), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_CpuStart_FallbackToPreviousAppPresent)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::CpuStart;

            FrameData previousApp = MakeFrame(PresentResult::Presented, 5'000, 80, 5'300,
                                              { { FrameType::Application, 5'800 } });
            chain.lastAppPresent = previousApp;

            auto frame = MakeFrame(PresentResult::Presented, 6'000, 60, 6'250,
                                   { { FrameType::Application, 6'700 } },
                                   0, 0);

            chain.UpdateAfterPresent(frame);

            // No appSimStartTime or pclSimStartTime, fallback uses previous app present CPU end:
            // 5'000 + 80 = 5'080
            Assert::AreEqual(uint64_t(5'080), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(0), chain.firstAppSimStartTime); // Not set yet
            Assert::AreEqual(uint64_t(6'700), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_CpuStart_TransitionsToAppProvider)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::CpuStart;

            auto frame = MakeFrame(PresentResult::Presented, 7'000, 70, 7'400,
                                   { { FrameType::Application, 7'900 } },
                                   20'000 /* appSimStartTime */);

            chain.UpdateAfterPresent(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(20'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(20'000), chain.firstAppSimStartTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_CpuStart_TransitionsToPCLatency)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::CpuStart;

            auto frame = MakeFrame(PresentResult::Presented, 8'000, 80, 8'400,
                                   { { FrameType::Application, 8'950 } },
                                   0 /* appSim */, 30'000 /* pclSim */);

            chain.UpdateAfterPresent(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::PCLatency);
            Assert::AreEqual(uint64_t(30'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(30'000), chain.firstAppSimStartTime);
        }
    };

    TEST_CLASS(UpdateAfterPresentFlipDelayTests)
    {
    public:
        TEST_METHOD(UpdateAfterPresent_FlipDelayTracking_PresentedWithDisplays_SetsFlipDelayAndScreenTime)
        {
            SwapChainCoreState chain{};
            auto frame = MakeFrame(PresentResult::Presented, 10'000, 50, 10'300,
                                   {
                                       { FrameType::Application, 10'800 },
                                       { FrameType::Repeated,    11'000 }
                                   },
                                   0, 0, 1234 /* flipDelay */);

            chain.UpdateAfterPresent(frame);

            Assert::AreEqual(uint64_t(11'000), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(1234), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(UpdateAfterPresent_FlipDelayTracking_PresentedNoDisplays_ZeroesFlipDelayAndScreenTime)
        {
            SwapChainCoreState chain{};
            auto frame = MakeFrame(PresentResult::Presented, 12'000, 40, 12'300,
                                   {}, 0, 0, 9999);

            chain.UpdateAfterPresent(frame);

            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(UpdateAfterPresent_NotPresented_DoesNotChangeLastDisplayedScreenTime)
        {
            SwapChainCoreState chain{};
            // Seed previous displayed state
            FrameData prev = MakeFrame(PresentResult::Presented, 1'000, 30, 1'200,
                                       { { FrameType::Application, 1'500 } });
            chain.UpdateAfterPresent(prev);
            Assert::AreEqual(uint64_t(1'500), chain.lastDisplayedScreenTime);

            // Not presented frame with displays (ignored for displayed tracking)
            auto frame = MakeFrame(static_cast<PresentResult>(7777), 2'000, 25, 2'150,
                                   { { FrameType::Application, 2'600 } });

            chain.UpdateAfterPresent(frame);

            Assert::AreEqual(uint64_t(1'500), chain.lastDisplayedScreenTime, L"Should remain unchanged.");
        }
    };
    TEST_CLASS(FrameTypeXefgAfmfIndexingTests)
    {
    public:
        TEST_METHOD(DisplayIndexing_IntelXefg_Multi_NoNext_AppIndexIsLast)
        {
            // 3x Intel_XEFG then a single Application
            FrameData present = MakeFrame(
                PresentResult::Presented,
                10'000, 500, 20'000,
                {
                    { FrameType::Intel_XEFG, 11'000 },
                    { FrameType::Intel_XEFG, 11'500 },
                    { FrameType::Intel_XEFG, 12'000 },
                    { FrameType::Application, 12'500 },
                });

            auto idx = DisplayIndexing::Calculate(present, nullptr);

            // No nextDisplayed: process [0..N-2] => [0..3)
            Assert::AreEqual(size_t(0), idx.startIndex);
            Assert::AreEqual(size_t(3), idx.endIndex);
            // App frame is at index 3 (outside processing range, postponed)
            Assert::AreEqual(size_t(3), idx.appIndex);
            Assert::IsFalse(idx.hasNextDisplayed);
        }

        TEST_METHOD(DisplayIndexing_AmdAfmf_Multi_WithNext_AppIndexProcessed)
        {
            // 3x AMD_AFMF then a single Application
            FrameData present = MakeFrame(
                PresentResult::Presented,
                20'000, 600, 30'000,
                {
                    { FrameType::AMD_AFMF, 21'000 },
                    { FrameType::AMD_AFMF, 21'500 },
                    { FrameType::AMD_AFMF, 22'000 },
                    { FrameType::Application, 22'500 },
                });

            FrameData nextDisplayed = MakeFrame(
                PresentResult::Presented,
                23'000, 400, 30'500,
                { { FrameType::Application, 24'000 } });

            auto idx = DisplayIndexing::Calculate(present, &nextDisplayed);

            // With nextDisplayed: process postponed last only => [N-1, N) => [3, 4)
            Assert::AreEqual(size_t(3), idx.startIndex);
            Assert::AreEqual(size_t(4), idx.endIndex);
            Assert::AreEqual(size_t(3), idx.appIndex);
            Assert::IsTrue(idx.hasNextDisplayed);
        }
    };

    TEST_CLASS(FrameTypeXefgAfmfMetricsTests)
    {
    public:
        TEST_METHOD(ComputeMetricsForPresent_IntelXefg_NoNext_AppNotProcessed_ChainNotUpdated)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // 3x Intel_XEFG then 1 Application; no nextDisplayed
            FrameData present = MakeFrame(
                PresentResult::Presented,
                30'000, 700, 40'000,
                {
                    { FrameType::Intel_XEFG, 31'000 },
                    { FrameType::Intel_XEFG, 31'500 },
                    { FrameType::Intel_XEFG, 32'000 },
                    { FrameType::Application, 32'500 },
                });

            auto metrics = ComputeMetricsForPresent(qpc, present, nullptr, chain);

            // Should process all but last => 3 metrics
            Assert::AreEqual(size_t(3), metrics.size());
            // Chain update postponed until nextDisplayed
            Assert::IsFalse(chain.lastPresent.has_value());
            Assert::IsFalse(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(ComputeMetricsForPresent_AmdAfmf_WithNext_AppProcessedAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // 3x AMD_AFMF then 1 Application; with nextDisplayed provided
            FrameData present = MakeFrame(
                PresentResult::Presented,
                40'000, 650, 50'000,
                {
                    { FrameType::AMD_AFMF, 41'000 },
                    { FrameType::AMD_AFMF, 41'400 },
                    { FrameType::AMD_AFMF, 41'800 },
                    { FrameType::Application, 42'200 },
                },
                39'500, /* appSimStartTime*/
                0,      /* pclSimStartTime*/
                999     /* flipDelay*/);

            FrameData nextDisplayed = MakeFrame(
                PresentResult::Presented,
                43'000, 500, 50'500,
                { { FrameType::Application, 44'000 } });

            auto metrics = ComputeMetricsForPresent(qpc, present, &nextDisplayed, chain);

            // Should process only postponed last => 1 metrics
            Assert::AreEqual(size_t(1), metrics.size());

            // UpdateAfterPresent has run
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::IsTrue(chain.lastAppPresent.has_value(), L"Last displayed is Application; lastAppPresent should be updated.");
            Assert::AreEqual(uint64_t(42'200), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(999), chain.lastDisplayedFlipDelay);
        }
    };
    TEST_CLASS(DisplayedDroppedDisplayedSequenceTests)
    {
    public:
        TEST_METHOD(Displayed_Dropped_Displayed_Sequence_IsHandledAcrossCalls)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // A: displayed once, but no nextDisplayed yet => postponed
            FrameData A = MakeFrame(
                PresentResult::Presented,
                50'000, 400, 50'500,
                { { FrameType::Application, 51'000 } });

            auto mA_pre = ComputeMetricsForPresent(qpc, A, nullptr, chain);
            Assert::AreEqual(size_t(0), mA_pre.size(), L"Single display postponed.");
            Assert::IsFalse(chain.lastPresent.has_value(), L"Chain is not updated without nextDisplayed.");

            // B: dropped (not presented/displayed)
            FrameData B = MakeFrame(
                PresentResult::Discarded,
                52'000, 300, 52'400,
                {} /* no displayed */);

            auto mB = ComputeMetricsForPresent(qpc, B, nullptr, chain);
            Assert::AreEqual(size_t(1), mB.size(), L"Dropped frame goes through not-displayed path.");
            Assert::IsTrue(chain.lastPresent.has_value(), L"Not-displayed path updates chain.");
            Assert::IsTrue(chain.lastAppPresent.has_value(), L"Not-displayed frame becomes lastAppPresent.");
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime, L"Not-displayed should leave lastDisplayedScreenTime at 0.");

            // C: displayed next; use it to process A's postponed last
            FrameData C = MakeFrame(
                PresentResult::Presented,
                53'000, 350, 53'400,
                { { FrameType::Application, 54'000 } });

            auto mA_post = ComputeMetricsForPresent(qpc, A, &C, chain);
            Assert::AreEqual(size_t(1), mA_post.size(), L"Postponed last display of A processed with nextDisplayed.");

            // Chain updated based on A (last display instance)
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::AreEqual(uint64_t(51'000), chain.lastDisplayedScreenTime);
        }
    };

    TEST_CLASS(MetricsValueTests)
    {
        TEST_METHOD(ComputeMetricsForPresent_NotDisplayed_msBetweenPresents_UsesLastPresentDelta)
        {
            // 10MHz QPC frequency
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // First frame: not displayed path (Presented but no Displayed entries)
            auto first = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'000'000,
                /*timeInPresent*/    10'000,
                /*readyTime*/        1'020'000,
                /*displayed*/{});  // no displayed frames => not-displayed path

            auto firstMetrics = ComputeMetricsForPresent(qpc, first, nullptr, chain);

            // We should get exactly one metrics entry
            Assert::AreEqual(size_t(1), firstMetrics.size(), L"First not-displayed frame should produce one metrics entry.");

            // With no prior lastPresent, msBetweenPresents should be zero
            Assert::AreEqual(
                0.0,
                firstMetrics[0].metrics.msBetweenPresents,
                0.0001,
                L"First frame should have msBetweenPresents == 0.");

            // Chain should now treat this as lastPresent / lastAppPresent
            Assert::IsTrue(chain.lastPresent.has_value());
            if (!chain.lastPresent.has_value())
            {
                Assert::Fail(L"lastPresent was unexpectedly empty.");
                return;
            }
            const auto& last = chain.lastPresent.value();
            Assert::AreEqual(uint64_t(1'000'000), last.presentStartTime);

            // Second frame: also not displayed, later in time
            auto second = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'016'660,   // ~16.666 ms later at 10MHz
                /*timeInPresent*/    10'000,
                /*readyTime*/        1'036'660,
                /*displayed*/{});

            auto secondMetrics = ComputeMetricsForPresent(qpc, second, nullptr, chain);

            Assert::AreEqual(
                size_t(1),
                secondMetrics.size(),
                L"Second not-displayed frame should also produce one metrics entry.");

            // Expected delta: use the same converter the implementation uses
            double expectedDelta = qpc.DeltaUnsignedMilliSeconds(
                first.presentStartTime,
                second.presentStartTime);

            Assert::AreEqual(
                expectedDelta,
                secondMetrics[0].metrics.msBetweenPresents,
                0.0001,
                L"msBetweenPresents should equal the unsigned delta between lastPresent and current presentStartTime.");
        }
        TEST_METHOD(ComputeMetricsForPresent_NotDisplayed_BaseTimingAndCpuStart_AreCorrect)
        {
            // 10 MHz QPC: 10,000,000 ticks per second
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // First frame: not displayed, becomes the baseline lastPresent/lastAppPresent.
            FrameData first = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'000'000,   // 0.1s
                /*timeInPresent*/    200'000,     // 0.02s
                /*readyTime*/        1'500'000,   // 0.15s → 50 ms after start
                /*displayed*/{}           // no displays => "not displayed" path
            );

            auto firstMetricsList = ComputeMetricsForPresent(qpc, first, nullptr, chain);
            Assert::AreEqual(
                size_t(1),
                firstMetricsList.size(),
                L"First not-displayed frame should produce one metrics entry.");

            const auto& firstMetrics = firstMetricsList[0].metrics;

            uint64_t expectedTimeInSecondsFirst = first.presentStartTime;
            Assert::AreEqual(
                expectedTimeInSecondsFirst,
                firstMetrics.timeInSeconds,
                L"timeInSeconds should come from QpcToSeconds(presentStartTime).");

            // No prior lastPresent → msBetweenPresents should be 0
            Assert::AreEqual(
                0.0,
                firstMetrics.msBetweenPresents,
                0.0001,
                L"First frame should have msBetweenPresents == 0.");

            // msInPresentApi = delta for TimeInPresent
            double expectedMsInPresentFirst = qpc.DurationMilliSeconds(first.timeInPresent);
            Assert::AreEqual(
                expectedMsInPresentFirst,
                firstMetrics.msInPresentApi,
                0.0001,
                L"msInPresentApi should equal QpcDeltaToMilliSeconds(timeInPresent).");

            // msUntilRenderComplete = delta between PresentStart and Ready
            double expectedMsUntilRenderCompleteFirst =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, first.readyTime);
            Assert::AreEqual(
                expectedMsUntilRenderCompleteFirst,
                firstMetrics.msUntilRenderComplete,
                0.0001,
                L"msUntilRenderComplete should equal delta from PresentStartTime to ReadyTime.");

            // With no prior present, CalculateCPUStart should return 0 → cpuStartQpc == 0
            Assert::AreEqual(
                uint64_t(0),
                firstMetrics.cpuStartQpc,
                L"First frame with no history should have cpuStartQpc == 0.");

            // Chain must now have lastPresent/lastAppPresent set to 'first'
            Assert::IsTrue(chain.lastPresent.has_value(), L"Expected lastPresent to be set.");
            if (!chain.lastPresent.has_value()) {
                Assert::Fail(L"lastPresent was unexpectedly empty.");
                return;
            }
            const auto& lastAfterFirst = chain.lastPresent.value();
            Assert::AreEqual(first.presentStartTime, lastAfterFirst.presentStartTime);

            // -------------------------------------------------------------------------
            // Second frame: also not displayed, later in time.
            // This should:
            //  - compute msBetweenPresents based on first→second start times
            //  - keep msInPresentApi/msUntilRenderComplete consistent
            //  - use CalculateCPUStart based on 'first' as lastAppPresent
            // -------------------------------------------------------------------------

            FrameData second = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'016'000,   // slightly later than first
                /*timeInPresent*/    300'000,     // 0.03s
                /*readyTime*/        1'516'000,   // 0.5s after first start
                /*displayed*/{}           // still "not displayed" path
            );

            auto secondMetricsList = ComputeMetricsForPresent(qpc, second, nullptr, chain);
            Assert::AreEqual(
                size_t(1),
                secondMetricsList.size(),
                L"Second not-displayed frame should produce one metrics entry.");

            const auto& secondMetrics = secondMetricsList[0].metrics;

            // msBetweenPresents should be based on lastPresent.start -> second.start
            double expectedBetween =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, second.presentStartTime);
            Assert::AreEqual(
                expectedBetween,
                secondMetrics.msBetweenPresents,
                0.0001,
                L"msBetweenPresents should equal delta between lastPresent and current presentStart.");

            // msInPresentApi / msUntilRenderComplete for second
            double expectedMsInPresentSecond = qpc.DurationMilliSeconds(second.timeInPresent);
            double expectedMsUntilRenderCompleteSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.readyTime);

            Assert::AreEqual(
                expectedMsInPresentSecond,
                secondMetrics.msInPresentApi,
                0.0001,
                L"Second frame msInPresentApi should match timeInPresent.");
            Assert::AreEqual(
                expectedMsUntilRenderCompleteSecond,
                secondMetrics.msUntilRenderComplete,
                0.0001,
                L"Second frame msUntilRenderComplete should match start→ready delta.");

            // cpuStartQpc for second should come from CalculateCPUStart:
            // lastAppPresent == first (no propagated times) → first.start + first.timeInPresent
            uint64_t expectedCpuStartSecond = first.presentStartTime + first.timeInPresent;
            Assert::AreEqual(
                expectedCpuStartSecond,
                secondMetrics.cpuStartQpc,
                L"cpuStartQpc should match CalculateCPUStart from lastAppPresent.");
        }
        TEST_METHOD(ComputeMetricsForPresent_DisplayedWithNext_BaseTimingAndCpuStart_AreCorrect)
        {
            // 10 MHz QPC
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Baseline frame: Presented but not displayed → not-displayed path
            FrameData first = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'000'000,
                /*timeInPresent*/    200'000,
                /*readyTime*/        1'500'000,
                /*displayed*/{});   // no displays

            auto firstMetricsList = ComputeMetricsForPresent(qpc, first, nullptr, chain);
            Assert::AreEqual(
                size_t(1),
                firstMetricsList.size(),
                L"Baseline not-displayed frame should produce one metrics entry.");

            // Chain should now have lastPresent/lastAppPresent == first
            Assert::IsTrue(chain.lastPresent.has_value(), L"Expected lastPresent to be set after baseline frame.");
            if (!chain.lastPresent.has_value()) {
                Assert::Fail(L"lastPresent was unexpectedly empty after baseline frame.");
                return;
            }

            // Second frame: Presented + one displayed instance, processed with a nextDisplayed
            FrameData second = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'016'000,      // slightly later than first
                /*timeInPresent*/    300'000,
                /*readyTime*/        1'616'000,
                std::vector<std::pair<FrameType, uint64_t>>{
                    { FrameType::Application, 2'000'000 }   // one displayed instance
            });

            // Dummy nextDisplayed with at least one display so the "with next" path is taken
            FrameData nextDisplayed = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 2'100'000,
                /*timeInPresent*/    100'000,
                /*readyTime*/        2'200'000,
                std::vector<std::pair<FrameType, uint64_t>>{
                    { FrameType::Application, 2'300'000 }
            });

            auto secondMetricsList = ComputeMetricsForPresent(qpc, second, &nextDisplayed, chain);

            Assert::AreEqual(
                size_t(1),
                secondMetricsList.size(),
                L"Displayed-with-next frame should produce one metrics entry (postponed last display).");

            const auto& secondMetrics = secondMetricsList[0].metrics;

            // timeInSeconds from presentStartTime
            auto expectedTimeInSecondsSecond = second.presentStartTime;
            Assert::AreEqual(
                expectedTimeInSecondsSecond,
                secondMetrics.timeInSeconds,
                L"timeInSeconds should match QpcToSeconds(presentStartTime) for displayed frame.");

            // msBetweenPresents: lastPresent.start (first) → second.start
            double expectedBetween =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, second.presentStartTime);
            Assert::AreEqual(
                expectedBetween,
                secondMetrics.msBetweenPresents,
                0.0001,
                L"msBetweenPresents should match delta between lastPresent and current presentStart for displayed frame.");

            // msInPresentApi from timeInPresent
            double expectedMsInPresentSecond = qpc.DurationMilliSeconds(second.timeInPresent);
            Assert::AreEqual(
                expectedMsInPresentSecond,
                secondMetrics.msInPresentApi,
                0.0001,
                L"msInPresentApi should match QpcDeltaToMilliSeconds(timeInPresent) for displayed frame.");

            // msUntilRenderComplete from start → ready
            double expectedMsUntilRenderCompleteSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.readyTime);
            Assert::AreEqual(
                expectedMsUntilRenderCompleteSecond,
                secondMetrics.msUntilRenderComplete,
                0.0001,
                L"msUntilRenderComplete should match start→ready delta for displayed frame.");

            // cpuStartQpc should come from CalculateCPUStart using baseline frame as lastAppPresent:
            // (no propagated times) → first.start + first.timeInPresent
            uint64_t expectedCpuStartSecond = first.presentStartTime + first.timeInPresent;
            Assert::AreEqual(
                expectedCpuStartSecond,
                secondMetrics.cpuStartQpc,
                L"cpuStartQpc for displayed frame should match CalculateCPUStart based on lastAppPresent.");
        }
    };
    TEST_CLASS(MsUntilDisplayedTests) {
    public: 
        TEST_METHOD(NotDisplayed_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0); SwapChainCoreState chain{};
            // Not displayed: Presented but no displayed entries
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 10'000;
            frame.readyTime = 1'010'000;
            frame.setFinalState(PresentResult::Presented);
            // No displayed entries

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            Assert::AreEqual(0.0, m.msUntilDisplayed, 0.0001);
        }
        TEST_METHOD(Displayed_ReturnsDeltaFromPresentStartToScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 2'000'000; // start
            frame.timeInPresent = 20'000;
            frame.readyTime = 2'050'000;
            frame.setFinalState(PresentResult::Presented);
            // Single displayed; will be postponed unless nextDisplayed provided
            frame.displayed.push_back({ FrameType::Application, 2'500'000 });

            FrameData next{}; // provide nextDisplayed to process postponed
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 3'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.getPresentStartTime(), frame.getDisplayedScreenTime(0));
            Assert::AreEqual(expected, m.msUntilDisplayed, 0.0001);
        }
        TEST_METHOD(DisplayedGeneratedFrame_AlsoReturnsDelta)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 15'000;
            frame.readyTime = 5'030'000;
            frame.setFinalState(PresentResult::Presented);
            // Displayed generated frame (e.g., Repeated/Composed/Desktop depending on enum)
            frame.displayed.push_back({ FrameType::Intel_XEFG, 5'100'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 6'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.getPresentStartTime(), frame.getDisplayedScreenTime(0));
            Assert::AreEqual(expected, m.msUntilDisplayed, 0.0001);
        }
    };
    TEST_CLASS(MsDisplayedTimeTests)
    {
    public:
        TEST_METHOD(NotDisplayed_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 10'000;
            frame.readyTime = 1'010'000;
            frame.setFinalState(PresentResult::Presented);

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            Assert::AreEqual(0.0, m.msDisplayedTime, 0.0001);
        }

        TEST_METHOD(DisplayedSingleDisplay_WithNextDisplay_ReturnsDeltaToNextScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 2'000'000;
            frame.timeInPresent = 20'000;
            frame.readyTime = 2'050'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 2'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 2'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            double expected = qpc.DeltaUnsignedMilliSeconds(2'500'000, 2'800'000);
            Assert::AreEqual(expected, m.msDisplayedTime, 0.0001);
        }

        TEST_METHOD(DisplayedMultipleDisplays_ProcessEachWithNextScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 3'000'000;
            frame.timeInPresent = 30'000;
            frame.readyTime = 3'050'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 3'100'000 });
            frame.displayed.push_back({ FrameType::Repeated, 3'400'000 });
            frame.displayed.push_back({ FrameType::Repeated, 3'700'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 4'000'000 });

            auto results1 = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), results1.size());

            double expected0 = qpc.DeltaUnsignedMilliSeconds(3'100'000, 3'400'000);
            Assert::AreEqual(expected0, results1[0].metrics.msDisplayedTime, 0.0001);

            double expected1 = qpc.DeltaUnsignedMilliSeconds(3'400'000, 3'700'000);
            Assert::AreEqual(expected1, results1[1].metrics.msDisplayedTime, 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results2.size());

            double expected2 = qpc.DeltaUnsignedMilliSeconds(3'700'000, 4'000'000);
            Assert::AreEqual(expected2, results2[0].metrics.msDisplayedTime, 0.0001);
        }
    };

    TEST_CLASS(MsBetweenDisplayChangeTests)
    {
    public:
        TEST_METHOD(FirstDisplayedFrame_NoChainHistory_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 5'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 5'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 6'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(0.0, m.msBetweenDisplayChange, 0.0001);
        }

        TEST_METHOD(SubsequentDisplayedFrame_UsesChainLastDisplayedScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastDisplayedScreenTime = 4'000'000;

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 5'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 5'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 6'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            double expected = qpc.DeltaUnsignedMilliSeconds(4'000'000, 5'500'000);
            Assert::AreEqual(expected, m.msBetweenDisplayChange, 0.0001);
        }

        TEST_METHOD(NotDisplayed_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastDisplayedScreenTime = 4'000'000;

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 5'100'000;
            frame.setFinalState(PresentResult::Presented);

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(0.0, m.msBetweenDisplayChange, 0.0001);
        }

        TEST_METHOD(MultipleDisplays_EachComputesDeltaFromPrior)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastDisplayedScreenTime = 3'000'000;

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 5'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 5'500'000 });
            frame.displayed.push_back({ FrameType::Repeated, 5'800'000 });
            frame.displayed.push_back({ FrameType::Repeated, 6'100'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 6'400'000 });

            auto results1 = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), results1.size());

            double expected0 = qpc.DeltaUnsignedMilliSeconds(3'000'000, 5'500'000);
            Assert::AreEqual(expected0, results1[0].metrics.msBetweenDisplayChange, 0.0001);

            double expected1 = qpc.DeltaUnsignedMilliSeconds(3'000'000, 5'800'000);
            Assert::AreEqual(expected1, results1[1].metrics.msBetweenDisplayChange, 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results2.size());

            double expected2 = qpc.DeltaUnsignedMilliSeconds(3'000'000, 6'100'000);
            Assert::AreEqual(expected2, results2[0].metrics.msBetweenDisplayChange, 0.0001);
        }
    };

    TEST_CLASS(MsFlipDelayTests)
    {
    public:
        TEST_METHOD(NotDisplayed_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 7'000'000;
            frame.timeInPresent = 70'000;
            frame.readyTime = 7'100'000;
            frame.flipDelay = 5'000;
            frame.setFinalState(PresentResult::Presented);

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (m.msFlipDelay.has_value()) {
                Assert::AreEqual(0.0, m.msFlipDelay.value(), 0.0001);
            }
        }

        TEST_METHOD(Displayed_WithFlipDelay_ReturnsFlipDelayInMs)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 7'000'000;
            frame.timeInPresent = 70'000;
            frame.readyTime = 7'100'000;
            frame.flipDelay = 100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 7'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 8'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (m.msFlipDelay.has_value()) {
                double expected = qpc.DurationMilliSeconds(100'000);
                Assert::AreEqual(expected, m.msFlipDelay.value(), 0.0001);
            }
        }

        TEST_METHOD(Displayed_WithoutFlipDelay_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 7'000'000;
            frame.timeInPresent = 70'000;
            frame.readyTime = 7'100'000;
            frame.flipDelay = 0;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 7'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 8'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (m.msFlipDelay.has_value()) {
                Assert::AreEqual(0.0, m.msFlipDelay.value(), 0.0001);
            }
        }

        TEST_METHOD(DisplayedWithGeneratedFrame_AlsoIncludesFlipDelay)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 7'000'000;
            frame.timeInPresent = 70'000;
            frame.readyTime = 7'100'000;
            frame.flipDelay = 50'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Repeated, 7'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 8'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (m.msFlipDelay.has_value()) {
                double expected = qpc.DurationMilliSeconds(50'000);
                Assert::AreEqual(expected, m.msFlipDelay.value(), 0.0001);
            }
        }
    };

    TEST_CLASS(ScreenTimeQpcTests)
    {
    public:
        TEST_METHOD(NotDisplayed_ReturnsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 9'000'000;
            frame.timeInPresent = 90'000;
            frame.readyTime = 9'100'000;
            frame.setFinalState(PresentResult::Presented);

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(0), m.screenTimeQpc);
        }

        TEST_METHOD(DisplayedSingleFrame_EqualsScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 9'000'000;
            frame.timeInPresent = 90'000;
            frame.readyTime = 9'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 9'500'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 10'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(9'500'000), m.screenTimeQpc);
        }

        TEST_METHOD(DisplayedMultipleFrames_EachHasOwnScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 9'000'000;
            frame.timeInPresent = 90'000;
            frame.readyTime = 9'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Application, 9'500'000 });
            frame.displayed.push_back({ FrameType::Repeated, 9'800'000 });
            frame.displayed.push_back({ FrameType::Repeated, 10'100'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 10'400'000 });

            auto results1 = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), results1.size());
            Assert::AreEqual(uint64_t(9'500'000), results1[0].metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(9'800'000), results1[1].metrics.screenTimeQpc);

            auto results2 = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results2.size());
            Assert::AreEqual(uint64_t(10'100'000), results2[0].metrics.screenTimeQpc);
        }

        TEST_METHOD(DisplayedGeneratedFrame_EqualsGeneratedFrameScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 9'000'000;
            frame.timeInPresent = 90'000;
            frame.readyTime = 9'100'000;
            frame.setFinalState(PresentResult::Presented);
            frame.displayed.push_back({ FrameType::Repeated, 9'700'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 10'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(9'700'000), m.screenTimeQpc);
        }
    };
    TEST_CLASS(NvCollapsedPresentTests)
    {
    public:
        TEST_METHOD(NvCollapsedPresent_AdjustsNextScreenTimeAndFlipDelay)
        {
            // Mirrors AdjustScreenTimeForCollapsedPresentNV behavior:
            // When current frame's screenTime > nextFrame's screenTime and current has flipDelay,
            // the next frame's screenTime and flipDelay are adjusted upward.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // First frame: collapsed present with significant flipDelay
            // Its adjusted screenTime will be later than the next frame's raw screenTime
            FrameData first{};
            first.presentStartTime = 4'000'000;
            first.timeInPresent = 50'000;
            first.readyTime = 4'100'000;
            first.flipDelay = 200'000;  // 0.02ms at 10MHz
            first.setFinalState(PresentResult::Presented);
            // First's screen time is 5'500'000
            first.displayed.push_back({ FrameType::Application, 5'500'000 });

            // Second frame (next displayed)
            FrameData second{};
            second.presentStartTime = 5'000'000;
            second.timeInPresent = 40'000;
            second.readyTime = 5'100'000;
            second.flipDelay = 100'000;  // Original flip delay for second frame
            second.setFinalState(PresentResult::Presented);
            // Second's raw screen time is 5'000'000, which is EARLIER than first's (5'500'000)
            // This triggers NV2 adjustment
            second.displayed.push_back({ FrameType::Application, 5'000'000 });

            // Process first frame with second as nextDisplayed
            auto resultsFirst = ComputeMetricsForPresent(qpc, first, &second, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            // Now process second frame (which should have been adjusted by NV2)
            FrameData third{};
            third.setFinalState(PresentResult::Presented);
            third.displayed.push_back({ FrameType::Application, 6'000'000 });

            auto resultsSecond = ComputeMetricsForPresent(qpc, second, &third, chain);
            Assert::AreEqual(size_t(1), resultsSecond.size());
            const auto& secondMetrics = resultsSecond[0].metrics;

            // NV2 adjustment: second's screenTime should be raised to first's screenTime
            // when first.screenTime (5'500'000) > second.screenTime (5'000'000)
            Assert::AreEqual(uint64_t(5'500'000), secondMetrics.screenTimeQpc,
                L"NV2 should adjust second's screenTime to first's screenTime (5'500'000)");

            // NV2 adjustment: second's flipDelay should be increased by the difference
            // effectiveSecondFlipDelay = 100'000 + (5'500'000 - 5'000'000) = 100'000 + 500'000 = 600'000
            uint64_t expectedEffectiveFlipDelaySecond = 100'000 + (5'500'000 - 5'000'000);
            double expectedMsFlipDelaySecond = qpc.DurationMilliSeconds(expectedEffectiveFlipDelaySecond);

            Assert::IsTrue(secondMetrics.msFlipDelay.has_value(),
                L"msFlipDelay should be set for displayed frame");
            if (secondMetrics.msFlipDelay.has_value()) {
                Assert::AreEqual(expectedMsFlipDelaySecond, secondMetrics.msFlipDelay.value(), 0.0001,
                    L"NV2 should adjust second's flipDelay to account for screenTime catch-up");
            }
        }

        TEST_METHOD(NvCollapsedPresent_NoCollapse_ScreenTimesAndFlipDelaysUnchanged)
        {
            // Sanity check: when there is NO collapsed present condition,
            // screen times and flip delays should pass through unchanged.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior displayed frame with screen time and flip delay
            chain.lastDisplayedScreenTime = 3'000'000;
            chain.lastDisplayedFlipDelay = 50'000;

            // Current frame with LATER screen time (no collapse)
            FrameData current{};
            current.presentStartTime = 4'000'000;
            current.timeInPresent = 50'000;
            current.readyTime = 4'100'000;
            current.flipDelay = 75'000;
            current.setFinalState(PresentResult::Presented);
            // Current screen time is LATER than lastDisplayedScreenTime, so no NV1 adjustment
            current.displayed.push_back({ FrameType::Application, 4'000'000 });

            FrameData next{};
            next.setFinalState(PresentResult::Presented);
            next.displayed.push_back({ FrameType::Application, 5'000'000 });

            auto results = ComputeMetricsForPresent(qpc, current, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& metrics = results[0].metrics;

            // No NV1 adjustment: screenTime should remain unchanged
            Assert::AreEqual(uint64_t(4'000'000), metrics.screenTimeQpc,
                L"No collapse: screenTime should remain at original value");

            // No adjustment to flipDelay: should use original 75'000
            double expectedMsFlipDelay = qpc.DurationMilliSeconds(75'000);

            Assert::IsTrue(metrics.msFlipDelay.has_value(),
                L"msFlipDelay should be set for displayed frame");
            if (metrics.msFlipDelay.has_value()) {
                Assert::AreEqual(expectedMsFlipDelay, metrics.msFlipDelay.value(), 0.0001,
                    L"No collapse: flipDelay should remain at original value");
            }
        }

        TEST_METHOD(NvCollapsedPresent_OnlyAdjustsWhenFirstScreenTimeGreaterThanSecond)
        {
            // NV2 should only adjust when first.screenTime > second.screenTime.
            // This test verifies that when second.screenTime >= first.screenTime,
            // no adjustment occurs.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // First frame with flip delay
            FrameData first{};
            first.presentStartTime = 4'000'000;
            first.timeInPresent = 50'000;
            first.readyTime = 4'100'000;
            first.flipDelay = 100'000;
            first.setFinalState(PresentResult::Presented);
            // First screen time is 5'000'000
            first.displayed.push_back({ FrameType::Application, 5'000'000 });

            // Second frame with screen time >= first (no collapse condition)
            FrameData second{};
            second.presentStartTime = 5'000'000;
            second.timeInPresent = 40'000;
            second.readyTime = 5'100'000;
            second.flipDelay = 50'000;
            second.setFinalState(PresentResult::Presented);
            // Second screen time is equal to first (5'000'000), so NV2 should NOT adjust
            second.displayed.push_back({ FrameType::Application, 5'000'000 });

            auto resultsFirst = ComputeMetricsForPresent(qpc, first, &second, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            FrameData third{};
            third.setFinalState(PresentResult::Presented);
            third.displayed.push_back({ FrameType::Application, 6'000'000 });

            auto resultsSecond = ComputeMetricsForPresent(qpc, second, &third, chain);
            Assert::AreEqual(size_t(1), resultsSecond.size());
            const auto& secondMetrics = resultsSecond[0].metrics;

            // NV2 should NOT adjust: second's screenTime should remain at 5'000'000
            Assert::AreEqual(uint64_t(5'000'000), secondMetrics.screenTimeQpc,
                L"NV2: when second.screenTime >= first.screenTime, no adjustment should occur");

            // flipDelay should remain at original 50'000
            double expectedMsFlipDelay = qpc.DurationMilliSeconds(50'000);

            Assert::IsTrue(secondMetrics.msFlipDelay.has_value(),
                L"msFlipDelay should be set for displayed frame");
            if (secondMetrics.msFlipDelay.has_value()) {
                Assert::AreEqual(expectedMsFlipDelay, secondMetrics.msFlipDelay.value(), 0.0001,
                    L"NV2: when no collapse, flipDelay should remain unchanged");
            }
        }
    };
}