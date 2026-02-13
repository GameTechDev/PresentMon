// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <CommonUtilities/qpc.h>
#include <CommonUtilities/mc/MetricsTypes.h>
#include <CommonUtilities/mc/MetricsCalculator.h>
#include <CommonUtilities/mc/SwapChainState.h>
#include <CommonUtilities/mc/UnifiedSwapChain.h>
#include <CommonUtilities/Math.h>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace pmon::util::metrics;
using namespace pmon::util;

namespace MetricsCoreTests
{
    // ============================================================================
    // SECTION 1: Core Types & Foundation
    // ============================================================================


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
            present.displayed.Clear();

            auto result = DisplayIndexing::Calculate(present, nullptr);

            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);
            Assert::AreEqual(size_t(0), result.appIndex);  // No displays → appIndex = 0
            Assert::IsFalse(result.hasNextDisplayed);
        }

        TEST_METHOD(Calculate_SingleDisplay_NoNext_Postponed)
        {
            FrameData present{};
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.finalState = PresentResult::Presented;

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
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.displayed.PushBack({ FrameType::Repeated, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            present.finalState = PresentResult::Presented;

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
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.displayed.PushBack({ FrameType::Repeated, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            present.finalState = PresentResult::Presented;

            FrameData next{};
            next.displayed.PushBack({ FrameType::Application, 4000 });

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
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.displayed.PushBack({ FrameType::Repeated, 2000 });
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
            present.displayed.PushBack({ FrameType::Repeated, 1000 });
            present.displayed.PushBack({ FrameType::Application, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            present.finalState = PresentResult::Presented;

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);
            Assert::AreEqual(size_t(1), result.appIndex);  // App at index 1
        }

        TEST_METHOD(Calculate_FindsAppFrameIndex_NotDisplayed)
        {
            FrameData present{};
            present.displayed.PushBack({ FrameType::Repeated, 1000 });
            present.displayed.PushBack({ FrameType::Application, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            // Not displayed

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Not displayed → empty range
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(0), result.endIndex);
        }

        TEST_METHOD(Calculate_AllRepeatedFrames_AppIndexInvalid)
        {
            FrameData present{};
            present.displayed.PushBack({ FrameType::Repeated, 1000 });
            present.displayed.PushBack({ FrameType::Repeated, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            present.finalState = PresentResult::Presented;

            auto result = DisplayIndexing::Calculate(present, nullptr);

            // Process [0..1], postpone [2]
            Assert::AreEqual(size_t(0), result.startIndex);
            Assert::AreEqual(size_t(2), result.endIndex);
            Assert::AreEqual(SIZE_MAX, result.appIndex);  // No app frame found
        }

        TEST_METHOD(Calculate_MultipleAppFrames_FindsFirst)
        {
            FrameData present{};
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.displayed.PushBack({ FrameType::Application, 2000 });
            present.displayed.PushBack({ FrameType::Repeated, 3000 });
            present.finalState = PresentResult::Presented;

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
            present.displayed.PushBack({ FrameType::Application, 1000 });
            present.finalState = PresentResult::Presented;

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

    TEST_CLASS(CalculateAnimationErrorSimStartTimeTests)
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

            auto result = CalculateAnimationErrorSimStartTime(swapChain, current, AnimationErrorSource::CpuStart);

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

            auto result = CalculateAnimationErrorSimStartTime(swapChain, current, AnimationErrorSource::AppProvider);

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

            auto result = CalculateAnimationErrorSimStartTime(swapChain, current, AnimationErrorSource::PCLatency);

            // Should use pclSimStartTime
            Assert::AreEqual(6000ull, result);
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
        const DisplayedVector& displayed,
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

    
TEST_CLASS(UnifiedSwapChainTests)
{
public:
    TEST_METHOD(Enqueue_V2_SeedsFirstPresent_ReturnsNoReady)
    {
        UnifiedSwapChain u{};

        // First present seeds baseline (no pipeline output).
        auto seed = MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {});
        auto out = u.Enqueue(std::move(seed), MetricsVersion::V2);

        Assert::AreEqual(size_t(0), out.size());
        Assert::IsTrue(u.swapChain.lastPresent.has_value());
        Assert::AreEqual(uint64_t(1'000'000), u.GetLastPresentQpc());
    }

    TEST_METHOD(Enqueue_V2_NotDisplayed_NoWaiting_ReturnsSingleOwnedItem)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        auto p = MakeFrame(static_cast<PresentResult>(9999), 2'000'000, 10, 2'000'010, {}); // not displayed
        auto out = u.Enqueue(std::move(p), MetricsVersion::V2);

        Assert::AreEqual(size_t(1), out.size());
        Assert::IsNull(out[0].presentPtr);
        Assert::IsNull(out[0].nextDisplayedPtr);
        Assert::AreEqual(uint64_t(2'000'000), out[0].present.presentStartTime);
    }

    TEST_METHOD(Enqueue_V2_Displayed_FirstDisplayed_ReturnsCurrentDisplayedPtrItemOnly)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        auto displayed = MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                                  { { FrameType::Application, 2'500'000 } });

        auto out = u.Enqueue(std::move(displayed), MetricsVersion::V2);

        Assert::AreEqual(size_t(1), out.size());
        Assert::IsNotNull(out[0].presentPtr);
        Assert::IsNull(out[0].nextDisplayedPtr);
        Assert::AreEqual(uint64_t(2'000'000), out[0].presentPtr->presentStartTime);
    }

    TEST_METHOD(Enqueue_V2_NotDisplayed_WithWaiting_IsBufferedUntilNextDisplayed)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        // First displayed => enters waitingDisplayed, produces current item (but may postpone metrics).
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                                 { { FrameType::Application, 2'500'000 } }), MetricsVersion::V2);

        // Not displayed while waiting => no ready work.
        auto out1 = u.Enqueue(MakeFrame(static_cast<PresentResult>(9999), 2'200'000, 10, 2'200'010, {}), MetricsVersion::V2);
        Assert::AreEqual(size_t(0), out1.size());

        // Next displayed => releases blocked.
        auto out2 = u.Enqueue(MakeFrame(PresentResult::Presented, 3'000'000, 10, 3'000'010,
                                       { { FrameType::Application, 3'500'000 } }), MetricsVersion::V2);

        // finalize previous + blocked + current
        Assert::AreEqual(size_t(3), out2.size());
        Assert::AreEqual(uint64_t(2'000'000), out2[0].present.presentStartTime); // finalize previous
        Assert::AreEqual(uint64_t(2'200'000), out2[1].present.presentStartTime); // released blocked
        Assert::IsNotNull(out2[2].presentPtr);                                   // current displayed
        Assert::AreEqual(uint64_t(3'000'000), out2[2].presentPtr->presentStartTime);
    }

    TEST_METHOD(Enqueue_V2_Displayed_WithWaiting_OrdersFinalizeThenBlockedThenCurrent)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        // Displayed A enters waiting
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                                 { { FrameType::Application, 2'500'000 } }), MetricsVersion::V2);

        // Buffer B and C
        (void)u.Enqueue(MakeFrame(static_cast<PresentResult>(9999), 2'100'000, 10, 2'100'010, {}), MetricsVersion::V2);
        (void)u.Enqueue(MakeFrame(static_cast<PresentResult>(9999), 2'200'000, 10, 2'200'010, {}), MetricsVersion::V2);

        // Next displayed D triggers: finalize A, then B,C, then current D
        auto out = u.Enqueue(MakeFrame(PresentResult::Presented, 3'000'000, 10, 3'000'010,
                                      { { FrameType::Application, 3'500'000 } }), MetricsVersion::V2);

        Assert::AreEqual(size_t(4), out.size());
        Assert::AreEqual(uint64_t(2'000'000), out[0].present.presentStartTime);
        Assert::IsNotNull(out[0].nextDisplayedPtr);
        Assert::AreEqual(uint64_t(3'000'000), out[0].nextDisplayedPtr->presentStartTime);

        Assert::AreEqual(uint64_t(2'100'000), out[1].present.presentStartTime);
        Assert::AreEqual(uint64_t(2'200'000), out[2].present.presentStartTime);

        Assert::IsNotNull(out[3].presentPtr);
        Assert::AreEqual(uint64_t(3'000'000), out[3].presentPtr->presentStartTime);
    }

    TEST_METHOD(Enqueue_V2_SanitizeDisplayed_RemovesAppThenRepeated)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        auto p = MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                           { { FrameType::Application, 2'500'000 },
                             { FrameType::Repeated,    2'700'000 } });

        auto out = u.Enqueue(std::move(p), MetricsVersion::V2);

        Assert::AreEqual(size_t(1), out.size());
        Assert::IsNotNull(out[0].presentPtr);

        Assert::AreEqual(size_t(1), out[0].presentPtr->displayed.Size());
        Assert::AreEqual((int)FrameType::Application, (int)out[0].presentPtr->displayed[0].first);
    }

    TEST_METHOD(Enqueue_V2_SanitizeDisplayed_RemovesRepeatedThenApp)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        auto p = MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                           { { FrameType::Repeated,    2'400'000 },
                             { FrameType::Application, 2'500'000 } });

        auto out = u.Enqueue(std::move(p), MetricsVersion::V2);

        Assert::AreEqual(size_t(1), out.size());
        Assert::IsNotNull(out[0].presentPtr);

        Assert::AreEqual(size_t(1), out[0].presentPtr->displayed.Size());
        Assert::AreEqual((int)FrameType::Application, (int)out[0].presentPtr->displayed[0].first);
    }

    TEST_METHOD(Pipeline_V2_PostponedLastDisplayInstance_EmittedOnFinalize)
    {
        QpcConverter qpc(10'000'000, 0);
        UnifiedSwapChain u{};

        // Seed history
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2);

        // First displayed has two instances: app then repeated.
        auto outA = u.Enqueue(MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                                       { { FrameType::Intel_XEFG,   2'500'000 },
                                         { FrameType::Application,  2'700'000 } }),
                             MetricsVersion::V2);

        Assert::AreEqual(size_t(1), outA.size());
        Assert::IsNotNull(outA[0].presentPtr);

        // Processing current displayed without next: should produce all-but-last => one result at 2'500'000.
        auto resA = ComputeMetricsForPresent(qpc, *outA[0].presentPtr, nullptr, u.swapChain, MetricsVersion::V2);
        Assert::AreEqual(size_t(1), resA.size());
        Assert::AreEqual(uint64_t(2'500'000), resA[0].metrics.screenTimeQpc);

        // Next displayed triggers finalize of the previous displayed (postponed last instance at 2'700'000).
        auto outB = u.Enqueue(MakeFrame(PresentResult::Presented, 3'000'000, 10, 3'000'010,
                                       { { FrameType::Application, 3'500'000 } }),
                             MetricsVersion::V2);

        Assert::AreEqual(size_t(2), outB.size());
        Assert::IsNotNull(outB[0].nextDisplayedPtr);

        auto resFinalize = ComputeMetricsForPresent(qpc, outB[0].present, outB[0].nextDisplayedPtr, u.swapChain, MetricsVersion::V2);
        Assert::AreEqual(size_t(1), resFinalize.size());
        Assert::AreEqual(uint64_t(2'700'000), resFinalize[0].metrics.screenTimeQpc);
    }

    TEST_METHOD(Pipeline_V2_NvCollapsed_AdjustmentPersistsViaNextDisplayedPtr)
    {
        QpcConverter qpc(10'000'000, 0);
        UnifiedSwapChain u{};

        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2);

        // First displayed: collapsed/runt-style (flipDelay set), screenTime is later than next's raw screenTime.
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 4'000'000, 50'000, 4'100'000,
                                 { { FrameType::Application, 5'500'000 } },
                                 0, 0, 200'000),
                        MetricsVersion::V2);

        // Second displayed: raw screenTime earlier -> should be adjusted upward by NV2 when finalizing first.
        auto out = u.Enqueue(MakeFrame(PresentResult::Presented, 5'000'000, 40'000, 5'100'000,
                                      { { FrameType::Application, 5'000'000 } },
                                      0, 0, 100'000),
                             MetricsVersion::V2);

        // Expect: finalize previous + current displayed
        Assert::AreEqual(size_t(2), out.size());
        Assert::IsNotNull(out[0].nextDisplayedPtr);
        Assert::IsNotNull(out[1].presentPtr);

        // Finalize first with look-ahead to second => mutates second via pointer.
        (void)ComputeMetricsForPresent(qpc, out[0].present, out[0].nextDisplayedPtr, u.swapChain, MetricsVersion::V2);

        // Mutation must persist on swapchain-owned second frame.
        Assert::AreEqual(uint64_t(5'500'000), out[1].presentPtr->displayed[0].second);
        Assert::AreEqual(uint64_t(100'000 + (5'500'000 - 5'000'000)), out[1].presentPtr->flipDelay);
    }

    TEST_METHOD(Pipeline_V1_NvCollapsed_AdjustsCurrentPresent)
    {
        QpcConverter qpc(10'000'000, 0);
        UnifiedSwapChain u{};

        // Establish previous displayed state (lastDisplayedScreenTime/flipDelay) via first displayed.
        auto out1 = u.Enqueue(MakeFrame(PresentResult::Presented, 4'000'000, 50'000, 4'100'000,
                                       { { FrameType::Application, 5'500'000 } },
                                       0, 0, 200'000),
                             MetricsVersion::V1);

        Assert::AreEqual(size_t(1), out1.size());

        (void)ComputeMetricsForPresent(qpc, out1[0].present, nullptr, u.swapChain, MetricsVersion::V1);

        // Second present has earlier raw screenTime; NV1 should adjust *current* present to lastDisplayedScreenTime.
        auto out2 = u.Enqueue(MakeFrame(PresentResult::Presented, 5'000'000, 40'000, 5'100'000,
                                       { { FrameType::Application, 5'000'000 } },
                                       0, 0, 100'000),
                             MetricsVersion::V1);

        Assert::AreEqual(size_t(1), out2.size());

        (void)ComputeMetricsForPresent(qpc, out2[0].present, nullptr, u.swapChain, MetricsVersion::V1);

        Assert::AreEqual(uint64_t(5'500'000), out2[0].present.displayed[0].second);
        Assert::AreEqual(uint64_t(100'000 + (5'500'000 - 5'000'000)), out2[0].present.flipDelay);
    }

    TEST_METHOD(Pipeline_V1_NoNvCollapse_DoesNotModifyCurrentPresent)
    {
        QpcConverter qpc(10'000'000, 0);
        UnifiedSwapChain u{};

        // Prior displayed state via first displayed.
        auto out1 = u.Enqueue(MakeFrame(PresentResult::Presented, 4'000'000, 50'000, 4'100'000,
                                       { { FrameType::Application, 5'000'000 } },
                                       0, 0, 200'000),
                             MetricsVersion::V1);
        (void)ComputeMetricsForPresent(qpc, out1[0].present, nullptr, u.swapChain, MetricsVersion::V1);

        // Current has later screenTime => no collapse.
        auto out2 = u.Enqueue(MakeFrame(PresentResult::Presented, 5'000'000, 40'000, 5'100'000,
                                       { { FrameType::Application, 6'000'000 } },
                                       0, 0, 100'000),
                             MetricsVersion::V1);

        // Preserve originals for comparison.
        const uint64_t origScreen = out2[0].present.displayed[0].second;
        const uint64_t origFlipDelay = out2[0].present.flipDelay;

        (void)ComputeMetricsForPresent(qpc, out2[0].present, nullptr, u.swapChain, MetricsVersion::V1);

        Assert::AreEqual(origScreen, out2[0].present.displayed[0].second);
        Assert::AreEqual(origFlipDelay, out2[0].present.flipDelay);
    }

    TEST_METHOD(Enqueue_V1_ClearsV2Buffers_AndIsAlwaysReady)
    {
        UnifiedSwapChain u{};
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}), MetricsVersion::V2); // seed

        // Create V2 waitingDisplayed + blocked.
        (void)u.Enqueue(MakeFrame(PresentResult::Presented, 2'000'000, 10, 2'000'010,
                                 { { FrameType::Application, 2'500'000 } }), MetricsVersion::V2);
        (void)u.Enqueue(MakeFrame(static_cast<PresentResult>(9999), 2'200'000, 10, 2'200'010, {}), MetricsVersion::V2);

        // V1 enqueue must clear V2 buffers and return one ready item.
        auto outV1 = u.Enqueue(MakeFrame(static_cast<PresentResult>(9999), 2'300'000, 10, 2'300'010, {}), MetricsVersion::V1);
        Assert::AreEqual(size_t(1), outV1.size());

        // Next V2 displayed should behave as "no waiting/no blocked": returns only current displayed item.
        auto outV2 = u.Enqueue(MakeFrame(PresentResult::Presented, 3'000'000, 10, 3'000'010,
                                        { { FrameType::Application, 3'500'000 } }), MetricsVersion::V2);

        Assert::AreEqual(size_t(1), outV2.size());
        Assert::IsNotNull(outV2[0].presentPtr);
        Assert::IsNull(outV2[0].nextDisplayedPtr);
    }
};


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
        
        TEST_METHOD(ComputeMetricsForPresent_IntelXefg_Discarded_NoNext_ChainNotUpdated)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // 3x Intel_XEFG then 1 Application; no nextDisplayed
            FrameData present = MakeFrame(
                PresentResult::Discarded,
                30'000, 700, 40'000,
                {
                    { FrameType::Intel_XEFG, 0 },
                });

            auto metrics = ComputeMetricsForPresent(qpc, present, nullptr, chain);

            // Should process 1 
            Assert::AreEqual(size_t(1), metrics.size());
            // Chain update postponed until nextDisplayed
            const auto& m = metrics[0].metrics;
            Assert::IsTrue(FrameType::Intel_XEFG == m.frameType, L"FrameType should be Intel_XEFG");
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
            first.gpuStartTime = 1'200'000; // 0.12s

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

            // msUntilRenderStart = delta between PresentStart and GPU start
            double expectedMsUntilRenderStart =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, first.gpuStartTime);
            Assert::AreEqual(
                expectedMsUntilRenderStart,
                firstMetrics.msUntilRenderStart,
                0.0001,
                L"msUntilRenderStart should equal delta from PresentStartTime to GPUStartTime.");

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
            second.gpuStartTime = 1'220'000; // 0.122s

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

            // msInPresentApi / msUntilRenderComplete / msUntilRenderStart for second
            double expectedMsInPresentSecond = qpc.DurationMilliSeconds(second.timeInPresent);
            double expectedMsUntilRenderCompleteSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.readyTime);
            double expectedMsUntilRenderStartSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.gpuStartTime);

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
            Assert::AreEqual(
                expectedMsUntilRenderStartSecond,
                secondMetrics.msUntilRenderStart,
                0.0001,
                L"Second frame msUntilRenderStart should match start→GPU start delta.");

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
                DisplayedVector{
                    { FrameType::Application, 2'000'000 }   // one displayed instance
            });
            second.gpuStartTime = 1'200'000;

            // Dummy nextDisplayed with at least one display so the "with next" path is taken
            FrameData nextDisplayed = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 2'100'000,
                /*timeInPresent*/    100'000,
                /*readyTime*/        2'200'000,
                DisplayedVector{
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

            // msUntilRenderStart from start → GPU start
            double expectedMsUntilRenderStartSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.gpuStartTime);
            Assert::AreEqual(
                expectedMsUntilRenderStartSecond,
                secondMetrics.msUntilRenderStart,
                0.0001,
                L"msUntilRenderStart should match start→GPU start delta for displayed frame.");
            
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
            frame.finalState = PresentResult::Presented;
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
            frame.finalState = PresentResult::Presented;
            // Single displayed; will be postponed unless nextDisplayed provided
            frame.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData next{}; // provide nextDisplayed to process postponed
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 3'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.presentStartTime, frame.displayed[0].second);
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
            frame.finalState = PresentResult::Presented;
            // Displayed generated frame (e.g., Repeated/Composed/Desktop depending on enum)
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 5'100'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.presentStartTime, frame.displayed[0].second);
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
            frame.finalState = PresentResult::Presented;

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'800'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 3'100'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 3'400'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 3'700'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 4'000'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'000'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'000'000 });

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
            frame.finalState = PresentResult::Presented;

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 5'800'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 6'100'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'400'000 });

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
            frame.finalState = PresentResult::Presented;

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 7'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 8'000'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 7'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 8'000'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Repeated, 7'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 8'000'000 });

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
            frame.finalState = PresentResult::Presented;

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 9'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 10'000'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 9'500'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 9'800'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 10'100'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 10'400'000 });

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
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Repeated, 9'700'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 10'000'000 });

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
            first.finalState = PresentResult::Presented;
            // First's screen time is 5'500'000
            first.displayed.PushBack({ FrameType::Application, 5'500'000 });

            // Second frame (next displayed)
            FrameData second{};
            second.presentStartTime = 5'000'000;
            second.timeInPresent = 40'000;
            second.readyTime = 5'100'000;
            second.flipDelay = 100'000;  // Original flip delay for second frame
            second.finalState = PresentResult::Presented;
            // Second's raw screen time is 5'000'000, which is EARLIER than first's (5'500'000)
            // This triggers NV2 adjustment
            second.displayed.PushBack({ FrameType::Application, 5'000'000 });

            // Process first frame with second as nextDisplayed
            auto resultsFirst = ComputeMetricsForPresent(qpc, first, &second, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            // Now process second frame (which should have been adjusted by NV2)
            FrameData third{};
            third.finalState = PresentResult::Presented;
            third.displayed.PushBack({ FrameType::Application, 6'000'000 });

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
            current.finalState = PresentResult::Presented;
            // Current screen time is LATER than lastDisplayedScreenTime, so no NV1 adjustment
            current.displayed.PushBack({ FrameType::Application, 4'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 5'000'000 });

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
            first.finalState = PresentResult::Presented;
            // First screen time is 5'000'000
            first.displayed.PushBack({ FrameType::Application, 5'000'000 });

            // Second frame with screen time >= first (no collapse condition)
            FrameData second{};
            second.presentStartTime = 5'000'000;
            second.timeInPresent = 40'000;
            second.readyTime = 5'100'000;
            second.flipDelay = 50'000;
            second.finalState = PresentResult::Presented;
            // Second screen time is equal to first (5'000'000), so NV2 should NOT adjust
            second.displayed.PushBack({ FrameType::Application, 5'000'000 });

            auto resultsFirst = ComputeMetricsForPresent(qpc, first, &second, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            FrameData third{};
            third.finalState = PresentResult::Presented;
            third.displayed.PushBack({ FrameType::Application, 6'000'000 });

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


        TEST_METHOD(NvCollapsedPresent_V1_AdjustsCurrentScreenTimeAndFlipDelay)
        {
            // Legacy PresentMon V1 behavior: when the previous displayed screen time (adjusted by flipDelay)
            // is greater than the current present's screen time, treat the current as a collapsed/runt frame
            // and adjust *this* present's screen time + flipDelay.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            chain.lastDisplayedScreenTime = 5'500'000;
            chain.lastDisplayedFlipDelay = 50'000;

            FrameData current{};
            current.presentStartTime = 4'000'000;
            current.timeInPresent = 50'000;
            current.readyTime = 4'100'000;
            current.flipDelay = 100'000;
            current.finalState = PresentResult::Presented;
            current.displayed.PushBack({ FrameType::Application, 5'000'000 });

            auto results = ComputeMetricsForPresent(qpc, current, nullptr, chain, MetricsVersion::V1);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(5'500'000), m.screenTimeQpc,
                L"NV1 should adjust current screenTime to lastDisplayedScreenTime");

            const uint64_t expectedFlipDelay = 100'000 + (5'500'000 - 5'000'000);
            Assert::IsTrue(m.msFlipDelay.has_value(), L"msFlipDelay should be set for displayed frame");
            if (m.msFlipDelay.has_value()) {
                Assert::AreEqual(qpc.DurationMilliSeconds(expectedFlipDelay), m.msFlipDelay.value(), 0.0001,
                    L"NV1 should adjust current flipDelay to account for screenTime catch-up");
            }

            // Validate the legacy-style mutation of the current present and that chain advanced using adjusted values.
            Assert::AreEqual(uint64_t(5'500'000), current.displayed[0].second,
                L"NV1 should update current.displayed[0].second");
            Assert::AreEqual(expectedFlipDelay, current.flipDelay,
                L"NV1 should update current.flipDelay");
            Assert::AreEqual(uint64_t(5'500'000), chain.lastDisplayedScreenTime,
                L"Chain should latch adjusted screenTime");
        }

    };
    TEST_CLASS(DisplayLatencyTests)
    {
    public:
        TEST_METHOD(DisplayLatency_SimpleCase_PositiveDelta)
        {
            // Scenario: Single displayed frame with well-separated timestamps.
            // cpuStart = 1'000'000, screenTime = 2'000'000
            // QPC frequency 10 MHz → 1'000'000 ticks = 0.1 ms
            // Expected: msDisplayLatency ≈ 0.1 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            // Set up chain with prior app present to establish cpuStart
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msDisplayLatency = screenTime - cpuStart = 2'000'000 - 1'000'000 = 1'000'000 ticks = 0.1 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'000'000);
            Assert::AreEqual(expected, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_CpuStartEqualsScreenTime)
        {
            // Scenario: CPU work finishes exactly when frame displays (degenerate case).
            // cpuStart = screenTime = 2'000'000
            // Expected: msDisplayLatency = 0.0 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData priorApp{};
            priorApp.presentStartTime = 1'700'000;
            priorApp.timeInPresent = 300'000;
            chain.lastAppPresent = priorApp;

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 1'700'000 + 300'000 = 2'000'000
            // msDisplayLatency = 2'000'000 - 2'000'000 = 0
            Assert::AreEqual(0.0, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_NotDisplayed_ReturnsZero)
        {
            // Scenario: Frame with no displayed entries (not displayed).
            // Expected: msDisplayLatency = 0.0 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            // No displayed entries

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(0.0, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_ZeroCpuStart)
        {
            // Scenario: No prior chain history; cpuStart defaults to 0.
            // cpuStart = 0, screenTime = 3'000'000
            // Expected: msDisplayLatency ≈ 0.3 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            // No prior app present set

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 3'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 3'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 0 (no prior app present)
            // msDisplayLatency = 3'000'000 - 0 = 3'000'000 ticks = 0.3 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 3'000'000);
            Assert::AreEqual(expected, m.msDisplayLatency, 0.0001);
        }
    };

    TEST_CLASS(ReadyTimeToDisplayLatencyTests)
    {
    public:
        TEST_METHOD(ReadyTimeToDisplay_SimpleCase_PositiveDelta)
        {
            // Scenario: Single displayed frame with GPU ready time before screen time.
            // readyTime = 1'500'000, screenTime = 2'000'000
            // QPC 10 MHz: 500'000 ticks = 0.05 ms
            // Expected: msReadyTimeToDisplayLatency ≈ 0.05 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'500'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // msReadyTimeToDisplayLatency = screenTime - readyTime = 2'000'000 - 1'500'000 = 500'000 ticks = 0.05 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'000'000);
            Assert::IsTrue(m.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expected, m.msReadyTimeToDisplayLatency.value(), 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_ReadyTimeEqualsScreenTime)
        {
            // Scenario: GPU finishes exactly when frame displays.
            // readyTime = screenTime = 2'000'000
            // Expected: msReadyTimeToDisplayLatency = 0.0 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 2'000'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsTrue(m.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(0.0, m.msReadyTimeToDisplayLatency.value(), 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_NotDisplayed_ReturnsZero)
        {
            // Scenario: Frame with no displayed entries.
            // Expected: msReadyTimeToDisplayLatency = 0.0 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'500'000;
            frame.finalState = PresentResult::Presented;
            // No displayed entries

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsFalse(m.msReadyTimeToDisplayLatency.has_value());
        }

        TEST_METHOD(ReadyTimeToDisplay_ReadyTimeZero)
        {
            // Scenario: Ready time not set (edge case, readyTime = 0).
            // readyTime = 70'000, screenTime = 2'000'000
            // Expected: msReadyTimeToDisplayLatency ≈ 0.2 ms (2'000'000 ticks)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 70'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // msReadyTimeToDisplayLatency = 2'000'000 - 70'000 = 1'930'000 ticks = 0.193 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(70'000, 2'000'000);
            Assert::IsTrue(m.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expected, m.msReadyTimeToDisplayLatency.value(), 0.0001);
        }
    };

    TEST_CLASS(MultiDisplayLatencyTests)
    {
    public:
        TEST_METHOD(DisplayLatency_MultipleDisplays_EachComputesIndependently)
        {
            // Scenario: Single FrameData with 3 displayed instances (e.g., frame interpolation).
            // Display 0: screenTime = 2'000'000
            // Display 1: screenTime = 2'100'000
            // Display 2: screenTime = 2'200'000
            // cpuStart = 1'000'000 (same for all)
            // QPC 10 MHz
            // Expected:
            // Metrics[0].msDisplayLatency ≈ 0.1 ms
            // Metrics[1].msDisplayLatency ≈ 0.11 ms
            // Metrics[2].msDisplayLatency ≈ 0.12 ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 2'100'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 2'200'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            // First call without next: process [0..1]
            auto results1 = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), results1.size());

            // First display: cpuStart = 1'000'000, screenTime = 2'000'000 → 0.1 ms
            double expected0 = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'000'000);
            Assert::AreEqual(expected0, results1[0].metrics.msDisplayLatency, 0.0001);

            // Second display: cpuStart = 1'000'000, screenTime = 2'100'000 → 0.11 ms
            double expected1 = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'100'000);
            Assert::AreEqual(expected1, results1[1].metrics.msDisplayLatency, 0.0001);

            // Second call with next: process [2]
            auto results2 = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results2.size());

            // Third display: cpuStart = 1'000'000, screenTime = 2'200'000 → 0.12 ms
            double expected2 = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'200'000);
            Assert::AreEqual(expected2, results2[0].metrics.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_MultipleDisplays_IndependentDeltas)
        {
            // Scenario: Multiple displays, each with different screenTime, but same readyTime.
            // readyTime = 1'500'000 (single value for the frame)
            // Display 0: screenTime = 2'000'000
            // Display 1: screenTime = 2'100'000
            // Display 2: screenTime = 2'200'000
            // QPC 10 MHz
            // Expected:
            // Metrics[0].msReadyTimeToDisplayLatency ≈ 0.05 ms (500'000 ticks)
            // Metrics[1].msReadyTimeToDisplayLatency ≈ 0.06 ms (600'000 ticks)
            // Metrics[2].msReadyTimeToDisplayLatency ≈ 0.07 ms (700'000 ticks)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'500'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 2'100'000 });
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 2'200'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(2), results.size());

            // Display 0: readyTime = 1'500'000, screenTime = 2'000'000 → 0.05 ms
            double expected0 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'000'000);
            Assert::IsTrue(results[0].metrics.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expected0, results[0].metrics.msReadyTimeToDisplayLatency.value(), 0.0001);

            // Display 0: readyTime = 1'500'000, screenTime = 2'000'000 → 0.05 ms
            double expected1 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'100'000);
            Assert::IsTrue(results[1].metrics.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expected1, results[1].metrics.msReadyTimeToDisplayLatency.value(), 0.0001);

            // Second call with next: process [2]
            auto results2 = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results2.size());
            double expected2 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'200'000);
            Assert::IsTrue(results2[0].metrics.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expected2, results2[0].metrics.msReadyTimeToDisplayLatency.value(), 0.0001);
        }
    };

    TEST_CLASS(NvCollapsedPresentLatencyTests)
    {
    public:
        TEST_METHOD(DisplayLatency_NvCollapsed_AdjustedScreenTime)
        {
            // Scenario: NV collapse adjustment modifies screenTime before metric computation.
            // cpuStart = 1'000'000
            // Display 0: screenTime = 4'000'000
            // Display 1: screenTime = 3'000'000
            // QPC 10 MHz
            // Assume the unified code applies NV adjustment
            // Expected: msDisplayLatency ≈ 0.295 ms (using adjusted screenTime 4'000'000 − 1'050'000)
            // Expected: msFlipDelay ≈ 0.103 ms (original 30'000 + adjustment)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.flipDelay = 50'000;
            frame.finalState = PresentResult::Presented;
            // Raw screen time is 4'000'000, greater than next screen time
            frame.displayed.PushBack({ FrameType::Application, 4'000'000 });

            FrameData next1{};
            next1.presentStartTime = 2'000'000;
            next1.timeInPresent = 50'000;
            next1.readyTime = 2'100'000;
            next1.flipDelay = 30'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 3'000'000 });

            FrameData next2{};
            next2.presentStartTime = 3'000'000;
            next2.timeInPresent = 50'000;
            next2.readyTime = 3'100'000;
            next2.finalState = PresentResult::Presented;
            next2.displayed.PushBack({ FrameType::Application, 5'000'000 });

            // Set up chain with prior app present to establish cpuStart
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results1 = ComputeMetricsForPresent(qpc, frame, &next1, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // No adjust of first frame msDisplayLatency = 4'000'000 - 1'000'000 = 3'000'000 ticks = 0.3 ms
            double expectedDisplayLatency = qpc.DeltaUnsignedMilliSeconds(1'000'000, 4'000'000);
            Assert::AreEqual(expectedDisplayLatency, results1[0].metrics.msDisplayLatency, 0.0001);
            double expectedFlipDelay = qpc.DurationMilliSeconds(frame.flipDelay);
            Assert::IsTrue(results1[0].metrics.msFlipDelay.has_value());
            Assert::AreEqual(expectedFlipDelay, results1[0].metrics.msFlipDelay.value(), 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, next1, &next2, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // After NV adjustment: screenTime = 4'000'000 -> set from NV FlipDelay adjustment
            // msDisplayLatency = 4'000'000 - 1'050'000 = 2'950'000 ticks = 0.295 ms
            // msFlipDelay = original 30'000 + (4'000'000 - 3'000'000) = 1'030'000 ticks = 0.103 ms
            double expectedDisplayLatency2 = qpc.DeltaUnsignedMilliSeconds(1'050'000, 4'000'000);
            Assert::AreEqual(expectedDisplayLatency2, results2[0].metrics.msDisplayLatency, 0.0001);
            double expectedFlipDelay2 = qpc.DurationMilliSeconds(1'030'000);
            Assert::IsTrue(results2[0].metrics.msFlipDelay.has_value());
            Assert::AreEqual(expectedFlipDelay2, results2[0].metrics.msFlipDelay.value(), 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_NvCollapsed_UsesAdjustedScreenTime)
        {
            // Scenario: NV collapse adjustment affects ReadyTimeToDisplay metric.
            // Adjusted screenTime = 4'000'000
            // readyTime = 2'100'000
            // QPC 10 MHz
            // Expected: msReadyTimeToDisplayLatency ≈ 0.19 ms (4'000'000 - 2'100'000 = 1'900'000 ticks)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.flipDelay = 50'000;
            frame.finalState = PresentResult::Presented;
            // Raw screen time is 4'000'000, greater than next screen time
            frame.displayed.PushBack({ FrameType::Application, 4'000'000 });

            FrameData next1{};
            next1.presentStartTime = 2'000'000;
            next1.timeInPresent = 50'000;
            next1.readyTime = 2'100'000;
            next1.flipDelay = 30'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 3'000'000 });

            FrameData next2{};
            next2.presentStartTime = 3'000'000;
            next2.timeInPresent = 50'000;
            next2.readyTime = 3'100'000;
            next2.finalState = PresentResult::Presented;
            next2.displayed.PushBack({ FrameType::Application, 5'000'000 });

            // Set up chain with prior app present to establish cpuStart
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results1 = ComputeMetricsForPresent(qpc, frame, &next1, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // No adjust of first frame ready time = 4'000'000 - 1'100'000 = 2'900'000 ticks = 0.29 ms
            double expectedReadyTimeLatency = qpc.DeltaUnsignedMilliSeconds(1'100'000, 4'000'000);
            Assert::IsTrue(results1[0].metrics.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expectedReadyTimeLatency, results1[0].metrics.msReadyTimeToDisplayLatency.value(), 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, next1, &next2, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // After NV adjustment: ready time latency = 4'000'000 - 2'100'000 = 1'900'000 ticks = 0.19 ms
            double expectedReadyTimeLatency2 = qpc.DeltaUnsignedMilliSeconds(2'100'000, 4'000'000);
            Assert::IsTrue(results2[0].metrics.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expectedReadyTimeLatency2, results2[0].metrics.msReadyTimeToDisplayLatency.value(), 0.0001);
        }
    };

    TEST_CLASS(DisplayLatencyEdgeCasesTests)
    {
    public:
        TEST_METHOD(DisplayLatency_ScreenTimeBeforeCpuStart)
        {
            // Scenario: Timestamps appear out-of-order (screen time earlier than CPU start).
            // cpuStart = 3'000'000
            // screenTime = 2'000'000 (earlier)
            // This is unusual but should be handled gracefully (likely as 0 or negative value).

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData priorApp{};
            priorApp.presentStartTime = 2'500'000;
            priorApp.timeInPresent = 500'000;
            chain.lastAppPresent = priorApp;

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 2'500'000 + 500'000 = 3'000'000
            // screenTime = 2'000'000 (earlier than cpuStart)
            // Result should be 0 or negative (implementation dependent)
            Assert::IsTrue(m.msDisplayLatency <= 0.0 || m.msDisplayLatency == 0.0);
        }

        TEST_METHOD(ReadyTimeToDisplay_ScreenTimeBeforeReadyTime)
        {
            // Scenario: Frame displays before GPU finishes (should not happen in practice).
            // readyTime = 3'000'000
            // screenTime = 2'000'000 (earlier)
            // Expected: 0 or negative value (defensive behavior)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 3'000'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // screenTime = 2'000'000, readyTime = 3'000'000
            // Result should be 0 or negative
            Assert::IsTrue(m.msReadyTimeToDisplayLatency <= 0.0 || m.msReadyTimeToDisplayLatency == 0.0);
        }

        TEST_METHOD(DisplayLatency_FirstFrame_NoPriorAppPresent)
        {
            // Scenario: First frame in swapchain; no prior lastAppPresent in chain state.
            // cpuStart derived from lastPresent only (fallback).
            // Single display with screenTime = 2'000'000
            // Expected: msDisplayLatency computed correctly using fallback CPU start

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            // No lastAppPresent set

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // No prior present, so cpuStart = 0
            // msDisplayLatency = 2'000'000 - 0 = 2'000'000 ticks = 0.2 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 2'000'000);
            Assert::AreEqual(expected, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_FrameWithAppPropagatedData)
        {
            // Scenario: lastAppPresent has appPropagatedPresentStartTime and appPropagatedTimeInPresent set.
            // CPU start should use these.
            // appPropagatedPresentStartTime = 800'000
            // appPropagatedTimeInPresent = 150'000
            // screenTime = 2'000'000
            // Expected cpuStart = 800'000 + 150'000 = 950'000
            // Expected msDisplayLatency ≈ 0.1055 ms (2'000'000 − 950'000 = 1'050'000 ticks)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData priorApp{};
            priorApp.presentStartTime = 1'000'000;
            priorApp.timeInPresent = 200'000;
            priorApp.appPropagatedPresentStartTime = 800'000;
            priorApp.appPropagatedTimeInPresent = 150'000;
            chain.lastAppPresent = priorApp;

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 800'000 + 150'000 = 950'000
            // msDisplayLatency = 2'000'000 - 950'000 = 1'050'000 ticks = 0.105 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(950'000, 2'000'000);
            Assert::AreEqual(expected, m.msDisplayLatency, 0.0001);
        }
    };
    TEST_CLASS(CPUMetricsTests)
    {
    public:
        TEST_METHOD(CPUBusy_BasicCase_StandardPath)
        {
            // No propagated data in lastAppPresent
            // cpuStart = 1'000'000 (prior frame start + timeInPresent)
            // presentStartTime = 1'100'000
            // QPC frequency: 10 MHz
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorFrame{};
            priorFrame.presentStartTime = 800'000;
            priorFrame.timeInPresent = 200'000;
            priorFrame.readyTime = 1'100'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            chain.lastAppPresent = priorFrame;

            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'400'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msCPUBusy = 1'100'000 - 1'000'000 = 100'000 ticks = 10 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'100'000);
            Assert::AreEqual(expected, m.msCPUBusy, 0.0001);
        }

        TEST_METHOD(CPUBusy_WithAppPropagatedData)
        {
            // lastAppPresent has appPropagatedPresentStartTime set
            // We need to ensure the frame is displayed so CPU metrics are computed
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame with propagated data
            FrameData priorApp{};
            priorApp.presentStartTime = 1'000'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'200'000;
            priorApp.appPropagatedPresentStartTime = 800'000;
            priorApp.appPropagatedTimeInPresent = 200'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'300'000 });

            chain.lastAppPresent = priorApp;

            // Current frame (app frame, displayed)
            FrameData frame{};
            frame.presentStartTime = 1'500'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'600'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'700'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 80'000;
            next.readyTime = 2'100'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'200'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000 (uses appPropagated from priorApp)
            // msCPUBusy = 1'500'000 - 1'000'000 = 500'000 ticks = 50 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'500'000);
            Assert::AreEqual(expected, m.msCPUBusy, 0.0001);
        }

        TEST_METHOD(CPUBusy_FirstFrameNoPriorAppPresent)
        {
            // No lastAppPresent in chain state
            // cpuStart = 0 (default fallback)
            // presentStartTime = 5'000'000
            // Expected msCPUBusy = 5'000'000 ticks = 500 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            // No prior app present set

            // Current frame (app frame, displayed)
            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 5'200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 6'000'000;
            next.timeInPresent = 80'000;
            next.readyTime = 6'100'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'300'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 0 (no prior app present)
            // msCPUBusy = 5'000'000 - 0 = 5'000'000 ticks = 500 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 5'000'000);
            Assert::AreEqual(expected, m.msCPUBusy, 0.0001);
        }

        TEST_METHOD(CPUBusy_ZeroTimeInPresent)
        {
            // cpuStart = 1'000'000
            // presentStartTime = 1'000'000 (same as cpuStart)
            // timeInPresent = 0 (zero present duration)
            // Expected msCPUBusy = 0
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorFrame{};
            priorFrame.presentStartTime = 800'000;
            priorFrame.timeInPresent = 200'000;
            priorFrame.readyTime = 1'100'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            chain.lastAppPresent = priorFrame;

            // Current frame with zero timeInPresent
            FrameData frame{};
            frame.presentStartTime = 1'000'000;  // Same as cpuStart
            frame.timeInPresent = 0;             // Zero present duration
            frame.readyTime = 1'000'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // presentStartTime = 1'000'000 (same as cpuStart)
            // msCPUBusy = 1'000'000 - 1'000'000 = 0 ticks = 0 ms
            Assert::AreEqual(0.0, m.msCPUBusy, 0.0001);
        }

        TEST_METHOD(CPUWait_BasicCase_StandardPath)
        {
            // timeInPresent = 200'000 ticks
            // Expected msCPUWait = 200'000 / 10'000'000 = 0.02 ms = 20 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorFrame{};
            priorFrame.presentStartTime = 800'000;
            priorFrame.timeInPresent = 100'000;
            priorFrame.readyTime = 1'100'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            chain.lastAppPresent = priorFrame;

            // Current frame with timeInPresent = 200'000
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 200'000;  // 20 ms
            frame.readyTime = 1'300'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'800'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'900'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msCPUWait = 200'000 ticks = 20 ms
            double expected = qpc.DurationMilliSeconds(200'000);
            Assert::AreEqual(expected, m.msCPUWait, 0.0001);
        }

        TEST_METHOD(CPUWait_WithAppPropagatedTimeInPresent)
        {
            // appPropagatedTimeInPresent = 150'000 ticks
            // Expected msCPUWait = 150'000 / 10'000'000 = 0.015 ms = 15 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorFrame{};
            priorFrame.presentStartTime = 800'000;
            priorFrame.timeInPresent = 100'000;
            priorFrame.readyTime = 1'100'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            chain.lastAppPresent = priorFrame;

            // Current frame with appPropagatedTimeInPresent = 150'000
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 200'000;  // Regular time (not used when propagated available)
            frame.readyTime = 1'300'000;
            frame.appPropagatedTimeInPresent = 150'000;  // 15 ms (propagated value)
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'800'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'900'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // When appPropagated is available, use it instead of regular timeInPresent
            // msCPUWait = 150'000 ticks = 15 ms
            double expected = qpc.DurationMilliSeconds(150'000);
            Assert::AreEqual(expected, m.msCPUWait, 0.0001);
        }

        TEST_METHOD(CPUWait_ZeroDuration)
        {
            // timeInPresent = 0
            // Expected msCPUWait = 0 / 10'000'000 = 0 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorFrame{};
            priorFrame.presentStartTime = 800'000;
            priorFrame.timeInPresent = 100'000;
            priorFrame.readyTime = 1'100'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            chain.lastAppPresent = priorFrame;

            // Current frame with zero timeInPresent
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 0;  // Zero CPU wait time
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'200'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msCPUWait = 0 ticks = 0 ms
            Assert::AreEqual(0.0, m.msCPUWait, 0.0001);
        }

        TEST_METHOD(CPUTime_IsDerivedCorrectly)
        {
            // Verify msCPUTime = msCPUBusy + msCPUWait.
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
         
            // Prior app frame sets cpuStart = 1'000'000.
            FrameData priorFrame{};
            priorFrame.presentStartTime = 900'000;
            priorFrame.timeInPresent = 100'000;
            priorFrame.readyTime = 1'050'000;
            priorFrame.finalState = PresentResult::Presented;
            priorFrame.displayed.PushBack({ FrameType::Application, 1'100'000 });
            chain.lastAppPresent = priorFrame;
         
            // Current frame: presentStartTime=1'100'000 => msCPUBusy=10ms; timeInPresent=200'000 => msCPUWait=20ms.
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 200'000;
            frame.readyTime = 1'350'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });
         
            // Next displayed frame required to process current frame's display.
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });
         
            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
         
            double expectedBusy = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'100'000);
            double expectedWait = qpc.DurationMilliSeconds(200'000);
            double expectedCpuTime = expectedBusy + expectedWait;
         
            Assert::AreEqual(expectedBusy, m.msCPUBusy, 0.0001);
            Assert::AreEqual(expectedWait, m.msCPUWait, 0.0001);
            Assert::AreEqual(expectedCpuTime, m.msCPUTime, 0.0001);
         }
    };

    // ============================================================================
    // GROUP B: CORE GPU METRICS (NON-VIDEO)
    // ============================================================================

    TEST_CLASS(GPUMetricsNonVideoTests)
    {
    public:
        TEST_METHOD(GPULatency_BasicCase_StandardPath)
        {
            // cpuStart = 1'000'000
            // gpuStartTime = 1'050'000
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msGPULatency = 1'050'000 - 1'000'000 = 50'000 ticks = 5 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'050'000);
            Assert::AreEqual(expected, m.msGPULatency, 0.0001);
        }

        TEST_METHOD(GPULatency_WithAppPropagatedGPUStart)
        {
            // appPropagatedGPUStartTime = 1'080'000
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with app propagated GPU start
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.gpuStartTime = 1'050'000;  // Not used when propagated available
            frame.gpuDuration = 200'000;
            frame.appPropagatedGPUStartTime = 1'080'000;
            frame.appPropagatedGPUDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 1'000'000
            // msGPULatency = 1'080'000 - 1'000'000 = 80'000 ticks = 8 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'080'000);
            Assert::AreEqual(expected, m.msGPULatency, 0.0001);
        }

        TEST_METHOD(GPULatency_GPUStartBeforeCpuStart)
        {
            // cpuStart = 2'000'000
            // gpuStartTime = 1'900'000 (impossible but defensive)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp{};
            priorApp.presentStartTime = 1'500'000;
            priorApp.timeInPresent = 500'000;  // cpuStart = 2'000'000
            priorApp.readyTime = 2'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 2'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with GPU start before CPU start
            FrameData frame{};
            frame.presentStartTime = 2'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 2'300'000;
            frame.gpuStartTime = 1'900'000;  // Earlier than cpuStart
            frame.gpuDuration = 300'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 2'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 2'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Result should be 0 or negative (defensive clamping)
            Assert::IsTrue(m.msGPULatency <= 0.0 || m.msGPULatency == 0.0);
        }

        TEST_METHOD(GPUBusy_BasicCase_StandardPath)
        {
            // gpuDuration = 500'000 ticks
            // Expected msGPUBusy = 500'000 / 10'000'000 = 0.05 ms = 50 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPUBusy = 500'000 ticks = 50 ms
            double expected = qpc.DurationMilliSeconds(500'000);
            Assert::AreEqual(expected, m.msGPUBusy, 0.0001);
        }

        TEST_METHOD(GPUBusy_ZeroDuration)
        {
            // gpuDuration = 0
            // Expected msGPUBusy = 0 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with zero GPU duration
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 0;  // No GPU work
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPUBusy = 0 ticks = 0 ms
            Assert::AreEqual(0.0, m.msGPUBusy, 0.0001);
        }

        TEST_METHOD(GPUBusy_WithAppPropagatedDuration)
        {
            // appPropagatedGPUDuration = 450'000 ticks
            // Expected msGPUBusy = 450'000 / 10'000'000 = 0.045 ms = 45 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with app propagated GPU duration
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;  // Not used when propagated available
            frame.appPropagatedGPUStartTime = 1'050'000;
            frame.appPropagatedGPUDuration = 450'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Uses appPropagated: 450'000 ticks = 45 ms
            double expected = qpc.DurationMilliSeconds(450'000);
            Assert::AreEqual(expected, m.msGPUBusy, 0.0001);
        }

        TEST_METHOD(GPUWait_BasicCase_BusyLessThanTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 → total = 600'000
            // gpuDuration (busy) = 500'000
            // msGPUWait should be 100'000 ticks = 10 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'600'000;
            frame.gpuStartTime = 1'000'000;
            frame.gpuDuration = 500'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'700'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'900'000;
            next.timeInPresent = 50'000;
            next.readyTime = 2'000'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'100'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 500'000 ticks = 50 ms
            // msGPUWait = 600'000 - 500'000 = 100'000 ticks = 10 ms
            double expectedTotal = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'600'000);
            double expectedBusy = qpc.DurationMilliSeconds(500'000);
            double expectedWait = std::max(0.0, expectedTotal - expectedBusy);
            Assert::AreEqual(expectedWait, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_BusyEqualsTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 → total = 600'000
            // gpuDuration = 600'000 (fully busy)
            // msGPUWait should be 0
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with GPU duration equal to total time
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'600'000;
            frame.gpuStartTime = 1'000'000;
            frame.gpuDuration = 600'000;  // Equal to total
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'700'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'900'000;
            next.timeInPresent = 50'000;
            next.readyTime = 2'000'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'100'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 600'000 ticks = 60 ms (equal to total)
            // msGPUWait = 600'000 - 600'000 = 0 ms
            Assert::AreEqual(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_BusyGreaterThanTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 → total = 600'000
            // gpuDuration = 700'000 (impossible, but defensive)
            // msGPUWait should clamp to 0
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with GPU duration greater than total (impossible case)
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'600'000;
            frame.gpuStartTime = 1'000'000;
            frame.gpuDuration = 700'000;  // Greater than total (impossible)
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'700'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'900'000;
            next.timeInPresent = 50'000;
            next.readyTime = 2'000'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'100'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 700'000 ticks = 70 ms (greater than total)
            // msGPUWait should clamp to 0
            Assert::AreEqual(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_WithAppPropagatedData)
        {
            // appPropagatedGPUStartTime = 1'000'000, appPropagatedReadyTime = 1'550'000 → total = 550'000
            // appPropagatedGPUDuration = 450'000
            // msGPUWait should be 100'000 ticks = 10 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with app propagated GPU data
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'600'000;
            frame.gpuStartTime = 1'000'000;
            frame.gpuDuration = 600'000;
            frame.appPropagatedGPUStartTime = 1'000'000;
            frame.appPropagatedReadyTime = 1'550'000;
            frame.appPropagatedGPUDuration = 450'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'700'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'900'000;
            next.timeInPresent = 50'000;
            next.readyTime = 2'000'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'100'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'550'000 - 1'000'000 = 550'000
            // msGPUBusy = 450'000 ticks = 45 ms
            // msGPUWait = 550'000 - 450'000 = 100'000 ticks = 10 ms
            double expectedTotal = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'550'000);
            double expectedBusy = qpc.DurationMilliSeconds(450'000);
            double expectedWait = std::max(0.0, expectedTotal - expectedBusy);
            Assert::AreEqual(expectedWait, m.msGPUWait, 0.0001);
        }
    };

    TEST_CLASS(GPUMetricsVideoTests)
    {
    public:
        TEST_METHOD(VideoBusy_BasicCase_StandardPath)
        {
            // gpuVideoDuration = 200'000 ticks
            // Expected msVideoBusy = 200'000 / 10'000'000 = 0.02 ms = 20 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;
            frame.gpuVideoDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msVideoBusy = 200'000 ticks = 20 ms
            double expected = qpc.DurationMilliSeconds(200'000);
            Assert::AreEqual(expected, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(VideoBusy_ZeroDuration)
        {
            // gpuVideoDuration = 0
            // Expected msVideoBusy = 0 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with zero video duration
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;
            frame.gpuVideoDuration = 0;  // No video work
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            Assert::AreEqual(0.0, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(VideoBusy_WithAppPropagatedData)
        {
            // appPropagatedGPUVideoDuration = 180'000 ticks
            // Expected msVideoBusy = 180'000 / 10'000'000 = 0.018 ms = 18 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with app propagated GPU video duration
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;
            frame.gpuVideoDuration = 200'000;  // Not used when propagated available
            frame.appPropagatedGPUStartTime = 1'050'000;
            frame.appPropagatedGPUDuration = 450'000;
            frame.appPropagatedGPUVideoDuration = 180'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Uses appPropagated: 180'000 ticks = 18 ms
            double expected = qpc.DurationMilliSeconds(180'000);
            Assert::AreEqual(expected, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(VideoBusy_OverlapWithGPUBusy)
        {
            // msGPUBusy = 50 ms (500'000 ticks)
            // msVideoBusy = 20 ms (200'000 ticks)
            // Verify both are independently computed (no constraint)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 500'000;
            frame.gpuVideoDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            double expectedGpuBusy = qpc.DurationMilliSeconds(500'000);
            double expectedVideoBusy = qpc.DurationMilliSeconds(200'000);

            Assert::AreEqual(expectedGpuBusy, m.msGPUBusy, 0.0001);
            Assert::AreEqual(expectedVideoBusy, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(VideoBusy_LargerThanGPUBusy)
        {
            // msGPUBusy = 30 ms (computed from gpuDuration)
            // msVideoBusy = 50 ms (computed from gpuVideoDuration)
            // Verify independent computation (no implicit constraints)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame where video duration > GPU duration
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'050'000;
            frame.gpuDuration = 300'000;  // 30 ms
            frame.gpuVideoDuration = 500'000;  // 50 ms (larger than gpuDuration)
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Verify both are computed independently
            Assert::IsTrue(m.msVideoBusy > m.msGPUBusy);
        }
    };

    TEST_CLASS(EdgeCasesAndMissingData)
    {
    public:
        TEST_METHOD(AllMetrics_NoGPUData_GPUMetricsZero)
        {
            // Frame with no GPU data (all GPU fields = 0)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 800'000,
                /*timeInPresent*/    200'000,
                /*readyTime*/        1'000'000,
                /*displayed*/{});

            chain.lastAppPresent = priorApp;

            // Current frame with no GPU data
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // CPU metrics should be non-zero
            Assert::IsTrue(m.msCPUBusy > 0);
            // GPU metrics should be zero
            Assert::AreEqual(0.0, m.msGPULatency, 0.0001);
            Assert::AreEqual(0.0, m.msGPUBusy, 0.0001);
            Assert::AreEqual(0.0, m.msGPUWait, 0.0001);
            Assert::AreEqual(0.0, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(GeneratedFrameMetrics_NotAppFrame_CPUGPUMetricsZero)
        {
            // Frame with only Repeated display type (not Application)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 800'000,
                /*timeInPresent*/    200'000,
                /*readyTime*/        1'000'000,
                /*displayed*/{});

            chain.lastAppPresent = priorApp;

            // Current frame with only Repeated display (not Application)
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'150'000;
            frame.gpuDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Repeated, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Generated frames have no CPU/GPU work attribution
            Assert::AreEqual(0.0, m.msCPUBusy, 0.0001);
            Assert::AreEqual(0.0, m.msCPUWait, 0.0001);
            Assert::AreEqual(0.0, m.msGPULatency, 0.0001);
            Assert::AreEqual(0.0, m.msGPUBusy, 0.0001);
            Assert::AreEqual(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(NotDisplayedFrame_AppFrameMetrics_Computed)
        {
            // Frame is not displayed (Discarded) but has CPU/GPU work
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame is not displayed (Discarded)
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'150'000;
            frame.gpuDuration = 200'000;
            frame.finalState = PresentResult::Discarded;
            // No displayed entries

            auto results = ComputeMetricsForPresent(qpc, frame, nullptr, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // CPU/GPU metrics should still be computed even for dropped frames
            Assert::IsTrue(m.msCPUBusy > 0);
            Assert::IsTrue(m.msGPUBusy > 0);
        }
    };

    TEST_CLASS(StateAndHistory)
    {
    public:
        TEST_METHOD(CPUStart_UsesLastAppPresent_WhenAvailable)
        {
            // chain.lastAppPresent set; current is app frame
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData lastApp{};
            lastApp.presentStartTime = 800'000;
            lastApp.timeInPresent = 200'000;
            lastApp.readyTime = 1'000'000;
            lastApp.finalState = PresentResult::Presented;
            lastApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = lastApp;

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart should be 800'000 + 200'000 = 1'000'000
            uint64_t expectedCpuStart = 800'000 + 200'000;
            Assert::AreEqual(expectedCpuStart, m.cpuStartQpc);
        }

        TEST_METHOD(CPUStart_FallsBackToLastPresent_WhenNoAppPresent)
        {
            // chain.lastAppPresent is empty; chain.lastPresent is set
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData lastPresent{};
            lastPresent.presentStartTime = 800'000;
            lastPresent.timeInPresent = 200'000;
            lastPresent.readyTime = 1'000'000;
            lastPresent.finalState = PresentResult::Presented;
            lastPresent.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastPresent = lastPresent;
            // lastAppPresent remains unset

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart falls back to lastPresent: 800'000 + 200'000 = 1'000'000
            uint64_t expectedCpuStart = 800'000 + 200'000;
            Assert::AreEqual(expectedCpuStart, m.cpuStartQpc);
        }

        TEST_METHOD(CPUStart_ReturnsZero_WhenNoChainHistory)
        {
            // No lastAppPresent; no lastPresent
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};  // Empty chain

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 5'200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 6'000'000;
            next.timeInPresent = 50'000;
            next.readyTime = 6'100'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'300'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 0 (no history)
            Assert::AreEqual(uint64_t(0), m.cpuStartQpc);
        }

        TEST_METHOD(ChainState_UpdatedAfterPresent_SingleDisplay)
        {
            // Process a displayed app frame; verify chain state is updated
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Current frame
            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 5'200'000;
            frame.flipDelay = 777;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'500'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 6'000'000;
            next.timeInPresent = 50'000;
            next.readyTime = 6'100'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 6'300'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            // Verify chain state was updated
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(5'500'000), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(777), chain.lastDisplayedFlipDelay);
        }
    };

    TEST_CLASS(NumericAndPrecision)
    {
    public:
        TEST_METHOD(CPUBusy_LargeValues_DoesNotOverflow)
        {
            // cpuStart = 1'000'000'000 (large QPC value)
            // presentStartTime = 1'100'000'000
            // Expected msCPUBusy = 100'000'000 ticks = 10'000 ms (10 seconds)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Prior app frame with large values
            FrameData priorApp{};
            priorApp.presentStartTime = 900'000'000;
            priorApp.timeInPresent = 100'000'000;
            priorApp.readyTime = 1'000'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with large values
            FrameData frame{};
            frame.presentStartTime = 1'100'000'000;
            frame.timeInPresent = 100'000'000;
            frame.readyTime = 1'200'000'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'600'000'000;
            next.timeInPresent = 50'000'000;
            next.readyTime = 1'700'000'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 900'000'000 + 100'000'000 = 1'000'000'000
            // msCPUBusy = 1'100'000'000 - 1'000'000'000 = 100'000'000 ticks = 10'000 ms (10 seconds)
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000'000, 1'100'000'000);
            Assert::AreEqual(expected, m.msCPUBusy, 0.0001);
            // Verify large value is reasonable (10 seconds)
            Assert::IsTrue(m.msCPUBusy > 9000 && m.msCPUBusy < 11000);
        }

        TEST_METHOD(GPULatency_SmallDelta_HighPrecision)
        {
            // cpuStart = 1'000'000
            // gpuStartTime = 1'000'001 (1 tick delta; tiny latency)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            chain.lastAppPresent = priorApp;

            // Current frame with small GPU latency
            FrameData frame{};
            frame.presentStartTime = 1'100'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'200'000;
            frame.gpuStartTime = 1'000'001;  // Only 1 tick later than cpuStart
            frame.gpuDuration = 200'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'300'000 });

            // Next displayed frame (required to process current frame's display)
            FrameData next{};
            next.presentStartTime = 1'500'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'600'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, &next, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPULatency = 1 tick at 10 MHz = 0.0001 ms (very small but non-zero)
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'000'001);
            Assert::AreEqual(expected, m.msGPULatency, 0.00001);
            Assert::IsTrue(m.msGPULatency > 0.0 && m.msGPULatency < 0.001);
        }

        TEST_METHOD(VideoBusy_ZeroAndNonzeroInSequence)
        {
            // Frame A: no video work
            // Frame B: with video work
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Frame A: zero video
            FrameData frameA{};
            frameA.presentStartTime = 1'000'000;
            frameA.timeInPresent = 100'000;
            frameA.readyTime = 1'200'000;
            frameA.gpuStartTime = 1'050'000;
            frameA.gpuDuration = 400'000;
            frameA.gpuVideoDuration = 0;  // No video
            frameA.finalState = PresentResult::Presented;
            frameA.displayed.PushBack({ FrameType::Application, 1'500'000 });

            FrameData nextA{};
            nextA.presentStartTime = 2'000'000;
            nextA.timeInPresent = 50'000;
            nextA.readyTime = 2'100'000;
            nextA.finalState = PresentResult::Presented;
            nextA.displayed.PushBack({ FrameType::Application, 2'200'000 });

            auto resultsA = ComputeMetricsForPresent(qpc, frameA, &nextA, chain);
            Assert::AreEqual(size_t(1), resultsA.size());
            Assert::AreEqual(0.0, resultsA[0].metrics.msVideoBusy, 0.0001);

            // Frame B: with video
            FrameData frameB{};
            frameB.presentStartTime = 2'100'000;
            frameB.timeInPresent = 100'000;
            frameB.readyTime = 2'300'000;
            frameB.gpuStartTime = 2'150'000;
            frameB.gpuDuration = 400'000;
            frameB.gpuVideoDuration = 300'000;  // 30 ms of video
            frameB.finalState = PresentResult::Presented;
            frameB.displayed.PushBack({ FrameType::Application, 2'600'000 });

            FrameData nextB{};
            nextB.presentStartTime = 3'000'000;
            nextB.timeInPresent = 50'000;
            nextB.readyTime = 3'100'000;
            nextB.finalState = PresentResult::Presented;
            nextB.displayed.PushBack({ FrameType::Application, 3'200'000 });

            auto resultsB = ComputeMetricsForPresent(qpc, frameB, &nextB, chain);
            Assert::AreEqual(size_t(1), resultsB.size());
            double expectedVideoBusy = qpc.DurationMilliSeconds(300'000);
            Assert::AreEqual(expectedVideoBusy, resultsB[0].metrics.msVideoBusy, 0.0001);
        }
    };

    TEST_CLASS(AnimationTime)
    {
    public:
        // ========================================================================
        // A1: AnimationTime_AppProvider_FirstFrame_ZeroWithoutAppSimStartTime
        // ========================================================================
        TEST_METHOD(AnimationTime_AppProvider_FirstFrame_ZeroWithoutAppSimStartTime)
        {
            // Scenario:
            // - SwapChainCoreState starts with CpuStart (default)
            // - Current frame: displayed, displayIndex == appIndex, but appSimStartTime == 0
            // - No app data means source stays CpuStart
            //
            // Expected Outcome:
            // - msAnimationTime = std::nullopt (no valid sim start time, no history)
            // - firstAppSimStartTime remains 0 in state
            // - Source remains CpuStart

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Verify initial state
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime);

            // Create a displayed app frame WITHOUT appSimStartTime
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 0;  // No app instrumentation yet
            frame.pclSimStartTime = 0;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Verify frame setup
            Assert::AreEqual(uint64_t(0), frame.appSimStartTime);
            Assert::AreEqual(size_t(1), frame.displayed.Size());

            // Create nextDisplayed to allow processing
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 400;
            next.readyTime = 2'500'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics for this frame
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            // Assert: Should have one computed metric
            Assert::AreEqual(size_t(1), metricsVector.size());

            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should have value a value of zero
            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should have a value of zero");
            Assert::AreEqual(double(0.0), result.metrics.msAnimationTime.value());

            // Assert: firstAppSimStartTime in state should remain 0
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"State: firstAppSimStartTime should remain 0 (no valid app sim start time detected)");

            // Assert: lastDisplayedSimStartTime should remain 0
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should remain 0 (no valid app sim start time detected)");
        }

        // ========================================================================
        // A2: AnimationTime_AppProvider_TransitionFrame_FirstValidAppSimStart
        // ========================================================================
        TEST_METHOD(AnimationTime_AppProvider_TransitionFrame_FirstValidAppSimStart)
        {
            // Scenario:
            // - Start with CpuStart source (default)
            // - Frame 1: App data arrives, triggers source switch to AppProvider
            // - Expected: msAnimationTime = 0 (first frame with valid app sim start)
            //
            // Expected Outcome:
            // - msAnimationTime = 0 (first frame with app instrumentation)
            // - firstAppSimStartTime is set to qpc(100)
            // - Source switches to AppProvider after UpdateChain

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Create a displayed app frame WITH appSimStartTime for the first time
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 100;  // First valid app sim start
            frame.pclSimStartTime = 0;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Create nextDisplayed
            FrameData frame2{};
            frame2.presentStartTime = 2'000'000;
            frame2.timeInPresent = 400;
            frame2.readyTime = 2'500'000;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &frame2, state);

            // Assert
            Assert::AreEqual(size_t(1), metricsVector.size());
            const ComputedMetrics& result = metricsVector[0];

            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should have a value");
            Assert::AreEqual(double(0.0), result.metrics.msAnimationTime.value(),
                L"msAnimationTime should be 0 on first frame with CpuStart source and no history");

            // Assert: State should be updated
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"State: firstAppSimStartTime should be set to first valid app sim start");
            Assert::AreEqual(uint64_t(100), state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should be set to current frame's app sim start");
            Assert::IsTrue(
                state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should transition to AppProvider after first appSimStartTime.");
        }

        // ========================================================================
        // A3: AnimationTime_AppProvider_SecondFrame_IncrementsCorrectly
        // ========================================================================
        TEST_METHOD(AnimationTime_AppProvider_SecondFrame_IncrementsCorrectly)
        {
            // Scenario:
            // - Frame 1: First app data, establishes firstAppSimStartTime = 100, switches to AppProvider
            // - Frame 2: appSimStartTime = 150 (50 QPC ticks later)
            // - QPC frequency = 10 MHz → 50 ticks = 5 µs = 0.005 ms
            //
            // Expected Outcome:
            // - msAnimationTime ≈ 0.005 ms (elapsed sim time from first to current)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // Start with CpuStart, will switch to AppProvider after frame1

            // Frame 1: First app data
            FrameData frame1{};
            frame1.presentStartTime = 500'000;
            frame1.timeInPresent = 300;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 900'000 });

            FrameData next1{};
            next1.presentStartTime = 1'500'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 1'500'000 });

            ComputeMetricsForPresent(qpc, frame1, &next1, state);
            // After this, source is AppProvider, firstAppSimStartTime = 100

            // Frame 2: Second app frame with incremented sim start
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 150;  // 50 ticks later
            frame.pclSimStartTime = 0;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Create nextDisplayed
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 400;
            next.readyTime = 2'500'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            Assert::AreEqual(size_t(1), metricsVector.size());
            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should be 0.005 ms
            Assert::IsTrue(result.metrics.msAnimationTime.has_value());
            double expectedMs = qpc.DeltaUnsignedMilliSeconds(100, 150);
            Assert::AreEqual(expectedMs, result.metrics.msAnimationTime.value(), 0.0001,
                L"msAnimationTime should reflect elapsed time from first app sim start");

            // Assert: firstAppSimStartTime unchanged
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain at first value");

            // Assert: lastDisplayedSimStartTime updated
            Assert::AreEqual(uint64_t(150), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should be updated to current frame's app sim start");
        }
        // ========================================================================
        // A4: AnimationTime_AppProvider_ThreeFrames_CumulativeElapsedTime
        // ========================================================================
        TEST_METHOD(AnimationTime_AppProvider_ThreeFrames_CumulativeElapsedTime)
        {
            // Scenario:
            //  - Start with CpuStart as the animation source.
            //  - Frame 1: first displayed app frame with appSimStartTime = 100.
            //             This SEEDS animation state (firstAppSimStartTime), but
            //             does not yet emit msAnimationTime.
            //  - Frame 2: displayed app frame with appSimStartTime = 150.
            //  - Frame 3: displayed app frame with appSimStartTime = 250.
            //
            // QPC frequency = 10 MHz.
            //
            // Expected outcome:
            //  - firstAppSimStartTime is latched to 100 and never changes.
            //  - Frame 1: msAnimationTime == nullopt (just seeds state).
            //  - Frame 2: msAnimationTime = (150 - 100) / 10e6.
            //  - Frame 3: msAnimationTime = (250 - 100) / 10e6.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // animationErrorSource defaults to CpuStart; will transition to AppProvider
            // when the first displayed frame with appSimStartTime arrives.

            // --------------------------------------------------------------------
            // Frame 1: first valid app sim start, displayed
            // --------------------------------------------------------------------
            FrameData frame1{};
            frame1.presentStartTime = 1'000'000;
            frame1.timeInPresent = 500;
            frame1.readyTime = 1'500'000;
            frame1.appSimStartTime = 100;
            frame1.pclSimStartTime = 0;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            FrameData next1{};
            next1.presentStartTime = 2'000'000;
            next1.timeInPresent = 400;
            next1.readyTime = 2'500'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 2'000'000 });

            auto metrics1 = ComputeMetricsForPresent(qpc, frame1, &next1, state);
            Assert::AreEqual(size_t(1), metrics1.size());

            Assert::IsTrue(
                metrics1[0].metrics.msAnimationTime.has_value(),
                L"First AppProvider frame should seed firstAppSimStartTime and animation time should be zero");
            Assert::AreEqual(double(0.0), metrics1[0].metrics.msAnimationTime.value(),
                L"msAnimationTime should be 0 on first frame with CpuStart source and no history");

            // After processing frame1, the chain should have latched sim start and
            // switched to AppProvider.
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(100), state.lastDisplayedSimStartTime);
            Assert::IsTrue(
                state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should transition to AppProvider after first appSimStartTime.");

            // --------------------------------------------------------------------
            // Frame 2: second displayed app frame
            // --------------------------------------------------------------------
            FrameData frame2{};
            frame2.presentStartTime = 3'000'000;
            frame2.timeInPresent = 500;
            frame2.readyTime = 3'500'000;
            frame2.appSimStartTime = 150;
            frame2.pclSimStartTime = 0;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 3'000'000 });

            FrameData next2{};
            next2.presentStartTime = 4'000'000;
            next2.timeInPresent = 400;
            next2.readyTime = 4'500'000;
            next2.finalState = PresentResult::Presented;
            next2.displayed.PushBack({ FrameType::Application, 4'000'000 });

            auto metrics2 = ComputeMetricsForPresent(qpc, frame2, &next2, state);
            Assert::AreEqual(size_t(1), metrics2.size());
            Assert::IsTrue(
                metrics2[0].metrics.msAnimationTime.has_value(),
                L"Second displayed app frame should report msAnimationTime.");

            double expected2 = qpc.DeltaUnsignedMilliSeconds(100, 150);
            Assert::AreEqual(
                expected2,
                metrics2[0].metrics.msAnimationTime.value(),
                0.0001,
                L"Frame 2's msAnimationTime should be relative to firstAppSimStartTime (100 → 150).");

            // firstAppSimStartTime should stay anchored at 100
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime, L"firstAppSimStartTime should not change.");
            // lastDisplayedSimStartTime should now reflect frame2
            Assert::AreEqual(uint64_t(150), state.lastDisplayedSimStartTime);

            // --------------------------------------------------------------------
            // Frame 3: third displayed app frame
            // --------------------------------------------------------------------
            FrameData frame3{};
            frame3.presentStartTime = 5'000'000;
            frame3.timeInPresent = 500;
            frame3.readyTime = 5'500'000;
            frame3.appSimStartTime = 250;
            frame3.pclSimStartTime = 0;
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 5'000'000 });

            FrameData next3{};
            next3.presentStartTime = 6'000'000;
            next3.timeInPresent = 400;
            next3.readyTime = 6'500'000;
            next3.finalState = PresentResult::Presented;
            next3.displayed.PushBack({ FrameType::Application, 6'000'000 });

            auto metrics3 = ComputeMetricsForPresent(qpc, frame3, &next3, state);
            Assert::AreEqual(size_t(1), metrics3.size());
            Assert::IsTrue(
                metrics3[0].metrics.msAnimationTime.has_value(),
                L"Third displayed app frame should report msAnimationTime.");

            double expected3 = qpc.DeltaUnsignedMilliSeconds(100, 250);
            Assert::AreEqual(
                expected3,
                metrics3[0].metrics.msAnimationTime.value(),
                0.0001,
                L"Frame 3's msAnimationTime should be relative to original firstAppSimStartTime (100 → 250).");

            // firstAppSimStartTime remains the original seed
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime, L"firstAppSimStartTime should remain at 100.");
            // lastDisplayedSimStartTime should now reflect frame3
            Assert::AreEqual(uint64_t(250), state.lastDisplayedSimStartTime);
        }
        // ========================================================================
        // A5: AnimationTime_AppProvider_SkippedFrame_StaysConsistent
        // ========================================================================
        TEST_METHOD(AnimationTime_AppProvider_SkippedFrame_StaysConsistent)
        {
            // We want to verify:
            // - Animation source is AppProvider (AppSimStartTime-based).
            // - A discarded (not displayed) frame with AppSimStartTime:
            //     * Produces no animation metrics.
            //     * Does NOT change firstAppSimStartTime / lastDisplayedSimStartTime /
            //       lastDisplayedAppScreenTime.
            // - A later displayed app frame still computes msAnimationTime correctly
            //   based on the original firstAppSimStartTime.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // Seed state as if we already had one displayed app frame at sim = 100.
            state.animationErrorSource = AnimationErrorSource::AppProvider;
            state.firstAppSimStartTime = 100;
            state.lastDisplayedSimStartTime = 100;
            state.lastDisplayedAppScreenTime = 1'000'000; // prior displayed screen time

            // --------------------------------------------------------------------
            // Frame 1: Discarded / not displayed, but with AppSimStartTime = 150
            // --------------------------------------------------------------------
            FrameData frameDropped{};
            frameDropped.presentStartTime = 2'000'000;
            frameDropped.timeInPresent = 50'000;
            frameDropped.readyTime = 2'050'000;
            frameDropped.appSimStartTime = 150;
            frameDropped.finalState = PresentResult::Discarded;
            // No displayed entries -> not displayed

            auto droppedResults = ComputeMetricsForPresent(qpc, frameDropped, nullptr, state);
            Assert::AreEqual(size_t(1), droppedResults.size());
            const auto& droppedMetrics = droppedResults[0].metrics;

            // Discarded / not-displayed frame must NOT produce animation metrics.
            Assert::IsFalse(droppedMetrics.msAnimationTime.has_value(),
                L"Discarded frame should not have msAnimationTime");
            Assert::IsFalse(droppedMetrics.msAnimationError.has_value(),
                L"Discarded frame should not have msAnimationError");

            // And it must NOT disturb the animation anchors from prior displayed frames.
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"firstAppSimStartTime should be unchanged by discarded frame");
            Assert::AreEqual(uint64_t(100), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should be unchanged by discarded frame");
            Assert::AreEqual(uint64_t(1'000'000), state.lastDisplayedAppScreenTime,
                L"lastDisplayedAppScreenTime should be unchanged by discarded frame");

            // --------------------------------------------------------------------
            // Frame 2: Next displayed app frame with AppSimStartTime = 200
            // --------------------------------------------------------------------
            FrameData frameDisplayed{};
            frameDisplayed.presentStartTime = 3'000'000;
            frameDisplayed.timeInPresent = 50'000;
            frameDisplayed.readyTime = 3'050'000;
            frameDisplayed.appSimStartTime = 200;
            frameDisplayed.finalState = PresentResult::Presented;
            frameDisplayed.displayed.PushBack({ FrameType::Application, 3'500'000 });

            // Dummy "next" frame – just to exercise the Case 3 path so that
            // UpdateAfterPresent() is called for frameDisplayed.
            FrameData frameNext{};
            frameNext.presentStartTime = 4'000'000;
            frameNext.timeInPresent = 50'000;
            frameNext.readyTime = 4'050'000;
            frameNext.appSimStartTime = 250;
            frameNext.finalState = PresentResult::Presented;
            frameNext.displayed.PushBack({ FrameType::Application, 4'500'000 });

            auto displayedResults = ComputeMetricsForPresent(qpc, frameDisplayed, &frameNext, state);
            Assert::AreEqual(size_t(1), displayedResults.size());
            const auto& displayedMetrics = displayedResults[0].metrics;

            Assert::IsTrue(displayedMetrics.msAnimationTime.has_value(),
                L"Displayed app frame should have msAnimationTime");

            // Animation time should be based purely on the AppProvider sim times:
            // firstAppSimStartTime = 100, currentSim = 200.
            const double expected = qpc.DeltaUnsignedMilliSeconds(100, 200);
            Assert::AreEqual(expected, displayedMetrics.msAnimationTime.value(), 0.0001);

            // After processing a displayed app frame via the Case 3 path,
            // state should now reflect that frame as the last displayed.
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain the first displayed AppSimStartTime");
            Assert::AreEqual(uint64_t(200), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should track the most recent DISPLAYED AppSimStartTime");
            Assert::AreEqual(uint64_t(3'500'000), state.lastDisplayedAppScreenTime,
                L"lastDisplayedAppScreenTime should track the most recent displayed screen time");
        }
        // ========================================================================
        // B1: AnimationTime_PCLatency_FirstFrame_ZeroWithoutPclSimStartTime
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_FirstFrame_ZeroWithoutPclSimStartTime)
        {
            // Scenario:
            // - SwapChainCoreState starts with CpuStart (default)
            // - Current frame: displayed, displayIndex == appIndex, but pclSimStartTime == 0
            // - No PCL data means source stays CpuStart
            //
            // Expected Outcome:
            // - msAnimationTime = std::nullopt (no valid sim start time, no history)
            // - firstAppSimStartTime remains 0 in state
            // - Source remains CpuStart

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Verify initial state
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime);

            // Create a displayed app frame WITHOUT pclSimStartTime
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 0;  // No PC latency instrumentation yet
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Verify frame setup
            Assert::AreEqual(uint64_t(0), frame.pclSimStartTime);
            Assert::AreEqual(size_t(1), frame.displayed.Size());

            // Create nextDisplayed to allow processing
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 400;
            next.readyTime = 2'500'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics for this frame
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            // Assert: Should have one computed metric
            Assert::AreEqual(size_t(1), metricsVector.size());

            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should be zero
            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should be 0 whentransitioning");
            Assert::AreEqual(double(0.0), result.metrics.msAnimationTime.value());

            // Assert: firstAppSimStartTime in state should remain 0
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"State: firstAppSimStartTime should remain 0 (no valid pcl sim start time detected)");

            // Assert: lastDisplayedSimStartTime should remain 0
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should remain 0 (no valid pcl sim start time detected)");
        }

        // ========================================================================
        // B2: AnimationTime_PCLatency_TransitionFrame_FirstValidPclSimStart
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_TransitionFrame_FirstValidPclSimStart)
        {
            // Scenario:
            // - Start with CpuStart source (default)
            // - Frame 1: PCL data arrives, triggers source switch to PCLatency
            // - Expected: msAnimationTime = 0 (first frame with valid pcl sim start)
            //
            // Expected Outcome:
            // - msAnimationTime = 0 (first frame with PCL instrumentation)
            // - firstAppSimStartTime is set to qpc(100)
            // - Source switches to PCLatency after UpdateChain

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Create a displayed app frame WITH pclSimStartTime for the first time
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 100;  // First valid pcl sim start
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Create nextDisplayed
            FrameData frame2{};
            frame2.presentStartTime = 2'000'000;
            frame2.timeInPresent = 400;
            frame2.readyTime = 2'500'000;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &frame2, state);

            // Assert
            Assert::AreEqual(size_t(1), metricsVector.size());
            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should have a value of zero
            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should have a value");

            // Assert: State should be updated
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"State: firstAppSimStartTime should be set to first valid pcl sim start");
            Assert::AreEqual(uint64_t(100), state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should be set to current frame's pcl sim start");
            Assert::IsTrue(
                state.animationErrorSource == AnimationErrorSource::PCLatency,
                L"Animation source should transition to PCLatency after first appSimStartTime.");
        }

        // ========================================================================
        // B3: AnimationTime_PCLatency_SecondFrame_IncrementsCorrectly
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_SecondFrame_IncrementsCorrectly)
        {
            // Scenario:
            // - Frame 2: pclSimStartTime = 200 (100 QPC ticks later)
            // - QPC frequency = 10 MHz → 100 ticks = 10 µs = 0.01 ms
            //
            // Expected Outcome:
            // - msAnimationTime ≈ 0.01 ms (elapsed sim time from first to current)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // Start with CpuStart, will switch to PCLatency after frame1

            // Frame 1: First PCL data
            FrameData frame1{};
            frame1.presentStartTime = 500'000;
            frame1.timeInPresent = 300;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 900'000 });

            FrameData next1{};
            next1.presentStartTime = 1'500'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 1'500'000 });

            ComputeMetricsForPresent(qpc, frame1, &next1, state);
            // After this, source is PCLatency, firstAppSimStartTime = 100

            // Frame 2: Second PCL frame with incremented sim start
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 200;  // 100 ticks later
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Create nextDisplayed
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 400;
            next.readyTime = 2'500'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            Assert::AreEqual(size_t(1), metricsVector.size());
            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should be 0.01 ms
            Assert::IsTrue(result.metrics.msAnimationTime.has_value());
            double expectedMs = qpc.DeltaUnsignedMilliSeconds(100, 200);
            Assert::AreEqual(expectedMs, result.metrics.msAnimationTime.value(), 0.0001,
                L"msAnimationTime should reflect elapsed time from first pcl sim start");

            // Assert: firstAppSimStartTime unchanged
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain at first value");

            // Assert: lastDisplayedSimStartTime updated
            Assert::AreEqual(uint64_t(200), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should be updated to current frame's pcl sim start");
        }

        // ========================================================================
        // B4: AnimationTime_PCLatency_ThreeFrames_CumulativeElapsedTime
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_ThreeFrames_CumulativeElapsedTime)
        {
            // Scenario:
            //  - Start with CpuStart as the animation source.
            //  - Frame 1: first displayed app frame with appSimStartTime = 100.
            //             This SEEDS animation state (firstAppSimStartTime), but
            //             does not yet emit msAnimationTime.
            //  - Frame 2: displayed app frame with appSimStartTime = 150.
            //  - Frame 3: displayed app frame with appSimStartTime = 250.
            //
            // QPC frequency = 10 MHz.
            //
            // Expected outcome:
            //  - firstAppSimStartTime is latched to 100 and never changes.
            //  - Frame 1: msAnimationTime == nullopt (just seeds state).
            //  - Frame 2: msAnimationTime = (150 - 100) / 10e6.
            //  - Frame 3: msAnimationTime = (250 - 100) / 10e6.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // animationErrorSource defaults to CpuStart; will transition to AppProvider
            // when the first displayed frame with appSimStartTime arrives.

            // --------------------------------------------------------------------
            // Frame 1: first valid app sim start, displayed
            // --------------------------------------------------------------------
            FrameData frame1{};
            frame1.presentStartTime = 1'000'000;
            frame1.timeInPresent = 500;
            frame1.readyTime = 1'500'000;
            frame1.appSimStartTime = 0;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            FrameData next1{};
            next1.presentStartTime = 2'000'000;
            next1.timeInPresent = 400;
            next1.readyTime = 2'500'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 2'000'000 });

            auto metrics1 = ComputeMetricsForPresent(qpc, frame1, &next1, state);
            Assert::AreEqual(size_t(1), metrics1.size());

            // First displayed app frame seeds state; animation time is reported as zero.
            Assert::IsTrue(
                metrics1[0].metrics.msAnimationTime.has_value(),
                L"msAnimationTime should report a value even when transitioning");

            // After processing frame1, the chain should have latched sim start and
            // switched to PCLatency.
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(100), state.lastDisplayedSimStartTime);
            Assert::IsTrue(
                state.animationErrorSource == AnimationErrorSource::PCLatency,
                L"Animation source should transition to PCLatency after first appSimStartTime.");

            // --------------------------------------------------------------------
            // Frame 2: second displayed app frame
            // --------------------------------------------------------------------
            FrameData frame2{};
            frame2.presentStartTime = 3'000'000;
            frame2.timeInPresent = 500;
            frame2.readyTime = 3'500'000;
            frame2.appSimStartTime = 0;
            frame2.pclSimStartTime = 150;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 3'000'000 });

            FrameData next2{};
            next2.presentStartTime = 4'000'000;
            next2.timeInPresent = 400;
            next2.readyTime = 4'500'000;
            next2.finalState = PresentResult::Presented;
            next2.displayed.PushBack({ FrameType::Application, 4'000'000 });

            auto metrics2 = ComputeMetricsForPresent(qpc, frame2, &next2, state);
            Assert::AreEqual(size_t(1), metrics2.size());
            Assert::IsTrue(
                metrics2[0].metrics.msAnimationTime.has_value(),
                L"Second displayed app frame should report msAnimationTime.");

            double expected2 = qpc.DeltaUnsignedMilliSeconds(100, 150);
            Assert::AreEqual(
                expected2,
                metrics2[0].metrics.msAnimationTime.value(),
                0.0001,
                L"Frame 2's msAnimationTime should be relative to firstAppSimStartTime (100 → 150).");

            // firstAppSimStartTime should stay anchored at 100
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime, L"firstAppSimStartTime should not change.");
            // lastDisplayedSimStartTime should now reflect frame2
            Assert::AreEqual(uint64_t(150), state.lastDisplayedSimStartTime);

            // --------------------------------------------------------------------
            // Frame 3: third displayed app frame
            // --------------------------------------------------------------------
            FrameData frame3{};
            frame3.presentStartTime = 5'000'000;
            frame3.timeInPresent = 500;
            frame3.readyTime = 5'500'000;
            frame3.appSimStartTime = 0;
            frame3.pclSimStartTime = 250;
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 5'000'000 });

            FrameData next3{};
            next3.presentStartTime = 6'000'000;
            next3.timeInPresent = 400;
            next3.readyTime = 6'500'000;
            next3.finalState = PresentResult::Presented;
            next3.displayed.PushBack({ FrameType::Application, 6'000'000 });

            auto metrics3 = ComputeMetricsForPresent(qpc, frame3, &next3, state);
            Assert::AreEqual(size_t(1), metrics3.size());
            Assert::IsTrue(
                metrics3[0].metrics.msAnimationTime.has_value(),
                L"Third displayed app frame should report msAnimationTime.");

            double expected3 = qpc.DeltaUnsignedMilliSeconds(100, 250);
            Assert::AreEqual(
                expected3,
                metrics3[0].metrics.msAnimationTime.value(),
                0.0001,
                L"Frame 3's msAnimationTime should be relative to original firstAppSimStartTime (100 → 250).");

            // firstAppSimStartTime remains the original seed
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime, L"firstAppSimStartTime should remain at 100.");
            // lastDisplayedSimStartTime should now reflect frame3
            Assert::AreEqual(uint64_t(250), state.lastDisplayedSimStartTime);
        }
        // ========================================================================
        // B5: AnimationTime_PCLatency_SkippedFrame_StaysConsistent
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_SkippedFrame_StaysConsistent)
        {
            // Scenario:
            //  - Chain is configured to use PCLatency as the animation source.
            //  - Frame 1: displayed, pclSimStartTime = 100 → seeds animation state.
            //  - Frame 2: discarded (not displayed), pclSimStartTime = 200 → should NOT
            //             produce animation time and should NOT advance animation state.
            //  - Frame 3: displayed again, pclSimStartTime = 300 → animation time should be
            //             measured from the original 100, skipping the discarded frame.
            //
            // Expectations:
            //  - Frame 1: msAnimationTime is std::nullopt, but firstAppSimStartTime and
            //             lastDisplayedSimStartTime are set to 100.
            //  - Frame 2: msAnimationTime is std::nullopt and state remains 100/100.
            //  - Frame 3: msAnimationTime == Delta(100, 300) and lastDisplayedSimStartTime == 300.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            //
            // Frame 1: first displayed PCL frame
            //
            FrameData frame1{};
            frame1.presentStartTime = 1'000'000;
            frame1.timeInPresent = 10'000;
            frame1.readyTime = 1'010'000;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 2'000'000 });

            FrameData next1{};
            next1.presentStartTime = 3'000'000;
            next1.timeInPresent = 10'000;
            next1.readyTime = 3'010'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 4'000'000 });

            auto metrics1 = ComputeMetricsForPresent(qpc, frame1, &next1, chain);
            Assert::AreEqual(size_t(1), metrics1.size());

            // First displayed frame seeds animation state; animation time will be reported
            // still as we are transitioning to PCLatency.
            Assert::IsTrue(
                metrics1[0].metrics.msAnimationTime.has_value(),
                L"Animation Time will be reported");
            Assert::AreEqual(double(0.0), metrics1[0].metrics.msAnimationTime.value());

            Assert::AreEqual(uint64_t(100), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(100), chain.lastDisplayedSimStartTime);
            Assert::IsTrue(AnimationErrorSource::PCLatency == chain.animationErrorSource);

            //
            // Frame 2: discarded (not displayed) but has a PCL sim start
            //
            FrameData frame2{};
            frame2.presentStartTime = 5'000'000;
            frame2.timeInPresent = 10'000;
            frame2.readyTime = 5'010'000;
            frame2.pclSimStartTime = 200;   // Has PCL sim start but not displayed
            frame2.finalState = PresentResult::Discarded; // Not presented → not displayed

            auto metrics2 = ComputeMetricsForPresent(qpc, frame2, nullptr, chain);
            Assert::AreEqual(size_t(1), metrics2.size());

            // Not displayed → no animation time, and animation state should not advance
            Assert::IsFalse(
                metrics2[0].metrics.msAnimationTime.has_value(),
                L"Non-displayed frame should not report animation time.");

            Assert::AreEqual(uint64_t(100), chain.firstAppSimStartTime,
                L"firstAppSimStartTime must remain anchored to Frame 1 after skipped frame.");
            Assert::AreEqual(uint64_t(100), chain.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime must remain anchored to Frame 1 after skipped frame.");

            //
            // Frame 3: displayed again after the skipped frame
            //
            FrameData frame3{};
            frame3.presentStartTime = 6'000'000;
            frame3.timeInPresent = 10'000;
            frame3.readyTime = 6'010'000;
            frame3.pclSimStartTime = 300;
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 7'000'000 });

            FrameData next3{};
            next3.presentStartTime = 8'000'000;
            next3.timeInPresent = 10'000;
            next3.readyTime = 8'010'000;
            next3.finalState = PresentResult::Presented;
            next3.displayed.PushBack({ FrameType::Application, 9'000'000 });

            auto metrics3 = ComputeMetricsForPresent(qpc, frame3, &next3, chain);
            Assert::AreEqual(size_t(1), metrics3.size());
            Assert::IsTrue(
                metrics3[0].metrics.msAnimationTime.has_value(),
                L"Displayed frame with valid PCL sim start should report animation time.");

            double expected3 = qpc.DeltaUnsignedMilliSeconds(100, 300);
            Assert::AreEqual(
                expected3,
                metrics3[0].metrics.msAnimationTime.value(),
                0.0001,
                L"Frame 3's msAnimationTime should be measured from Frame 1's PCL sim start, skipping Frame 2.");

            Assert::AreEqual(uint64_t(100), chain.firstAppSimStartTime,
                L"firstAppSimStartTime should remain at Frame 1's value.");
            Assert::AreEqual(uint64_t(300), chain.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should advance to Frame 3's PCL sim start.");
        }
        // ========================================================================
        // B6: AnimationTime_PCLatency_FallsBackToCpuStart_WhenPclSimStartTimeZero
        // ========================================================================
        TEST_METHOD(AnimationTime_PCLatency_FallsBackToCpuStart_WhenPclSimStartTimeZero)
        {
            // Scenario:
            // - Start with CpuStart source
            // - Frame 1: PCL data establishes PCLatency source
            // - Frame 2: pclSimStartTime = 0 (PCL data disappears)
            // - Source should stay PCLatency (no fallback), msAnimationTime = nullopt
            //
            // Expected Outcome:
            // - msAnimationTime = nullopt (PCL data missing)
            // - animationErrorSource remains PCLatency (no fallback)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // Frame 1: First PCL data
            FrameData frame1{};
            frame1.presentStartTime = 500'000;
            frame1.timeInPresent = 300;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 900'000 });

            FrameData next1{};
            next1.presentStartTime = 1'500'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 1'500'000 });

            auto metrics1 = ComputeMetricsForPresent(qpc, frame1, &next1, state);
            Assert::AreEqual(size_t(1), metrics1.size());
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime);
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::PCLatency);

            // Frame 2: PCL data missing (pclSimStartTime = 0)
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 0;  // PCL data disappeared
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Create nextDisplayed
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            // Assert: Should have one computed metric
            Assert::AreEqual(size_t(1), metricsVector.size());

            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should be zero
            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should be 0 whentransitioning");
            Assert::AreEqual(double(0.0), result.metrics.msAnimationTime.value());

            // Assert: State should remain PCLatency
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::PCLatency,
                L"animationErrorSource should remain PCLatency (no fallback)");

            // Assert: firstAppSimStartTime unchanged
            Assert::AreEqual(uint64_t(100), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain unchanged");
        }
        // ========================================================================
        // D1: AnimationTime_CpuStart_FirstFrame_ZeroWithoutHistory
        // ========================================================================
        TEST_METHOD(AnimationTime_CpuStart_FirstFrame_ZeroWithoutHistory)
        {
            // Scenario:
            // - SwapChainCoreState is in initial state (no prior frames)
            // - Current frame: displayed, displayIndex == appIndex
            // - animationErrorSource = CpuStart
            // - No lastAppPresent or lastPresent in chain
            //
            // Expected Outcome:
            // - msAnimationTime = std::nullopt (cpuStart = 0, cannot initialize firstAppSimStartTime)
            // - firstAppSimStartTime remains 0 in state
            // - lastDisplayedSimStartTime remains 0 in state

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Verify initial state
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime);

            // Create a displayed app frame with CpuStart source
            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 500;
            frame.readyTime = 1'500'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 0;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Verify frame setup
            Assert::AreEqual(size_t(1), frame.displayed.Size());

            // Create nextDisplayed to allow processing
            FrameData next{};
            next.presentStartTime = 2'000'000;
            next.timeInPresent = 400;
            next.readyTime = 2'500'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'000'000 });

            // Action: Compute metrics for this frame
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            // Assert: Should have one computed metric
            Assert::AreEqual(size_t(1), metricsVector.size());

            const ComputedMetrics& result = metricsVector[0];

            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should have a value");
            Assert::AreEqual(double(0.0), result.metrics.msAnimationTime.value(),
                L"msAnimationTime should be 0 on first frame with CpuStart source and no history");
            // Assert: State should not be updated
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"State: firstAppSimStartTime should remain 0 (no valid CPU start available)");

            // Assert: lastDisplayedSimStartTime should remain 0
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should remain 0");
        }

        // ========================================================================
        // D2: AnimationTime_CpuStart_TransitionFrame_FirstValidCpuStart
        // ========================================================================
        TEST_METHOD(AnimationTime_CpuStart_TransitionFrame_FirstValidCpuStart)
        {
            // Scenario:
            // - Prior state: firstAppSimStartTime == 0 (no prior CPU start)
            // - Chain has lastAppPresent with valid timing data
            // - Current frame: displayed, displayIndex == appIndex
            // - cpuStart = lastAppPresent.presentStartTime + lastAppPresent.timeInPresent = 1'000'000
            // - animationErrorSource = CpuStart
            //
            // Expected Outcome:
            // - msAnimationTime = 0 (first frame with valid CPU start)
            // - lastDisplayedSimStartTime is updated to cpuStart (1'000'000)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // Set up prior app present for CPU start calculation
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            priorApp.readyTime = 1'000'000;
            priorApp.finalState = PresentResult::Presented;
            priorApp.displayed.PushBack({ FrameType::Application, 1'100'000 });

            state.lastAppPresent = priorApp;

            // Create a displayed app frame with CpuStart source
            FrameData frame{};
            frame.presentStartTime = 1'200'000;
            frame.timeInPresent = 100'000;
            frame.readyTime = 1'300'000;
            frame.appSimStartTime = 0;
            frame.pclSimStartTime = 0;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 1'400'000 });

            // Create nextDisplayed
            FrameData next{};
            next.presentStartTime = 1'600'000;
            next.timeInPresent = 50'000;
            next.readyTime = 1'700'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 1'800'000 });

            // Action: Compute metrics
            auto metricsVector = ComputeMetricsForPresent(qpc, frame, &next, state);

            // Assert
            Assert::AreEqual(size_t(1), metricsVector.size());
            const ComputedMetrics& result = metricsVector[0];

            // Assert: msAnimationTime should be 0 (first transition frame)
            Assert::IsTrue(result.metrics.msAnimationTime.has_value(),
                L"msAnimationTime should have a value on first valid CPU start");
            Assert::AreEqual(0.0, result.metrics.msAnimationTime.value(), 0.0001,
                L"msAnimationTime should be 0 on first transition frame");

            // Assert: State should be updated with CPU start
            // cpuStart = 800'000 + 200'000 = 1'000'000
            uint64_t expectedCpuStart = 800'000 + 200'000;
            Assert::AreEqual(expectedCpuStart, state.lastDisplayedSimStartTime,
                L"State: lastDisplayedSimStartTime should be set to CPU start value");
        }

        // ========================================================================
        // D3: AnimationTime_CpuStart_IncreasesAcrossFramesWithoutProvider
        // ========================================================================
        TEST_METHOD(AnimationTime_CpuStart_IncreasesAcrossFramesWithoutProvider)
        {
            // Scenario:
            //  - animationErrorSource = CpuStart
            //  - No App or PCL sim start timestamps on any frame.
            //  - We seed lastAppPresent so the first tested frame already has
            //    a non-zero CpuStart.
            //  - We then process two displayed frames and expect:
            //      * Both report msAnimationTime (has_value()).
            //      * Frame 2's msAnimationTime > Frame 1's msAnimationTime.
            //      * firstAppSimStartTime stays 0 (no provider events yet).

            QpcConverter      qpc(10'000'000, 500'000);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::CpuStart;

            // Seed lastAppPresent so CalculateCPUStart() for frame1 is non-zero.
            // cpuStart1 = prior.presentStartTime + prior.timeInPresent
            FrameData prior{};
            prior.presentStartTime = 1'000'000;
            prior.timeInPresent = 100'000;   // cpuStart1 = 1'100'000
            prior.readyTime = 1'200'000;
            prior.finalState = PresentResult::Presented;
            prior.displayed.PushBack({ FrameType::Application, 1'300'000 });
            state.lastAppPresent = prior;

            // Sanity: no provider sim-start yet.
            state.firstAppSimStartTime = 0;
            state.lastDisplayedSimStartTime = 0;

            // --------------------------------------------------------------------
            // Frame 1: Presented + displayed, no App/PCL sim start
            // --------------------------------------------------------------------
            FrameData frame1{};
            frame1.presentStartTime = 2'000'000;
            frame1.timeInPresent = 80'000;
            frame1.readyTime = 2'100'000;
            frame1.appSimStartTime = 0;
            frame1.pclSimStartTime = 0;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 2'500'000 });

            FrameData next1{};
            next1.presentStartTime = 3'000'000;
            next1.timeInPresent = 50'000;
            next1.readyTime = 3'100'000;
            next1.finalState = PresentResult::Presented;
            next1.displayed.PushBack({ FrameType::Application, 3'500'000 });

            auto metrics1 = ComputeMetricsForPresent(qpc, frame1, &next1, state);
            Assert::AreEqual(size_t(1), metrics1.size());
            const auto& m1 = metrics1[0].metrics;

            Assert::IsTrue(m1.msAnimationTime.has_value(),
                L"CpuStart animation should report msAnimationTime even without App/PCL provider.");
            const double anim1 = m1.msAnimationTime.value();
            Assert::IsTrue(anim1 > 0.0,
                L"First CpuStart-driven frame should have a positive animation time relative to session/start anchor.");

            // No provider yet → firstAppSimStartTime must remain 0.
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"firstAppSimStartTime should not be set until App/PCL provider events arrive.");

            // After frame1, lastAppPresent is now frame1; CpuStart for frame2 will be later.
            // --------------------------------------------------------------------
            // Frame 2: later Presented + displayed frame, still no App/PCL provider
            // --------------------------------------------------------------------
            FrameData frame2{};
            frame2.presentStartTime = 4'000'000;
            frame2.timeInPresent = 120'000;
            frame2.readyTime = 4'200'000;
            frame2.appSimStartTime = 0;
            frame2.pclSimStartTime = 0;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 4'600'000 });

            FrameData next2{};
            next2.presentStartTime = 5'000'000;
            next2.timeInPresent = 50'000;
            next2.readyTime = 5'100'000;
            next2.finalState = PresentResult::Presented;
            next2.displayed.PushBack({ FrameType::Application, 5'500'000 });

            auto metrics2 = ComputeMetricsForPresent(qpc, frame2, &next2, state);
            Assert::AreEqual(size_t(1), metrics2.size());
            const auto& m2 = metrics2[0].metrics;

            Assert::IsTrue(m2.msAnimationTime.has_value(),
                L"Second CpuStart-driven frame should also report msAnimationTime.");
            const double anim2 = m2.msAnimationTime.value();

            Assert::IsTrue(anim2 > anim1,
                L"CpuStart-based animation time should increase across frames as CpuStart advances.");

            // Still no provider events → anchor remains "non-provider", so firstAppSimStartTime should be 0.
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"firstAppSimStartTime should still be 0 without App/PCL provider data.");
        }
    };

    // ============================================================================
    // SECTION: Animation Error Tests
    // ============================================================================

    TEST_CLASS(AnimationErrorTests)
    {
    public:
        // Section B: Animation Error – AppProvider Source

        TEST_METHOD(AnimationError_AppProvider_NoLastDisplayedFrame_Nullopt)
        {
            // Scenario: First app-displayed frame with appSimStartTime
            // Expected: msAnimationError = std::nullopt (no prior frame data)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData present{};
            present.presentStartTime = 1000;
            present.timeInPresent = 100;
            present.appSimStartTime = 150;
            present.finalState = PresentResult::Presented;
            present.displayed.PushBack({ FrameType::Application, 200 });

            FrameData nextPresent{};
            nextPresent.presentStartTime = 2000;
            nextPresent.finalState = PresentResult::Presented;
            nextPresent.displayed.PushBack({ FrameType::Application, 2100 });

            auto results = ComputeMetricsForPresent(qpc, present, &nextPresent, state);

            Assert::AreEqual(size_t(1), results.size());
            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt without prior displayed frame");
        }

        TEST_METHOD(AnimationError_AppProvider_TwoFrames_PositiveError)
        {
            // Scenario: Two frames with sim elapsed = 50, display elapsed = 50
            // Expected: msAnimationError = 0 ms (cadences match)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            // Frame 1 setup
            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 150;  // sim elapsed = 50
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // display elapsed = 50

            // Process frame 1
            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            auto results1 = ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            // Process frame 2
            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results2 = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::AreEqual(size_t(1), results2.size());
            Assert::IsTrue(results2[0].metrics.msAnimationError.has_value());
            Assert::AreEqual(0.0, results2[0].metrics.msAnimationError.value(), 0.0001,
                L"msAnimationError should be 0 when sim and display cadences match");
        }

        TEST_METHOD(AnimationError_AppProvider_TwoFrames_SimSlowerThanDisplay)
        {
            // Scenario: sim elapsed = 40 ticks, display elapsed = 50 ticks
            // Expected: msAnimationError = -0.001 ms (sim slower)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 140;  // sim elapsed = 40
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // display elapsed = 50

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 140);     // 0.004 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 1050); // 0.005 ms
            double expected = simElapsed - displayElapsed;  // -0.001 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001);
        }

        TEST_METHOD(AnimationError_AppProvider_TwoFrames_SimFasterThanDisplay)
        {
            // Scenario: sim elapsed = 60 ticks, display elapsed = 50 ticks
            // Expected: msAnimationError = +0.001 ms (sim faster)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 160;  // sim elapsed = 60
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // display elapsed = 50

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 160);     // 0.006 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 1050); // 0.005 ms
            double expected = simElapsed - displayElapsed;  // +0.001 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001);
        }

        TEST_METHOD(AnimationError_AppProvider_BackwardsSimStartTime_Nullopt)
        {
            // Scenario: Current sim start goes backward in time
            // Expected: msAnimationError = std::nullopt
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 150;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1050 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 140;  // backwards!
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1100 });

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt when sim start goes backward");
        }

        TEST_METHOD(AnimationError_AppProvider_CurrentSimStartTimeZero_Nullopt)
        {
            // Scenario: Current frame has no app instrumentation (appSimStartTime = 0)
            // Expected: msAnimationError = std::nullopt
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState swapChain{};
            swapChain.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 0;  // no instrumentation
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, swapChain);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, swapChain);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt without valid sim start time");
        }

        TEST_METHOD(AnimationError_AppProvider_ZeroDisplayDelta_ErrorIsSimElapsed)
        {
            // Scenario: Display elapsed = 0 (same screen time)
            // Expected: msAnimationError = simElapsed - 0
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 150;  // sim elapsed = 50
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1000 });  // same screen time!

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value());
        }

        // Section C: Animation Error – PCLatency Source

        TEST_METHOD(AnimationError_PCLatency_TwoFrames_ValidPclSimStart)
        {
            // Scenario: PCL source, sim elapsed = 40, display elapsed = 50
            // Expected: msAnimationError = -0.001 ms
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.pclSimStartTime = 140;  // PCL sim elapsed = 40
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // display elapsed = 50

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 140);     // 0.004 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 1050); // 0.005 ms
            double expected = simElapsed - displayElapsed;  // -0.001 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001);
        }

        TEST_METHOD(AnimationError_PCLatency_CurrentPclSimStartZero_Nullopt)
        {
            // Scenario: PCL source, but current frame has pclSimStartTime = 0
            // Expected: msAnimationError = std::nullopt (no fallback to app in PCL mode)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.pclSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.pclSimStartTime = 0;  // PCL unavailable
            frame2.appSimStartTime = 150;  // app available, but should not be used
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt when PCL source unavailable");
        }

        TEST_METHOD(AnimationError_PCLatency_TransitionFromZero_FirstValidPclSimStart)
        {
            // Scenario: First frame with valid PCL sim start
            // Expected: msAnimationError = std::nullopt (no prior PCL frame)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData present{};
            present.presentStartTime = 1000;
            present.timeInPresent = 100;
            present.pclSimStartTime = 100;  // first valid PCL
            present.finalState = PresentResult::Presented;
            present.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData nextPresent{};
            nextPresent.finalState = PresentResult::Presented;
            nextPresent.displayed.PushBack({ FrameType::Application, 2000 });

            auto results = ComputeMetricsForPresent(qpc, present, &nextPresent, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt on first valid PCL frame");
        }

        TEST_METHOD(AnimationError_PCLatency_TransitionFromAppToPcl_SourceSwitches)
        {
            // Scenario: Switching from CpuStart source to PCL
            // Expected: Source auto-switches when PCL data arrives
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::CpuStart;

            // Frame 1: Establish baseline
            FrameData frame1{};
            frame1.presentStartTime = 800;
            frame1.timeInPresent = 100;  // CPU end = 900
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1800 });

            // Frame 2: Continue with CPU source
            FrameData frame2{};
            frame2.presentStartTime = 1000;
            frame2.timeInPresent = 100;  // CPU end = 1100
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 2000 });

            // Frame 3: PCL becomes available, triggers source switch
            FrameData frame3{};
            frame3.presentStartTime = 1200;
            frame3.timeInPresent = 100;
            frame3.pclSimStartTime = 150;  // PCL data present
            frame3.appSimStartTime = 150;
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 2100 });

            // Frame 4: Next frame for completion
            FrameData frame4{};
            frame4.presentStartTime = 1400;
            frame4.timeInPresent = 100;
            frame4.finalState = PresentResult::Presented;
            frame4.displayed.PushBack({ FrameType::Application, 2200 });

            ComputeMetricsForPresent(qpc, frame1, &frame2, state);
            ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            // Frame 3 processes with CpuStart source, then UpdateChain switches to PCLatency
            auto results = ComputeMetricsForPresent(qpc, frame3, &frame4, state);

            // Animation error computed using CPU start (source still CpuStart during calculation)
            Assert::IsTrue(results[0].metrics.msAnimationError.has_value(),
                L"Animation error should be computed with CPU start before source switch");
            
            // After UpdateChain, source should have switched to PCLatency
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::PCLatency,
                L"Source should auto-switch to PCLatency after UpdateChain");
        }

        TEST_METHOD(AnimationError_PCLatency_SourcePriority_PclWinsOverApp)
        {
            // Scenario: Both PCL and App sim start present
            // Expected: Use PCL (source priority)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.pclSimStartTime = 100;
            frame1.appSimStartTime = 200;  // different value
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.pclSimStartTime = 150;  // PCL elapsed = 50
            frame2.appSimStartTime = 300;  // app elapsed = 100 (should be ignored)
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // display elapsed = 50

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            // Should use PCL: sim=50, display=50, error=0
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 150);  // PCL: 0.005 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 1050);
            double expected = simElapsed - displayElapsed;  // 0.0 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001,
                L"Should use PCL source, not app source");
        }

        // Section D: Animation Error – CpuStart Source

        TEST_METHOD(AnimationError_CpuStart_ComputedFromCpuPresent)
        {
            // Scenario: CpuStart source, CPU-derived sim times
            // Expected: Error computed from CPU present end times
            // Note: Need baseline frame to establish lastDisplayedSimStartTime
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::CpuStart;

            // Baseline frame to establish initial state
            FrameData frame1{};
            frame1.presentStartTime = 800;
            frame1.timeInPresent = 100;  // CPU sim start for next frame = 900
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1900 });

            FrameData frame2{};
            frame2.presentStartTime = 1000;
            frame2.timeInPresent = 100;  // CPU sim start for next frame = 1100
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 2000 });

            FrameData frame3{};
            frame3.presentStartTime = 1200;
            frame3.timeInPresent = 100;  // CPU sim start = 1300, elapsed from 1100 = 200
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 2050 });  // display elapsed from 2000 = 50

            FrameData dummyNext1{};
            dummyNext1.finalState = PresentResult::Presented;
            dummyNext1.displayed.PushBack({ FrameType::Application, 2500 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext1, state);

            FrameData dummyNext2{};
            dummyNext2.finalState = PresentResult::Presented;
            dummyNext2.displayed.PushBack({ FrameType::Application, 3000 });
            ComputeMetricsForPresent(qpc, frame2, &dummyNext2, state);

            FrameData frame4{};
            frame4.finalState = PresentResult::Presented;
            frame4.displayed.PushBack({ FrameType::Application, 4000 });
            auto results = ComputeMetricsForPresent(qpc, frame3, &frame4, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(1100, 1300);    // 0.020 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(2000, 2050); // 0.005 ms
            double expected = simElapsed - displayElapsed;  // 0.015 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001);
        }

        TEST_METHOD(AnimationError_CpuStart_Frame2DisplayIsGreaterThanFrame1Display)
        {
            // Scenario: CpuStart source, CPU-derived sim times
            //           Frame 2 has a display time earlier than Frame 1
            // Expected: NA reported for animation error
            // Note: Need baseline frame to establish lastDisplayedSimStartTime
            //       and lastDisplayedScreenTime
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::CpuStart;
            state.lastDisplayedScreenTime = 55454524262;
            state.lastDisplayedSimStartTime = 55454168764;
            state.lastDisplayedAppScreenTime = 55454524262;
            FrameData frame3{};
            frame3.presentStartTime = 55454299820;
            frame3.timeInPresent = 24537;
            state.lastPresent = frame3;

            FrameData frame1{};
            frame1.presentStartTime = 55454457377;
            frame1.timeInPresent = 2411;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 55454512384 });

            FrameData frame2{};
            frame2.presentStartTime = 55454612236;
            frame2.timeInPresent = 3056;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 55454615330 });

            ComputeMetricsForPresent(qpc, frame1, nullptr, state);
            auto results = ComputeMetricsForPresent(qpc, frame1, &frame2, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value());
        }

        TEST_METHOD(AnimationError_CpuStart_TransitionToAppProvider_Nullopt)
        {
            // Scenario: Source switches from CpuStart to AppProvider mid-stream
            // Expected: msAnimationError = std::nullopt (first frame of new source)
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::CpuStart;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 2000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 100;  // app instrumentation appears
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 2050 });

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 3000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 4000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt on source transition");
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Source should auto-switch to AppProvider");
        }

        // Section E: Disabled or Edge Cases

        TEST_METHOD(AnimationError_NotAppDisplayed_BothNullopt)
        {
            // Scenario: Frame not app-displayed (wrong displayIndex)
            // Expected: Both animation metrics = std::nullopt
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;
            state.firstAppSimStartTime = 100;
            state.lastDisplayedSimStartTime = 100;

            FrameData present{};
            present.presentStartTime = 1000;
            present.timeInPresent = 100;
            present.appSimStartTime = 150;
            present.finalState = PresentResult::Presented;
            present.displayed.PushBack({ FrameType::Repeated, 2000 });  // Not Application!

            FrameData nextPresent{};
            nextPresent.finalState = PresentResult::Presented;
            nextPresent.displayed.PushBack({ FrameType::Application, 3000 });

            auto results = ComputeMetricsForPresent(qpc, present, &nextPresent, state);

            Assert::AreEqual(size_t(1), results.size());
            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt for non-app frames");
            Assert::IsFalse(results[0].metrics.msAnimationTime.has_value(),
                L"msAnimationTime should be nullopt for non-app frames");
        }

        TEST_METHOD(AnimationError_FirstFrameEver_BothNullopt)
        {
            // Scenario: Very first frame, no prior state
            // Expected: Both animation metrics = std::nullopt
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};  // all zeros
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData present{};
            present.presentStartTime = 1000;
            present.timeInPresent = 100;
            present.appSimStartTime = 0;  // no instrumentation
            present.pclSimStartTime = 0;
            present.finalState = PresentResult::Presented;
            present.displayed.PushBack({ FrameType::Application, 2000 });

            FrameData nextPresent{};
            nextPresent.finalState = PresentResult::Presented;
            nextPresent.displayed.PushBack({ FrameType::Application, 3000 });

            auto results = ComputeMetricsForPresent(qpc, present, &nextPresent, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value());
            Assert::IsTrue(results[0].metrics.msAnimationTime.has_value());
        }

        TEST_METHOD(AnimationError_BackwardsScreenTime_ErrorStillComputed)
        {
            // Scenario: Screen time goes backward
            // Expected: nullopt for animation error
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1100 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 150;  // sim elapsed = 50
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1050 });  // screen time backward!

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"Error should be nullopt with backwards screen time");
        }

        TEST_METHOD(AnimationError_VeryLargeCadenceMismatch_LargeError)
        {
            // Scenario: Sim running much faster than display
            // Expected: Large positive error
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 500;  // sim elapsed = 400 ticks
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Application, 1010 });  // display elapsed = 10 ticks

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 2000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            FrameData frame3{};
            frame3.finalState = PresentResult::Presented;
            frame3.displayed.PushBack({ FrameType::Application, 3000 });
            auto results = ComputeMetricsForPresent(qpc, frame2, &frame3, state);

            Assert::IsTrue(results[0].metrics.msAnimationError.has_value());
            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 500);    // 0.040 ms
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 1010); // 0.001 ms
            double expected = simElapsed - displayElapsed;  // 0.039 ms
            Assert::AreEqual(expected, results[0].metrics.msAnimationError.value(), 0.0001,
                L"Large cadence mismatch should produce large positive error");
        }

        TEST_METHOD(AnimationError_RepeatedFrameType_BothNullopt)
        {
            // Scenario: Frame displayed but type is Repeated, not Application
            // Expected: Animation metrics should be nullopt
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;
            state.firstAppSimStartTime = 100;
            state.lastDisplayedSimStartTime = 100;

            FrameData present{};
            present.presentStartTime = 1000;
            present.timeInPresent = 100;
            present.appSimStartTime = 150;
            present.finalState = PresentResult::Presented;
            present.displayed.PushBack({ FrameType::Repeated, 2000 });

            FrameData nextPresent{};
            nextPresent.finalState = PresentResult::Presented;
            nextPresent.displayed.PushBack({ FrameType::Application, 3000 });

            auto results = ComputeMetricsForPresent(qpc, present, &nextPresent, state);

            Assert::IsFalse(results[0].metrics.msAnimationError.has_value(),
                L"msAnimationError should be nullopt for Repeated frame type");
            Assert::IsFalse(results[0].metrics.msAnimationTime.has_value(),
                L"msAnimationTime should be nullopt for Repeated frame type");
        }

        TEST_METHOD(AnimationError_MultipleDisplayInstances_OnlyLastAppIndex)
        {
            // Scenario: Present has 3 display instances, only middle one is Application
            // Expected: Animation computed only for Application display instance
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            state.animationErrorSource = AnimationErrorSource::AppProvider;

            // Setup prior frame
            FrameData frame1{};
            frame1.presentStartTime = 1000;
            frame1.timeInPresent = 100;
            frame1.appSimStartTime = 100;
            frame1.finalState = PresentResult::Presented;
            frame1.displayed.PushBack({ FrameType::Application, 1000 });

            // Frame with multiple display instances
            FrameData frame2{};
            frame2.presentStartTime = 2000;
            frame2.timeInPresent = 100;
            frame2.appSimStartTime = 150;
            frame2.finalState = PresentResult::Presented;
            frame2.displayed.PushBack({ FrameType::Repeated, 2000 });      // [0]
            frame2.displayed.PushBack({ FrameType::Application, 2050 });   // [1] - appIndex
            frame2.displayed.PushBack({ FrameType::Repeated, 2100 });      // [2]

            FrameData dummyNext{};
            dummyNext.finalState = PresentResult::Presented;
            dummyNext.displayed.PushBack({ FrameType::Application, 3000 });
            ComputeMetricsForPresent(qpc, frame1, &dummyNext, state);

            // Process frame2 without next (should process [0] and [1])
            auto resultsPartial = ComputeMetricsForPresent(qpc, frame2, nullptr, state);
            Assert::AreEqual(size_t(2), resultsPartial.size());

            // First display instance (Repeated) - no animation metrics
            Assert::IsFalse(resultsPartial[0].metrics.msAnimationError.has_value(),
                L"Display [0] (Repeated) should not have animation error");

            // Second display instance (Application) - has animation metrics
            Assert::IsTrue(resultsPartial[1].metrics.msAnimationError.has_value(),
                L"Display [1] (Application) should have animation error");

            double simElapsed = qpc.DeltaUnsignedMilliSeconds(100, 150);
            double displayElapsed = qpc.DeltaUnsignedMilliSeconds(1000, 2050);
            double expected = simElapsed - displayElapsed;
            Assert::AreEqual(expected, resultsPartial[1].metrics.msAnimationError.value(), 0.0001);
        }
        TEST_METHOD(Animation_AppProvider_PendingSequence_P1P2P3)
        {
            // This test mimics the real ReportMetrics pipeline for a single swapchain:
            //
            //  P1 arrives:
            //    - ComputeMetricsForPresent(P1, nullptr, state)   // Case 2, pending = P1
            //
            //  P2 arrives:
            //    - ComputeMetricsForPresent(P1, &P2, state)       // Case 3, finalize P1
            //    - ComputeMetricsForPresent(P2, nullptr, state)   // Case 2, pending = P2
            //
            //  P3 arrives:
            //    - ComputeMetricsForPresent(P2, &P3, state)       // Case 3, finalize P2
            //    - ComputeMetricsForPresent(P3, nullptr, state)   // Case 2, pending = P3
            //
            // We verify:
            //  - P1's final metrics: no animation error/time (it seeds animation state).
            //  - P2's final metrics: non-null msAnimationTime & msAnimationError == 0.0.
            //  - SwapChainCoreState's animation fields evolve correctly:
            //      firstAppSimStartTime = 100
            //      lastDisplayedSimStartTime = 200 after P2
            //      lastDisplayedAppScreenTime = P2's screenTime
            //
            // QPC frequency = 10 MHz.
            // App sim times:   P1=100, P2=200, P3=300
            // Screen times:    P1=1'000'000, P2=1'100'000, P3=1'200'000
            //   -> sim Δ P1→P2:       100 ticks = 0.01 ms
            //   -> display Δ P1→P2: 100'000 ticks = 0.01 ms
            //   => animation error for P2 = 0.0 ms
            //   => animation time for P2 = (200 - 100) / 10e6 = 0.01 ms

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // --------------------------------------------------------------------
            // P1: first displayed app frame with AppProvider data
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 10'000;
            p1.readyTime = 510'000;
            p1.appSimStartTime = 475'000;
            p1.pclSimStartTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // Arrival of P1 -> Case 2 (no nextDisplayed yet), becomes pending
            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"First call for P1 with next=nullptr should produce no metrics (pending only).");

            // Animation state should still be in CpuStart mode; no provider seeded yet.
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::CpuStart);
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedAppScreenTime);

            // --------------------------------------------------------------------
            // P2: second displayed app frame
            // --------------------------------------------------------------------
            FrameData p2{};
            p2.presentStartTime = 600'000;
            p2.timeInPresent = 10'000;
            p2.readyTime = 610'000;
            p2.appSimStartTime = 575'000;
            p2.pclSimStartTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // Arrival of P2:
            // 1) Flush pending P1 using P2 as nextDisplayed -> Case 3, finalize P1.
            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& p1_metrics = p1_final[0].metrics;

            // P1 is the FIRST provider-driven frame, so it should only seed the state:
            // no animation error or time yet.
            Assert::IsFalse(p1_metrics.msAnimationError.has_value(),
                L"P1 should not report animation error; it seeds the animation state.");
            Assert::IsTrue(p1_metrics.msAnimationTime.has_value(),
                L"P1 should report back 0.0.");
            Assert::AreEqual(double(0.0), p1_metrics.msAnimationTime.value());

            // UpdateAfterPresent should have run for P1 and switched to AppProvider:
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should transition to AppProvider after P1.");
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should latch P1's appSimStartTime.");
            Assert::AreEqual(uint64_t(475'000), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should match P1's appSimStartTime.");
            Assert::AreEqual(uint64_t(1'000'000), state.lastDisplayedAppScreenTime,
                L"lastDisplayedAppScreenTime should match P1's screenTime.");

            // 2) Now process P2's arrival as pending: Case 2 with next=nullptr
            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size(),
                L"First call for P2 with next=nullptr should produce no metrics (pending only).");

            // --------------------------------------------------------------------
            // P3: third displayed app frame
            // --------------------------------------------------------------------
            FrameData p3{};
            p3.presentStartTime = 700'000;
            p3.timeInPresent = 10'000;
            p3.readyTime = 710'000;
            p3.appSimStartTime = 675'000;
            p3.pclSimStartTime = 0;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'200'000 });

            // Arrival of P3:
            // 1) Flush pending P2 using P3 as nextDisplayed -> Case 3, finalize P2.
            auto p2_final = ComputeMetricsForPresent(qpc, p2, &p3, state);
            Assert::AreEqual(size_t(1), p2_final.size());
            const auto& p2_metrics = p2_final[0].metrics;

            // For P2:
            //  - previous displayed sim start = P1.appSimStartTime = 100
            //  - current sim start          = P2.appSimStartTime = 200
            //  - previous screen time       = P1.screenTime      = 1'000'000
            //  - current screen time        = P2.screenTime      = 1'100'000
            //  => simElapsed      = 100 ticks → 0.01 ms
            //  => displayElapsed  = 100'000 ticks → 0.01 ms
            //  => animationError  = 0.0 ms
            //  => animationTime   = (200 - 100) ticks from firstAppSimStartTime -> 0.01 ms

            Assert::IsTrue(p2_metrics.msAnimationError.has_value(),
                L"P2 should report animation error.");
            Assert::IsTrue(p2_metrics.msAnimationTime.has_value(),
                L"P2 should report animation time.");

            double expectedError = 0.0;
            Assert::AreEqual(expectedError, p2_metrics.msAnimationError.value(), 0.0001,
                L"P2's msAnimationError should be 0.0 when sim and display deltas match.");

            double expectedAnim = qpc.DeltaUnsignedMilliSeconds(475'000, 575'000);
            Assert::AreEqual(expectedAnim, p2_metrics.msAnimationTime.value(), 0.0001,
                L"P2's msAnimationTime should be based on firstAppSimStartTime (100) to current sim (200).");

            // After finalizing P2, chain state should now reflect P2 as "last displayed"
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain anchored to P1.");
            Assert::AreEqual(uint64_t(575'000), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should advance to P2's appSimStartTime.");
            Assert::AreEqual(uint64_t(1'100'000), state.lastDisplayedAppScreenTime,
                L"lastDisplayedAppScreenTime should advance to P2's screenTime.");

            // 2) Finally, process P3 as the new pending (Case 2 with next=nullptr).
            auto p3_phase1 = ComputeMetricsForPresent(qpc, p3, nullptr, state);
            Assert::AreEqual(size_t(0), p3_phase1.size(),
                L"First call for P3 with next=nullptr should produce no metrics (pending only).");
        }
        // ========================================================================
        // A7: Animation_AppProvider_PendingSequence_P2Discarded_SkipsAnimation
        // ========================================================================
        TEST_METHOD(Animation_AppProvider_PendingSequence_P2Discarded_SkipsAnimation)
        {
            // This test mimics the real ReportMetrics pipeline for a single swapchain
            // when a middle frame (P2) is discarded and never displayed.
            //
            //  P1 arrives (displayed, has AppProvider sim start):
            //    - ComputeMetricsForPresent(P1, nullptr, state)   // Case 2, pending = P1
            //
            //  P2 arrives (DISCARDED, not displayed, but with appSimStartTime):
            //    - ComputeMetricsForPresent(P2, nullptr, state)   // Case 1, not displayed
            //      * Should produce a single metrics entry with NO animation time/error
            //      * Must NOT change firstAppSimStartTime / lastDisplayedSimStartTime
            //
            //  P3 arrives (displayed, has AppProvider sim start):
            //    - ComputeMetricsForPresent(P1, &P3, state)       // Case 3, finalize P1
            //      * P1 is the FIRST provider-driven displayed frame, so it seeds state.
            //      * P1 should not report animation time/error.
            //      * State switches to AppProvider and latches P1's sim + screen times.
            //    - ComputeMetricsForPresent(P3, nullptr, state)   // Case 2, pending = P3
            //
            // App sim times are in QPC domain:
            //   P1.appSimStartTime = 475'000
            //   P2.appSimStartTime = 575'000
            //   P3.appSimStartTime = 675'000
            //
            // Screen times:
            //   P1.screenTime = 1'000'000
            //   P2 has no screenTime (discarded, not displayed)
            //   P3.screenTime = 1'100'000
            //
            // We verify:
            //   - P2's metrics have no msAnimationTime / msAnimationError.
            //   - P2 does not change firstAppSimStartTime / lastDisplayedSimStartTime.
            //   - After finalizing P1 with P3 as nextDisplayed:
            //       * animationErrorSource == AppProvider
            //       * firstAppSimStartTime      == 475'000
            //       * lastDisplayedSimStartTime == 475'000
            //       * lastDisplayedAppScreenTime == 1'000'000

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // --------------------------------------------------------------------
            // P1: first displayed app frame with AppProvider data
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 10'000;
            p1.readyTime = 510'000;
            p1.appSimStartTime = 475'000;   // APC-provided sim start (QPC ticks)
            p1.pclSimStartTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 }); // screen time

            // P1 arrives -> Case 2 (no nextDisplayed yet), becomes pending
            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"First call for P1 with next=nullptr should produce no metrics (pending only).");

            // Still in CpuStart mode; no provider-seeded animation state yet.
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::CpuStart);
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(0), state.lastDisplayedAppScreenTime);

            // --------------------------------------------------------------------
            // P2: discarded, not displayed, but with AppProvider sim start
            // --------------------------------------------------------------------
            FrameData p2{};
            p2.presentStartTime = 600'000;
            p2.timeInPresent = 10'000;
            p2.readyTime = 610'000;
            p2.appSimStartTime = 575'000;   // APC timestamp, but this frame is not displayed
            p2.pclSimStartTime = 0;
            p2.finalState = PresentResult::Discarded;
            // No displayed entries -> not displayed

            auto p2_results = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(1), p2_results.size(),
                L"Discarded frame should produce a single not-displayed metrics entry.");

            const auto& p2_metrics = p2_results[0].metrics;

            // Discarded / not-displayed frame must NOT produce animation metrics.
            Assert::IsFalse(p2_metrics.msAnimationTime.has_value(),
                L"P2 (discarded) should not have msAnimationTime.");
            Assert::IsFalse(p2_metrics.msAnimationError.has_value(),
                L"P2 (discarded) should not have msAnimationError.");

            // And it must NOT disturb animation anchors, since it's not displayed.
            Assert::AreEqual(uint64_t(0), state.firstAppSimStartTime,
                L"P2 must not set firstAppSimStartTime; only displayed App/PCL frames do that.");
            Assert::AreEqual(uint64_t(0), state.lastDisplayedSimStartTime,
                L"P2 must not change lastDisplayedSimStartTime when not displayed.");
            Assert::AreEqual(uint64_t(0), state.lastDisplayedAppScreenTime,
                L"P2 must not change lastDisplayedAppScreenTime when not displayed.");

            // (Optional sanity: lastSimStartTime may track P2's appSimStartTime, which is fine for
            // simulation plumbing but not for animation anchors.)

            // --------------------------------------------------------------------
            // P3: next displayed app frame
            // --------------------------------------------------------------------
            FrameData p3{};
            p3.presentStartTime = 700'000;
            p3.timeInPresent = 10'000;
            p3.readyTime = 710'000;
            p3.appSimStartTime = 675'000;   // another +100'000 ticks in sim space
            p3.pclSimStartTime = 0;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'100'000 }); // next screen time

            // P3 arrives:
            // 1) Flush pending P1 using P3 as nextDisplayed -> Case 3, finalize P1.
            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p3, state);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& p1_metrics = p1_final[0].metrics;

            // P1 is the FIRST displayed frame with AppProvider sim start.
            // It should only seed animation state; no error/time yet.
            Assert::IsFalse(p1_metrics.msAnimationError.has_value(),
                L"P1 should not report animation error; it seeds the animation state.");
            Assert::IsTrue(p1_metrics.msAnimationTime.has_value(),
                L"P1 should have an animation time of 0.0.");
            Assert::AreEqual(double(0.0), p1_metrics.msAnimationTime.value());

            // After finalizing P1, we must now be in AppProvider mode with anchors from P1.
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should transition to AppProvider after first displayed AppSimStart frame (P1).");
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should latch P1's appSimStartTime.");
            Assert::AreEqual(uint64_t(475'000), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should match P1's appSimStartTime after P1 is finalized.");
            Assert::AreEqual(uint64_t(1'000'000), state.lastDisplayedAppScreenTime,
                L"lastDisplayedAppScreenTime should match P1's screenTime.");

            // 2) Process P3 as the new pending frame (Case 2 with next=nullptr).
            auto p3_phase1 = ComputeMetricsForPresent(qpc, p3, nullptr, state);
            Assert::AreEqual(size_t(0), p3_phase1.size(),
                L"First call for P3 with next=nullptr should produce no metrics (pending only).");

            // State remains anchored to P1 until a later frame finalizes P3 with a true nextDisplayed.
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should remain anchored to P1 after P3's pending pass.");
            Assert::AreEqual(uint64_t(475'000), state.lastDisplayedSimStartTime,
                L"lastDisplayedSimStartTime should still reflect P1 until P3 is finalized.");
        }
    };
    // ============================================================================
    // SECTION: Input Latency Tests
    // ============================================================================

    TEST_CLASS(InputLatencyTests)
    {
    public:
        // ========================================================================
        // Test 1: ClickToPhoton - displayed frame uses its own click
        // ========================================================================
        TEST_METHOD(InputLatency_ClickToPhoton_DisplayedFrame_UsesOwnClickTime)
        {
            // Scenario:
            // - P1 (displayed frame) has its own mouseClickTime = 400'000
            // - P1 computes msClickToPhotonLatency from click to its own display time
            // - No pending click should remain in the chain
            //
            // Expected:
            // - P1's msClickToPhotonLatency uses P1's own click (400'000 -> 1'000'000)
            // - state.lastReceivedNotDisplayedMouseClickTime == 0

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // P1: displayed app frame with its own click
            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 100'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.appSimStartTime = 450'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // P2: next displayed frame
            FrameData p2{};
            p2.presentStartTime = 1'050'000;
            p2.timeInPresent = 50'000;
            p2.mouseClickTime = 0;
            p2.inputTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // P1 arrives (pending)
            auto p1_pending = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_pending.size(), L"P1 pending should be empty");

            // P2 arrives, finalizes P1
            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            auto p2_pending = ComputeMetricsForPresent(qpc, p2, nullptr, state);

            // Assertions for P1
            Assert::AreEqual(size_t(1), p1_final.size());
            Assert::IsTrue(p1_final[0].metrics.msClickToPhotonLatency.has_value(),
                L"P1 should have msClickToPhotonLatency");

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            Assert::AreEqual(expected, p1_final[0].metrics.msClickToPhotonLatency.value(), 0.0001,
                L"P1's click-to-photon should use its own click time");

            // Verify no pending click remains
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedMouseClickTime,
                L"No pending click should remain after P1 used its own click");
        }

        // ========================================================================
        // Test 2: ClickToPhoton - dropped frame carries click to next displayed
        // ========================================================================
        TEST_METHOD(InputLatency_ClickToPhoton_DroppedFrame_CarriesClickToNextDisplayed)
        {
            // Scenario:
            // - P1 (dropped, not displayed) has mouseClickTime = 400'000
            // - P1 does not produce msClickToPhotonLatency
            // - P1 stores click in lastReceivedNotDisplayedMouseClickTime
            // - P2 (displayed, no own click) uses the stored click from P1
            //
            // Expected:
            // - P1: msClickToPhotonLatency == nullopt
            // - After P1: state.lastReceivedNotDisplayedMouseClickTime == 400'000
            // - P2: msClickToPhotonLatency uses stored click (400'000 -> 1'000'000)
            // - After P2: state.lastReceivedNotDisplayedMouseClickTime == 0 (consumed)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // P1: dropped frame with click
            FrameData p1{};
            p1.presentStartTime = 300'000;
            p1.timeInPresent = 50'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.finalState = PresentResult::Discarded;
            // displayed is empty (not displayed)

            // P2: first displayed frame (no own click)
            FrameData p2{};
            p2.presentStartTime = 900'000;
            p2.timeInPresent = 100'000;
            p2.mouseClickTime = 0;
            p2.inputTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // P3: later frame to finalize P2
            FrameData p3{};
            p3.presentStartTime = 1'050'000;
            p3.timeInPresent = 50'000;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // P1 arrives (dropped)
            auto p1_results = ComputeMetricsForPresent(qpc, p1, nullptr, state);

            // Assertions for P1
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsFalse(p1_results[0].metrics.msClickToPhotonLatency.has_value(),
                L"P1 (dropped) should not have msClickToPhotonLatency");
            Assert::AreEqual(uint64_t(400'000), state.lastReceivedNotDisplayedMouseClickTime,
                L"P1's click should be stored as pending");

            // P2 arrives (pending)
            auto p2_pending = ComputeMetricsForPresent(qpc, p2, nullptr, state);

            // P3 arrives, finalizes P2
            auto p2_final = ComputeMetricsForPresent(qpc, p2, &p3, state);
            auto p3_pending = ComputeMetricsForPresent(qpc, p3, nullptr, state);

            // Assertions for P2
            Assert::AreEqual(size_t(1), p2_final.size());
            Assert::IsTrue(p2_final[0].metrics.msClickToPhotonLatency.has_value(),
                L"P2 should have msClickToPhotonLatency using P1's stored click");

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            Assert::AreEqual(expected, p2_final[0].metrics.msClickToPhotonLatency.value(), 0.0001,
                L"P2's click-to-photon should use P1's stored click");

            // Optional: verify pending click is consumed
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedMouseClickTime,
                L"Pending click should be consumed after P2 uses it");
        }

        // ========================================================================
        // Test 3: AllInputPhoton - multiple dropped frames, last input wins
        // ========================================================================
        TEST_METHOD(InputLatency_AllInputPhoton_MultipleDroppedFrames_LastInputWins)
        {
            // Scenario:
            // - P1 (dropped) has inputTime = 300'000
            // - P2 (dropped) has inputTime = 450'000 (should override P1)
            // - P3 (displayed, no own input) uses the last stored input (450'000)
            //
            // Expected:
            // - P1: msAllInputPhotonLatency == nullopt, state stores 300'000
            // - P2: msAllInputPhotonLatency == nullopt, state updates to 450'000
            // - P3: msAllInputPhotonLatency uses 450'000 (last wins)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // P1: dropped with first input
            FrameData p1{};
            p1.presentStartTime = 200'000;
            p1.timeInPresent = 50'000;
            p1.inputTime = 300'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Discarded;
            // displayed empty

            // P2: dropped with later input (overrides P1)
            FrameData p2{};
            p2.presentStartTime = 400'000;
            p2.timeInPresent = 50'000;
            p2.inputTime = 450'000;
            p2.mouseClickTime = 0;
            p2.finalState = PresentResult::Discarded;
            // displayed empty

            // P3: first displayed frame (no own input)
            FrameData p3{};
            p3.presentStartTime = 900'000;
            p3.timeInPresent = 100'000;
            p3.inputTime = 0;
            p3.mouseClickTime = 0;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // P4: later frame to finalize P3
            FrameData p4{};
            p4.presentStartTime = 1'050'000;
            p4.timeInPresent = 50'000;
            p4.finalState = PresentResult::Presented;
            p4.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // P1 arrives (dropped)
            auto p1_results = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsFalse(p1_results[0].metrics.msAllInputPhotonLatency.has_value(),
                L"P1 (dropped) should not have msAllInputPhotonLatency");
            Assert::AreEqual(uint64_t(300'000), state.lastReceivedNotDisplayedAllInputTime,
                L"P1's input should be stored");

            // P2 arrives (dropped, overrides P1)
            auto p2_results = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsFalse(p2_results[0].metrics.msAllInputPhotonLatency.has_value(),
                L"P2 (dropped) should not have msAllInputPhotonLatency");
            Assert::AreEqual(uint64_t(450'000), state.lastReceivedNotDisplayedAllInputTime,
                L"P2's input should override P1's stored input (last wins)");

            // P3 arrives (pending)
            auto p3_pending = ComputeMetricsForPresent(qpc, p3, nullptr, state);

            // P4 arrives, finalizes P3
            auto p3_final = ComputeMetricsForPresent(qpc, p3, &p4, state);
            auto p4_pending = ComputeMetricsForPresent(qpc, p4, nullptr, state);

            // Assertions for P3
            Assert::AreEqual(size_t(1), p3_final.size());
            Assert::IsTrue(p3_final[0].metrics.msAllInputPhotonLatency.has_value(),
                L"P3 should have msAllInputPhotonLatency using last stored input");

            double expected = qpc.DeltaUnsignedMilliSeconds(450'000, 1'000'000);
            Assert::AreEqual(expected, p3_final[0].metrics.msAllInputPhotonLatency.value(), 0.0001,
                L"P3's all-input-to-photon should use P2's input (last wins)");
        }

        // ========================================================================
        // Test 4: AllInputPhoton - displayed frame with own input overrides pending
        // ========================================================================
        TEST_METHOD(InputLatency_AllInputPhoton_DisplayedFrame_WithOwnInput_OverridesPending)
        {
            // Scenario:
            // - P0 (dropped) seeds pending input = 300'000
            // - P1 (displayed) has its own inputTime = 500'000
            // - P1's own input should override the pending 300'000
            //
            // Expected:
            // - P0: state.lastReceivedNotDisplayedAllInputTime == 300'000
            // - P1: msAllInputPhotonLatency uses P1's own input (500'000 -> 1'000'000)

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // P0: dropped, seeds pending input
            FrameData p0{};
            p0.presentStartTime = 200'000;
            p0.timeInPresent = 50'000;
            p0.inputTime = 300'000;
            p0.mouseClickTime = 0;
            p0.finalState = PresentResult::Discarded;
            // displayed empty

            // P1: displayed with its own input
            FrameData p1{};
            p1.presentStartTime = 900'000;
            p1.timeInPresent = 100'000;
            p1.inputTime = 500'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // P2: later frame to finalize P1
            FrameData p2{};
            p2.presentStartTime = 1'050'000;
            p2.timeInPresent = 50'000;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // P0 arrives (dropped)
            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(uint64_t(300'000), state.lastReceivedNotDisplayedAllInputTime,
                L"P0's input should be stored as pending");

            // P1 arrives (pending)
            auto p1_pending = ComputeMetricsForPresent(qpc, p1, nullptr, state);

            // P2 arrives, finalizes P1
            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            auto p2_pending = ComputeMetricsForPresent(qpc, p2, nullptr, state);

            // Assertions for P1
            Assert::AreEqual(size_t(1), p1_final.size());
            Assert::IsTrue(p1_final[0].metrics.msAllInputPhotonLatency.has_value(),
                L"P1 should have msAllInputPhotonLatency using its own input");

            double expected = qpc.DeltaUnsignedMilliSeconds(500'000, 1'000'000);
            Assert::AreEqual(expected, p1_final[0].metrics.msAllInputPhotonLatency.value(), 0.0001,
                L"P1's all-input-to-photon should use its own input (500'000), not pending (300'000)");
        }

        // ========================================================================
        // Test 5: InstrumentedInputTime - uses same sim start as animation source (AppProvider)
        // ========================================================================
        TEST_METHOD(InputLatency_InstrumentedInputTime_UsesAppInputSample)
        {
            // Scenario:
            // - Frame 1: First AppProvider frame (appSimStartTime = 475'000) - seeds state
            // - Frame 2: Has appInputSample = 500'000, appSimStartTime = 575'000
            // - msInstrumentedInputTime should use Frame 2's appInputSample time = 500'000
            // - Frame 2: Display time is 1'100'000
            //
            // Expected:
            // - After Frame 1: animationErrorSource == AppProvider
            // - Frame 2: msInstrumentedInputTime = (1'100'000 - 500'000) in ms

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};
            // state.animationErrorSource defaults to CpuStart

            // P1: First AppProvider frame (no input)
            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 100'000;
            p1.appSimStartTime = 475'000;
            p1.pclSimStartTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            // P2: Frame with input (we assert on this)
            FrameData p2{};
            p2.presentStartTime = 1'000'000;
            p2.timeInPresent = 100'000;
            p2.appSimStartTime = 575'000;
            p2.pclSimStartTime = 0;
            p2.appInputSample.first = 500'000; // Using same value for simplicity
            p2.appInputSample.second = InputDeviceType::Mouse;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            // P3: Later frame to finalize P2
            FrameData p3{};
            p3.presentStartTime = 1'500'000;
            p3.timeInPresent = 100'000;
            p3.appSimStartTime = 675'000;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'200'000 });

            // P1 arrives (pending)
            auto p1_pending = ComputeMetricsForPresent(qpc, p1, nullptr, state);

            // P2 arrives, finalizes P1 (switches to AppProvider)
            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            auto p2_pending = ComputeMetricsForPresent(qpc, p2, nullptr, state);

            // Verify state after P1
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should switch to AppProvider after P1");
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should be set to P1's appSimStartTime");

            // P3 arrives, finalizes P2
            auto p2_final = ComputeMetricsForPresent(qpc, p2, &p3, state);
            auto p3_pending = ComputeMetricsForPresent(qpc, p3, nullptr, state);

            // Assertions for P2
            Assert::AreEqual(size_t(1), p2_final.size());

            // Verify P2 has animation time (sanity check we're in AppProvider mode)
            Assert::IsTrue(p2_final[0].metrics.msAnimationTime.has_value(),
                L"P2 should have msAnimationTime (AppProvider mode)");

            // Verify msInstrumentedInputTime is present
            Assert::IsTrue(p2_final[0].metrics.msInstrumentedInputTime.has_value(),
                L"P2 should have msInstrumentedInputTime");

            // Calculate expected: app input -> p2 screen
            double expectedInstr = qpc.DeltaUnsignedMilliSeconds(500'000, 1'100'000);
            Assert::AreEqual(expectedInstr, p2_final[0].metrics.msInstrumentedInputTime.value(), 0.0001,
                L"msInstrumentedInputTime should be P2 app input time to P2 screen time");
        }
    };
    TEST_CLASS(PcLatencyTests)
    {
    public:

        TEST_METHOD(PcLatency_PendingSequence_DroppedDroppedDisplayed_P0P1P2P3)
        {
            // This test mimics the real ReportMetrics pipeline for a single swapchain,
            // focusing on the PC Latency accumulation across *dropped* frames, and its
            // completion when the corresponding frame finally reaches the screen.
            //
            // We use four presents:
            //
            //  P0: DROPPED
            //      - PclInputPingTime = 10'000
            //      - PclSimStartTime  = 20'000
            //      -> initializes accumulatedInput2FrameStartTime with Δ(PING0, SIM0).
            //
            //  P1: DROPPED
            //      - PclInputPingTime = 0
            //      - PclSimStartTime  = 30'000
            //      -> extends accumulatedInput2FrameStartTime with Δ(SIM0, SIM1).
            //
            //  P2: DISPLAYED
            //      - PclInputPingTime = 0
            //      - PclSimStartTime  = 40'000
            //      - ScreenTime       = 50'000
            //
            //  P3: DISPLAYED
            //      - no PCL data; used only as "nextDisplayed" for P2 to mimic ReportMetrics.
            //
            // QPC frequency = 10 MHz.
            //
            // Timing (ticks):
            //   PING0 = 10'000
            //   SIM0  = 20'000
            //   SIM1  = 30'000
            //   SIM2  = 40'000
            //   SCR2  = 50'000
            //
            //   Δ(PING0, SIM0) = 10'000 ticks = 1.0 ms
            //   Δ(SIM0,  SIM1) = 10'000 ticks = 1.0 ms
            //   Δ(SIM1,  SIM2) = 10'000 ticks = 1.0 ms
            //   => full input→frame-start for this chain = 3.0 ms
            //
            //   Δ(SIM2,  SCR2) = 10'000 ticks = 1.0 ms
            //
            //   Legacy behavior:
            //     - AccumulatedInput2FrameStartTime builds to 3.0 ms over P0/P1/P2.
            //     - EMA seeds from that full 3.0 ms sample when P2 finally completes.
            //     - PC Latency for P2 ≈ 3.0 ms (input→frame-start) + 1.0 ms (sim→screen).
            //
            // The pipeline calls we mimic:
            //
            //  P0 arrives (dropped):
            //    - ComputeMetricsForPresent(P0, nullptr, state)   // Case 1, not displayed
            //
            //  P1 arrives (dropped):
            //    - ComputeMetricsForPresent(P1, nullptr, state)   // Case 1, not displayed
            //
            //  P2 arrives (displayed):
            //    - ComputeMetricsForPresent(P2, nullptr, state)   // Case 2, pending only (no metrics yet)
            //
            //  P3 arrives (displayed):
            //    - ComputeMetricsForPresent(P2, &P3, state)       // Case 3, finalize P2
            //    - ComputeMetricsForPresent(P3, nullptr, state)   // Case 2, pending = P3
            //
            // We verify:
            //  - P0 & P1 produce no msPcLatency (dropped frames).
            //  - accumulatedInput2FrameStartTime grows after P0 and P1.
            //  - P2's first call (next=nullptr) produces no metrics and does NOT
            //    disturb the accumulatedInput2FrameStartTime.
            //  - The final P2 call (with next=P3) produces a non-null msPcLatency,
            //    and resets accumulatedInput2FrameStartTime and lastReceivedNotDisplayedPclSimStart
            //    to 0, matching the legacy PCL behavior.
            //
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            const uint32_t PROCESS_ID = 1234;
            const uint64_t SWAPCHAIN = 0xABC0;

            // --------------------------------------------------------------------
            // P0: DROPPED, first PCL frame with Ping+Sim
            // --------------------------------------------------------------------
            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.presentStartTime = 0;
            p0.timeInPresent = 0;
            p0.readyTime = 0;
            p0.appSimStartTime = 0;

            p0.pclInputPingTime = 10'000;  // PING0
            p0.pclSimStartTime = 20'000;  // SIM0

            p0.finalState = PresentResult::Discarded;
            p0.displayed.Clear();          // not displayed

            // P0 arrival -> Case 1 (not displayed), process immediately.
            auto p0_metrics_list = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_metrics_list.size(),
                L"P0: not-displayed present should produce a single metrics record.");

            const auto& p0_metrics = p0_metrics_list[0].metrics;

            // Dropped frames never report PC latency directly.
            Assert::IsFalse(p0_metrics.msPcLatency.has_value(),
                L"P0: dropped frame should not report msPcLatency.");

            // Accumulator should have been initialized from Ping0 -> Sim0.
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > 0.0,
                L"P0: accumulatedInput2FrameStartTime should be initialized and > 0.");
            Assert::AreEqual(uint64_t(20'000), state.lastReceivedNotDisplayedPclSimStart,
                L"P0: lastReceivedNotDisplayedPclSimStart should match P0's pclSimStartTime (20'000).");

            const double accumAfterP0 = state.accumulatedInput2FrameStartTime;

            // --------------------------------------------------------------------
            // P1: DROPPED, continuation of same PCL chain (Sim only)
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.presentStartTime = 0;
            p1.timeInPresent = 0;
            p1.readyTime = 0;
            p1.appSimStartTime = 0;

            p1.pclInputPingTime = 0;       // no new ping
            p1.pclSimStartTime = 30'000;  // SIM1

            p1.finalState = PresentResult::Discarded;
            p1.displayed.Clear();          // not displayed

            auto p1_metrics_list = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(1), p1_metrics_list.size(),
                L"P1: not-displayed present should produce a single metrics record.");

            const auto& p1_metrics = p1_metrics_list[0].metrics;

            Assert::IsFalse(p1_metrics.msPcLatency.has_value(),
                L"P1: dropped frame should not report msPcLatency.");

            // Accumulator should have grown: now includes SIM0->SIM1 as well.
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > accumAfterP0,
                L"P1: accumulatedInput2FrameStartTime should be greater than after P0.");
            Assert::AreEqual(uint64_t(30'000), state.lastReceivedNotDisplayedPclSimStart,
                L"P1: lastReceivedNotDisplayedPclSimStart should match P1's pclSimStartTime (30'000).");

            const double accumAfterP1 = state.accumulatedInput2FrameStartTime;

            // --------------------------------------------------------------------
            // P2: DISPLAYED, Sim only – this is the frame where the pending input
            // will finally be visible on-screen. However, the metrics for P2 are
            // only finalized when we know P3 (nextDisplayed), just like in the
            // ReportMetrics pipeline.
            // --------------------------------------------------------------------
            FrameData p2{};
            p2.processId = PROCESS_ID;
            p2.swapChainAddress = SWAPCHAIN;
            p2.presentStartTime = 0;
            p2.timeInPresent = 0;
            p2.readyTime = 0;
            p2.appSimStartTime = 0;

            p2.pclInputPingTime = 0;        // no new ping
            p2.pclSimStartTime = 40'000;   // SIM2

            p2.finalState = PresentResult::Presented;
            p2.displayed.Clear();
            p2.displayed.PushBack({ FrameType::Application, 50'000 });  // SCR2

            // P2 arrival: Case 2 (displayed, no nextDisplayed), becomes pending.
            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size(),
                L"P2 (phase 1): first call with nextDisplayed=nullptr should produce no metrics (pending only).");

            // The pending call MUST NOT disturb the accumulated PCL chain.
            Assert::AreEqual(accumAfterP1, state.accumulatedInput2FrameStartTime, 1e-9,
                L"P2 (phase 1): accumulatedInput2FrameStartTime should remain unchanged while pending.");
            Assert::AreEqual(uint64_t(30'000), state.lastReceivedNotDisplayedPclSimStart,
                L"P2 (phase 1): lastReceivedNotDisplayedPclSimStart should remain at P1's sim start (30'000).");

            // --------------------------------------------------------------------
            // P3: DISPLAYED, used only as nextDisplayed when finalizing P2.
            // --------------------------------------------------------------------
            FrameData p3{};
            p3.processId = PROCESS_ID;
            p3.swapChainAddress = SWAPCHAIN;
            p3.presentStartTime = 0;
            p3.timeInPresent = 0;
            p3.readyTime = 0;
            p3.appSimStartTime = 0;

            p3.pclInputPingTime = 0;
            p3.pclSimStartTime = 0; // no PCL for P3 itself

            p3.finalState = PresentResult::Presented;
            p3.displayed.Clear();
            p3.displayed.PushBack({ FrameType::Application, 60'000 }); // some later screen time

            // P3 arrival:
            //  1) Flush pending P2 using P3 as nextDisplayed -> Case 3, finalize P2.
            auto p2_final = ComputeMetricsForPresent(qpc, p2, &p3, state);
            Assert::AreEqual(size_t(1), p2_final.size(),
                L"P2 (final): expected exactly one metrics record when flushing with nextDisplayed=P3.");
            const auto& p2_metrics = p2_final[0].metrics;

            //  2) Now process P3's arrival as pending -> Case 2 with next=nullptr.
            auto p3_phase1 = ComputeMetricsForPresent(qpc, p3, nullptr, state);
            Assert::AreEqual(size_t(0), p3_phase1.size(),
                L"P3 (phase 1): first call with nextDisplayed=nullptr should produce no metrics (pending only).");

            // --------------------------------------------------------------------
            // Assertions for the P2 finalization (this is where PC Latency must appear).
            // --------------------------------------------------------------------

            // Precondition: we had a non-zero accumulated input→frame-start before finalizing P2.
            Assert::IsTrue(accumAfterP1 > 0.0,
                L"Precondition: expected non-zero accumulatedInput2FrameStartTime before P2 finalization.");

            // 1) PC Latency should be populated and positive for P2 when it finally
            //    reaches the screen after the dropped chain.
            Assert::IsTrue(p2_metrics.msPcLatency.has_value(),
                L"P2 (final): msPcLatency should be populated for the displayed frame completing the dropped PCL chain.");
            Assert::IsTrue(p2_metrics.msPcLatency.value() > 0.0,
                L"P2 (final): msPcLatency should be positive.");

            // 2) After completion, the accumulated input→frame-start time and the
            //    last-not-displayed PCL sim start should be reset to zero, matching
            //    the legacy PCL behavior.
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 1e-9,
                L"P2 (final): accumulatedInput2FrameStartTime should be reset to 0 after completion.");
            Assert::AreEqual(uint64_t{ 0 }, state.lastReceivedNotDisplayedPclSimStart,
                L"P2 (final): lastReceivedNotDisplayedPclSimStart should be reset to 0 after completion.");
        }

        TEST_METHOD(PcLatency_NoPclData_AllFrames_NoLatency)
        {
            // Scenario:
            //  - P0 is dropped with no PC Latency timestamps.
            //  - P1 and P2 are displayed app frames but likewise carry no pclSimStartTime/pclInputPingTime.
            //  - We run the ReportMetrics-style scheduling: dropped frames are processed immediately,
            //    displayed frames are first queued (Case 2) and then finalized by the arrival of the
            //    next displayed present (Case 3).
            // QPC plan (ticks at 10 MHz):
            //  - Screen times: SCR1 = 100'000, SCR2 = 120'000, SCR3 = 140'000.
            // Expectations:
            //  - Every metrics record reports msPcLatency.has_value() == false.
            //  - accumulatedInput2FrameStartTime remains 0.0 throughout the sequence.
            //  - lastReceivedNotDisplayedPclSimStart never departs from 0 since no PCL timestamps exist.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            const uint32_t PROCESS_ID = 77;
            const uint64_t SWAPCHAIN = 0x11AAu;

            // --------------------------------------------------------------------
            // P0: dropped frame without any PCL data
            // --------------------------------------------------------------------
            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.pclInputPingTime = 0;
            p0.pclSimStartTime = 0;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_results.size(),
                L"P0 (dropped) should emit one metrics record.");
            Assert::IsFalse(p0_results[0].metrics.msPcLatency.has_value(),
                L"P0 should not report msPcLatency without PCL data.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"P0 should not modify accumulatedInput2FrameStartTime when there is no PCL data.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"P0 should leave lastReceivedNotDisplayedPclSimStart at 0.");

            // --------------------------------------------------------------------
            // P1: displayed frame without PCL data (pending, then finalized by P2)
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"P1 pending pass should not emit metrics.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"State.accumulatedInput2FrameStartTime must remain 0 after P1 pending pass.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"lastReceivedNotDisplayedPclSimStart should remain 0 after P1 pending pass.");

            // --------------------------------------------------------------------
            // P2: displayed frame without PCL data (finalizes P1, then becomes pending)
            // --------------------------------------------------------------------
            FrameData p2{};
            p2.processId = PROCESS_ID;
            p2.swapChainAddress = SWAPCHAIN;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 120'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            Assert::AreEqual(size_t(1), p1_final.size(),
                L"Finalizing P1 should emit exactly one metrics record.");
            Assert::IsFalse(p1_final[0].metrics.msPcLatency.has_value(),
                L"P1 final metrics should not report msPcLatency without PCL data.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulated input-to-frame-start time must remain 0 after finalizing P1.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"lastReceivedNotDisplayedPclSimStart should remain 0 after finalizing P1.");

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size(),
                L"P2 pending pass should not emit metrics.");

            // --------------------------------------------------------------------
            // P3: helper displayed frame to flush P2
            // --------------------------------------------------------------------
            FrameData p3{};
            p3.processId = PROCESS_ID;
            p3.swapChainAddress = SWAPCHAIN;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 140'000 });

            auto p2_final = ComputeMetricsForPresent(qpc, p2, &p3, state);
            Assert::AreEqual(size_t(1), p2_final.size(),
                L"Finalizing P2 should emit exactly one metrics record.");
            Assert::IsFalse(p2_final[0].metrics.msPcLatency.has_value(),
                L"P2 final metrics should not report msPcLatency without PCL data.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulated input-to-frame-start time must still be 0 after P2.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"lastReceivedNotDisplayedPclSimStart should remain 0 through the entire sequence.");

            auto p3_phase1 = ComputeMetricsForPresent(qpc, p3, nullptr, state);
            Assert::AreEqual(size_t(0), p3_phase1.size(),
                L"P3 pending pass is only for completeness and should not emit metrics.");
        }

        TEST_METHOD(PcLatency_SingleDisplayed_DirectSample_FirstEma)
        {
            // Scenario:
            //  - Single displayed frame P0 provides both pclInputPingTime and pclSimStartTime.
            //  - There is no subsequent present, so we exercise Case 2 (nextDisplayed == nullptr)
            //    with two display samples on P0 to mirror the ReportMetrics behavior where the
            //    last display instance is deferred.
            //  - The first metrics record must report msPcLatency immediately and seed the EMA.
            // Timing (ticks at 10 MHz):
            //  - Ping0 = 10'000, Sim0 = 20'000 (Δ = 1.0 ms)
            //  - Display samples: SCR0 = 50'000, SCR0b = 60'000 (provides nextScreenTime).
            // Expectations:
            //  - msPcLatency.has_value() == true and > 0.
            //  - accumulatedInput2FrameStartTime remains 0 (no dropped chain).
            //  - Input2FrameStartTimeEma equals CalculateEma(0.0, Δ(PING0,SIM0), 0.1).
            //  - msPcLatency equals EMA + Δ(SIM0, SCR0), proving pclSimStartTime was used.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });
            p0.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_results.size(),
                L"P0 should emit metrics immediately when nextDisplayed == nullptr and two display samples exist.");

            const auto& p0_metrics = p0_results[0].metrics;
            Assert::IsTrue(p0_metrics.msPcLatency.has_value(),
                L"P0 should report msPcLatency for a direct PCL sample.");
            Assert::IsTrue(p0_metrics.msPcLatency.value() > 0.0,
                L"P0 msPcLatency should be positive.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Direct PCL sample should not touch accumulatedInput2FrameStartTime.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"No dropped frames occurred, so there should be no pending pclSimStart.");

            double deltaPingSim = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            double expectedEma = pmon::util::CalculateEma(0.0, deltaPingSim, 0.1);
            Assert::AreEqual(expectedEma, state.Input2FrameStartTimeEma, 0.0001,
                L"Input2FrameStartTimeEma should be seeded from the first Δ(PING,SIM).");

            double expectedLatency = expectedEma + qpc.DeltaSignedMilliSeconds(20'000, 50'000);
            Assert::AreEqual(expectedLatency, p0_metrics.msPcLatency.value(), 0.0001,
                L"msPcLatency should use pclSimStartTime (not lastSimStartTime) plus the seeded EMA.");
        }

        TEST_METHOD(PcLatency_TwoDisplayed_DirectSamples_UpdateEma)
        {
            // Scenario:
            //  - Two displayed frames (P0, P1) each provide direct PCL samples (Ping + Sim).
            //  - We mimic the ReportMetrics scheduling:
            //      P0 arrives  -> pending only (Case 2).
            //      P1 arrives  -> finalize P0 with nextDisplayed = P1 (Case 3), then queue P1 (Case 2).
            //      P2 helper   -> finalize P1 (Case 3) to observe the EMA update.
            // Expectations:
            //  - P0 final metrics report msPcLatency and seed the EMA.
            //  - P1 pending call emits no metrics.
            //  - After P1 is finalized, Input2FrameStartTimeEma changes (≠ value after P0) and stays > 0.
            //  - accumulatedInput2FrameStartTime stays 0 because no dropped chain exists.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // --------------------------------------------------------------------
            // P0: first displayed frame with direct PCL sample
            // --------------------------------------------------------------------
            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(0), p0_phase1.size(),
                L"P0 pending pass should not emit metrics.");

            // --------------------------------------------------------------------
            // P1: second displayed frame with direct PCL sample
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.pclInputPingTime = 30'000;
            p1.pclSimStartTime = 40'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, state);
            Assert::AreEqual(size_t(1), p0_final.size(),
                L"Finalizing P0 with nextDisplayed=P1 should emit exactly one metrics record.");
            Assert::IsTrue(p0_final[0].metrics.msPcLatency.has_value(),
                L"P0 should report msPcLatency when finalized.");
            double emaAfterP0 = state.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP0 > 0.0,
                L"EMA after P0 should be positive.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulated input-to-frame-start time should remain zero after P0.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"P1 pending pass should not emit metrics.");

            // --------------------------------------------------------------------
            // P2: helper displayed frame to flush P1
            // --------------------------------------------------------------------
            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            Assert::AreEqual(size_t(1), p1_final.size(),
                L"Finalizing P1 should emit exactly one metrics record.");
            Assert::IsTrue(p1_final[0].metrics.msPcLatency.has_value(),
                L"P1 should report msPcLatency when finalized.");
            double emaAfterP1 = state.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP1 > 0.0,
                L"EMA after P1 should stay positive.");
            Assert::IsTrue(emaAfterP1 != emaAfterP0,
                L"EMA after P1 must differ from the first-sample EMA after P0.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"No dropped chain should mean accumulatedInput2FrameStartTime stays at 0.");

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size(),
                L"P2 pending pass is only to mirror the pipeline; it should emit no metrics.");
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_InitializesAccum)
        {
            // Scenario:
            //  - A dropped frame P0 carries both pclInputPingTime and pclSimStartTime.
            //  - Without any displayed frame to consume it, the PC Latency accumulator should
            //    be initialized to Δ(PING0, SIM0) while msPcLatency remains absent.
            // Expectations:
            //  - P0's metrics do not expose msPcLatency (it was dropped).
            //  - accumulatedInput2FrameStartTime equals Δ(PING0, SIM0) and > 0.
            //  - lastReceivedNotDisplayedPclSimStart latches SIM0.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_results.size(),
                L"Dropped frames should emit one metrics record immediately.");
            Assert::IsFalse(p0_results[0].metrics.msPcLatency.has_value(),
                L"Dropped frames must not report msPcLatency.");

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > 0.0,
                L"Accumulated input-to-frame-start time should be initialized.");
            Assert::AreEqual(expectedAccum, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulator should equal Δ(PING0, SIM0).");
            Assert::AreEqual(uint64_t(20'000), state.lastReceivedNotDisplayedPclSimStart,
                L"lastReceivedNotDisplayedPclSimStart should track P0's pclSimStartTime.");
        }

        TEST_METHOD(PcLatency_DroppedChain_SimOnly_ExtendsAccum)
        {
            // Scenario:
            //  - P0 (dropped) has both Ping and Sim, seeding the accumulator.
            //  - P1 (dropped) has only pclSimStartTime and should extend the accumulator by the
            //    delta between SIM0 and SIM1.
            // Expectations:
            //  - P1 still reports no msPcLatency.
            //  - accumulatedInput2FrameStartTime after P1 > accumulated time after P0.
            //  - lastReceivedNotDisplayedPclSimStart equals SIM1.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsFalse(p0_results[0].metrics.msPcLatency.has_value());
            double accumAfterP0 = state.accumulatedInput2FrameStartTime;

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;

            auto p1_results = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(1), p1_results.size(),
                L"Second dropped frame should emit one metrics record.");
            Assert::IsFalse(p1_results[0].metrics.msPcLatency.has_value(),
                L"Dropped frames never report msPcLatency.");
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > accumAfterP0,
                L"Accumulator should grow when a sim-only dropped frame follows an existing chain.");
            Assert::AreEqual(uint64_t(30'000), state.lastReceivedNotDisplayedPclSimStart,
                L"Sim-only dropped frames still update lastReceivedNotDisplayedPclSimStart.");
        }

        TEST_METHOD(PcLatency_Dropped_SimOnly_NoAccum_NoEffect)
        {
            // Scenario:
            //  - A single dropped frame P0 only provides pclSimStartTime (no ping) and there is
            //    no existing accumulator.
            // Expectations:
            //  - msPcLatency remains absent.
            //  - accumulatedInput2FrameStartTime stays at 0 (chain not started).
            //  - lastReceivedNotDisplayedPclSimStart updates to SIM0 for possible future chaining.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 0;
            p0.pclSimStartTime = 25'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsFalse(p0_results[0].metrics.msPcLatency.has_value());
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulator should remain 0 when a sim-only drop has no pending chain.");
            Assert::AreEqual(uint64_t(25'000), state.lastReceivedNotDisplayedPclSimStart,
                L"Sim-only drop should remember its pclSimStartTime even if no accumulator exists yet.");
        }

        TEST_METHOD(PcLatency_Displayed_SimOnly_NoAccum_UsesExistingEma)
        {
            // Scenario:
            //  - P0 is displayed with a direct PCL sample, seeding the EMA.
            //  - P1 is displayed with only pclSimStartTime (no ping) and there is no accumulated chain.
            //  - We follow the full pipeline:
            //      P0 pending, then finalized by P1 (Case 3).
            //      P1 pending, then finalized by helper P2.
            // Expectations:
            //  - P1 final metrics report msPcLatency despite having no new ping.
            //  - accumulatedInput2FrameStartTime stays at 0 (no dropped chain was active).
            //  - Input2FrameStartTimeEma remains positive (not reset to 0).

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 35'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, state);
            Assert::AreEqual(size_t(1), p0_final.size());
            Assert::IsTrue(p0_final[0].metrics.msPcLatency.has_value());
            double emaAfterP0 = state.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP0 > 0.0);

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& p1_metrics = p1_final[0].metrics;
            Assert::IsTrue(p1_metrics.msPcLatency.has_value(),
                L"P1 should report msPcLatency despite missing pclInputPingTime.");
            Assert::IsTrue(p1_metrics.msPcLatency.value() > 0.0,
                L"P1 msPcLatency should stay positive.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"No dropped chain means the accumulator must stay zero.");
            Assert::IsTrue(state.Input2FrameStartTimeEma > 0.0,
                L"EMA should not be reset when a sim-only displayed frame uses existing history.");

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size());
        }

        TEST_METHOD(PcLatency_Displayed_NoPclSim_UsesLastSimStart)
        {
            // Scenario:
            //  - P0 is displayed with a full PCL sample to seed both the EMA and lastSimStartTime.
            //  - P1 is displayed without any PCL timestamps (pclSimStartTime == 0, pclInputPingTime == 0).
            //  - P1 should still produce msPcLatency by combining the existing EMA with the fallback
            //    state.lastSimStartTime recorded after P0.
            // Call schedule mirrors ReportMetrics: P0 pending, finalized by P1; P1 pending, finalized by P2.
            // Expectations:
            //  - P1 final metrics report msPcLatency.has_value() == true.
            //  - Input2FrameStartTimeEma is unchanged from the P0 sample (no new data).
            //  - msPcLatency equals EMA_after_P0 + Δ(lastSimStartTime_after_P0, SCR1), proving the fallback path.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 30'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, state);
            Assert::AreEqual(size_t(1), p0_final.size());
            Assert::IsTrue(p0_final[0].metrics.msPcLatency.has_value());
            double emaAfterP0 = state.Input2FrameStartTimeEma;
            uint64_t fallbackSimStart = state.lastSimStartTime;
            Assert::IsTrue(emaAfterP0 > 0.0,
                L"EMA must be initialized after the first direct sample.");
            Assert::AreEqual(uint64_t(30'000), fallbackSimStart,
                L"lastSimStartTime should latch P0's pclSimStartTime when it is displayed.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 110'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, state);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& p1_metrics = p1_final[0].metrics;
            Assert::IsTrue(p1_metrics.msPcLatency.has_value(),
                L"P1 should still report msPcLatency using the fallback lastSimStartTime.");
            Assert::AreEqual(emaAfterP0, state.Input2FrameStartTimeEma, 0.0001,
                L"EMA should remain unchanged when no new PCL sample exists.");
            double expectedLatency = emaAfterP0 + qpc.DeltaSignedMilliSeconds(fallbackSimStart, 90'000);
            Assert::AreEqual(expectedLatency, p1_metrics.msPcLatency.value(), 0.0001,
                L"msPcLatency should use the stored EMA plus the delta from lastSimStartTime to screen time.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulator should remain zero in this scenario.");

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(0), p2_phase1.size());
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_OverwritesOldAccum)
        {
            // Scenario:
            //  - Dropped frames P0 (Ping+Sim) and P1 (Sim only) create an accumulated chain A_old.
            //  - A new dropped frame P2 arrives with its own Ping+Sim and should overwrite (not extend)
            //    the accumulator, effectively starting a brand new chain.
            // Expectations:
            //  - Accumulator after P2 equals Δ(PING2, SIM2) exactly (no residue from A_old).
            //  - lastReceivedNotDisplayedPclSimStart equals SIM2.
            //  - P2 still reports no msPcLatency.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;
            ComputeMetricsForPresent(qpc, p0, nullptr, state);

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;
            ComputeMetricsForPresent(qpc, p1, nullptr, state);

            double accumBeforeP2 = state.accumulatedInput2FrameStartTime;
            Assert::IsTrue(accumBeforeP2 > 0.0,
                L"Precondition: accumulator should already be non-zero before introducing P2.");

            FrameData p2{};
            p2.pclInputPingTime = 100'000;
            p2.pclSimStartTime = 120'000;
            p2.finalState = PresentResult::Discarded;

            auto p2_results = ComputeMetricsForPresent(qpc, p2, nullptr, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsFalse(p2_results[0].metrics.msPcLatency.has_value());

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(100'000, 120'000);
            Assert::AreEqual(expectedAccum, state.accumulatedInput2FrameStartTime, 0.0001,
                L"New dropped frame with Ping+Sim should overwrite the accumulator with its own delta.");
            Assert::AreEqual(uint64_t(120'000), state.lastReceivedNotDisplayedPclSimStart,
                L"lastReceivedNotDisplayedPclSimStart should latch the newest sim start.");
        }

        TEST_METHOD(PcLatency_IncompleteDroppedChain_DoesNotAffectDirectSample)
        {
            // Scenario:
            //  - D0 (dropped, Ping+Sim) followed by D1 (dropped, Sim-only) builds an incomplete chain.
            //  - No displayed frame consumes it before a new direct-sample present P0 arrives.
            //  - P0 should be treated as a fresh direct measurement: EMA behaves like a first sample
            //    from P0 alone and the stale accumulator is cleared.
            // Expectations:
            //  - D0/D1 never report msPcLatency.
            //  - P0 final metrics report msPcLatency.has_value() == true.
            //  - Input2FrameStartTimeEma after P0 equals CalculateEma(0.0, Δ(P0.Ping, P0.Sim), 0.1).
            //  - accumulatedInput2FrameStartTime resets to 0 after the displayed frame completes.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            // Dropped chain D0 -> D1 (incomplete)
            FrameData d0{};
            d0.pclInputPingTime = 10'000;
            d0.pclSimStartTime = 20'000;
            d0.finalState = PresentResult::Discarded;
            auto d0_results = ComputeMetricsForPresent(qpc, d0, nullptr, state);
            Assert::AreEqual(size_t(1), d0_results.size());
            Assert::IsFalse(d0_results[0].metrics.msPcLatency.has_value());

            FrameData d1{};
            d1.pclInputPingTime = 0;
            d1.pclSimStartTime = 30'000;
            d1.finalState = PresentResult::Discarded;
            auto d1_results = ComputeMetricsForPresent(qpc, d1, nullptr, state);
            Assert::AreEqual(size_t(1), d1_results.size());
            Assert::IsFalse(d1_results[0].metrics.msPcLatency.has_value());
            double accumBeforeDisplayed = state.accumulatedInput2FrameStartTime;
            Assert::IsTrue(accumBeforeDisplayed > 0.0,
                L"Incomplete chain should leave a non-zero accumulator.");

            // Displayed P0 with a brand-new direct sample
            FrameData p0{};
            p0.pclInputPingTime = 100'000;
            p0.pclSimStartTime = 120'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 150'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, state);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 180'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, state);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& p0_metrics = p0_final[0].metrics;
            Assert::IsTrue(p0_metrics.msPcLatency.has_value(),
                L"Displayed frame with direct PCL data must report msPcLatency.");
            Assert::IsTrue(p0_metrics.msPcLatency.value() > 0.0,
                L"msPcLatency should be positive for P0.");

            double expectedFirstEma = pmon::util::CalculateEma(0.0,
                qpc.DeltaUnsignedMilliSeconds(100'000, 120'000),
                0.1);
            Assert::AreEqual(expectedFirstEma, state.Input2FrameStartTimeEma, 0.0001,
                L"EMA after P0 should match a first-sample EMA that ignores stale accumulation.");
            Assert::AreEqual(0.0, state.accumulatedInput2FrameStartTime, 0.0001,
                L"Accumulator must be cleared once the displayed frame consumes the chain.");
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart,
                L"Pending pclSimStart markers should be cleared once the chain completes.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, state);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }
    };
    TEST_CLASS(InstrumentedMetricsTests)
    {
    public:

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_FullData_UsesPclSimStart)
        {
            // This test verifies the "instrumented CPU/GPU" metrics on an application frame:
            //
            //   - msInstrumentedSleep
            //   - msInstrumentedGpuLatency
            //   - msBetweenSimStarts (PCL sim preferred over App sim)
            //
            // We construct:
            //
            //   QPC frequency = 10 MHz
            //
            //   Pre-state in swapChain:
            //     lastSimStartTime = 10'000 (this represents the previous frame's sim start)
            //
            //   P0 (the frame under test) – APP FRAME:
            //     appSleepStartTime =  1'000
            //     appSleepEndTime   = 11'000    // Δsleep = 10'000 ticks
            //     appSimStartTime   =100'000    // should NOT be used for between-sim-starts
            //     pclSimStartTime   = 20'000    // PCL sim should win for between-sim-starts
            //     gpuStartTime      = 21'000    // GPU start time used for GPU latency
            //     displayed         = one Application entry at screenTime = 50'000
            //
            //   Derived deltas:
            //     sleep Δ:        11'000 -  1'000 = 10'000 ticks
            //     PCL sim Δ:     20'000 - 10'000 = 10'000 ticks
            //     GPU latency Δ: 21'000 - 11'000 = 10'000 ticks
            //
            // With QPC = 10 MHz, 10'000 ticks = 0.001 ms.
            //
            // Call pattern (ReportMetrics-style for a single displayed app frame):
            //
            //   P0 arrives:
            //       ComputeMetricsForPresent(P0, nullptr, chain)   // Case 2, pending only
            //
            //   P1 arrives later:
            //       ComputeMetricsForPresent(P0, &P1, chain)       // Case 3, finalize P0
            //       ComputeMetricsForPresent(P1, nullptr, chain)   // pending P1 (ignored in this test)
            //
            // We verify on P0's final metrics:
            //   - msInstrumentedSleep has a value and matches Δ(appSleepStart, appSleepEnd).
            //   - msInstrumentedGpuLatency has a value and matches Δ(appSleepEnd, gpuStartTime).
            //   - msBetweenSimStarts has a value and matches Δ(lastSimStartTime, P0.pclSimStartTime),
            //     proving PCL sim is preferred over App sim for between-sim-starts.

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Seed lastSimStartTime to simulate a previous frame.
            chain.lastSimStartTime = 10'000;
            chain.animationErrorSource = AnimationErrorSource::PCLatency;

            const uint32_t PROCESS_ID = 1234;
            const uint64_t SWAPCHAIN = 0xABC0;

            // --------------------------------------------------------------------
            // P0: Application frame with full instrumented CPU/GPU data
            // --------------------------------------------------------------------
            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;

            p0.presentStartTime = 0;
            p0.timeInPresent = 0;
            p0.readyTime = 0;

            // Instrumented CPU / sim:
            p0.appSleepStartTime = 1'000;
            p0.appSleepEndTime = 11'000;
            p0.appSimStartTime = 100'000;   // should NOT be used for between-sim-starts
            p0.pclSimStartTime = 20'000;   // should be used instead

            // GPU start time for GPU latency:
            p0.gpuStartTime = 21'000;

            // Mark as displayed Application frame
            p0.finalState = PresentResult::Presented;
            p0.displayed.Clear();
            p0.displayed.PushBack({ FrameType::Application, 50'000 }); // screenTime = 50'000

            // First call: P0 arrives, becomes pending (no metrics yet).
            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size(),
                L"P0 (phase 1): pending-only call with nextDisplayed=nullptr should produce no metrics.");

            // --------------------------------------------------------------------
            // P1: simple next displayed app frame (used only as nextDisplayed for P0)
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;

            p1.presentStartTime = 0;
            p1.timeInPresent = 0;
            p1.readyTime = 0;

            p1.finalState = PresentResult::Presented;
            p1.displayed.Clear();
            p1.displayed.PushBack({ FrameType::Application, 60'000 }); // later display, not important

            // Second call: P1 arrives, finalize P0 using P1 as nextDisplayed (Case 3).
            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size(),
                L"P0 (final): expected exactly one metrics record when flushed with nextDisplayed=P1.");

            const auto& m0 = p0_final[0].metrics;

            // --------------------------------------------------------------------
            // Assertions for P0's instrumented CPU/GPU metrics
            // --------------------------------------------------------------------
            // Expected values based on our chosen QPC times:
            double expectedSleepMs = qpc.DeltaUnsignedMilliSeconds(1'000, 11'000);  // 10'000 ticks
            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(11'000, 21'000);   // 10'000 ticks
            double expectedBetween = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000); // 10'000 ticks

            // 1) Instrumented sleep
            Assert::IsTrue(m0.msInstrumentedSleep.has_value(),
                L"P0: msInstrumentedSleep should have a value for valid AppSleepStart/End.");
            Assert::AreEqual(expectedSleepMs, m0.msInstrumentedSleep.value(), 1e-6,
                L"P0: msInstrumentedSleep did not match expected Δ(AppSleepStart, AppSleepEnd).");

            // 2) Instrumented GPU latency (start = AppSleepEndTime since it is non-zero)
            Assert::IsTrue(m0.msInstrumentedGpuLatency.has_value(),
                L"P0: msInstrumentedGpuLatency should have a value when InstrumentedStartTime and gpuStartTime are valid.");
            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency.value(), 1e-6,
                L"P0: msInstrumentedGpuLatency did not match expected Δ(AppSleepEndTime, gpuStartTime).");

            // 3) Between sim starts: PCL sim (20'000) must win over App sim (100'000)
            Assert::IsTrue(m0.msBetweenSimStarts.has_value(),
                L"P0: msBetweenSimStarts should have a value when lastSimStartTime and PclSimStartTime are non-zero.");
            Assert::AreEqual(expectedBetween, m0.msBetweenSimStarts.value(), 1e-6,
                L"P0: msBetweenSimStarts should be based on PCL sim start, not App sim start.");
        }
        TEST_METHOD(InstrumentedDisplay_AppFrame_FullData_ComputesAll)
        {
            // This test verifies the "instrumented display" metrics on a displayed
            // application frame:
            //
            //   - msInstrumentedRenderLatency
            //   - msReadyTimeToDisplayLatency
            //   - msInstrumentedLatency (total app-instrumented latency)
            //
            // New invariant (unified metrics):
            //   These metrics are only computed when:
            //     - the frame is an Application frame (isAppFrame == true), AND
            //     - the frame is displayed (isDisplayed == true).
            //
            // We construct:
            //
            //   QPC frequency = 10 MHz
            //
            //   P0 (the frame under test) – DISPLAYED APP FRAME:
            //     appRenderSubmitStartTime = 10'000
            //     readyTime                = 20'000
            //     appSleepEndTime          =  5'000   // used as InstrumentedStartTime
            //     screenTime               = 30'000 (Application entry in displayed)
            //
            //   Derived deltas:
            //     render latency:      30'000 - 10'000 = 20'000 ticks
            //     ready→display:       30'000 - 20'000 = 10'000 ticks
            //     total inst. latency: 30'000 -  5'000 = 25'000 ticks
            //
            // With QPC = 10 MHz:
            //   10'000 ticks = 0.001 ms
            //   20'000 ticks = 0.002 ms
            //   25'000 ticks = 0.0025 ms
            //
            // Call pattern (mirroring ReportMetrics for a displayed app frame):
            //
            //   P0 arrives:
            //       ComputeMetricsForPresent(P0, nullptr, chain)   // Case 2, pending only
            //
            //   P1 arrives:
            //       ComputeMetricsForPresent(P0, &P1, chain)       // Case 3, finalize P0
            //       ComputeMetricsForPresent(P1, nullptr, chain)   // pending P1 (ignored)
            //
            // We verify on P0's final metrics:
            //   - msInstrumentedRenderLatency has a value and matches Δ(appRenderSubmitStartTime, screenTime).
            //   - msReadyTimeToDisplayLatency has a value and matches Δ(readyTime, screenTime).
            //   - msInstrumentedLatency has a value and matches Δ(appSleepEndTime, screenTime).

            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint32_t PROCESS_ID = 1234;
            const uint64_t SWAPCHAIN = 0xABC0;

            // --------------------------------------------------------------------
            // P0: Displayed Application frame with full instrumented display data
            // --------------------------------------------------------------------
            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;

            p0.presentStartTime = 0;
            p0.timeInPresent = 0;
            p0.readyTime = 20'000; // ReadyTime

            // Instrumented markers
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSleepEndTime = 5'000;
            p0.appSimStartTime = 0;     // not needed in this test

            // Mark as displayed Application frame with a single screen time.
            p0.finalState = PresentResult::Presented;
            p0.displayed.Clear();
            p0.displayed.PushBack({ FrameType::Application, 30'000 }); // screenTime = 30'000

            // First call: P0 arrives, becomes pending.
            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size(),
                L"P0 (phase 1): pending-only call with nextDisplayed=nullptr should produce no metrics.");

            // --------------------------------------------------------------------
            // P1: Next displayed app frame (used only as nextDisplayed for P0)
            // --------------------------------------------------------------------
            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;

            p1.presentStartTime = 0;
            p1.timeInPresent = 0;
            p1.readyTime = 0;

            p1.finalState = PresentResult::Presented;
            p1.displayed.Clear();
            p1.displayed.PushBack({ FrameType::Application, 40'000 }); // later display

            // Second call: finalize P0 with nextDisplayed=P1
            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size(),
                L"P0 (final): expected exactly one metrics record when flushed with nextDisplayed=P1.");

            const auto& m0 = p0_final[0].metrics;

            // For completeness, process P1 as pending (not used in this test).
            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"P1 (phase 1): first call with nextDisplayed=nullptr should produce no metrics (pending only).");

            // --------------------------------------------------------------------
            // Assertions for P0's instrumented display metrics
            // --------------------------------------------------------------------
            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 30'000); // 20'000 ticks
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(20'000, 30'000); // 10'000 ticks
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 30'000); // 25'000 ticks

            // Render latency
            Assert::IsTrue(m0.msInstrumentedRenderLatency.has_value(),
                L"P0: msInstrumentedRenderLatency should have a value for a displayed app frame with AppRenderSubmitStartTime.");
            Assert::AreEqual(expectedRenderMs, m0.msInstrumentedRenderLatency.value(), 1e-6,
                L"P0: msInstrumentedRenderLatency did not match expected Δ(AppRenderSubmitStartTime, screenTime).");

            // Ready-to-display latency
            Assert::IsTrue(m0.msReadyTimeToDisplayLatency.has_value(),
                L"P0: msReadyTimeToDisplayLatency should have a value when ReadyTime and screenTime are valid.");
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency.value(), 1e-6,
                L"P0: msReadyTimeToDisplayLatency did not match expected Δ(ReadyTime, screenTime).");

            // Total instrumented latency: from appSleepEndTime to screenTime
            Assert::IsTrue(m0.msInstrumentedLatency.has_value(),
                L"P0: msInstrumentedLatency should have a value when there is a valid instrumented start time.");
            Assert::AreEqual(expectedTotalMs, m0.msInstrumentedLatency.value(), 1e-6,
                L"P0: msInstrumentedLatency did not match expected Δ(AppSleepEndTime, screenTime).");
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleep_UsesAppSimStart)
        {
            // Scenario:
            //   - Validate the instrumented CPU/GPU metrics when the application never enters an
            //     instrumented sleep, forcing GPU latency to fall back to appSimStart.
            //   - Also ensure msBetweenSimStarts uses the stored lastSimStartTime → appSimStart delta
            //     when no PCL sim timestamp is present.
            //
            // QPC frequency: 10 MHz.
            //
            // Pre-state:
            //   chain.lastSimStartTime = 40'000 (represents the previous frame's sim start).
            //
            // P0 (displayed Application frame):
            //   appSleepStartTime = 0
            //   appSleepEndTime   = 0
            //   appSimStartTime   = 70'000   (used for GPU latency + between-sim-starts)
            //   pclSimStartTime   = 0        (forces AppSim fallback)
            //   gpuStartTime      = 90'000
            //   screenTime        = 120'000  (Application entry)
            //   Δ(sim start vs last) = 30'000 ticks, Δ(sim start → gpu) = 20'000 ticks.
            //
            // Call pattern (Case 2/3):
            //   P0 pending → Compute(..., nullptr)
            //   P1 arrives → Compute(P0, &P1) to finalize P0, then Compute(P1, nullptr) to seed next pending.
            //
            // Expectations on P0 final metrics:
            //   - msInstrumentedSleep has no value (no sleep interval).
            //   - msInstrumentedGpuLatency uses appSimStartTime (70'000 → 90'000).
            //   - msBetweenSimStarts computes 40'000 → 70'000.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 40'000;
            chain.animationErrorSource = AnimationErrorSource::AppProvider;

            const uint32_t PROCESS_ID = 4321;
            const uint64_t SWAPCHAIN = 0x2222;

            // P0: displayed Application frame with no sleep range but valid appSimStart
            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.appSimStartTime = 70'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.Clear();
            p0.displayed.PushBack({ FrameType::Application, 120'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size(),
                L"P0 (phase 1) should stay pending when nextDisplayed is unavailable.");

            // P1: minimal next displayed application frame
            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.finalState = PresentResult::Presented;
            p1.displayed.Clear();
            p1.displayed.PushBack({ FrameType::Application, 150'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size(),
                L"P0 (final) should emit exactly one metrics record once nextDisplayed is provided.");

            const auto& m0 = p0_final[0].metrics;
            Assert::IsFalse(m0.msInstrumentedSleep.has_value(),
                L"P0: Instrumented sleep must be absent when the app never emitted sleep markers.");
            Assert::IsTrue(m0.msInstrumentedGpuLatency.has_value(),
                L"P0: GPU latency should fall back to AppSimStart when no sleep end exists.");
            Assert::IsTrue(m0.msBetweenSimStarts.has_value(),
                L"P0: Between-sim-starts should use the stored lastSimStartTime when AppSimStart is valid.");

            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(70'000, 90'000);
            double expectedBetweenMs = qpc.DeltaUnsignedMilliSeconds(40'000, 70'000);

            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency.value(), 1e-6,
                L"P0: msInstrumentedGpuLatency should measure Δ(AppSimStartTime, gpuStartTime).");
            Assert::AreEqual(expectedBetweenMs, m0.msBetweenSimStarts.value(), 1e-6,
                L"P0: msBetweenSimStarts should use AppSimStart when no PCL sim exists.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size(),
                L"P1 (phase 1) remains pending for completeness.");
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleepNoSim_NoInstrumentedCpuGpu)
        {
            // Scenario:
            //   - Displayed Application frame with no instrumented sleep markers and neither appSimStartTime
            //     nor pclSimStartTime populated. GPU start exists, but there is no instrumented start anchor.
            //
            // QPC frequency: 10 MHz.
            //   Pre-state: chain.lastSimStartTime = 55'000.
            //   P0 fields: appSleepStart=0, appSleepEnd=0, appSimStart=0, pclSimStart=0, gpuStart=80'000,
            //              screenTime=100'000 (Application display).
            //   Derived deltas: none are valid because the start markers are zero.
            //
            // Call pattern (Case 2/3):
            //   P0 pending → Compute(..., nullptr)
            //   P1 arrives → Compute(P0, &P1) to flush, then Compute(P1, nullptr) for completeness.
            //
            // Expectations: all three instrumented CPU metrics stay std::nullopt for P0.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 55'000;

            const uint32_t PROCESS_ID = 9876;
            const uint64_t SWAPCHAIN = 0xEF00;

            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.gpuStartTime = 80'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.Clear();
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 120'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());

            const auto& m0 = p0_final[0].metrics;
            Assert::IsFalse(m0.msInstrumentedSleep.has_value(),
                L"P0: sleep metrics require both start and end markers.");
            Assert::IsFalse(m0.msInstrumentedGpuLatency.has_value(),
                L"P0: GPU latency must remain off without an instrumented start time.");
            Assert::IsFalse(m0.msBetweenSimStarts.has_value(),
                L"P0: between-sim-starts cannot be computed without a new sim start.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NotDisplayed_StillComputed)
        {
            // Scenario:
            //   - Dropped (not displayed) Application frame with full instrumented CPU/GPU markers, including PCL sim.
            //   - Validates that CPU/GPU metrics still compute even when the frame never displays, while
            //     display-only metrics remain unset.
            //
            // QPC frequency: 10 MHz.
            //   Pre-state: chain.lastSimStartTime = 5'000.
            //   P0 fields: appSleepStart=10'000, appSleepEnd=25'000, appSimStart=30'000,
            //              gpuStart=45'000, no displayed entries (finalState = Discarded).
            //   Derived deltas: sleep Δ = 15'000 ticks, GPU latency Δ = 20'000 ticks,
            //                  between-sim-starts Δ = 30'000 ticks.
            //
            // Call pattern: Case 1 (pure dropped) → single ComputeMetricsForPresent call with nextDisplayed == nullptr.
            //
            // Expectations: instrumented sleep/GPU/betweenSimStarts all have values with the deltas above,
            //               and display instrumented metrics remain std::nullopt.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 5'000;
            chain.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData p0{};
            p0.appSleepStartTime = 10'000;
            p0.appSleepEndTime = 25'000;
            p0.appSimStartTime = 30'000;
            p0.gpuStartTime = 45'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(1), p0_results.size(),
                L"Dropped frames should emit their metrics immediately (Case 1).");

            const auto& m0 = p0_results[0].metrics;
            double expectedSleepMs = qpc.DeltaUnsignedMilliSeconds(10'000, 25'000);
            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(25'000, 45'000);
            double expectedBetweenMs = qpc.DeltaUnsignedMilliSeconds(5'000, 30'000);

            Assert::IsTrue(m0.msInstrumentedSleep.has_value());
            Assert::AreEqual(expectedSleepMs, m0.msInstrumentedSleep.value(), 1e-6);

            Assert::IsTrue(m0.msInstrumentedGpuLatency.has_value());
            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency.value(), 1e-6);

            Assert::IsTrue(m0.msBetweenSimStarts.has_value());
            Assert::AreEqual(expectedBetweenMs, m0.msBetweenSimStarts.value(), 1e-6);

            Assert::IsFalse(m0.msInstrumentedRenderLatency.has_value(),
                L"Display-dependent metrics must stay off for non-displayed frames.");
            Assert::IsFalse(m0.msReadyTimeToDisplayLatency.has_value());
            Assert::IsFalse(m0.msInstrumentedLatency.has_value());
        }

        TEST_METHOD(InstrumentedCpuGpu_NonAppFrame_Ignored)
        {
            // Scenario:
            //   - Displayed frame whose sole display entry is FrameType::Repeated, so DisplayIndexing never
            //     marks an Application display.
            //   - Even with instrumented CPU/GPU markers present, msInstrumentedSleep/GpuLatency/BetweenSimStarts
            //     must remain unset because the display instance is not an app frame.
            //
            // QPC frequency: 10 MHz.
            //   Pre-state: chain.lastSimStartTime = 60'000.
            //   P0 fields: appSleepStart=11'000, appSleepEnd=21'000, appSimStart=70'000, pclSimStart=72'000,
            //              gpuStart=90'000, displayed[0] = (Repeated, 120'000).
            //   Derived deltas (that should be ignored): sleep Δ = 10'000 ticks, GPU Δ = 69'000 ticks,
            //              between-sim-starts Δ = 10'000 ticks.
            //
            // Call pattern: Case 2/3 (P0 pending, finalized by a synthetic P1, then P1 pending).
            //
            // Expectations: all instrumented CPU metrics remain std::nullopt for P0.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 60'000;

            const uint32_t PROCESS_ID = 5555;
            const uint64_t SWAPCHAIN = 0xDEADBEEF;

            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.appSleepStartTime = 11'000;
            p0.appSleepEndTime = 21'000;
            p0.appSimStartTime = 70'000;
            p0.pclSimStartTime = 72'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.Clear();
            p0.displayed.PushBack({ FrameType::Repeated, 120'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 150'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& m0 = p0_final[0].metrics;

            Assert::IsFalse(m0.msInstrumentedSleep.has_value(),
                L"Non-app displays must not emit instrumented CPU metrics.");
            Assert::IsFalse(m0.msInstrumentedGpuLatency.has_value());
            Assert::IsFalse(m0.msBetweenSimStarts.has_value());

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoRenderSubmit_RenderLatencyOff)
        {
            // Scenario:
            //   - Displayed Application frame missing appRenderSubmitStartTime but with readyTime + sleep end.
            //
            // QPC frequency: 10 MHz.
            //   P0 fields: readyTime = 80'000, appSleepEndTime = 50'000, no render submit, screenTime = 100'000.
            //   Derived deltas: ready→display Δ = 20'000 ticks, total latency Δ = 50'000 ticks.
            //
            // Call pattern: Case 2/3 (P0 pending, finalized by P1, then P1 pending).
            //
            // Expectations: msInstrumentedRenderLatency = nullopt, while msReadyTimeToDisplayLatency and
            //               msInstrumentedLatency match the deltas above.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.readyTime = 80'000;
            p0.appSleepEndTime = 50'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 130'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& m0 = p0_final[0].metrics;

            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(80'000, 100'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(50'000, 100'000);

            Assert::IsFalse(m0.msInstrumentedRenderLatency.has_value(),
                L"Render latency must remain off without appRenderSubmitStartTime.");
            Assert::IsTrue(m0.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency.value(), 1e-6);

            Assert::IsTrue(m0.msInstrumentedLatency.has_value());
            Assert::AreEqual(expectedTotalMs, m0.msInstrumentedLatency.value(), 1e-6);

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleep_UsesAppSimStart)
        {
            // Scenario:
            //   - Displayed Application frame lacks appSleepEndTime but provides appSimStartTime.
            //
            // QPC timeline (10 MHz):
            //   P0.appRenderSubmitStartTime = 10'000
            //   P0.appSimStartTime         =  5'000
            //   P0.readyTime               = 30'000
            //   P0.screenTime              = 60'000
            //   Derived deltas:
            //     - Render latency: 50'000 ticks
            //     - Ready→display: 30'000 ticks
            //     - Total instrumented latency (AppSim→screen): 55'000 ticks
            //
            // Call pattern: Case 2/3.
            //
            // Expectations: render + ready latencies computed normally; msInstrumentedLatency should fall back
            //               to AppSimStartTime since sleep end is missing.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSimStartTime = 5'000;
            p0.readyTime = 30'000;
            p0.appSleepEndTime = 0;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& m0 = p0_final[0].metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 60'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(30'000, 60'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 60'000);

            Assert::IsTrue(m0.msInstrumentedRenderLatency.has_value());
            Assert::AreEqual(expectedRenderMs, m0.msInstrumentedRenderLatency.value(), 1e-6);
            Assert::IsTrue(m0.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency.value(), 1e-6);
            Assert::IsTrue(m0.msInstrumentedLatency.has_value());
            Assert::AreEqual(expectedTotalMs, m0.msInstrumentedLatency.value(), 1e-6,
                L"Total latency should fall back to AppSimStartTime when sleep end is missing.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleepNoSim_NoTotalLatency)
        {
            // Scenario:
            //   - Displayed Application frame has render submit + ready markers but neither appSleepEndTime
            //     nor appSimStartTime, so the total instrumented latency should be disabled.
            //
            // QPC values (10 MHz): appRenderSubmitStartTime = 12'000, readyTime = 32'000, screenTime = 70'000.
            //   Derived deltas:
            //     - Render latency Δ = 58'000 ticks
            //     - Ready→display Δ = 38'000 ticks
            //
            // Call pattern: Case 2/3.
            //
            // Expectations: render + ready metrics populated with the deltas above; msInstrumentedLatency is nullopt.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 12'000;
            p0.readyTime = 32'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& m0 = p0_final[0].metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(12'000, 70'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(32'000, 70'000);

            Assert::IsTrue(m0.msInstrumentedRenderLatency.has_value());
            Assert::AreEqual(expectedRenderMs, m0.msInstrumentedRenderLatency.value(), 1e-6);
            Assert::IsTrue(m0.msReadyTimeToDisplayLatency.has_value());
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency.value(), 1e-6);
            Assert::IsFalse(m0.msInstrumentedLatency.has_value(),
                L"Total instrumented latency must stay off without an instrumented start.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedDisplay_NonAppFrame_Ignored)
        {
            // Scenario:
            //   - Displayed frame whose first (and only) display entry is FrameType::Repeated, so the
            //     DisplayIndexing logic never flags an application frame for this present.
            //
            // QPC values (10 MHz): appRenderSubmitStartTime = 10'000, readyTime = 30'000, appSleepEndTime = 5'000,
            //                      screenTime = 60'000. These deltas should all be ignored.
            //
            // Call pattern: Case 2/3.
            //
            // Expectations: all instrumented display metrics remain std::nullopt for P0.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 10'000;
            p0.readyTime = 30'000;
            p0.appSleepEndTime = 5'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 60'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            const auto& m0 = p0_final[0].metrics;

            Assert::IsFalse(m0.msInstrumentedRenderLatency.has_value());
            Assert::IsFalse(m0.msInstrumentedLatency.has_value());

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NotDisplayed_NoDisplayMetrics)
        {
            // Scenario:
            //   - Application frame with appRenderSubmit/ready/sleep/appSim markers that gets discarded (not displayed).
            //   - Ensures display-only instrumented metrics stay unset when there is no screen time.
            //
            // QPC values: appRenderSubmitStart = 9'000, readyTime = 19'000, appSleepEnd = 4'000,
            //             appSimStart = 2'000. No displayed entries, so screenTime is undefined.
            //
            // Call pattern: Case 1 (single call, nextDisplayed == nullptr).
            //
            // Expectations: msInstrumentedRenderLatency / msReadyTimeToDisplayLatency / msInstrumentedLatency are nullopt.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 9'000;
            p0.readyTime = 19'000;
            p0.appSleepEndTime = 4'000;
            p0.appSimStartTime = 2'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& m0 = p0_results[0].metrics;

            Assert::IsFalse(m0.msInstrumentedRenderLatency.has_value());
            Assert::IsFalse(m0.msReadyTimeToDisplayLatency.has_value());
            Assert::IsFalse(m0.msInstrumentedLatency.has_value());
        }

        TEST_METHOD(InstrumentedInput_DroppedAppFrame_PendingProviderInput_ConsumedOnDisplay)
        {
            // Scenario:
            //   - P0 is a dropped Application frame that carries an App provider input sample at 20'000 ticks.
            //   - P1 is the next displayed Application frame (screenTime = 70'000) without its own sample.
            //   - P2 is a synthetic follower to flush P1, mirroring the ReportMetrics Case 2/3 pattern.
            //
            // QPC-derived delta: 70'000 - 20'000 = 50'000 ticks = 5 ms (with 10 MHz QPC).
            //
            // Call pattern:
            //   - P0 (Case 1) → Compute(..., nullptr) populates lastReceivedNotDisplayedAppProviderInputTime.
            //   - P1 pending → Compute(P1, nullptr).
            //   - P1 final → Compute(P1, &P2) produces metrics.
            //
            // Expectations:
            //   - Cached provider input equals 20'000 after P0.
            //   - P1 reports msInstrumentedInputTime = 5 ms and clears all pending caches afterward.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t pendingInputTime = 20'000;

            // P0: Dropped Application frame with provider input.
            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::AreEqual(pendingInputTime, chain.lastReceivedNotDisplayedAppProviderInputTime,
                L"Dropped provider input should be cached until a displayed frame consumes it.");

            // P1: Displayed Application frame without its own AppInputSample.
            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());

            // P2: Simple next displayed frame to flush P1.
            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, chain);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& m1 = p1_final[0].metrics;

            Assert::IsTrue(m1.msInstrumentedInputTime.has_value(),
                L"P1 should consume the cached provider input time once it is displayed.");
            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(pendingInputTime, 70'000);
            Assert::AreEqual(expectedInputMs, m1.msInstrumentedInputTime.value(), 1e-6);

            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime,
                L"Pending provider input cache must be cleared after consumption.");
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedMouseClickTime);

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, chain);
            Assert::AreEqual(size_t(0), p2_phase1.size());
        }

        TEST_METHOD(InstrumentedInput_DisplayedAppFrame_WithOwnSample_IgnoresPending)
        {
            // Scenario:
            //   - P0 (dropped) seeds pending provider input at 10'000 ticks.
            //   - P1 (displayed Application) carries its own sample at 15'000 ticks and displays at 60'000.
            //   - P2 finalizes P1.
            //
            // QPC-derived deltas:
            //   - Pending path would have produced 50'000 ticks, but we expect 45'000 ticks from P1's own sample.
            //
            // Call pattern: identical to Test 10 (Case 1 for P0, Case 2/3 for P1).
            //
            // Expectations:
            //   - Cached pending input updated after P0.
            //   - P1 final metrics use Δ(15'000, 60'000) only and clear the pending cache.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t pendingInputTime = 10'000;
            const uint64_t directInputTime = 15'000;

            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Keyboard };
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::AreEqual(pendingInputTime, chain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.appInputSample = { directInputTime, InputDeviceType::Mouse };
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 80'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, chain);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& m1 = p1_final[0].metrics;

            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(directInputTime, 60'000);
            Assert::IsTrue(m1.msInstrumentedInputTime.has_value());
            Assert::AreEqual(expectedInputMs, m1.msInstrumentedInputTime.value(), 1e-6,
                L"P1 must prefer its own input marker over pending values.");

            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime);

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, chain);
            Assert::AreEqual(size_t(0), p2_phase1.size());
        }

        TEST_METHOD(InstrumentedInput_NonAppFrame_DoesNotAffectInstrumentedInputTime)
        {
            // Scenario:
            //   - P0 is a displayed frame with FrameType::Repeated at screenTime = 50'000 and a provider
            //     input sample at 25'000 ticks. Because it is not an Application frame, it must NOT seed the
            //     pending provider cache.
            //   - P1 is the next displayed Application frame (screenTime = 80'000) without its own sample.
            //   - P2 finalizes P1.
            //
            // QPC expectation: since no pending sample exists, msInstrumentedInputTime for P1 must be nullopt.
            //
            // Call pattern: Case 2/3 for both P0 and P1 (since both are displayed frames).
            //
            // Expectations:
            //   - After P0 finalization, chain.lastReceivedNotDisplayedAppProviderInputTime remains 0.
            //   - P1 final metrics leave msInstrumentedInputTime unset.

            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t ignoredInputTime = 25'000;

            FrameData p0{};
            p0.appInputSample = { ignoredInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 50'000 });

            auto p0_phase1 = ComputeMetricsForPresent(qpc, p0, nullptr, chain);
            Assert::AreEqual(size_t(0), p0_phase1.size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 80'000 });

            auto p0_final = ComputeMetricsForPresent(qpc, p0, &p1, chain);
            Assert::AreEqual(size_t(1), p0_final.size());
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime,
                L"Non-app frames should not seed the pending provider input cache.");

            auto p1_phase1 = ComputeMetricsForPresent(qpc, p1, nullptr, chain);
            Assert::AreEqual(size_t(0), p1_phase1.size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p1_final = ComputeMetricsForPresent(qpc, p1, &p2, chain);
            Assert::AreEqual(size_t(1), p1_final.size());
            const auto& m1 = p1_final[0].metrics;
            Assert::IsFalse(m1.msInstrumentedInputTime.has_value(),
                L"P1 should not report instrumented input latency because no app-frame pending sample existed.");

            auto p2_phase1 = ComputeMetricsForPresent(qpc, p2, nullptr, chain);
            Assert::AreEqual(size_t(0), p2_phase1.size());
        }
    };
}
