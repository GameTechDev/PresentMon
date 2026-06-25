// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <CommonUtilities/test/FloatAssert.h>
#include <CommonUtilities/qpc.h>
#include <CommonUtilities/mc/MetricsTypes.h>
#include <CommonUtilities/mc/MetricsCalculator.h>
#include <CommonUtilities/mc/SwapChainState.h>
#include <CommonUtilities/mc/UnifiedSwapChain.h>
#include <CommonUtilities/Math.h>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using pmon::util::test::AssertAreEqualWithinTolerance;
using namespace pmon::util::metrics;
using namespace pmon::util;

namespace MetricsCoreTests
{
    namespace
    {
        bool HasMetricValue(double value)
        {
            return !IsMissingFrameMetricValue(value);
        }
    }

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

            AssertAreEqualWithinTolerance(49.998, swapChain.accumulatedInput2FrameStartTime, 0.001);
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
    // SECTION 2: Helper Functions
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

    TEST_CLASS(AnimationErrorTrackerTests)
    {
    public:
        TEST_METHOD(ResolveInterval_SameSourceZeroDisplayElapsed_PublishesAnimationTimeAndMissingError)
        {
            QpcConverter qpc(1000, 0);
            AnimationErrorTracker tracker{};

            AnimationErrorTracker::AppAnchor anchor{};
            anchor.source = AnimationErrorSource::AppProvider;
            anchor.simStartTime = 1000;
            Assert::IsTrue(tracker.TryStartTimelineAtAnchor(anchor));

            AnimationErrorTracker::AppAnchor closingAnchor{};
            closingAnchor.source = AnimationErrorSource::AppProvider;
            closingAnchor.simStartTime = 1016;

            ReadyDisplayRow row{};
            row.previousDisplayedScreenTime = 200;
            row.screenTime = 200;

            const auto contexts = tracker.ResolveIntervalAndAdvanceAnchor(qpc, closingAnchor, { row });

            Assert::AreEqual(size_t(1), contexts.size());
            Assert::IsFalse(HasMetricValue(contexts[0].msAnimationError));
            Assert::AreEqual(16.0, contexts[0].msAnimationTime, 0.0001);
            Assert::AreEqual(uint64_t(1016), contexts[0].resolvedSimStartTime);
            Assert::IsTrue(contexts[0].hasResolvedSimStart);
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

    

TEST_CLASS(ComputeMetricsForPresentTests)
    {
    public:
        TEST_METHOD(ComputeMetricsForPresent_NotDisplayed_NoDisplays_ProducesSingleMetricsAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 10'000, 500, 10'500, {}); // Presented but no displays => not displayed path
            auto metrics = ComputeMetricsForPresent(qpc, frame, chain);

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

            auto metrics = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), metrics.size());
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime, L"Not displayed path should not update displayed screen time.");
        }

        TEST_METHOD(ComputeMetricsForPresent_DisplayedSingleDisplay_ProducesSingleMetricsAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            auto frame = MakeFrame(PresentResult::Presented, 5'000, 200, 5'500,
                                   { { FrameType::Application, 6'000 } });

            auto metrics = ComputeMetricsForPresent(qpc, frame, chain);

            Assert::AreEqual(size_t(1), metrics.size(), L"Legacy present metrics emit one row immediately.");
            Assert::IsTrue(chain.lastPresent.has_value());
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(6'000), metrics[0].metrics.screenTimeQpc);
        }

        TEST_METHOD(ProcessPresent_MultiDisplayAppAnchor_UsesAppDisplayRowAndUpdatesChain)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 10'000;
            frame.timeInPresent = 300;
            frame.readyTime = 10'800;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 11'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 11'500 });
            frame.displayed.PushBack({ FrameType::Repeated, 12'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(frame));

            Assert::AreEqual(size_t(1), rows.size(), L"App anchor publishes when in-present lookahead is known.");
            Assert::IsTrue(swapChain.swapChain.lastPresent.has_value());
            Assert::IsTrue(swapChain.swapChain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(11'000), rows[0].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(11'000), swapChain.swapChain.lastDisplayedScreenTime);
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

            chain.UpdateAfterPresentV1(frame);

            Assert::AreEqual(uint64_t(10'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(10'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(1'500), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_AppProvider_BothPresent_RemainsAppProvider)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::AppProvider;
            chain.firstAppSimStartTime = 40'000;
            chain.lastDisplayedSimStartTime = 41'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            auto frame = MakeFrame(PresentResult::Presented, 1'100, 55, 1'300,
                                   { { FrameType::Application, 1'650 } },
                                   10'000 /* appSimStartTime */, 12'000 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(10'000), chain.lastDisplayedSimStartTime);
            Assert::IsTrue(chain.lastDisplayedSimStartTime != uint64_t(12'000));
            Assert::AreEqual(uint64_t(1'650), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_AppProvider_MissingApp_NoPcl_LeavesAnchorsUnchanged)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::AppProvider;
            chain.firstAppSimStartTime = 40'000;
            chain.lastDisplayedSimStartTime = 41'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            auto frame = MakeFrame(PresentResult::Presented, 2'000, 40, 2'300,
                                   { { FrameType::Application, 9'950 } },
                                   0 /* appSimStartTime */, 0 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(41'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(8'800), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_AppProvider_MissingApp_WithPcl_LeavesAnchorsUnchanged)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::AppProvider;
            chain.firstAppSimStartTime = 40'000;
            chain.lastDisplayedSimStartTime = 41'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            auto frame = MakeFrame(PresentResult::Presented, 2'100, 40, 2'400,
                                   { { FrameType::Application, 9'960 } },
                                   0 /* appSimStartTime */, 52'000 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(41'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(8'800), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_PCLatency_UpdatesSimStartAndFirstAppSim)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::PCLatency;

            auto frame = MakeFrame(PresentResult::Presented, 2'000, 40, 2'300,
                                   { { FrameType::Application, 2'700 } },
                                   0 /* appSimStartTime */, 12'345 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::AreEqual(uint64_t(12'345), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(12'345), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(2'700), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_PCLatency_MissingPcl_NoApp_LeavesAnchorsUnchanged)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::PCLatency;
            chain.firstAppSimStartTime = 30'000;
            chain.lastDisplayedSimStartTime = 31'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            FrameData previousApp = MakeFrame(PresentResult::Presented, 5'000, 80, 5'300,
                                              { { FrameType::Application, 5'800 } });
            chain.lastAppPresent = previousApp;

            auto frame = MakeFrame(PresentResult::Presented, 9'000, 90, 9'400,
                                   { { FrameType::Application, 9'950 } },
                                   0 /* appSimStartTime */, 0 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::PCLatency);
            Assert::AreEqual(uint64_t(30'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(31'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(8'800), chain.lastDisplayedAppScreenTime);
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

            chain.UpdateAfterPresentV1(frame);

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

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(20'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(20'000), chain.firstAppSimStartTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_CpuStart_BothPresent_TransitionsDirectlyToAppProvider)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::CpuStart;
            chain.firstAppSimStartTime = 15'000;
            chain.lastDisplayedSimStartTime = 15'000;

            auto frame = MakeFrame(PresentResult::Presented, 7'500, 75, 7'900,
                                   { { FrameType::Application, 8'300 } },
                                   20'000 /* appSimStartTime */, 30'000 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

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

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::PCLatency);
            Assert::AreEqual(uint64_t(30'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(30'000), chain.firstAppSimStartTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_PCLatency_TransitionsToAppProvider_WhenAppOnly)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::PCLatency;
            chain.firstAppSimStartTime = 30'000;
            chain.lastDisplayedSimStartTime = 31'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            auto frame = MakeFrame(PresentResult::Presented, 9'000, 90, 9'400,
                                   { { FrameType::Application, 9'950 } },
                                   40'000 /* appSimStartTime */, 0 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(40'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(9'950), chain.lastDisplayedAppScreenTime);
        }

        TEST_METHOD(UpdateAfterPresent_AnimationSource_PCLatency_TransitionsToAppProvider_WhenAppAndPclBothPresent)
        {
            SwapChainCoreState chain{};
            chain.animationErrorSource = AnimationErrorSource::PCLatency;
            chain.firstAppSimStartTime = 30'000;
            chain.lastDisplayedSimStartTime = 31'000;
            chain.lastDisplayedAppScreenTime = 8'800;

            auto frame = MakeFrame(PresentResult::Presented, 10'000, 100, 10'400,
                                   { { FrameType::Application, 10'950 } },
                                   40'000 /* appSimStartTime */, 50'000 /* pclSimStart */);

            chain.UpdateAfterPresentV1(frame);

            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::AreEqual(uint64_t(40'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(10'950), chain.lastDisplayedAppScreenTime);
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

            chain.UpdateAfterPresentV1(frame);

            Assert::AreEqual(uint64_t(11'000), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(1234), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(UpdateAfterPresent_FlipDelayTracking_PresentedNoDisplays_ZeroesFlipDelayAndScreenTime)
        {
            SwapChainCoreState chain{};
            auto frame = MakeFrame(PresentResult::Presented, 12'000, 40, 12'300,
                                   {}, 0, 0, 9999);

            chain.UpdateAfterPresentV1(frame);

            Assert::AreEqual(uint64_t(0), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), chain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(UpdateAfterPresent_NotPresented_DoesNotChangeLastDisplayedScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData seed{};
            seed.presentStartTime = 1'000;
            seed.timeInPresent = 30;
            seed.readyTime = 1'200;
            seed.finalState = PresentResult::Presented;
            seed.displayed.PushBack({ FrameType::Application, 1'500 });

            (void)swapChain.ProcessPresent(qpc, std::move(seed));
            Assert::AreEqual(uint64_t(1'500), swapChain.swapChain.lastDisplayedScreenTime);

            FrameData notPresented{};
            notPresented.presentStartTime = 2'000;
            notPresented.timeInPresent = 25;
            notPresented.readyTime = 2'150;
            notPresented.finalState = static_cast<PresentResult>(7777);
            notPresented.displayed.PushBack({ FrameType::Application, 2'600 });

            (void)swapChain.ProcessPresent(qpc, std::move(notPresented));

            Assert::AreEqual(uint64_t(1'500), swapChain.swapChain.lastDisplayedScreenTime,
                L"Should remain unchanged.");
        }
    };

    TEST_CLASS(UpdateAfterBootstrapPresentV2Tests)
    {
    public:
        TEST_METHOD(UpdateAfterBootstrapPresentV2_AppInMiddle_UsesAppDisplayForAppState)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrapPresent{};
            bootstrapPresent.presentStartTime = 10'000;
            bootstrapPresent.timeInPresent = 50;
            bootstrapPresent.readyTime = 10'300;
            bootstrapPresent.finalState = PresentResult::Presented;
            bootstrapPresent.appSimStartTime = 40'000;
            bootstrapPresent.flipDelay = 1234;
            bootstrapPresent.displayed.PushBack({ FrameType::Repeated, 10'700 });
            bootstrapPresent.displayed.PushBack({ FrameType::Application, 10'800 });
            bootstrapPresent.displayed.PushBack({ FrameType::Repeated, 11'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(bootstrapPresent));
            Assert::AreEqual(size_t(0), rows.size(), L"Bootstrap present seeds state only.");

            const auto& chain = swapChain.swapChain;
            Assert::AreEqual(uint64_t(11'000), chain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(1234), chain.lastDisplayedFlipDelay);
            Assert::AreEqual(uint64_t(10'800), chain.lastDisplayedAppScreenTime);
            Assert::AreEqual(uint64_t(40'000), chain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(40'000), chain.firstAppSimStartTime);
            Assert::IsTrue(chain.animationErrorSource == AnimationErrorSource::AppProvider);
            Assert::IsTrue(chain.lastAppPresent.has_value());
            Assert::IsTrue(chain.lastPresent.has_value());
        }
    };

    TEST_CLASS(ProcessPresentXefgAfmfReleaseTests)
    {
    public:
        TEST_METHOD(IntelXefg_MultiEntryPresent_HoldsClosedIntervalWithoutLookahead)
        {
            // AppA seed, then one present gen+gen+gen+AppB with no further display: timeline
            // origin AppA publishes when the first gen supplies lookahead; the closed interval
            // (Gen1..Gen3, AppB) stays held with no closing-app lookahead.
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData seed{};
            seed.presentStartTime = 9'000;
            seed.timeInPresent = 400;
            seed.readyTime = 9'500;
            seed.finalState = PresentResult::Presented;
            seed.appSimStartTime = 8'000;
            seed.displayed.PushBack({ FrameType::Application, 10'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(seed)).size());

            FrameData present{};
            present.presentStartTime = 10'000;
            present.timeInPresent = 500;
            present.readyTime = 20'000;
            present.finalState = PresentResult::Presented;
            present.appSimStartTime = 9'500;
            present.displayed.PushBack({ FrameType::Intel_XEFG, 11'000 });
            present.displayed.PushBack({ FrameType::Intel_XEFG, 11'500 });
            present.displayed.PushBack({ FrameType::Intel_XEFG, 12'000 });
            present.displayed.PushBack({ FrameType::Application, 12'500 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(present));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual((int)FrameType::Application, (int)rows[0].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(10'000), rows[0].computed.metrics.screenTimeQpc);
            Assert::IsFalse(HasMetricValue(rows[0].computed.metrics.msAnimationError));
        }

        TEST_METHOD(AmdAfmf_MultiEntryPresent_ReleasesClosedIntervalWithLookahead)
        {
            // AppA seed, then gen+gen+gen+AppB, then AppC for closing-app lookahead: the
            // multi-gen present only releases timeline origin AppA; the full closed interval
            // (3x AMD_AFMF, AppB) releases on the next present.
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData seed{};
            seed.presentStartTime = 19'000;
            seed.timeInPresent = 400;
            seed.readyTime = 19'500;
            seed.finalState = PresentResult::Presented;
            seed.appSimStartTime = 18'000;
            seed.displayed.PushBack({ FrameType::Application, 20'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(seed)).size());

            FrameData present{};
            present.presentStartTime = 20'000;
            present.timeInPresent = 600;
            present.readyTime = 30'000;
            present.finalState = PresentResult::Presented;
            present.appSimStartTime = 19'500;
            present.displayed.PushBack({ FrameType::AMD_AFMF, 21'000 });
            present.displayed.PushBack({ FrameType::AMD_AFMF, 21'500 });
            present.displayed.PushBack({ FrameType::AMD_AFMF, 22'000 });
            present.displayed.PushBack({ FrameType::Application, 22'500 });

            auto afterMultiGenPresent = swapChain.ProcessPresent(qpc, std::move(present));
            Assert::AreEqual(size_t(1), afterMultiGenPresent.size());
            Assert::AreEqual((int)FrameType::Application, (int)afterMultiGenPresent[0].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(20'000), afterMultiGenPresent[0].computed.metrics.screenTimeQpc);
            Assert::IsFalse(HasMetricValue(afterMultiGenPresent[0].computed.metrics.msAnimationError));

            FrameData lookahead{};
            lookahead.presentStartTime = 23'000;
            lookahead.timeInPresent = 400;
            lookahead.readyTime = 30'500;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 20'000;
            lookahead.displayed.PushBack({ FrameType::Application, 24'000 });

            auto afterLookaheadPresent = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(4), afterLookaheadPresent.size());
            Assert::AreEqual((int)FrameType::AMD_AFMF, (int)afterLookaheadPresent[0].computed.metrics.frameType);
            Assert::AreEqual((int)FrameType::AMD_AFMF, (int)afterLookaheadPresent[1].computed.metrics.frameType);
            Assert::AreEqual((int)FrameType::AMD_AFMF, (int)afterLookaheadPresent[2].computed.metrics.frameType);
            Assert::AreEqual((int)FrameType::Application, (int)afterLookaheadPresent[3].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(21'000), afterLookaheadPresent[0].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(21'500), afterLookaheadPresent[1].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(22'000), afterLookaheadPresent[2].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(22'500), afterLookaheadPresent[3].computed.metrics.screenTimeQpc);
        }
    };

    TEST_CLASS(DisplayedDroppedDisplayedSequenceTests)
    {
    public:
        TEST_METHOD(Displayed_Dropped_Displayed_Sequence_IsHandledAcrossCalls)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData a{};
            a.presentStartTime = 50'000;
            a.timeInPresent = 400;
            a.readyTime = 50'500;
            a.finalState = PresentResult::Presented;
            a.appSimStartTime = 50'000;
            a.displayed.PushBack({ FrameType::Application, 51'000 });

            auto rowsA = swapChain.ProcessPresent(qpc, std::move(a));
            Assert::AreEqual(size_t(0), rowsA.size(), L"Timeline origin waits for display lookahead.");
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastDisplayedScreenTime);

            FrameData b{};
            b.presentStartTime = 52'000;
            b.timeInPresent = 300;
            b.readyTime = 52'400;
            b.finalState = PresentResult::Discarded;

            auto rowsB = swapChain.ProcessPresent(qpc, std::move(b));
            Assert::AreEqual(size_t(0), rowsB.size(), L"Dropped present held after first app anchor is seeded.");
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastDisplayedScreenTime);

            FrameData c{};
            c.presentStartTime = 53'000;
            c.timeInPresent = 350;
            c.readyTime = 53'400;
            c.finalState = PresentResult::Presented;
            c.appSimStartTime = 52'000;
            c.displayed.PushBack({ FrameType::Application, 54'000 });

            auto rowsC = swapChain.ProcessPresent(qpc, std::move(c));
            Assert::IsTrue(rowsC.size() >= size_t(1), L"Lookahead publishes the held origin row.");

            bool publishedOriginA = false;
            for (const auto& row : rowsC) {
                if (row.computed.metrics.screenTimeQpc == 51'000) {
                    publishedOriginA = true;
                    break;
                }
            }
            Assert::IsTrue(publishedOriginA);
            Assert::AreEqual(uint64_t(51'000), swapChain.swapChain.lastDisplayedScreenTime);
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

            auto firstMetrics = ComputeMetricsForPresent(qpc, first, chain);

            // We should get exactly one metrics entry
            Assert::AreEqual(size_t(1), firstMetrics.size(), L"First not-displayed frame should produce one metrics entry.");

            // With no prior lastPresent, msBetweenPresents should be zero
            AssertAreEqualWithinTolerance(
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

            auto secondMetrics = ComputeMetricsForPresent(qpc, second, chain);

            Assert::AreEqual(
                size_t(1),
                secondMetrics.size(),
                L"Second not-displayed frame should also produce one metrics entry.");

            // Expected delta: use the same converter the implementation uses
            double expectedDelta = qpc.DeltaUnsignedMilliSeconds(
                first.presentStartTime,
                second.presentStartTime);

            AssertAreEqualWithinTolerance(
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
                /*readyTime*/        1'500'000,   // 0.15s -> 50 ms after start
                /*displayed*/{}           // no displays => "not displayed" path
            );
            first.gpuStartTime = 1'200'000; // 0.12s

            auto firstMetricsList = ComputeMetricsForPresent(qpc, first, chain);
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

            // No prior lastPresent -> msBetweenPresents should be 0
            Assert::AreEqual(
                0.0,
                firstMetrics.msBetweenPresents,
                0.0001,
                L"First frame should have msBetweenPresents == 0.");

            // msInPresentApi = delta for TimeInPresent
            double expectedMsInPresentFirst = qpc.DurationMilliSeconds(first.timeInPresent);
            AssertAreEqualWithinTolerance(
                expectedMsInPresentFirst,
                firstMetrics.msInPresentApi,
                0.0001,
                L"msInPresentApi should equal QpcDeltaToMilliSeconds(timeInPresent).");

            // msUntilRenderComplete = delta between PresentStart and Ready
            double expectedMsUntilRenderCompleteFirst =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, first.readyTime);
            AssertAreEqualWithinTolerance(
                expectedMsUntilRenderCompleteFirst,
                firstMetrics.msUntilRenderComplete,
                0.0001,
                L"msUntilRenderComplete should equal delta from PresentStartTime to ReadyTime.");

            // msUntilRenderStart = delta between PresentStart and GPU start
            double expectedMsUntilRenderStart =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, first.gpuStartTime);
            AssertAreEqualWithinTolerance(
                expectedMsUntilRenderStart,
                firstMetrics.msUntilRenderStart,
                0.0001,
                L"msUntilRenderStart should equal delta from PresentStartTime to GPUStartTime.");

            // With no prior present, CalculateCPUStart should return 0 -> cpuStartQpc == 0
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
            //  - compute msBetweenPresents based on first->second start times
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

            auto secondMetricsList = ComputeMetricsForPresent(qpc, second, chain);
            Assert::AreEqual(
                size_t(1),
                secondMetricsList.size(),
                L"Second not-displayed frame should produce one metrics entry.");

            const auto& secondMetrics = secondMetricsList[0].metrics;

            // msBetweenPresents should be based on lastPresent.start -> second.start
            double expectedBetween =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, second.presentStartTime);
            AssertAreEqualWithinTolerance(
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

            AssertAreEqualWithinTolerance(
                expectedMsInPresentSecond,
                secondMetrics.msInPresentApi,
                0.0001,
                L"Second frame msInPresentApi should match timeInPresent.");
            AssertAreEqualWithinTolerance(
                expectedMsUntilRenderCompleteSecond,
                secondMetrics.msUntilRenderComplete,
                0.0001,
                L"Second frame msUntilRenderComplete should match start->ready delta.");
            Assert::AreEqual(
                expectedMsUntilRenderStartSecond,
                secondMetrics.msUntilRenderStart,
                0.0001,
                L"Second frame msUntilRenderStart should match start->GPU start delta.");

            // cpuStartQpc for second should come from CalculateCPUStart:
            // lastAppPresent == first (no propagated times) -> first.start + first.timeInPresent
            uint64_t expectedCpuStartSecond = first.presentStartTime + first.timeInPresent;
            Assert::AreEqual(
                expectedCpuStartSecond,
                secondMetrics.cpuStartQpc,
                L"cpuStartQpc should match CalculateCPUStart from lastAppPresent.");
        }
        TEST_METHOD(ComputeMetricsForPresent_Displayed_BaseTimingAndCpuStart_AreCorrect)
        {
            // 10 MHz QPC
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            // Baseline frame: Presented but not displayed -> not-displayed path
            FrameData first = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'000'000,
                /*timeInPresent*/    200'000,
                /*readyTime*/        1'500'000,
                /*displayed*/{});   // no displays
            
            auto firstMetricsList = ComputeMetricsForPresent(qpc, first, chain);
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

            // Second frame: Presented + one displayed instance.
            FrameData second = MakeFrame(
                PresentResult::Presented,
                /*presentStartTime*/ 1'016'000,      // slightly later than first
                /*timeInPresent*/    300'000,
                /*readyTime*/        1'616'000,
                DisplayedVector{
                    { FrameType::Application, 2'000'000 }   // one displayed instance
            });
            second.gpuStartTime = 1'200'000;

            auto secondMetricsList = ComputeMetricsForPresent(qpc, second, chain);

            Assert::AreEqual(
                size_t(1),
                secondMetricsList.size(),
                L"Displayed V1 frame should produce one metrics entry.");

            const auto& secondMetrics = secondMetricsList[0].metrics;

            // timeInSeconds from presentStartTime
            auto expectedTimeInSecondsSecond = second.presentStartTime;
            Assert::AreEqual(
                expectedTimeInSecondsSecond,
                secondMetrics.timeInSeconds,
                L"timeInSeconds should match QpcToSeconds(presentStartTime) for displayed frame.");

            // msBetweenPresents: lastPresent.start (first) -> second.start
            double expectedBetween =
                qpc.DeltaUnsignedMilliSeconds(first.presentStartTime, second.presentStartTime);
            AssertAreEqualWithinTolerance(
                expectedBetween,
                secondMetrics.msBetweenPresents,
                0.0001,
                L"msBetweenPresents should match delta between lastPresent and current presentStart for displayed frame.");

            // msInPresentApi from timeInPresent
            double expectedMsInPresentSecond = qpc.DurationMilliSeconds(second.timeInPresent);
            AssertAreEqualWithinTolerance(
                expectedMsInPresentSecond,
                secondMetrics.msInPresentApi,
                0.0001,
                L"msInPresentApi should match QpcDeltaToMilliSeconds(timeInPresent) for displayed frame.");

            // msUntilRenderComplete from start -> ready
            double expectedMsUntilRenderCompleteSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.readyTime);
            AssertAreEqualWithinTolerance(
                expectedMsUntilRenderCompleteSecond,
                secondMetrics.msUntilRenderComplete,
                0.0001,
                L"msUntilRenderComplete should match start->ready delta for displayed frame.");

            // msUntilRenderStart from start -> GPU start
            double expectedMsUntilRenderStartSecond =
                qpc.DeltaUnsignedMilliSeconds(second.presentStartTime, second.gpuStartTime);
            AssertAreEqualWithinTolerance(
                expectedMsUntilRenderStartSecond,
                secondMetrics.msUntilRenderStart,
                0.0001,
                L"msUntilRenderStart should match start->GPU start delta for displayed frame.");
            
            // cpuStartQpc should come from CalculateCPUStart using baseline frame as lastAppPresent:
            // (no propagated times) -> first.start + first.timeInPresent
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            AssertAreEqualWithinTolerance(0.0, m.msUntilDisplayed, 0.0001);
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
            frame.displayed.PushBack({ FrameType::Application, 2'500'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.presentStartTime, frame.displayed[0].second);
            AssertAreEqualWithinTolerance(expected, m.msUntilDisplayed, 0.0001);
        }
        TEST_METHOD(DisplayedApplicationFrame_AlsoReturnsDelta)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 15'000;
            frame.readyTime = 5'030'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 5'100'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            double expected = qpc.DeltaUnsignedMilliSeconds(frame.presentStartTime, frame.displayed[0].second);
            AssertAreEqualWithinTolerance(expected, m.msUntilDisplayed, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
            AssertAreEqualWithinTolerance(0.0, m.msDisplayedTime, 0.0001);
        }

        TEST_METHOD(DisplayedSingleDisplay_WithNextDisplay_ReturnsDeltaToNextScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 2'000'000;
            frame.timeInPresent = 20'000;
            frame.readyTime = 2'050'000;
            frame.finalState = PresentResult::Presented;
            frame.displayed.PushBack({ FrameType::Application, 2'500'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(frame)).size());

            FrameData next{};
            next.presentStartTime = 2'600'000;
            next.timeInPresent = 15'000;
            next.readyTime = 2'650'000;
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 2'800'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(next));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(2'500'000), rows[0].computed.metrics.screenTimeQpc);

            double expected = qpc.DeltaUnsignedMilliSeconds(2'500'000, 2'800'000);
            AssertAreEqualWithinTolerance(expected, rows[0].computed.metrics.msDisplayedTime, 0.0001);
        }

        TEST_METHOD(DisplayedMultipleDisplays_ProcessEachWithNextScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 3'000'000;
            frame.timeInPresent = 30'000;
            frame.readyTime = 3'050'000;
            frame.finalState = PresentResult::Presented;
            frame.appSimStartTime = 3'000'000;
            frame.displayed.PushBack({ FrameType::Application, 3'100'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 3'400'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 3'700'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(frame));
            Assert::AreEqual(size_t(1), originRows.size());

            double expectedOrigin = qpc.DeltaUnsignedMilliSeconds(3'100'000, 3'400'000);
            AssertAreEqualWithinTolerance(expectedOrigin, originRows[0].computed.metrics.msDisplayedTime, 0.0001);

            FrameData closingApp{};
            closingApp.presentStartTime = 3'800'000;
            closingApp.timeInPresent = 25'000;
            closingApp.readyTime = 3'850'000;
            closingApp.finalState = PresentResult::Presented;
            closingApp.appSimStartTime = 3'300'000;
            closingApp.displayed.PushBack({ FrameType::Application, 4'000'000 });

            auto heldIntervalRows = swapChain.ProcessPresent(qpc, std::move(closingApp));
            Assert::AreEqual(size_t(0), heldIntervalRows.size());

            FrameData lookahead{};
            lookahead.presentStartTime = 4'100'000;
            lookahead.timeInPresent = 20'000;
            lookahead.readyTime = 4'150'000;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 3'320'000;
            lookahead.displayed.PushBack({ FrameType::Application, 4'300'000 });

            auto intervalRows = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(3), intervalRows.size());

            double expectedFirstGen = qpc.DeltaUnsignedMilliSeconds(3'400'000, 3'700'000);
            AssertAreEqualWithinTolerance(expectedFirstGen, intervalRows[0].computed.metrics.msDisplayedTime, 0.0001);

            double expectedSecondGen = qpc.DeltaUnsignedMilliSeconds(3'700'000, 4'000'000);
            AssertAreEqualWithinTolerance(expectedSecondGen, intervalRows[1].computed.metrics.msDisplayedTime, 0.0001);

            double expectedClosingApp = qpc.DeltaUnsignedMilliSeconds(4'000'000, 4'300'000);
            AssertAreEqualWithinTolerance(expectedClosingApp, intervalRows[2].computed.metrics.msDisplayedTime, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            AssertAreEqualWithinTolerance(0.0, m.msBetweenDisplayChange, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            double expected = qpc.DeltaUnsignedMilliSeconds(4'000'000, 5'500'000);
            AssertAreEqualWithinTolerance(expected, m.msBetweenDisplayChange, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            AssertAreEqualWithinTolerance(0.0, m.msBetweenDisplayChange, 0.0001);
        }

        TEST_METHOD(MultipleDisplays_EachComputesDeltaFromPreviousDisplayedEntry)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 2'500'000;
            bootstrap.timeInPresent = 50'000;
            bootstrap.readyTime = 2'600'000;
            bootstrap.finalState = PresentResult::Presented;
            bootstrap.appSimStartTime = 2'900'000;
            bootstrap.displayed.PushBack({ FrameType::Application, 3'000'000 });

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            Assert::AreEqual(uint64_t(3'000'000), swapChain.swapChain.lastDisplayedScreenTime);

            FrameData frame{};
            frame.presentStartTime = 5'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 5'100'000;
            frame.finalState = PresentResult::Presented;
            frame.appSimStartTime = 6'000'000;
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 5'500'000 });
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 5'800'000 });
            frame.displayed.PushBack({ FrameType::Application, 6'100'000 });

            auto preAnchorRows = swapChain.ProcessPresent(qpc, std::move(frame));
            Assert::AreEqual(size_t(2), preAnchorRows.size());

            double expected0 = qpc.DeltaUnsignedMilliSeconds(3'000'000, 5'500'000);
            AssertAreEqualWithinTolerance(expected0, preAnchorRows[0].computed.metrics.msBetweenDisplayChange, 0.0001);

            double expected1 = qpc.DeltaUnsignedMilliSeconds(5'500'000, 5'800'000);
            AssertAreEqualWithinTolerance(expected1, preAnchorRows[1].computed.metrics.msBetweenDisplayChange, 0.0001);

            FrameData lookahead{};
            lookahead.presentStartTime = 6'200'000;
            lookahead.timeInPresent = 30'000;
            lookahead.readyTime = 6'250'000;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 6'050'000;
            lookahead.displayed.PushBack({ FrameType::Application, 6'400'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(1), originRows.size());

            double expected2 = qpc.DeltaUnsignedMilliSeconds(5'800'000, 6'100'000);
            AssertAreEqualWithinTolerance(expected2, originRows[0].computed.metrics.msBetweenDisplayChange, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsFalse(HasMetricValue(m.msFlipDelay),
                L"msFlipDelay should be missing for a non-displayed frame.");
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (HasMetricValue(m.msFlipDelay)) {
                double expected = qpc.DurationMilliSeconds(100'000);
                AssertAreEqualWithinTolerance(expected, m.msFlipDelay, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsFalse(HasMetricValue(m.msFlipDelay),
                L"msFlipDelay should be missing when flipDelay is zero.");
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
            frame.displayed.PushBack({ FrameType::Application, 7'500'000 });

            FrameData next{};
            next.finalState = PresentResult::Presented;
            next.displayed.PushBack({ FrameType::Application, 8'000'000 });

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            if (HasMetricValue(m.msFlipDelay)) {
                double expected = qpc.DurationMilliSeconds(50'000);
                AssertAreEqualWithinTolerance(expected, m.msFlipDelay, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(9'500'000), m.screenTimeQpc);
        }

        TEST_METHOD(DisplayedMultipleFrames_EachHasOwnScreenTime)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 9'000'000;
            frame.timeInPresent = 90'000;
            frame.readyTime = 9'100'000;
            frame.finalState = PresentResult::Presented;
            frame.appSimStartTime = 9'000'000;
            frame.displayed.PushBack({ FrameType::Application, 9'500'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 9'800'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 10'100'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(frame));
            Assert::AreEqual(size_t(1), originRows.size());
            Assert::AreEqual(uint64_t(9'500'000), originRows[0].computed.metrics.screenTimeQpc);

            FrameData closingApp{};
            closingApp.presentStartTime = 10'200'000;
            closingApp.timeInPresent = 30'000;
            closingApp.readyTime = 10'250'000;
            closingApp.finalState = PresentResult::Presented;
            closingApp.appSimStartTime = 9'300'000;
            closingApp.displayed.PushBack({ FrameType::Application, 10'400'000 });

            auto heldIntervalRows = swapChain.ProcessPresent(qpc, std::move(closingApp));
            Assert::AreEqual(size_t(0), heldIntervalRows.size());

            FrameData lookahead{};
            lookahead.presentStartTime = 10'500'000;
            lookahead.timeInPresent = 25'000;
            lookahead.readyTime = 10'550'000;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 9'320'000;
            lookahead.displayed.PushBack({ FrameType::Application, 10'700'000 });

            auto intervalRows = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(3), intervalRows.size());
            Assert::AreEqual(uint64_t(9'800'000), intervalRows[0].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(10'100'000), intervalRows[1].computed.metrics.screenTimeQpc);
        }

        TEST_METHOD(V1_FirstDisplayNonAppFrame_UsesFirstDisplayScreenTime)
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(9'700'000), m.screenTimeQpc);
        }
    };
    TEST_CLASS(NvCollapsedPresentTests)
    {
    public:
        TEST_METHOD(NvCollapsedPresent_V1_AdjustsCurrentScreenTimeAndFlipDelayFromPreviousFrame)
        {
            // V1 correction uses prior displayed state. When the previous displayed
            // screen time is greater than the current screen time, the current frame's
            // screen time and flip delay are adjusted upward.

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

            // Second frame.
            FrameData second{};
            second.presentStartTime = 5'000'000;
            second.timeInPresent = 40'000;
            second.readyTime = 5'100'000;
            second.flipDelay = 100'000;  // Original flip delay for second frame
            second.finalState = PresentResult::Presented;
            // Second's raw screen time is earlier than first's, so V1 correction applies.
            second.displayed.PushBack({ FrameType::Application, 5'000'000 });

            auto resultsFirst = ComputeMetricsForPresent(qpc, first, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            // Now process second frame, which should be adjusted by the V1 NV correction.
            auto resultsSecond = ComputeMetricsForPresent(qpc, second, chain);
            Assert::AreEqual(size_t(1), resultsSecond.size());
            const auto& secondMetrics = resultsSecond[0].metrics;

            // V1 adjustment: second's screenTime should be raised to first's screenTime
            // when first.screenTime (5'500'000) > second.screenTime (5'000'000).
            Assert::AreEqual(uint64_t(5'500'000), secondMetrics.screenTimeQpc,
                L"V1 should adjust second's screenTime to first's screenTime (5'500'000)");

            // V1 adjustment: second's flipDelay should be increased by the difference.
            // effectiveSecondFlipDelay = 100'000 + (5'500'000 - 5'000'000) = 100'000 + 500'000 = 600'000
            uint64_t expectedEffectiveFlipDelaySecond = 100'000 + (5'500'000 - 5'000'000);
            double expectedMsFlipDelaySecond = qpc.DurationMilliSeconds(expectedEffectiveFlipDelaySecond);

            Assert::IsTrue(HasMetricValue(secondMetrics.msFlipDelay),
                L"msFlipDelay should be set for displayed frame");
            if (HasMetricValue(secondMetrics.msFlipDelay)) {
                AssertAreEqualWithinTolerance(expectedMsFlipDelaySecond, secondMetrics.msFlipDelay, 0.0001,
                    L"V1 should adjust second's flipDelay to account for screenTime catch-up");
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

            auto results = ComputeMetricsForPresent(qpc, current, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& metrics = results[0].metrics;

            // No NV1 adjustment: screenTime should remain unchanged
            Assert::AreEqual(uint64_t(4'000'000), metrics.screenTimeQpc,
                L"No collapse: screenTime should remain at original value");

            // No adjustment to flipDelay: should use original 75'000
            double expectedMsFlipDelay = qpc.DurationMilliSeconds(75'000);

            Assert::IsTrue(HasMetricValue(metrics.msFlipDelay),
                L"msFlipDelay should be set for displayed frame");
            if (HasMetricValue(metrics.msFlipDelay)) {
                AssertAreEqualWithinTolerance(expectedMsFlipDelay, metrics.msFlipDelay, 0.0001,
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
            // Second screen time is equal to first (5'000'000), so V1 should not adjust.
            second.displayed.PushBack({ FrameType::Application, 5'000'000 });

            auto resultsFirst = ComputeMetricsForPresent(qpc, first, chain);
            Assert::AreEqual(size_t(1), resultsFirst.size());

            auto resultsSecond = ComputeMetricsForPresent(qpc, second, chain);
            Assert::AreEqual(size_t(1), resultsSecond.size());
            const auto& secondMetrics = resultsSecond[0].metrics;

            // V1 should not adjust: second's screenTime should remain at 5'000'000.
            Assert::AreEqual(uint64_t(5'000'000), secondMetrics.screenTimeQpc,
                L"V1: when second.screenTime >= first.screenTime, no adjustment should occur");

            // flipDelay should remain at original 50'000
            double expectedMsFlipDelay = qpc.DurationMilliSeconds(50'000);

            Assert::IsTrue(HasMetricValue(secondMetrics.msFlipDelay),
                L"msFlipDelay should be set for displayed frame");
            if (HasMetricValue(secondMetrics.msFlipDelay)) {
                AssertAreEqualWithinTolerance(expectedMsFlipDelay, secondMetrics.msFlipDelay, 0.0001,
                    L"V1: when no collapse, flipDelay should remain unchanged");
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

            auto results = ComputeMetricsForPresent(qpc, current, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::AreEqual(uint64_t(5'500'000), m.screenTimeQpc,
                L"NV1 should adjust current screenTime to lastDisplayedScreenTime");

            const uint64_t expectedFlipDelay = 100'000 + (5'500'000 - 5'000'000);
            Assert::IsTrue(HasMetricValue(m.msFlipDelay), L"msFlipDelay should be set for displayed frame");
            if (HasMetricValue(m.msFlipDelay)) {
                AssertAreEqualWithinTolerance(qpc.DurationMilliSeconds(expectedFlipDelay), m.msFlipDelay, 0.0001,
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
            // QPC frequency 10 MHz -> 1'000'000 ticks = 0.1 ms
            // Expected: msDisplayLatency approx 0.1 ms

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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msDisplayLatency = screenTime - cpuStart = 2'000'000 - 1'000'000 = 1'000'000 ticks = 0.1 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'000'000);
            AssertAreEqualWithinTolerance(expected, m.msDisplayLatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 1'700'000 + 300'000 = 2'000'000
            // msDisplayLatency = 2'000'000 - 2'000'000 = 0
            AssertAreEqualWithinTolerance(0.0, m.msDisplayLatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            AssertAreEqualWithinTolerance(0.0, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_ZeroCpuStart)
        {
            // Scenario: No prior chain history; cpuStart defaults to 0.
            // cpuStart = 0, screenTime = 3'000'000
            // Expected: msDisplayLatency approx 0.3 ms

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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 0 (no prior app present)
            // msDisplayLatency = 3'000'000 - 0 = 3'000'000 ticks = 0.3 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 3'000'000);
            AssertAreEqualWithinTolerance(expected, m.msDisplayLatency, 0.0001);
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
            // Expected: msReadyTimeToDisplayLatency approx 0.05 ms

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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // msReadyTimeToDisplayLatency = screenTime - readyTime = 2'000'000 - 1'500'000 = 500'000 ticks = 0.05 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'000'000);
            Assert::IsTrue(HasMetricValue(m.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expected, m.msReadyTimeToDisplayLatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsTrue(HasMetricValue(m.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(0.0, m.msReadyTimeToDisplayLatency, 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_NotDisplayed_ReturnsZero)
        {
            // Scenario: Frame with no displayed entries.
            // Expected: msReadyTimeToDisplayLatency is missing.

            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'500'000;
            frame.finalState = PresentResult::Presented;
            // No displayed entries

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            Assert::IsFalse(HasMetricValue(m.msReadyTimeToDisplayLatency));
        }

        TEST_METHOD(ReadyTimeToDisplay_ReadyTimeZero)
        {
            // Scenario: Ready time is set to a non-zero value before screen time.
            // readyTime = 70'000, screenTime = 2'000'000
            // Expected: msReadyTimeToDisplayLatency approx 0.193 ms

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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // msReadyTimeToDisplayLatency = 2'000'000 - 70'000 = 1'930'000 ticks = 0.193 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(70'000, 2'000'000);
            Assert::IsTrue(HasMetricValue(m.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expected, m.msReadyTimeToDisplayLatency, 0.0001);
        }
    };

    TEST_CLASS(MultiDisplayLatencyTests)
    {
    public:
        TEST_METHOD(DisplayLatency_MultipleDisplays_EachComputesIndependently)
        {
            // cpuStart = 800'000 + 200'000 = 1'000'000 for each display instance on the multi-flip present.
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 800'000;
            bootstrap.timeInPresent = 200'000;
            bootstrap.readyTime = 900'000;
            bootstrap.finalState = PresentResult::Presented;
            bootstrap.displayed.PushBack({ FrameType::Application, 1'500'000 });

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'100'000;
            frame.finalState = PresentResult::Presented;
            frame.appSimStartTime = 1'000'000;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 2'100'000 });
            frame.displayed.PushBack({ FrameType::Repeated, 2'200'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(frame));
            Assert::AreEqual(size_t(1), originRows.size());

            double expected0 = qpc.DeltaUnsignedMilliSeconds(1'000'000, 2'000'000);
            AssertAreEqualWithinTolerance(expected0, originRows[0].computed.metrics.msDisplayLatency, 0.0001);

            FrameData closingApp{};
            closingApp.presentStartTime = 2'300'000;
            closingApp.timeInPresent = 40'000;
            closingApp.readyTime = 2'350'000;
            closingApp.finalState = PresentResult::Presented;
            closingApp.appSimStartTime = 1'050'000;
            closingApp.displayed.PushBack({ FrameType::Application, 2'500'000 });

            (void)swapChain.ProcessPresent(qpc, std::move(closingApp));

            FrameData lookahead{};
            lookahead.presentStartTime = 2'400'000;
            lookahead.timeInPresent = 30'000;
            lookahead.readyTime = 2'450'000;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 1'070'000;
            lookahead.displayed.PushBack({ FrameType::Application, 2'600'000 });

            auto intervalRows = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(3), intervalRows.size());

            double expected1 = qpc.DeltaUnsignedMilliSeconds(1'050'000, 2'100'000);
            AssertAreEqualWithinTolerance(expected1, intervalRows[0].computed.metrics.msDisplayLatency, 0.0001);

            double expected2 = qpc.DeltaUnsignedMilliSeconds(1'050'000, 2'200'000);
            AssertAreEqualWithinTolerance(expected2, intervalRows[1].computed.metrics.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_MultipleDisplays_IndependentDeltas)
        {
            // Same readyTime on the multi-flip present; each display row uses its own screenTime.
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData frame{};
            frame.presentStartTime = 1'000'000;
            frame.timeInPresent = 50'000;
            frame.readyTime = 1'500'000;
            frame.finalState = PresentResult::Presented;
            frame.appSimStartTime = 1'000'000;
            frame.displayed.PushBack({ FrameType::Application, 2'000'000 });
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 2'100'000 });
            frame.displayed.PushBack({ FrameType::Intel_XEFG, 2'200'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(frame));
            Assert::AreEqual(size_t(1), originRows.size());

            double expected0 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'000'000);
            Assert::IsTrue(HasMetricValue(originRows[0].computed.metrics.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expected0, originRows[0].computed.metrics.msReadyTimeToDisplayLatency, 0.0001);

            FrameData closingApp{};
            closingApp.presentStartTime = 2'300'000;
            closingApp.timeInPresent = 40'000;
            closingApp.readyTime = 2'350'000;
            closingApp.finalState = PresentResult::Presented;
            closingApp.appSimStartTime = 1'050'000;
            closingApp.displayed.PushBack({ FrameType::Application, 2'500'000 });

            (void)swapChain.ProcessPresent(qpc, std::move(closingApp));

            FrameData lookahead{};
            lookahead.presentStartTime = 2'400'000;
            lookahead.timeInPresent = 30'000;
            lookahead.readyTime = 2'450'000;
            lookahead.finalState = PresentResult::Presented;
            lookahead.appSimStartTime = 1'070'000;
            lookahead.displayed.PushBack({ FrameType::Application, 2'600'000 });

            auto intervalRows = swapChain.ProcessPresent(qpc, std::move(lookahead));
            Assert::AreEqual(size_t(3), intervalRows.size());

            double expected1 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'100'000);
            Assert::IsTrue(HasMetricValue(intervalRows[0].computed.metrics.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expected1, intervalRows[0].computed.metrics.msReadyTimeToDisplayLatency, 0.0001);

            double expected2 = qpc.DeltaUnsignedMilliSeconds(1'500'000, 2'200'000);
            Assert::IsTrue(HasMetricValue(intervalRows[1].computed.metrics.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expected2, intervalRows[1].computed.metrics.msReadyTimeToDisplayLatency, 0.0001);
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
            // Expected: msDisplayLatency approx 0.295 ms (using adjusted screenTime 4'000'000 - 1'050'000)
            // Expected: msFlipDelay approx 0.103 ms (original 30'000 + adjustment)

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

            // Set up chain with prior app present to establish cpuStart
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results1 = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // No adjust of first frame msDisplayLatency = 4'000'000 - 1'000'000 = 3'000'000 ticks = 0.3 ms
            double expectedDisplayLatency = qpc.DeltaUnsignedMilliSeconds(1'000'000, 4'000'000);
            AssertAreEqualWithinTolerance(expectedDisplayLatency, results1[0].metrics.msDisplayLatency, 0.0001);
            double expectedFlipDelay = qpc.DurationMilliSeconds(frame.flipDelay);
            Assert::IsTrue(HasMetricValue(results1[0].metrics.msFlipDelay));
            AssertAreEqualWithinTolerance(expectedFlipDelay, results1[0].metrics.msFlipDelay, 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, next1, chain);
            Assert::AreEqual(size_t(1), results2.size());

            // After NV adjustment: screenTime = 4'000'000 -> set from NV FlipDelay adjustment
            // msDisplayLatency = 4'000'000 - 1'050'000 = 2'950'000 ticks = 0.295 ms
            // msFlipDelay = original 30'000 + (4'000'000 - 3'000'000) = 1'030'000 ticks = 0.103 ms
            double expectedDisplayLatency2 = qpc.DeltaUnsignedMilliSeconds(1'050'000, 4'000'000);
            AssertAreEqualWithinTolerance(expectedDisplayLatency2, results2[0].metrics.msDisplayLatency, 0.0001);
            double expectedFlipDelay2 = qpc.DurationMilliSeconds(1'030'000);
            Assert::IsTrue(HasMetricValue(results2[0].metrics.msFlipDelay));
            AssertAreEqualWithinTolerance(expectedFlipDelay2, results2[0].metrics.msFlipDelay, 0.0001);
        }

        TEST_METHOD(ReadyTimeToDisplay_NvCollapsed_UsesAdjustedScreenTime)
        {
            // Scenario: NV collapse adjustment affects ReadyTimeToDisplay metric.
            // Adjusted screenTime = 4'000'000
            // readyTime = 2'100'000
            // QPC 10 MHz
            // Expected: msReadyTimeToDisplayLatency approx 0.19 ms (4'000'000 - 2'100'000 = 1'900'000 ticks)

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

            // Set up chain with prior app present to establish cpuStart
            FrameData priorApp{};
            priorApp.presentStartTime = 800'000;
            priorApp.timeInPresent = 200'000;
            chain.lastAppPresent = priorApp;

            auto results1 = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results1.size());

            // No adjust of first frame ready time = 4'000'000 - 1'100'000 = 2'900'000 ticks = 0.29 ms
            double expectedReadyTimeLatency = qpc.DeltaUnsignedMilliSeconds(1'100'000, 4'000'000);
            Assert::IsTrue(HasMetricValue(results1[0].metrics.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyTimeLatency, results1[0].metrics.msReadyTimeToDisplayLatency, 0.0001);

            auto results2 = ComputeMetricsForPresent(qpc, next1, chain);
            Assert::AreEqual(size_t(1), results2.size());

            // After NV adjustment: ready time latency = 4'000'000 - 2'100'000 = 1'900'000 ticks = 0.19 ms
            double expectedReadyTimeLatency2 = qpc.DeltaUnsignedMilliSeconds(2'100'000, 4'000'000);
            Assert::IsTrue(HasMetricValue(results2[0].metrics.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyTimeLatency2, results2[0].metrics.msReadyTimeToDisplayLatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // No prior present, so cpuStart = 0
            // msDisplayLatency = 2'000'000 - 0 = 2'000'000 ticks = 0.2 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 2'000'000);
            AssertAreEqualWithinTolerance(expected, m.msDisplayLatency, 0.0001);
        }

        TEST_METHOD(DisplayLatency_FrameWithAppPropagatedData)
        {
            // Scenario: lastAppPresent has appPropagatedPresentStartTime and appPropagatedTimeInPresent set.
            // CPU start should use these.
            // appPropagatedPresentStartTime = 800'000
            // appPropagatedTimeInPresent = 150'000
            // screenTime = 2'000'000
            // Expected cpuStart = 800'000 + 150'000 = 950'000
            // Expected msDisplayLatency approx 0.1055 ms (2'000'000 - 950'000 = 1'050'000 ticks)

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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;

            // cpuStart = 800'000 + 150'000 = 950'000
            // msDisplayLatency = 2'000'000 - 950'000 = 1'050'000 ticks = 0.105 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(950'000, 2'000'000);
            AssertAreEqualWithinTolerance(expected, m.msDisplayLatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msCPUBusy = 1'100'000 - 1'000'000 = 100'000 ticks = 10 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'100'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000 (uses appPropagated from priorApp)
            // msCPUBusy = 1'500'000 - 1'000'000 = 500'000 ticks = 50 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'500'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 0 (no prior app present)
            // msCPUBusy = 5'000'000 - 0 = 5'000'000 ticks = 500 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(0, 5'000'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // presentStartTime = 1'000'000 (same as cpuStart)
            // msCPUBusy = 1'000'000 - 1'000'000 = 0 ticks = 0 ms
            AssertAreEqualWithinTolerance(0.0, m.msCPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msCPUWait = 200'000 ticks = 20 ms
            double expected = qpc.DurationMilliSeconds(200'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUWait, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // When appPropagated is available, use it instead of regular timeInPresent
            // msCPUWait = 150'000 ticks = 15 ms
            double expected = qpc.DurationMilliSeconds(150'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUWait, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msCPUWait = 0 ticks = 0 ms
            AssertAreEqualWithinTolerance(0.0, m.msCPUWait, 0.0001);
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
         
            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());
            const auto& m = results[0].metrics;
         
            double expectedBusy = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'100'000);
            double expectedWait = qpc.DurationMilliSeconds(200'000);
            double expectedCpuTime = expectedBusy + expectedWait;
         
            AssertAreEqualWithinTolerance(expectedBusy, m.msCPUBusy, 0.0001);
            AssertAreEqualWithinTolerance(expectedWait, m.msCPUWait, 0.0001);
            AssertAreEqualWithinTolerance(expectedCpuTime, m.msCPUTime, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 800'000 + 200'000 = 1'000'000
            // msGPULatency = 1'050'000 - 1'000'000 = 50'000 ticks = 5 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'050'000);
            AssertAreEqualWithinTolerance(expected, m.msGPULatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 1'000'000
            // msGPULatency = 1'080'000 - 1'000'000 = 80'000 ticks = 8 ms
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'080'000);
            AssertAreEqualWithinTolerance(expected, m.msGPULatency, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPUBusy = 500'000 ticks = 50 ms
            double expected = qpc.DurationMilliSeconds(500'000);
            AssertAreEqualWithinTolerance(expected, m.msGPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPUBusy = 0 ticks = 0 ms
            AssertAreEqualWithinTolerance(0.0, m.msGPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Uses appPropagated: 450'000 ticks = 45 ms
            double expected = qpc.DurationMilliSeconds(450'000);
            AssertAreEqualWithinTolerance(expected, m.msGPUBusy, 0.0001);
        }

        TEST_METHOD(GPUWait_BasicCase_BusyLessThanTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 -> total = 600'000
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 500'000 ticks = 50 ms
            // msGPUWait = 600'000 - 500'000 = 100'000 ticks = 10 ms
            double expectedTotal = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'600'000);
            double expectedBusy = qpc.DurationMilliSeconds(500'000);
            double expectedWait = std::max(0.0, expectedTotal - expectedBusy);
            AssertAreEqualWithinTolerance(expectedWait, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_BusyEqualsTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 -> total = 600'000
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 600'000 ticks = 60 ms (equal to total)
            // msGPUWait = 600'000 - 600'000 = 0 ms
            AssertAreEqualWithinTolerance(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_BusyGreaterThanTotal)
        {
            // gpuStartTime = 1'000'000, readyTime = 1'600'000 -> total = 600'000
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'600'000 - 1'000'000 = 600'000
            // msGPUBusy = 700'000 ticks = 70 ms (greater than total)
            // msGPUWait should clamp to 0
            AssertAreEqualWithinTolerance(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(GPUWait_WithAppPropagatedData)
        {
            // appPropagatedGPUStartTime = 1'000'000, appPropagatedReadyTime = 1'550'000 -> total = 550'000
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Total = 1'550'000 - 1'000'000 = 550'000
            // msGPUBusy = 450'000 ticks = 45 ms
            // msGPUWait = 550'000 - 450'000 = 100'000 ticks = 10 ms
            double expectedTotal = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'550'000);
            double expectedBusy = qpc.DurationMilliSeconds(450'000);
            double expectedWait = std::max(0.0, expectedTotal - expectedBusy);
            AssertAreEqualWithinTolerance(expectedWait, m.msGPUWait, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msVideoBusy = 200'000 ticks = 20 ms
            double expected = qpc.DurationMilliSeconds(200'000);
            AssertAreEqualWithinTolerance(expected, m.msVideoBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            AssertAreEqualWithinTolerance(0.0, m.msVideoBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // Uses appPropagated: 180'000 ticks = 18 ms
            double expected = qpc.DurationMilliSeconds(180'000);
            AssertAreEqualWithinTolerance(expected, m.msVideoBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            double expectedGpuBusy = qpc.DurationMilliSeconds(500'000);
            double expectedVideoBusy = qpc.DurationMilliSeconds(200'000);

            AssertAreEqualWithinTolerance(expectedGpuBusy, m.msGPUBusy, 0.0001);
            AssertAreEqualWithinTolerance(expectedVideoBusy, m.msVideoBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // CPU metrics should be non-zero
            Assert::IsTrue(m.msCPUBusy > 0);
            // GPU metrics should be zero
            AssertAreEqualWithinTolerance(0.0, m.msGPULatency, 0.0001);
            AssertAreEqualWithinTolerance(0.0, m.msGPUBusy, 0.0001);
            AssertAreEqualWithinTolerance(0.0, m.msGPUWait, 0.0001);
            AssertAreEqualWithinTolerance(0.0, m.msVideoBusy, 0.0001);
        }

        TEST_METHOD(GeneratedFrameMetrics_NotAppFrame_CPUGPUMetricsZero)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 800'000;
            bootstrap.timeInPresent = 200'000;
            bootstrap.readyTime = 1'000'000;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData generated{};
            generated.presentStartTime = 1'100'000;
            generated.timeInPresent = 100'000;
            generated.readyTime = 1'200'000;
            generated.gpuStartTime = 1'150'000;
            generated.gpuDuration = 200'000;
            generated.finalState = PresentResult::Presented;
            generated.displayed.PushBack({ FrameType::Repeated, 1'300'000 });

            auto heldGenerated = swapChain.ProcessPresent(qpc, std::move(generated));
            Assert::AreEqual(size_t(0), heldGenerated.size());

            FrameData firstAppAnchor{};
            firstAppAnchor.presentStartTime = 1'500'000;
            firstAppAnchor.timeInPresent = 50'000;
            firstAppAnchor.readyTime = 1'600'000;
            firstAppAnchor.finalState = PresentResult::Presented;
            firstAppAnchor.appSimStartTime = 1'000'000;
            firstAppAnchor.displayed.PushBack({ FrameType::Application, 1'700'000 });

            auto publishedRows = swapChain.ProcessPresent(qpc, std::move(firstAppAnchor));
            Assert::AreEqual(size_t(1), publishedRows.size());
            Assert::AreEqual((int)FrameType::Repeated, (int)publishedRows[0].computed.metrics.frameType);

            const auto& m = publishedRows[0].computed.metrics;
            Assert::IsTrue(IsMissingFrameMetricValue(m.msCPUBusy));
            Assert::IsTrue(IsMissingFrameMetricValue(m.msCPUWait));
            AssertAreEqualWithinTolerance(0.0, m.msGPULatency, 0.0001);
            AssertAreEqualWithinTolerance(0.0, m.msGPUBusy, 0.0001);
            AssertAreEqualWithinTolerance(0.0, m.msGPUWait, 0.0001);
        }

        TEST_METHOD(NotDisplayedGeneratedFrame_PreservesFrameTypeAndDoesNotUpdateAppHistory)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 800'000;
            bootstrap.timeInPresent = 200'000;
            bootstrap.readyTime = 1'000'000;
            bootstrap.finalState = PresentResult::Presented;
            bootstrap.displayed.PushBack({ FrameType::Application, 1'100'000 });

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            Assert::IsTrue(swapChain.swapChain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(800'000), swapChain.swapChain.lastAppPresent->presentStartTime);

            FrameData generated{};
            generated.presentStartTime = 1'100'000;
            generated.timeInPresent = 100'000;
            generated.readyTime = 1'200'000;
            generated.gpuStartTime = 1'150'000;
            generated.gpuDuration = 200'000;
            generated.finalState = PresentResult::Discarded;
            generated.displayed.PushBack({ FrameType::Repeated, 1'300'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(generated));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual((int)FrameType::Repeated, (int)rows[0].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(0), rows[0].computed.metrics.screenTimeQpc);
            Assert::IsTrue(IsMissingFrameMetricValue(rows[0].computed.metrics.msCPUBusy));
            Assert::IsTrue(swapChain.swapChain.lastAppPresent.has_value());
            Assert::AreEqual(uint64_t(800'000), swapChain.swapChain.lastAppPresent->presentStartTime);
            Assert::IsTrue(swapChain.swapChain.lastPresent.has_value());
            Assert::AreEqual(uint64_t(1'100'000), swapChain.swapChain.lastPresent->presentStartTime);
        }

        TEST_METHOD(NotDisplayedPresent_MultipleFrameTypeEntries_ProducesRowPerEntry)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 800'000;
            bootstrap.timeInPresent = 200'000;
            bootstrap.readyTime = 1'000'000;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData dropped{};
            dropped.presentStartTime = 1'100'000;
            dropped.timeInPresent = 100'000;
            dropped.readyTime = 1'200'000;
            dropped.finalState = PresentResult::Discarded;
            dropped.displayed.PushBack({ FrameType::Intel_XEFG, 1'300'000 });
            dropped.displayed.PushBack({ FrameType::Application, 1'400'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(dropped));
            Assert::AreEqual(size_t(2), rows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)rows[0].computed.metrics.frameType);
            Assert::AreEqual((int)FrameType::Application, (int)rows[1].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(0), rows[0].computed.metrics.screenTimeQpc);
            Assert::AreEqual(uint64_t(0), rows[1].computed.metrics.screenTimeQpc);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // cpuStart = 900'000'000 + 100'000'000 = 1'000'000'000
            // msCPUBusy = 1'100'000'000 - 1'000'000'000 = 100'000'000 ticks = 10'000 ms (10 seconds)
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000'000, 1'100'000'000);
            AssertAreEqualWithinTolerance(expected, m.msCPUBusy, 0.0001);
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

            auto results = ComputeMetricsForPresent(qpc, frame, chain);
            Assert::AreEqual(size_t(1), results.size());

            const auto& m = results[0].metrics;
            // msGPULatency = 1 tick at 10 MHz = 0.0001 ms (very small but non-zero)
            double expected = qpc.DeltaUnsignedMilliSeconds(1'000'000, 1'000'001);
            AssertAreEqualWithinTolerance(expected, m.msGPULatency, 0.00001);
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

            auto resultsA = ComputeMetricsForPresent(qpc, frameA, chain);
            Assert::AreEqual(size_t(1), resultsA.size());
            AssertAreEqualWithinTolerance(0.0, resultsA[0].metrics.msVideoBusy, 0.0001);

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

            auto resultsB = ComputeMetricsForPresent(qpc, frameB, chain);
            Assert::AreEqual(size_t(1), resultsB.size());
            double expectedVideoBusy = qpc.DurationMilliSeconds(300'000);
            AssertAreEqualWithinTolerance(expectedVideoBusy, resultsB[0].metrics.msVideoBusy, 0.0001);
        }
    };

    TEST_CLASS(AnimationFrameGenerationDesignTests)
    {
    public:
        static std::vector<FrameMetrics> Process(
            QpcConverter& qpc,
            UnifiedSwapChain& swapChain,
            FrameData frame)
        {
            std::vector<FrameMetrics> metrics;
            auto rows = swapChain.ProcessPresent(qpc, std::move(frame));
            metrics.reserve(rows.size());
            for (const auto& row : rows) {
                metrics.push_back(row.computed.metrics);
            }
            return metrics;
        }

        TEST_METHOD(AppOnly_EmitsOriginThenIntervalOnLookahead)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }, 1000));

            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }, 1016)).size());

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }, 1032));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual((int)FrameType::Application, (int)rows[0].frameType);
            Assert::AreEqual(16.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(0.0, rows[0].msAnimationError, 0.0001);
            Assert::AreEqual(16.0, rows[0].msDisplayedTime, 0.0001);
        }

        TEST_METHOD(AppOnly_CpuStartUsesBootstrapThenIngestPreviousWhenOriginHeld)
        {
            // CpuStart uses the previous present end as the simulation start. The
            // first app anchor uses the bootstrap seed as its predecessor, then the
            // next app anchor must use ingest-time previous present history while
            // the prior rows are still held.
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }));

            auto firstIntervalRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }));
            Assert::AreEqual(size_t(1), firstIntervalRows.size());
            Assert::AreEqual(uint64_t(100), firstIntervalRows[0].screenTimeQpc);
            Assert::AreEqual(0.0, firstIntervalRows[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(firstIntervalRows[0].msAnimationError));

            auto bootstrapIntervalRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1016, 16, 1016,
                { { FrameType::Application, 132 } }));
            Assert::AreEqual(size_t(1), bootstrapIntervalRows.size());
            Assert::AreEqual(uint64_t(116), bootstrapIntervalRows[0].screenTimeQpc);
            Assert::AreEqual(998.0, bootstrapIntervalRows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(982.0, bootstrapIntervalRows[0].msAnimationError, 0.0001);

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1032, 16, 1032,
                { { FrameType::Application, 148 } }));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(132), rows[0].screenTimeQpc);
            Assert::AreEqual(1014.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(0.0, rows[0].msAnimationError, 0.0001);
        }


        TEST_METHOD(TraceStart_PreFirstAppAnchorGeneratedRows_EmitWithAnimationNotSet)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));

            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 800, 1, 800,
                { { FrameType::Intel_XEFG, 100 } })).size());

            auto gen1Rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 810, 1, 810,
                { { FrameType::Intel_XEFG, 108 } }));
            Assert::AreEqual(size_t(1), gen1Rows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)gen1Rows[0].frameType);
            Assert::AreEqual(uint64_t(100), gen1Rows[0].screenTimeQpc);
            Assert::AreEqual(8.0, gen1Rows[0].msDisplayedTime, 0.0001);
            Assert::IsFalse(HasMetricValue(gen1Rows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(gen1Rows[0].msAnimationTime));

            auto gen2Rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 820, 1, 820,
                { { FrameType::Intel_XEFG, 116 } }));
            Assert::AreEqual(size_t(1), gen2Rows.size());
            Assert::AreEqual(uint64_t(108), gen2Rows[0].screenTimeQpc);
            Assert::IsFalse(HasMetricValue(gen2Rows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(gen2Rows[0].msAnimationTime));

            auto samePresentRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 825, 1, 825,
                {
                    { FrameType::Intel_XEFG, 120 },
                    { FrameType::Intel_XEFG, 128 },
                }));
            Assert::AreEqual(size_t(2), samePresentRows.size());
            Assert::AreEqual(uint64_t(116), samePresentRows[0].screenTimeQpc);
            Assert::AreEqual(uint64_t(120), samePresentRows[1].screenTimeQpc);
            Assert::IsFalse(HasMetricValue(samePresentRows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(samePresentRows[1].msAnimationError));

            auto afterFirstAnchorRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 1, 900,
                { { FrameType::Application, 136 } }, 1000));
            Assert::AreEqual(size_t(1), afterFirstAnchorRows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)afterFirstAnchorRows[0].frameType);
            Assert::AreEqual(uint64_t(128), afterFirstAnchorRows[0].screenTimeQpc);
            Assert::AreEqual(8.0, afterFirstAnchorRows[0].msDisplayedTime, 0.0001);
            Assert::IsFalse(HasMetricValue(afterFirstAnchorRows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(afterFirstAnchorRows[0].msAnimationTime));

            auto originAnchorRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 910, 1, 910,
                { { FrameType::Intel_XEFG, 144 } }));
            Assert::AreEqual(size_t(1), originAnchorRows.size());
            Assert::AreEqual((int)FrameType::Application, (int)originAnchorRows[0].frameType);
            Assert::AreEqual(uint64_t(136), originAnchorRows[0].screenTimeQpc);
            Assert::AreEqual(8.0, originAnchorRows[0].msDisplayedTime, 0.0001);
            Assert::AreEqual(0.0, originAnchorRows[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(originAnchorRows[0].msAnimationError));
        }

        TEST_METHOD(TraceStart_FirstCpuStartAnchor_WithOnlyGeneratedHistory_PublishesMissingAnimationTime)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 790;
            bootstrap.timeInPresent = 10;
            bootstrap.readyTime = 790;
            bootstrap.finalState = PresentResult::Presented;
            bootstrap.displayed.PushBack({ FrameType::Intel_XEFG, 100 });
            (void)Process(qpc, swapChain, std::move(bootstrap));

            FrameData firstGenerated{};
            firstGenerated.presentStartTime = 800;
            firstGenerated.timeInPresent = 10;
            firstGenerated.readyTime = 800;
            firstGenerated.finalState = PresentResult::Presented;
            firstGenerated.displayed.PushBack({ FrameType::Intel_XEFG, 108 });
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, std::move(firstGenerated)).size());

            FrameData secondGenerated{};
            secondGenerated.presentStartTime = 810;
            secondGenerated.timeInPresent = 10;
            secondGenerated.readyTime = 810;
            secondGenerated.finalState = PresentResult::Presented;
            secondGenerated.displayed.PushBack({ FrameType::Intel_XEFG, 116 });
            auto preAnchorRows = Process(qpc, swapChain, std::move(secondGenerated));
            Assert::AreEqual(size_t(1), preAnchorRows.size());
            Assert::IsFalse(HasMetricValue(preAnchorRows[0].msAnimationTime));

            FrameData firstAppAnchor{};
            firstAppAnchor.presentStartTime = 900;
            firstAppAnchor.timeInPresent = 10;
            firstAppAnchor.readyTime = 900;
            firstAppAnchor.finalState = PresentResult::Presented;
            firstAppAnchor.displayed.PushBack({ FrameType::Application, 124 });
            auto finalPreAnchorRows = Process(qpc, swapChain, std::move(firstAppAnchor));
            Assert::AreEqual(size_t(1), finalPreAnchorRows.size());
            Assert::IsFalse(HasMetricValue(finalPreAnchorRows[0].msAnimationTime));

            FrameData lookahead{};
            lookahead.presentStartTime = 910;
            lookahead.timeInPresent = 10;
            lookahead.readyTime = 910;
            lookahead.finalState = PresentResult::Presented;
            lookahead.displayed.PushBack({ FrameType::Intel_XEFG, 132 });
            auto originRows = Process(qpc, swapChain, std::move(lookahead));
            Assert::AreEqual(size_t(1), originRows.size());
            Assert::AreEqual((int)FrameType::Application, (int)originRows[0].frameType);
            Assert::AreEqual(uint64_t(124), originRows[0].screenTimeQpc);

            // The origin app anchor has no provider or PC-latency timestamp, and
            // the only prior observed frames are generated. Do not derive a CPU
            // simulation start from generated-frame history.
            Assert::IsFalse(HasMetricValue(originRows[0].msAnimationTime));
            Assert::IsFalse(HasMetricValue(originRows[0].msAnimationError));
        }

        TEST_METHOD(FirstAppAnchor_PublishesTimelineOriginWithAnimationTimeFromSession)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));

            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 800, 1, 800,
                { { FrameType::Application, 100 } }, 1000)).size());

            auto heldThenReleased = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 810, 1, 810,
                { { FrameType::Intel_XEFG, 108 } }));
            Assert::AreEqual(size_t(1), heldThenReleased.size());
            Assert::AreEqual((int)FrameType::Application, (int)heldThenReleased[0].frameType);
            Assert::AreEqual(uint64_t(100), heldThenReleased[0].screenTimeQpc);
            Assert::AreEqual(8.0, heldThenReleased[0].msDisplayedTime, 0.0001);
            Assert::AreEqual(0.0, heldThenReleased[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(heldThenReleased[0].msAnimationError));

            UnifiedSwapChain swapChain2{};
            (void)Process(qpc, swapChain2, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));

            auto samePresentLookahead = Process(qpc, swapChain2, MakeFrame(PresentResult::Presented, 800, 1, 800,
                {
                    { FrameType::Application, 100 },
                    { FrameType::Intel_XEFG, 108 },
                },
                1000));
            Assert::AreEqual(size_t(1), samePresentLookahead.size());
            Assert::AreEqual((int)FrameType::Application, (int)samePresentLookahead[0].frameType);
            Assert::AreEqual(8.0, samePresentLookahead[0].msDisplayedTime, 0.0001);
            Assert::AreEqual(0.0, samePresentLookahead[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(samePresentLookahead[0].msAnimationError));
        }

        TEST_METHOD(PublishPolicy_FirstPresentOnSwapChainProducesNoOutput)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            const auto firstPresentFrame = MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 }, { FrameType::Intel_XEFG, 108 } },
                1000);

            UnifiedSwapChain ingestAndApplyOnly{};
            auto readyOnFirstPresent = ingestAndApplyOnly.EnqueueReadyDisplayRows(qpc, firstPresentFrame);
            Assert::IsTrue(readyOnFirstPresent.size() > 0);

            auto publishedOnFirstPresent = swapChain.ProcessPresent(qpc, firstPresentFrame);
            Assert::AreEqual(size_t(0), publishedOnFirstPresent.size());

            auto publishedOnSecondPresent = swapChain.ProcessPresent(qpc, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                { { FrameType::Application, 116 }, { FrameType::Intel_XEFG, 124 } },
                1016));
            Assert::AreEqual(size_t(1), publishedOnSecondPresent.size());
            Assert::AreEqual((int)FrameType::Application, (int)publishedOnSecondPresent[0].computed.metrics.frameType);
        }

        TEST_METHOD(GenAndApp_OnSecondPresent_GenIncludedInFirstClosedInterval)
        {
            // Seed present does not ingest; app+gen on first process emits only app row (gen held);
            // second process emits gen row with animation time from first present, and app row with animation 
            // time from second present. This verifies that the gen frame is included in the first closed 
            // interval, even though it was not ingested at the time of the first present.
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));

            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 }, { FrameType::Intel_XEFG, 108 } },
                1000)).size());

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                { { FrameType::Application, 116 }, { FrameType::Intel_XEFG, 124 } },
                1016));

            Assert::AreEqual(size_t(2), rows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG,     (int)rows[0].frameType);
            Assert::AreEqual((int)FrameType::Application,    (int)rows[1].frameType);
            Assert::AreEqual(uint64_t(108),                  rows[0].screenTimeQpc);
            Assert::AreEqual(uint64_t(116),                  rows[1].screenTimeQpc);
            Assert::AreEqual(8.0,  rows[0].msAnimationTime,  0.0001);
            Assert::AreEqual(16.0, rows[1].msAnimationTime,  0.0001);
            Assert::AreEqual(0.0,  rows[0].msAnimationError, 0.0001);
            Assert::AreEqual(0.0,  rows[1].msAnimationError, 0.0001);
            Assert::AreEqual(8.0,  rows[1].msDisplayedTime,  0.0001);
        }

        TEST_METHOD(PresentFrameTypeInfo_GeneratedOnlyPresentsWaitForNextAppAndLookahead)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }, 1000));

            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                { { FrameType::Intel_XEFG, 106 } })).size());
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1001, 1, 1001,
                { { FrameType::Intel_XEFG, 112 } })).size());
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1002, 1, 1002,
                { { FrameType::Intel_XEFG, 118 } })).size());

            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 1, 1100,
                { { FrameType::Application, 124 } }, 1024)).size());

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1101, 1, 1101,
                { { FrameType::Intel_XEFG, 132 } }));

            Assert::AreEqual(size_t(4), rows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)rows[0].frameType);
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)rows[1].frameType);
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)rows[2].frameType);
            Assert::AreEqual((int)FrameType::Application, (int)rows[3].frameType);
            Assert::AreEqual(6.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(12.0, rows[1].msAnimationTime, 0.0001);
            Assert::AreEqual(18.0, rows[2].msAnimationTime, 0.0001);
            Assert::AreEqual(24.0, rows[3].msAnimationTime, 0.0001);
            Assert::AreEqual(8.0, rows[3].msDisplayedTime, 0.0001);
        }

        TEST_METHOD(FlipFrameTypeInfo_AppInMiddle_OnlyClosedIntervalRowsEmit)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                {
                    { FrameType::Intel_XEFG, 90 },
                    { FrameType::Application, 100 },
                    { FrameType::Intel_XEFG, 108 },
                },
                1000));

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                {
                    { FrameType::Intel_XEFG, 116 },
                    { FrameType::Application, 124 },
                    { FrameType::Intel_XEFG, 132 },
                },
                1024));

            Assert::AreEqual(size_t(3), rows.size());
            Assert::AreEqual(uint64_t(108), rows[0].screenTimeQpc);
            Assert::AreEqual(uint64_t(116), rows[1].screenTimeQpc);
            Assert::AreEqual(uint64_t(124), rows[2].screenTimeQpc);
            Assert::AreEqual(8.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(16.0, rows[1].msAnimationTime, 0.0001);
            Assert::AreEqual(24.0, rows[2].msAnimationTime, 0.0001);
            Assert::AreEqual(8.0, rows[2].msDisplayedTime, 0.0001);
        }

        TEST_METHOD(FlipFrameTypeInfo_RepeatedAppInMiddle_GeneratedRowsIncludedInInterval)
        {
            // Regression: SanitizeDisplayedRepeatedPresents was collapsing
            // [Repeated, Application, Repeated] -> [Application] because the loop
            // removed rep+app then app+rep in two consecutive passes. Sequences with
            // two or more Repeated entries are AMD AFMF frame-generation shapes and
            // must not be sanitized.
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                {
                    { FrameType::Repeated,    90  },
                    { FrameType::Application, 100 },
                    { FrameType::Repeated,    108 },
                },
                1000));

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                {
                    { FrameType::Repeated,    116 },
                    { FrameType::Application, 124 },
                    { FrameType::Repeated,    132 },
                },
                1024));

            Assert::AreEqual(size_t(3), rows.size());
            Assert::AreEqual((int)FrameType::Repeated,     (int)rows[0].frameType);
            Assert::AreEqual((int)FrameType::Repeated,     (int)rows[1].frameType);
            Assert::AreEqual((int)FrameType::Application,  (int)rows[2].frameType);
            Assert::AreEqual(uint64_t(108), rows[0].screenTimeQpc);
            Assert::AreEqual(uint64_t(116), rows[1].screenTimeQpc);
            Assert::AreEqual(uint64_t(124), rows[2].screenTimeQpc);
            Assert::AreEqual(8.0,  rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(16.0, rows[1].msAnimationTime, 0.0001);
            Assert::AreEqual(24.0, rows[2].msAnimationTime, 0.0001);
            Assert::AreEqual(8.0,  rows[2].msDisplayedTime, 0.0001);
        }

        TEST_METHOD(CpuStartFallback_UsesPreviousPresentEndWhenProviderAndPclAreUnavailable)
        {
            // With no app-provider or PC-latency sim start, CpuStart falls back to
            // previous present end. The bootstrap seed is a valid first predecessor.
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }));

            auto bootstrapIntervalRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }));
            Assert::AreEqual(size_t(1), bootstrapIntervalRows.size());
            Assert::AreEqual(uint64_t(100), bootstrapIntervalRows[0].screenTimeQpc);
            Assert::AreEqual(0.0, bootstrapIntervalRows[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(bootstrapIntervalRows[0].msAnimationError));

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(116), rows[0].screenTimeQpc);
            Assert::AreEqual(998.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(982.0, rows[0].msAnimationError, 0.0001);
            Assert::AreEqual(16.0, rows[0].msDisplayedTime, 0.0001);
        }

        TEST_METHOD(SourceTransition_CpuStartToAppProvider_StartsNewTimeline)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }));

            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }, 2000)).size());

            auto transitionRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }, 2016));

            Assert::AreEqual(size_t(1), transitionRows.size());
            Assert::IsFalse(HasMetricValue(transitionRows[0].msAnimationError));
            Assert::AreEqual(0.0, transitionRows[0].msAnimationTime, 0.0001);

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1200, 16, 1200,
                { { FrameType::Application, 148 } }, 2032));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(16.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(0.0, rows[0].msAnimationError, 0.0001);
        }

        TEST_METHOD(SourceTransition_PendingGeneratedRowsAndNewTimelineOriginPublish)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } })).size());

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 910, 1, 910,
                { { FrameType::Intel_XEFG, 108 } }));

            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1010, 1, 1010,
                { { FrameType::Intel_XEFG, 116 } })).size());

            auto transitionRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1020, 1, 1020,
                { { FrameType::Application, 132 } }, 2000));

            Assert::AreEqual(size_t(2), transitionRows.size());
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)transitionRows[0].frameType);
            Assert::AreEqual((int)FrameType::Intel_XEFG, (int)transitionRows[1].frameType);
            Assert::AreEqual(uint64_t(108), transitionRows[0].screenTimeQpc);
            Assert::AreEqual(uint64_t(116), transitionRows[1].screenTimeQpc);
            Assert::AreEqual(8.0, transitionRows[0].msDisplayedTime, 0.0001);
            Assert::AreEqual(16.0, transitionRows[1].msDisplayedTime, 0.0001);
            Assert::IsFalse(HasMetricValue(transitionRows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(transitionRows[0].msAnimationTime));
            Assert::IsFalse(HasMetricValue(transitionRows[1].msAnimationError));
            Assert::IsFalse(HasMetricValue(transitionRows[1].msAnimationTime));

            auto newOriginRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1030, 1, 1030,
                { { FrameType::Application, 148 } }, 2016));
            Assert::AreEqual(size_t(1), newOriginRows.size());
            Assert::AreEqual((int)FrameType::Application, (int)newOriginRows[0].frameType);
            Assert::AreEqual(uint64_t(132), newOriginRows[0].screenTimeQpc);
            Assert::AreEqual(16.0, newOriginRows[0].msDisplayedTime, 0.0001);
            Assert::AreEqual(0.0, newOriginRows[0].msAnimationTime, 0.0001);
            Assert::IsFalse(HasMetricValue(newOriginRows[0].msAnimationError));
        }

        TEST_METHOD(SourceTransition_PclToAppProvider_StartsNewTimeline)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }, 0, 1000));

            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }, 2000, 1016)).size());

            auto transitionRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }, 2016));

            Assert::AreEqual(size_t(1), transitionRows.size());
            Assert::IsFalse(HasMetricValue(transitionRows[0].msAnimationError));
            Assert::AreEqual(0.0, transitionRows[0].msAnimationTime, 0.0001);
        }

        TEST_METHOD(NonMonotonicSimStart_PreservesAccumulatedAnimationTime)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 1, 900,
                { { FrameType::Application, 100 } }, 1000));
            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 1, 1000,
                { { FrameType::Application, 116 } }, 1016)).size());

            auto firstRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1010, 1, 1010,
                { { FrameType::Application, 132 } }, 1008));
            Assert::AreEqual(size_t(1), firstRows.size());
            Assert::AreEqual(16.0, firstRows[0].msAnimationTime, 0.0001);

            auto invalidRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1020, 1, 1020,
                { { FrameType::Application, 148 } }, 1008));
            Assert::AreEqual(size_t(1), invalidRows.size());
            Assert::IsFalse(HasMetricValue(invalidRows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(invalidRows[0].msAnimationTime));

            auto secondInvalidRows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1030, 1, 1030,
                { { FrameType::Application, 164 } }, 1024));
            Assert::AreEqual(size_t(1), secondInvalidRows.size());
            Assert::IsFalse(HasMetricValue(secondInvalidRows[0].msAnimationError));
            Assert::IsFalse(HasMetricValue(secondInvalidRows[0].msAnimationTime));

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1040, 1, 1040,
                { { FrameType::Application, 180 } }, 1040));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(32.0, rows[0].msAnimationTime, 0.0001);
            Assert::AreEqual(0.0, rows[0].msAnimationError, 0.0001);
        }

        TEST_METHOD(DroppedFrame_BeforeFirstAnchor_DoesNotCorruptPreviousDisplayedScreenTime)
        {
            // Regression: UpdateAfterReadyDisplayRow zeroed lastDisplayedScreenTime for
            // not-displayed rows. A displayed app frame processed as the swap chain seed
            // present sets lastDisplayedScreenTime via the display queue; a subsequent
            // dropped frame (before the first animation anchor) must not zero it.
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }, 1000));

            // Dropped present before the first animation anchor. No anchor exists yet,
            // so this is emitted immediately and UpdateAfterReadyDisplayRow is called.
            // lastDisplayedScreenTime must remain 100 after this.
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Discarded, 950, 10, 950, {}));

            // The first queued app anchor. Its previousDisplayedScreenTime must be
            // derived from the seed (100), not from the zeroed-out state (0).
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }, 1016)).size());

            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }, 1032));

            Assert::AreEqual(size_t(1), rows.size());
            // simStep = (1032 - 1016) / 1 = 16ms; displayStep = delta(116, 132) = 16ms
            Assert::AreEqual(0.0, rows[0].msAnimationError, 0.0001);
            Assert::AreEqual(16.0, rows[0].msAnimationTime, 0.0001);
        }

        TEST_METHOD(DroppedFrames_DoNotEnterAnimationIntervals)
        {
            QpcConverter qpc(1000, 0);
            UnifiedSwapChain swapChain{};

            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1, 1, 1, {}));
            (void)Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 900, 100, 900,
                { { FrameType::Application, 100 } }, 1000));
            Assert::AreEqual(size_t(0), Process(qpc, swapChain, MakeFrame(PresentResult::Discarded, 950, 10, 950,
                {}, 1008)).size());
            // Expect 1 row from the second application frame, the discarded frame will be be waiting in the
            // blocked rows and not emitted until the next displayed app frame below.
            Assert::AreEqual(size_t(1), Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1000, 16, 1000,
                { { FrameType::Application, 116 } }, 1016)).size());

            // Both discarded and application frame now will be emitted.
            auto rows = Process(qpc, swapChain, MakeFrame(PresentResult::Presented, 1100, 16, 1100,
                { { FrameType::Application, 132 } }, 1032));

            Assert::AreEqual(size_t(2), rows.size());
            Assert::IsFalse(HasMetricValue(rows[0].msAnimationTime));
            Assert::AreEqual(16.0, rows[1].msAnimationTime, 0.0001);
            Assert::AreEqual(0.0, rows[1].msAnimationError, 0.0001);
        }

        TEST_METHOD(NvCollapsedAdjustment_AdjustsNextReadyDisplayRow)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            (void)swapChain.EnqueueReadyDisplayRows(qpc, MakeFrame(PresentResult::Presented, 1'000'000, 10, 1'000'010, {}));
            (void)swapChain.EnqueueReadyDisplayRows(qpc, MakeFrame(PresentResult::Presented, 4'000'000, 50'000, 4'100'000,
                { { FrameType::Application, 5'500'000 } },
                1'000'000,
                0,
                200'000));

            auto releasedOriginRows = swapChain.EnqueueReadyDisplayRows(qpc, MakeFrame(PresentResult::Presented, 5'000'000, 40'000, 5'100'000,
                { { FrameType::Application, 5'000'000 } },
                1'000'100,
                0,
                100'000));
            Assert::AreEqual(size_t(1), releasedOriginRows.size());
            Assert::AreEqual(uint64_t(5'500'000), releasedOriginRows[0].screenTime);

            auto rows = swapChain.EnqueueReadyDisplayRows(qpc, MakeFrame(PresentResult::Presented, 6'000'000, 40'000, 6'100'000,
                { { FrameType::Application, 6'000'000 } },
                1'000'200));

            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(5'500'000), rows[0].screenTime);
            Assert::AreEqual(uint64_t(600'000), rows[0].present.flipDelay);
            Assert::AreEqual(uint64_t(5'500'000), rows[0].present.displayed[0].second);
        }
    };
    // ============================================================================
    // SECTION: Input Latency Tests
    // ============================================================================

    TEST_CLASS(InputLatencyTests)
    {
    public:
        // ========================================================================
        // V1: ComputeMetricsForPresent (one row per call)
        // ========================================================================

        TEST_METHOD(InputLatency_ClickToPhoton_DisplayedFrame_UsesOwnClickTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 100'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.appSimStartTime = 450'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);

            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(HasMetricValue(p1_results[0].metrics.msClickToPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p1_results[0].metrics.msClickToPhotonLatency, 0.0001);
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedMouseClickTime);
        }

        TEST_METHOD(InputLatency_ClickToPhoton_DisplayedFrame_UsesOwnClickTime_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p1{};
            p1.presentStartTime = 500'000;
            p1.timeInPresent = 100'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.appSimStartTime = 450'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p1)).size());

            FrameData p2{};
            p2.presentStartTime = 1'050'000;
            p2.timeInPresent = 50'000;
            p2.mouseClickTime = 0;
            p2.inputTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(1'000'000), rows[0].computed.metrics.screenTimeQpc);
            Assert::IsTrue(HasMetricValue(rows[0].computed.metrics.msClickToPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, rows[0].computed.metrics.msClickToPhotonLatency, 0.0001);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedMouseClickTime);
        }

        TEST_METHOD(InputLatency_ClickToPhoton_DroppedFrame_CarriesClickToNextDisplayed)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p1{};
            p1.presentStartTime = 300'000;
            p1.timeInPresent = 50'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.finalState = PresentResult::Discarded;

            FrameData p2{};
            p2.presentStartTime = 900'000;
            p2.timeInPresent = 100'000;
            p2.mouseClickTime = 0;
            p2.inputTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'000'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p1_results[0].metrics.msClickToPhotonLatency));
            Assert::AreEqual(uint64_t(400'000), state.lastReceivedNotDisplayedMouseClickTime);

            auto p2_results = ComputeMetricsForPresent(qpc, p2, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsTrue(HasMetricValue(p2_results[0].metrics.msClickToPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p2_results[0].metrics.msClickToPhotonLatency, 0.0001);
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedMouseClickTime);
        }

        TEST_METHOD(InputLatency_ClickToPhoton_DroppedFrame_CarriesClickToNextDisplayed_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p1{};
            p1.presentStartTime = 300'000;
            p1.timeInPresent = 50'000;
            p1.mouseClickTime = 400'000;
            p1.inputTime = 0;
            p1.finalState = PresentResult::Discarded;

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p1_rows[0].computed.metrics.msClickToPhotonLatency));
            Assert::AreEqual(uint64_t(400'000), swapChain.swapChain.lastReceivedNotDisplayedMouseClickTime);

            FrameData p2{};
            p2.presentStartTime = 900'000;
            p2.timeInPresent = 100'000;
            p2.mouseClickTime = 0;
            p2.inputTime = 0;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'000'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p2)).size());

            FrameData p3{};
            p3.presentStartTime = 1'050'000;
            p3.timeInPresent = 50'000;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'100'000 });

            auto p3_rows = swapChain.ProcessPresent(qpc, std::move(p3));
            Assert::AreEqual(size_t(1), p3_rows.size());
            Assert::AreEqual(uint64_t(1'000'000), p3_rows[0].computed.metrics.screenTimeQpc);
            Assert::IsTrue(HasMetricValue(p3_rows[0].computed.metrics.msClickToPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(400'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p3_rows[0].computed.metrics.msClickToPhotonLatency, 0.0001);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedMouseClickTime);
        }

        TEST_METHOD(InputLatency_AllInputPhoton_MultipleDroppedFrames_LastInputWins)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p1{};
            p1.presentStartTime = 200'000;
            p1.timeInPresent = 50'000;
            p1.inputTime = 300'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Discarded;

            FrameData p2{};
            p2.presentStartTime = 400'000;
            p2.timeInPresent = 50'000;
            p2.inputTime = 450'000;
            p2.mouseClickTime = 0;
            p2.finalState = PresentResult::Discarded;

            FrameData p3{};
            p3.presentStartTime = 900'000;
            p3.timeInPresent = 100'000;
            p3.inputTime = 0;
            p3.mouseClickTime = 0;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'000'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p1_results[0].metrics.msAllInputPhotonLatency));
            Assert::AreEqual(uint64_t(300'000), state.lastReceivedNotDisplayedAllInputTime);

            auto p2_results = ComputeMetricsForPresent(qpc, p2, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p2_results[0].metrics.msAllInputPhotonLatency));
            Assert::AreEqual(uint64_t(450'000), state.lastReceivedNotDisplayedAllInputTime);

            auto p3_results = ComputeMetricsForPresent(qpc, p3, state);
            Assert::AreEqual(size_t(1), p3_results.size());
            Assert::IsTrue(HasMetricValue(p3_results[0].metrics.msAllInputPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(450'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p3_results[0].metrics.msAllInputPhotonLatency, 0.0001);
        }

        TEST_METHOD(InputLatency_AllInputPhoton_MultipleDroppedFrames_LastInputWins_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p1{};
            p1.presentStartTime = 200'000;
            p1.timeInPresent = 50'000;
            p1.inputTime = 300'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Discarded;

            FrameData p2{};
            p2.presentStartTime = 400'000;
            p2.timeInPresent = 50'000;
            p2.inputTime = 450'000;
            p2.mouseClickTime = 0;
            p2.finalState = PresentResult::Discarded;

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::AreEqual(uint64_t(300'000), swapChain.swapChain.lastReceivedNotDisplayedAllInputTime);

            auto p2_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p2_rows.size());
            Assert::AreEqual(uint64_t(450'000), swapChain.swapChain.lastReceivedNotDisplayedAllInputTime);

            FrameData p3{};
            p3.presentStartTime = 900'000;
            p3.timeInPresent = 100'000;
            p3.inputTime = 0;
            p3.mouseClickTime = 0;
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 1'000'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p3)).size());

            FrameData p4{};
            p4.presentStartTime = 1'050'000;
            p4.timeInPresent = 50'000;
            p4.finalState = PresentResult::Presented;
            p4.displayed.PushBack({ FrameType::Application, 1'100'000 });

            auto p4_rows = swapChain.ProcessPresent(qpc, std::move(p4));
            Assert::AreEqual(size_t(1), p4_rows.size());
            Assert::AreEqual(uint64_t(1'000'000), p4_rows[0].computed.metrics.screenTimeQpc);
            Assert::IsTrue(HasMetricValue(p4_rows[0].computed.metrics.msAllInputPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(450'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p4_rows[0].computed.metrics.msAllInputPhotonLatency, 0.0001);
        }

        TEST_METHOD(InputLatency_AllInputPhoton_DisplayedFrame_WithOwnInput_OverridesPending)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.presentStartTime = 200'000;
            p0.timeInPresent = 50'000;
            p0.inputTime = 300'000;
            p0.mouseClickTime = 0;
            p0.finalState = PresentResult::Discarded;

            FrameData p1{};
            p1.presentStartTime = 900'000;
            p1.timeInPresent = 100'000;
            p1.inputTime = 500'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::AreEqual(uint64_t(300'000), state.lastReceivedNotDisplayedAllInputTime);

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(HasMetricValue(p1_results[0].metrics.msAllInputPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(500'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p1_results[0].metrics.msAllInputPhotonLatency, 0.0001);
        }

        TEST_METHOD(InputLatency_AllInputPhoton_DisplayedFrame_WithOwnInput_OverridesPending_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.presentStartTime = 200'000;
            p0.timeInPresent = 50'000;
            p0.inputTime = 300'000;
            p0.mouseClickTime = 0;
            p0.finalState = PresentResult::Discarded;

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::AreEqual(uint64_t(300'000), swapChain.swapChain.lastReceivedNotDisplayedAllInputTime);

            FrameData p1{};
            p1.presentStartTime = 900'000;
            p1.timeInPresent = 100'000;
            p1.inputTime = 500'000;
            p1.mouseClickTime = 0;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 1'000'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p1)).size());

            FrameData p2{};
            p2.presentStartTime = 1'050'000;
            p2.timeInPresent = 50'000;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 1'100'000 });

            auto p2_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p2_rows.size());
            Assert::AreEqual(uint64_t(1'000'000), p2_rows[0].computed.metrics.screenTimeQpc);
            Assert::IsTrue(HasMetricValue(p2_rows[0].computed.metrics.msAllInputPhotonLatency));

            double expected = qpc.DeltaUnsignedMilliSeconds(500'000, 1'000'000);
            AssertAreEqualWithinTolerance(expected, p2_rows[0].computed.metrics.msAllInputPhotonLatency, 0.0001);
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

            (void)ComputeMetricsForPresent(qpc, p1, state);
            auto p2Metrics = ComputeMetricsForPresent(qpc, p2, state);

            // Verify state after P1
            Assert::IsTrue(state.animationErrorSource == AnimationErrorSource::AppProvider,
                L"Animation source should switch to AppProvider after P1");
            Assert::AreEqual(uint64_t(475'000), state.firstAppSimStartTime,
                L"firstAppSimStartTime should be set to P1's appSimStartTime");

            (void)ComputeMetricsForPresent(qpc, p3, state);

            // Assertions for P2
            Assert::AreEqual(size_t(1), p2Metrics.size());

            // Verify msInstrumentedInputTime is present
            Assert::IsTrue(HasMetricValue(p2Metrics[0].metrics.msInstrumentedInputTime),
                L"P2 should have msInstrumentedInputTime");

            // Calculate expected: app input -> p2 screen
            double expectedInstr = qpc.DeltaUnsignedMilliSeconds(500'000, 1'100'000);
            AssertAreEqualWithinTolerance(expectedInstr, p2Metrics[0].metrics.msInstrumentedInputTime, 0.0001,
                L"msInstrumentedInputTime should be P2 app input time to P2 screen time");
        }
    };
    TEST_CLASS(PcLatencyTests)
    {
    public:
        // V1: ComputeMetricsForPresent (nullptr next). V2: *_ProcessPresent duplicates.

        TEST_METHOD(PcLatency_PendingSequence_DroppedDroppedDisplayed_P0P1P2P3)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_metrics_list = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_metrics_list.size());
            Assert::IsFalse(HasMetricValue(p0_metrics_list[0].metrics.msPcLatency));
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > 0.0);
            Assert::AreEqual(uint64_t(20'000), state.lastReceivedNotDisplayedPclSimStart);
            const double accumAfterP0 = state.accumulatedInput2FrameStartTime;

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;

            auto p1_metrics_list = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_metrics_list.size());
            Assert::IsFalse(HasMetricValue(p1_metrics_list[0].metrics.msPcLatency));
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > accumAfterP0);
            Assert::AreEqual(uint64_t(30'000), state.lastReceivedNotDisplayedPclSimStart);
            const double accumAfterP1 = state.accumulatedInput2FrameStartTime;

            FrameData p2{};
            p2.pclInputPingTime = 0;
            p2.pclSimStartTime = 40'000;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p2_results = ComputeMetricsForPresent(qpc, p2, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            const auto& p2_metrics = p2_results[0].metrics;

            Assert::IsTrue(accumAfterP1 > 0.0);
            Assert::IsTrue(HasMetricValue(p2_metrics.msPcLatency));
            Assert::IsTrue(p2_metrics.msPcLatency > 0.0);
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 1e-9);
            Assert::AreEqual(uint64_t{ 0 }, state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_PendingSequence_DroppedDroppedDisplayed_P0P1P2P3_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::IsFalse(HasMetricValue(p0_rows[0].computed.metrics.msPcLatency));
            Assert::IsTrue(swapChain.swapChain.accumulatedInput2FrameStartTime > 0.0);
            const double accumAfterP0 = swapChain.swapChain.accumulatedInput2FrameStartTime;

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::IsTrue(swapChain.swapChain.accumulatedInput2FrameStartTime > accumAfterP0);
            const double accumAfterP1 = swapChain.swapChain.accumulatedInput2FrameStartTime;

            FrameData p2{};
            p2.pclInputPingTime = 0;
            p2.pclSimStartTime = 40'000;
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 50'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p2)).size());
            AssertAreEqualWithinTolerance(accumAfterP1, swapChain.swapChain.accumulatedInput2FrameStartTime, 1e-9);

            FrameData p3{};
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p3_rows = swapChain.ProcessPresent(qpc, std::move(p3));
            Assert::AreEqual(size_t(1), p3_rows.size());
            Assert::AreEqual(uint64_t(50'000), p3_rows[0].computed.metrics.screenTimeQpc);

            Assert::IsTrue(HasMetricValue(p3_rows[0].computed.metrics.msPcLatency));
            Assert::IsTrue(p3_rows[0].computed.metrics.msPcLatency > 0.0);
            AssertAreEqualWithinTolerance(0.0, swapChain.swapChain.accumulatedInput2FrameStartTime, 1e-9);
            Assert::AreEqual(uint64_t{ 0 }, swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_NoPclData_AllFrames_NoLatency)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsFalse(HasMetricValue(p0_results[0].metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsFalse(HasMetricValue(p1_results[0].metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 120'000 });

            auto p2_results = ComputeMetricsForPresent(qpc, p2, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsFalse(HasMetricValue(p2_results[0].metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);
        }

        TEST_METHOD(PcLatency_NoPclData_AllFrames_NoLatency_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.finalState = PresentResult::Discarded;
            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::IsFalse(HasMetricValue(p0_rows[0].computed.metrics.msPcLatency));

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 100'000 });
            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p1)).size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 120'000 });
            auto p2_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p2_rows.size());
            Assert::IsFalse(HasMetricValue(p2_rows[0].computed.metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);

            FrameData p3{};
            p3.finalState = PresentResult::Presented;
            p3.displayed.PushBack({ FrameType::Application, 140'000 });
            auto p3_rows = swapChain.ProcessPresent(qpc, std::move(p3));
            Assert::AreEqual(size_t(1), p3_rows.size());
            Assert::IsFalse(HasMetricValue(p3_rows[0].computed.metrics.msPcLatency));
        }

        TEST_METHOD(PcLatency_SingleDisplayed_DirectSample_FirstEma)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& p0_metrics = p0_results[0].metrics;
            Assert::IsTrue(HasMetricValue(p0_metrics.msPcLatency));
            Assert::IsTrue(p0_metrics.msPcLatency > 0.0);
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);

            double deltaPingSim = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            double expectedEma = pmon::util::CalculateEma(0.0, deltaPingSim, 0.1);
            Assert::AreEqual(expectedEma, state.Input2FrameStartTimeEma, 0.0001);

            double expectedLatency = expectedEma + qpc.DeltaSignedMilliSeconds(20'000, 50'000);
            AssertAreEqualWithinTolerance(expectedLatency, p0_metrics.msPcLatency, 0.0001);
        }

        TEST_METHOD(PcLatency_SingleDisplayed_DirectSample_FirstEma_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 60'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(50'000), rows[0].computed.metrics.screenTimeQpc);

            const auto& p0_metrics = rows[0].computed.metrics;
            Assert::IsTrue(HasMetricValue(p0_metrics.msPcLatency));
            Assert::IsTrue(p0_metrics.msPcLatency > 0.0);

            double deltaPingSim = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            double expectedEma = pmon::util::CalculateEma(0.0, deltaPingSim, 0.1);
            Assert::AreEqual(expectedEma, swapChain.swapChain.Input2FrameStartTimeEma, 0.0001);

            double expectedLatency = expectedEma + qpc.DeltaSignedMilliSeconds(20'000, 50'000);
            AssertAreEqualWithinTolerance(expectedLatency, p0_metrics.msPcLatency, 0.0001);
        }

        TEST_METHOD(PcLatency_TwoDisplayed_DirectSamples_UpdateEma)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsTrue(HasMetricValue(p0_results[0].metrics.msPcLatency));
            double emaAfterP0 = state.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP0 > 0.0);

            FrameData p1{};
            p1.pclInputPingTime = 30'000;
            p1.pclSimStartTime = 40'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(HasMetricValue(p1_results[0].metrics.msPcLatency));
            double emaAfterP1 = state.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP1 > 0.0);
            Assert::IsTrue(emaAfterP1 != emaAfterP0);
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);
        }

        TEST_METHOD(PcLatency_TwoDisplayed_DirectSamples_UpdateEma_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.pclInputPingTime = 30'000;
            p1.pclSimStartTime = 40'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::IsTrue(HasMetricValue(p0_rows[0].computed.metrics.msPcLatency));
            double emaAfterP0 = swapChain.swapChain.Input2FrameStartTimeEma;

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::IsTrue(HasMetricValue(p1_rows[0].computed.metrics.msPcLatency));
            double emaAfterP1 = swapChain.swapChain.Input2FrameStartTimeEma;
            Assert::IsTrue(emaAfterP1 != emaAfterP0);
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_InitializesAccum)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsFalse(HasMetricValue(p0_results[0].metrics.msPcLatency));

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            Assert::AreEqual(expectedAccum, state.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(20'000), state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_InitializesAccum_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto rows = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::IsFalse(HasMetricValue(rows[0].computed.metrics.msPcLatency));

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);
            Assert::AreEqual(expectedAccum, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(20'000), swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_DroppedChain_SimOnly_ExtendsAccum)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            double accumAfterP0 = state.accumulatedInput2FrameStartTime;

            FrameData p1{};
            p1.pclInputPingTime = 0;
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsFalse(HasMetricValue(p1_results[0].metrics.msPcLatency));
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > accumAfterP0);
            Assert::AreEqual(uint64_t(30'000), state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_DroppedChain_SimOnly_ExtendsAccum_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;

            (void)swapChain.ProcessPresent(qpc, std::move(p0));
            const double accumAfterP0 = swapChain.swapChain.accumulatedInput2FrameStartTime;

            FrameData p1{};
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;

            (void)swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::IsTrue(swapChain.swapChain.accumulatedInput2FrameStartTime > accumAfterP0);
            Assert::AreEqual(uint64_t(30'000), swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_Dropped_SimOnly_NoAccum_NoEffect)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 0;
            p0.pclSimStartTime = 25'000;
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsFalse(HasMetricValue(p0_results[0].metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(25'000), state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_Dropped_SimOnly_NoAccum_NoEffect_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclSimStartTime = 25'000;
            p0.finalState = PresentResult::Discarded;

            (void)swapChain.ProcessPresent(qpc, std::move(p0));
            AssertAreEqualWithinTolerance(0.0, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(25'000), swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_Displayed_SimOnly_NoAccum_UsesExistingEma)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsTrue(HasMetricValue(p0_results[0].metrics.msPcLatency));
            Assert::IsTrue(state.Input2FrameStartTimeEma > 0.0);

            FrameData p1{};
            p1.pclSimStartTime = 35'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(HasMetricValue(p1_results[0].metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);
            Assert::IsTrue(state.Input2FrameStartTimeEma > 0.0);
        }

        TEST_METHOD(PcLatency_Displayed_SimOnly_NoAccum_UsesExistingEma_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.pclSimStartTime = 35'000;
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::IsTrue(HasMetricValue(p0_rows[0].computed.metrics.msPcLatency));

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::IsTrue(HasMetricValue(p1_rows[0].computed.metrics.msPcLatency));
            AssertAreEqualWithinTolerance(0.0, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);
        }

        TEST_METHOD(PcLatency_Displayed_NoPclSim_UsesLastSimStart)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 30'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsTrue(HasMetricValue(p0_results[0].metrics.msPcLatency));
            double emaAfterP0 = state.Input2FrameStartTimeEma;
            uint64_t fallbackSimStart = state.lastSimStartTime;
            Assert::AreEqual(uint64_t(30'000), fallbackSimStart);

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, state);
            Assert::AreEqual(size_t(1), p1_results.size());
            const auto& p1_metrics = p1_results[0].metrics;
            Assert::IsTrue(HasMetricValue(p1_metrics.msPcLatency));
            AssertAreEqualWithinTolerance(emaAfterP0, state.Input2FrameStartTimeEma, 0.0001);

            double expectedLatency = emaAfterP0 + qpc.DeltaSignedMilliSeconds(fallbackSimStart, 90'000);
            AssertAreEqualWithinTolerance(expectedLatency, p1_metrics.msPcLatency, 0.0001);
        }

        TEST_METHOD(PcLatency_Displayed_NoPclSim_UsesLastSimStart_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 30'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), p0_rows.size());
            double emaAfterP0 = swapChain.swapChain.Input2FrameStartTimeEma;
            uint64_t fallbackSimStart = swapChain.swapChain.lastSimStartTime;

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 110'000 });

            auto p1_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p1_rows.size());
            Assert::AreEqual(uint64_t(90'000), p1_rows[0].computed.metrics.screenTimeQpc);

            double expectedLatency = emaAfterP0 + qpc.DeltaSignedMilliSeconds(fallbackSimStart, 90'000);
            AssertAreEqualWithinTolerance(expectedLatency, p1_rows[0].computed.metrics.msPcLatency, 0.0001);
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_OverwritesOldAccum)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;
            ComputeMetricsForPresent(qpc, p0, state);

            FrameData p1{};
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;
            ComputeMetricsForPresent(qpc, p1, state);

            Assert::IsTrue(state.accumulatedInput2FrameStartTime > 0.0);

            FrameData p2{};
            p2.pclInputPingTime = 100'000;
            p2.pclSimStartTime = 120'000;
            p2.finalState = PresentResult::Discarded;

            auto p2_results = ComputeMetricsForPresent(qpc, p2, state);
            Assert::AreEqual(size_t(1), p2_results.size());
            Assert::IsFalse(HasMetricValue(p2_results[0].metrics.msPcLatency));

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(100'000, 120'000);
            AssertAreEqualWithinTolerance(expectedAccum, state.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(120'000), state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_Dropped_DirectPcl_OverwritesOldAccum_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.pclInputPingTime = 10'000;
            p0.pclSimStartTime = 20'000;
            p0.finalState = PresentResult::Discarded;
            (void)swapChain.ProcessPresent(qpc, std::move(p0));

            FrameData p1{};
            p1.pclSimStartTime = 30'000;
            p1.finalState = PresentResult::Discarded;
            (void)swapChain.ProcessPresent(qpc, std::move(p1));

            FrameData p2{};
            p2.pclInputPingTime = 100'000;
            p2.pclSimStartTime = 120'000;
            p2.finalState = PresentResult::Discarded;

            (void)swapChain.ProcessPresent(qpc, std::move(p2));

            double expectedAccum = qpc.DeltaUnsignedMilliSeconds(100'000, 120'000);
            AssertAreEqualWithinTolerance(expectedAccum, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(120'000), swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_IncompleteDroppedChain_DoesNotAffectDirectSample)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState state{};

            FrameData d0{};
            d0.pclInputPingTime = 10'000;
            d0.pclSimStartTime = 20'000;
            d0.finalState = PresentResult::Discarded;
            auto d0_results = ComputeMetricsForPresent(qpc, d0, state);
            Assert::AreEqual(size_t(1), d0_results.size());
            Assert::IsFalse(HasMetricValue(d0_results[0].metrics.msPcLatency));

            FrameData d1{};
            d1.pclSimStartTime = 30'000;
            d1.finalState = PresentResult::Discarded;
            auto d1_results = ComputeMetricsForPresent(qpc, d1, state);
            Assert::AreEqual(size_t(1), d1_results.size());
            Assert::IsFalse(HasMetricValue(d1_results[0].metrics.msPcLatency));
            Assert::IsTrue(state.accumulatedInput2FrameStartTime > 0.0);

            FrameData p0{};
            p0.pclInputPingTime = 100'000;
            p0.pclSimStartTime = 120'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 150'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, state);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& p0_metrics = p0_results[0].metrics;
            Assert::IsTrue(HasMetricValue(p0_metrics.msPcLatency));

            double expectedFirstEma = pmon::util::CalculateEma(0.0,
                qpc.DeltaUnsignedMilliSeconds(100'000, 120'000),
                0.1);
            AssertAreEqualWithinTolerance(expectedFirstEma, state.Input2FrameStartTimeEma, 0.0001);
            AssertAreEqualWithinTolerance(0.0, state.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(0), state.lastReceivedNotDisplayedPclSimStart);
        }

        TEST_METHOD(PcLatency_IncompleteDroppedChain_DoesNotAffectDirectSample_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData d0{};
            d0.pclInputPingTime = 10'000;
            d0.pclSimStartTime = 20'000;
            d0.finalState = PresentResult::Discarded;
            (void)swapChain.ProcessPresent(qpc, std::move(d0));

            FrameData d1{};
            d1.pclSimStartTime = 30'000;
            d1.finalState = PresentResult::Discarded;
            (void)swapChain.ProcessPresent(qpc, std::move(d1));
            Assert::IsTrue(swapChain.swapChain.accumulatedInput2FrameStartTime > 0.0);

            FrameData p0{};
            p0.pclInputPingTime = 100'000;
            p0.pclSimStartTime = 120'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 150'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 180'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::IsTrue(HasMetricValue(rows[0].computed.metrics.msPcLatency));

            double expectedFirstEma = pmon::util::CalculateEma(0.0,
                qpc.DeltaUnsignedMilliSeconds(100'000, 120'000),
                0.1);
            AssertAreEqualWithinTolerance(expectedFirstEma, swapChain.swapChain.Input2FrameStartTimeEma, 0.0001);
            AssertAreEqualWithinTolerance(0.0, swapChain.swapChain.accumulatedInput2FrameStartTime, 0.0001);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedPclSimStart);
        }
    };
    TEST_CLASS(InstrumentedMetricsTests)
    {
    public:
        // V1: ComputeMetricsForPresent (nullptr next). V2: *_ProcessPresent duplicates.

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_FullData_UsesPclSimStart)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            chain.lastSimStartTime = 10'000;
            chain.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData p0{};
            p0.presentStartTime = 0;
            p0.timeInPresent = 0;
            p0.readyTime = 0;
            p0.appSleepStartTime = 1'000;
            p0.appSleepEndTime = 11'000;
            p0.appSimStartTime = 100'000;
            p0.pclSimStartTime = 20'000;
            p0.gpuStartTime = 21'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            double expectedSleepMs = qpc.DeltaUnsignedMilliSeconds(1'000, 11'000);
            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(11'000, 21'000);
            double expectedBetween = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedSleep));
            Assert::AreEqual(expectedSleepMs, m0.msInstrumentedSleep, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msBetweenSimStarts));
            AssertAreEqualWithinTolerance(expectedBetween, m0.msBetweenSimStarts, 1e-6);
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_FullData_UsesPclSimStart_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            swapChain.swapChain.lastSimStartTime = 10'000;
            swapChain.swapChain.animationErrorSource = AnimationErrorSource::PCLatency;

            FrameData p0{};
            p0.appSleepStartTime = 1'000;
            p0.appSleepEndTime = 11'000;
            p0.appSimStartTime = 100'000;
            p0.pclSimStartTime = 20'000;
            p0.gpuStartTime = 21'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 50'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 60'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(50'000), rows[0].computed.metrics.screenTimeQpc);

            const auto& m0 = rows[0].computed.metrics;
            double expectedSleepMs = qpc.DeltaUnsignedMilliSeconds(1'000, 11'000);
            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(11'000, 21'000);
            double expectedBetween = qpc.DeltaUnsignedMilliSeconds(10'000, 20'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedSleep));
            Assert::AreEqual(expectedSleepMs, m0.msInstrumentedSleep, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msBetweenSimStarts));
            AssertAreEqualWithinTolerance(expectedBetween, m0.msBetweenSimStarts, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_FullData_ComputesAll)
        {
            QpcConverter      qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.presentStartTime = 0;
            p0.timeInPresent = 0;
            p0.readyTime = 20'000;
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSleepEndTime = 5'000;
            p0.appSimStartTime = 0;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 30'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 30'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(20'000, 30'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 30'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::AreEqual(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            Assert::AreEqual(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_FullData_ComputesAll_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.readyTime = 20'000;
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSleepEndTime = 5'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 30'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 40'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(30'000), rows[0].computed.metrics.screenTimeQpc);

            const auto& m0 = rows[0].computed.metrics;
            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 30'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(20'000, 30'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 30'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::AreEqual(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            Assert::AreEqual(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            Assert::AreEqual(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleep_UsesAppSimStart)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 40'000;
            chain.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData p0{};
            p0.appSimStartTime = 70'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 120'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep));
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsTrue(HasMetricValue(m0.msBetweenSimStarts));

            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(70'000, 90'000);
            double expectedBetweenMs = qpc.DeltaUnsignedMilliSeconds(40'000, 70'000);

            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency, 1e-6);
            Assert::AreEqual(expectedBetweenMs, m0.msBetweenSimStarts, 1e-6);
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleep_UsesAppSimStart_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            swapChain.swapChain.lastSimStartTime = 40'000;
            swapChain.swapChain.animationErrorSource = AnimationErrorSource::AppProvider;

            FrameData p0{};
            p0.appSimStartTime = 70'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 120'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 150'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            Assert::AreEqual(uint64_t(120'000), rows[0].computed.metrics.screenTimeQpc);

            const auto& m0 = rows[0].computed.metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep));
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsTrue(HasMetricValue(m0.msBetweenSimStarts));

            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(70'000, 90'000);
            double expectedBetweenMs = qpc.DeltaUnsignedMilliSeconds(40'000, 70'000);

            Assert::AreEqual(expectedGpuMs, m0.msInstrumentedGpuLatency, 1e-6);
            Assert::AreEqual(expectedBetweenMs, m0.msBetweenSimStarts, 1e-6);
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleepNoSim_NoInstrumentedCpuGpu)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 55'000;

            FrameData p0{};
            p0.gpuStartTime = 80'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsFalse(HasMetricValue(m0.msBetweenSimStarts));
        }

        TEST_METHOD(InstrumentedCpuGpu_AppFrame_NoSleepNoSim_NoInstrumentedCpuGpu_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            swapChain.swapChain.lastSimStartTime = 55'000;

            FrameData p0{};
            p0.gpuStartTime = 80'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 120'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());

            const auto& m0 = rows[0].computed.metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsFalse(HasMetricValue(m0.msBetweenSimStarts));
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
            //   Derived deltas: sleep Delta = 15'000 ticks, GPU latency Delta = 20'000 ticks,
            //                  between-sim-starts Delta = 30'000 ticks.
            //
            // Call pattern: Case 1 (pure dropped) -> single ComputeMetricsForPresent call.
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

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size(),
                L"Dropped frames should emit their metrics immediately (Case 1).");

            const auto& m0 = p0_results[0].metrics;
            double expectedSleepMs = qpc.DeltaUnsignedMilliSeconds(10'000, 25'000);
            double expectedGpuMs = qpc.DeltaUnsignedMilliSeconds(25'000, 45'000);
            double expectedBetweenMs = qpc.DeltaUnsignedMilliSeconds(5'000, 30'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedSleep));
            AssertAreEqualWithinTolerance(expectedSleepMs, m0.msInstrumentedSleep, 1e-6);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedGpuLatency));
            AssertAreEqualWithinTolerance(expectedGpuMs, m0.msInstrumentedGpuLatency, 1e-6);

            Assert::IsTrue(HasMetricValue(m0.msBetweenSimStarts));
            AssertAreEqualWithinTolerance(expectedBetweenMs, m0.msBetweenSimStarts, 1e-6);

            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency),
                L"Display-dependent metrics must stay off for non-displayed frames.");
            Assert::IsFalse(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
        }

        TEST_METHOD(InstrumentedCpuGpu_V1_FirstDisplayNonAppFrame_Ignored)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};
            chain.lastSimStartTime = 60'000;

            FrameData p0{};
            p0.appSleepStartTime = 11'000;
            p0.appSleepEndTime = 21'000;
            p0.appSimStartTime = 70'000;
            p0.pclSimStartTime = 72'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 120'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsFalse(HasMetricValue(m0.msBetweenSimStarts));
        }

        TEST_METHOD(InstrumentedCpuGpu_NonAppFrame_Ignored_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            const uint32_t PROCESS_ID = 5555;
            const uint64_t SWAPCHAIN = 0xDEADBEEF;

            FrameData bootstrap{};
            bootstrap.processId = PROCESS_ID;
            bootstrap.swapChainAddress = SWAPCHAIN;
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));
            swapChain.swapChain.lastSimStartTime = 60'000;

            FrameData p0{};
            p0.processId = PROCESS_ID;
            p0.swapChainAddress = SWAPCHAIN;
            p0.presentStartTime = 100'000;
            p0.timeInPresent = 10'000;
            p0.readyTime = 110'000;
            p0.appSleepStartTime = 11'000;
            p0.appSleepEndTime = 21'000;
            p0.appSimStartTime = 70'000;
            p0.pclSimStartTime = 72'000;
            p0.gpuStartTime = 90'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 120'000 });

            auto heldRepeated = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(0), heldRepeated.size());

            FrameData p1{};
            p1.processId = PROCESS_ID;
            p1.swapChainAddress = SWAPCHAIN;
            p1.presentStartTime = 130'000;
            p1.timeInPresent = 5'000;
            p1.readyTime = 140'000;
            p1.finalState = PresentResult::Presented;
            p1.appSimStartTime = 80'000;
            p1.displayed.PushBack({ FrameType::Application, 150'000 });

            auto publishedRows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), publishedRows.size());
            Assert::AreEqual((int)FrameType::Repeated, (int)publishedRows[0].computed.metrics.frameType);

            const auto& m0 = publishedRows[0].computed.metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedSleep),
                L"Non-app displays must not emit instrumented CPU metrics.");
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedGpuLatency));
            Assert::IsFalse(HasMetricValue(m0.msBetweenSimStarts));
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoRenderSubmit_RenderLatencyOff)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.readyTime = 80'000;
            p0.appSleepEndTime = 50'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& m0 = p0_results[0].metrics;

            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(80'000, 100'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(50'000, 100'000);

            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            AssertAreEqualWithinTolerance(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoRenderSubmit_RenderLatencyOff_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.readyTime = 80'000;
            p0.appSleepEndTime = 50'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 100'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 130'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            const auto& m0 = rows[0].computed.metrics;

            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(80'000, 100'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(50'000, 100'000);

            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            AssertAreEqualWithinTolerance(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleep_UsesAppSimStart)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSimStartTime = 5'000;
            p0.readyTime = 30'000;
            p0.appSleepEndTime = 0;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& m0 = p0_results[0].metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 60'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(30'000, 60'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 60'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            AssertAreEqualWithinTolerance(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            AssertAreEqualWithinTolerance(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleep_UsesAppSimStart_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSimStartTime = 5'000;
            p0.readyTime = 30'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 60'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            const auto& m0 = rows[0].computed.metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(10'000, 60'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(30'000, 60'000);
            double expectedTotalMs = qpc.DeltaUnsignedMilliSeconds(5'000, 60'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            AssertAreEqualWithinTolerance(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msInstrumentedLatency));
            AssertAreEqualWithinTolerance(expectedTotalMs, m0.msInstrumentedLatency, 1e-6);
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleepNoSim_NoTotalLatency)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.appRenderSubmitStartTime = 12'000;
            p0.readyTime = 32'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& m0 = p0_results[0].metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(12'000, 70'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(32'000, 70'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            AssertAreEqualWithinTolerance(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
        }

        TEST_METHOD(InstrumentedDisplay_AppFrame_NoSleepNoSim_NoTotalLatency_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.appRenderSubmitStartTime = 12'000;
            p0.readyTime = 32'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Application, 70'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p0)).size());

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto rows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), rows.size());
            const auto& m0 = rows[0].computed.metrics;

            double expectedRenderMs = qpc.DeltaUnsignedMilliSeconds(12'000, 70'000);
            double expectedReadyMs = qpc.DeltaUnsignedMilliSeconds(32'000, 70'000);

            Assert::IsTrue(HasMetricValue(m0.msInstrumentedRenderLatency));
            AssertAreEqualWithinTolerance(expectedRenderMs, m0.msInstrumentedRenderLatency, 1e-6);
            Assert::IsTrue(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            AssertAreEqualWithinTolerance(expectedReadyMs, m0.msReadyTimeToDisplayLatency, 1e-6);
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
        }

        TEST_METHOD(InstrumentedDisplay_V1_FirstDisplayNonAppFrame_Ignored)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            FrameData p0{};
            p0.readyTime = 30'000;
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSleepEndTime = 5'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 60'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());

            const auto& m0 = p0_results[0].metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
        }

        TEST_METHOD(InstrumentedDisplay_NonAppFrame_Ignored_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.presentStartTime = 50'000;
            p0.timeInPresent = 8'000;
            p0.readyTime = 30'000;
            p0.appRenderSubmitStartTime = 10'000;
            p0.appSleepEndTime = 5'000;
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 60'000 });

            auto heldRepeated = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(0), heldRepeated.size());

            FrameData p1{};
            p1.presentStartTime = 70'000;
            p1.timeInPresent = 5'000;
            p1.readyTime = 80'000;
            p1.finalState = PresentResult::Presented;
            p1.appSimStartTime = 40'000;
            p1.displayed.PushBack({ FrameType::Application, 90'000 });

            auto publishedRows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), publishedRows.size());
            Assert::AreEqual((int)FrameType::Repeated, (int)publishedRows[0].computed.metrics.frameType);

            const auto& m0 = publishedRows[0].computed.metrics;
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
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
            // Call pattern: Case 1 (single call).
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

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            const auto& m0 = p0_results[0].metrics;

            Assert::IsFalse(HasMetricValue(m0.msInstrumentedRenderLatency));
            Assert::IsFalse(HasMetricValue(m0.msReadyTimeToDisplayLatency));
            Assert::IsFalse(HasMetricValue(m0.msInstrumentedLatency));
        }

        TEST_METHOD(InstrumentedInput_DroppedAppFrame_PendingProviderInput_ConsumedOnDisplay)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t pendingInputTime = 20'000;

            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p0_results[0].metrics.msInstrumentedInputTime));
            Assert::AreEqual(pendingInputTime, chain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, chain);
            Assert::AreEqual(size_t(1), p1_results.size());
            const auto& m1 = p1_results[0].metrics;

            Assert::IsTrue(HasMetricValue(m1.msInstrumentedInputTime));
            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(pendingInputTime, 70'000);
            AssertAreEqualWithinTolerance(expectedInputMs, m1.msInstrumentedInputTime, 1e-6);

            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime);
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedMouseClickTime);
        }

        TEST_METHOD(InstrumentedInput_DroppedAppFrame_PendingProviderInput_ConsumedOnDisplay_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            const uint64_t pendingInputTime = 20'000;

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Discarded;

            auto p0_rows = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(1), p0_rows.size());
            Assert::AreEqual(pendingInputTime, swapChain.swapChain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 70'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p1)).size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 90'000 });

            auto p2_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p2_rows.size());
            Assert::AreEqual(uint64_t(70'000), p2_rows[0].computed.metrics.screenTimeQpc);

            const auto& m1 = p2_rows[0].computed.metrics;
            Assert::IsTrue(HasMetricValue(m1.msInstrumentedInputTime));
            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(pendingInputTime, 70'000);
            AssertAreEqualWithinTolerance(expectedInputMs, m1.msInstrumentedInputTime, 1e-6);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(InstrumentedInput_DisplayedAppFrame_WithOwnSample_IgnoresPending)
        {
            QpcConverter       qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t pendingInputTime = 10'000;
            const uint64_t directInputTime = 15'000;

            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Keyboard };
            p0.finalState = PresentResult::Discarded;

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::AreEqual(pendingInputTime, chain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.appInputSample = { directInputTime, InputDeviceType::Mouse };
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 60'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, chain);
            Assert::AreEqual(size_t(1), p1_results.size());
            const auto& m1 = p1_results[0].metrics;

            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(directInputTime, 60'000);
            Assert::IsTrue(HasMetricValue(m1.msInstrumentedInputTime));
            AssertAreEqualWithinTolerance(expectedInputMs, m1.msInstrumentedInputTime, 1e-6);
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(InstrumentedInput_DisplayedAppFrame_WithOwnSample_IgnoresPending_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            const uint64_t pendingInputTime = 10'000;
            const uint64_t directInputTime = 15'000;

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.appInputSample = { pendingInputTime, InputDeviceType::Keyboard };
            p0.finalState = PresentResult::Discarded;

            (void)swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(pendingInputTime, swapChain.swapChain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.appInputSample = { directInputTime, InputDeviceType::Mouse };
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 60'000 });

            Assert::AreEqual(size_t(0), swapChain.ProcessPresent(qpc, std::move(p1)).size());

            FrameData p2{};
            p2.finalState = PresentResult::Presented;
            p2.displayed.PushBack({ FrameType::Application, 80'000 });

            auto p2_rows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), p2_rows.size());
            Assert::AreEqual(uint64_t(60'000), p2_rows[0].computed.metrics.screenTimeQpc);

            const auto& m1 = p2_rows[0].computed.metrics;
            double expectedInputMs = qpc.DeltaUnsignedMilliSeconds(directInputTime, 60'000);
            Assert::IsTrue(HasMetricValue(m1.msInstrumentedInputTime));
            AssertAreEqualWithinTolerance(expectedInputMs, m1.msInstrumentedInputTime, 1e-6);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedAppProviderInputTime);
        }

        TEST_METHOD(InstrumentedInput_V1_FirstDisplayNonAppFrame_DoesNotAffectInstrumentedInputTime)
        {
            QpcConverter qpc(10'000'000, 0);
            SwapChainCoreState chain{};

            const uint64_t ignoredInputTime = 25'000;

            FrameData p0{};
            p0.appInputSample = { ignoredInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 50'000 });

            auto p0_results = ComputeMetricsForPresent(qpc, p0, chain);
            Assert::AreEqual(size_t(1), p0_results.size());
            Assert::AreEqual(uint64_t(0), chain.lastReceivedNotDisplayedAppProviderInputTime);

            FrameData p1{};
            p1.finalState = PresentResult::Presented;
            p1.displayed.PushBack({ FrameType::Application, 80'000 });

            auto p1_results = ComputeMetricsForPresent(qpc, p1, chain);
            Assert::AreEqual(size_t(1), p1_results.size());
            Assert::IsTrue(IsMissingFrameMetricValue(p1_results[0].metrics.msInstrumentedInputTime));
        }

        TEST_METHOD(InstrumentedInput_NonAppFrame_DoesNotAffectInstrumentedInputTime_ProcessPresent)
        {
            QpcConverter qpc(10'000'000, 0);
            UnifiedSwapChain swapChain{};

            const uint64_t ignoredInputTime = 25'000;

            FrameData bootstrap{};
            bootstrap.presentStartTime = 1;
            bootstrap.timeInPresent = 1;
            bootstrap.readyTime = 1;
            bootstrap.finalState = PresentResult::Presented;

            (void)swapChain.ProcessPresent(qpc, std::move(bootstrap));

            FrameData p0{};
            p0.presentStartTime = 40'000;
            p0.timeInPresent = 5'000;
            p0.readyTime = 45'000;
            p0.appInputSample = { ignoredInputTime, InputDeviceType::Mouse };
            p0.finalState = PresentResult::Presented;
            p0.displayed.PushBack({ FrameType::Repeated, 50'000 });

            auto heldRepeated = swapChain.ProcessPresent(qpc, std::move(p0));
            Assert::AreEqual(size_t(0), heldRepeated.size());

            FrameData p1{};
            p1.presentStartTime = 60'000;
            p1.timeInPresent = 5'000;
            p1.readyTime = 70'000;
            p1.finalState = PresentResult::Presented;
            p1.appSimStartTime = 55'000;
            p1.displayed.PushBack({ FrameType::Application, 80'000 });

            auto repeatedRows = swapChain.ProcessPresent(qpc, std::move(p1));
            Assert::AreEqual(size_t(1), repeatedRows.size());
            Assert::AreEqual((int)FrameType::Repeated, (int)repeatedRows[0].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(0), swapChain.swapChain.lastReceivedNotDisplayedAppProviderInputTime,
                L"Non-app frames should not seed the pending provider input cache.");

            FrameData p2{};
            p2.presentStartTime = 85'000;
            p2.timeInPresent = 5'000;
            p2.readyTime = 95'000;
            p2.finalState = PresentResult::Presented;
            p2.appSimStartTime = 75'000;
            p2.displayed.PushBack({ FrameType::Application, 100'000 });

            auto originRows = swapChain.ProcessPresent(qpc, std::move(p2));
            Assert::AreEqual(size_t(1), originRows.size());
            Assert::AreEqual((int)FrameType::Application, (int)originRows[0].computed.metrics.frameType);
            Assert::AreEqual(uint64_t(80'000), originRows[0].computed.metrics.screenTimeQpc);

            const auto& m1 = originRows[0].computed.metrics;
            Assert::IsTrue(IsMissingFrameMetricValue(m1.msInstrumentedInputTime),
                L"P1 should report missing instrumented input latency as NaN when no app-frame sample exists.");
        }
    };
}
