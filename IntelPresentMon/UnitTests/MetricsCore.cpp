// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <CommonUtilities/qpc.h>
#include <CommonUtilities/mc/MetricsTypes.h>
#include <CommonUtilities/mc/MetricsCalculator.h>
#include <CommonUtilities/mc/SwapChainCoreState.h>
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
}