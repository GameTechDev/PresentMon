// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"
#include "../CommonUtilities/mc/SwapChainCoreState.h"
#include "../CommonUtilities/mc/MetricsTypes.h"
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SwapChainTests
{
    // Mock types for testing template instantiation
    struct MockPresentEvent {
        uint64_t timestamp = 0;
        int data = 0;
    };

    TEST_CLASS(SwapChainCoreStateTests)
    {
    public:
        
        TEST_METHOD(DefaultConstruction_AllFieldsInitialized)
        {
            // Test with a simple type to verify default initialization
            pmon::util::metrics::SwapChainCoreState<int> core;
            
            // Verify timing state defaults to 0
            Assert::AreEqual(uint64_t(0), core.lastSimStartTime);
            Assert::AreEqual(uint64_t(0), core.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(0), core.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), core.lastDisplayedAppScreenTime);
            Assert::AreEqual(uint64_t(0), core.firstAppSimStartTime);
            
            // Verify dropped frame tracking defaults to 0
            Assert::AreEqual(uint64_t(0), core.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(0), core.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(uint64_t(0), core.lastReceivedNotDisplayedAppProviderInputTime);
            Assert::AreEqual(uint64_t(0), core.lastReceivedNotDisplayedPclSimStart);
            Assert::AreEqual(uint64_t(0), core.lastReceivedNotDisplayedPclInputTime);
            
            // Verify PC Latency accumulation defaults to 0.0
            Assert::AreEqual(0.0, core.accumulatedInput2FrameStartTime);
            
            // Verify NVIDIA-specific defaults to 0
            Assert::AreEqual(uint64_t(0), core.lastDisplayedFlipDelay);
            
            // Verify animation error source defaults to CpuStart
            Assert::IsTrue(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Verify optional presents are empty
            Assert::IsFalse(core.lastPresent.has_value());
            Assert::IsFalse(core.lastAppPresent.has_value());
            
            // Verify pending presents vector is empty
            Assert::AreEqual(size_t(0), core.pendingPresents.size());
        }

        TEST_METHOD(SharedPtrInstantiation_ConsolePattern)
        {
            // Simulates Console usage with shared_ptr
            using ConsoleCore = pmon::util::metrics::SwapChainCoreState<std::shared_ptr<MockPresentEvent>>;
            ConsoleCore core;
            
            // Create a mock present event
            auto present = std::make_shared<MockPresentEvent>();
            present->timestamp = 12345;
            present->data = 99;
            
            // Add to pending presents
            core.pendingPresents.push_back(present);
            Assert::AreEqual(size_t(1), core.pendingPresents.size());
            
            // Set as last present
            core.lastPresent = present;
            Assert::IsTrue(core.lastPresent.has_value());
            Assert::AreEqual(uint64_t(12345), (*core.lastPresent)->timestamp);
            
            // Verify reference counting works (same pointer)
            Assert::IsTrue(core.pendingPresents[0].get() == core.lastPresent->get());
        }

        TEST_METHOD(ValueTypeInstantiation_MiddlewarePattern)
        {
            // Simulates Middleware usage with value copies
            using MiddlewareCore = pmon::util::metrics::SwapChainCoreState<MockPresentEvent>;
            MiddlewareCore core;
            
            // Create a mock present event
            MockPresentEvent present{};
            present.timestamp = 54321;
            present.data = 42;
            
            // Add to pending presents (value copy)
            core.pendingPresents.push_back(present);
            Assert::AreEqual(size_t(1), core.pendingPresents.size());
            
            // Set as last present (value copy)
            core.lastPresent = present;
            Assert::IsTrue(core.lastPresent.has_value());
            Assert::AreEqual(uint64_t(54321), core.lastPresent->timestamp);
            
            // Modify original - copies should be independent
            present.timestamp = 99999;
            Assert::AreEqual(uint64_t(54321), core.pendingPresents[0].timestamp);
            Assert::AreEqual(uint64_t(54321), core.lastPresent->timestamp);
        }

        TEST_METHOD(PendingPresents_VectorOperations)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Add multiple items
            core.pendingPresents.push_back(1);
            core.pendingPresents.push_back(2);
            core.pendingPresents.push_back(3);
            
            Assert::AreEqual(size_t(3), core.pendingPresents.size());
            Assert::AreEqual(1, core.pendingPresents[0]);
            Assert::AreEqual(2, core.pendingPresents[1]);
            Assert::AreEqual(3, core.pendingPresents[2]);
            
            // Clear pending presents
            core.pendingPresents.clear();
            Assert::AreEqual(size_t(0), core.pendingPresents.size());
        }

        TEST_METHOD(OptionalPresents_HasValue)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Initially empty
            Assert::IsFalse(core.lastPresent.has_value());
            Assert::IsFalse(core.lastAppPresent.has_value());
            
            // Set lastPresent
            core.lastPresent = 42;
            Assert::IsTrue(core.lastPresent.has_value());
            Assert::AreEqual(42, *core.lastPresent);
            Assert::IsFalse(core.lastAppPresent.has_value());
            
            // Set lastAppPresent
            core.lastAppPresent = 99;
            Assert::IsTrue(core.lastPresent.has_value());
            Assert::IsTrue(core.lastAppPresent.has_value());
            Assert::AreEqual(42, *core.lastPresent);
            Assert::AreEqual(99, *core.lastAppPresent);
            
            // Reset lastPresent
            core.lastPresent.reset();
            Assert::IsFalse(core.lastPresent.has_value());
            Assert::IsTrue(core.lastAppPresent.has_value());
        }

        TEST_METHOD(TimingState_AssignmentAndRetrieval)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Set timing values
            core.lastSimStartTime = 1000;
            core.lastDisplayedSimStartTime = 2000;
            core.lastDisplayedScreenTime = 3000;
            core.lastDisplayedAppScreenTime = 4000;
            core.firstAppSimStartTime = 5000;
            
            // Verify retrieval
            Assert::AreEqual(uint64_t(1000), core.lastSimStartTime);
            Assert::AreEqual(uint64_t(2000), core.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(3000), core.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(4000), core.lastDisplayedAppScreenTime);
            Assert::AreEqual(uint64_t(5000), core.firstAppSimStartTime);
        }

        TEST_METHOD(DroppedFrameTracking_AssignmentAndRetrieval)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Set dropped frame tracking values
            core.lastReceivedNotDisplayedAllInputTime = 1111;
            core.lastReceivedNotDisplayedMouseClickTime = 2222;
            core.lastReceivedNotDisplayedAppProviderInputTime = 3333;
            core.lastReceivedNotDisplayedPclSimStart = 4444;
            core.lastReceivedNotDisplayedPclInputTime = 5555;
            
            // Verify retrieval
            Assert::AreEqual(uint64_t(1111), core.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(2222), core.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(uint64_t(3333), core.lastReceivedNotDisplayedAppProviderInputTime);
            Assert::AreEqual(uint64_t(4444), core.lastReceivedNotDisplayedPclSimStart);
            Assert::AreEqual(uint64_t(5555), core.lastReceivedNotDisplayedPclInputTime);
        }

        TEST_METHOD(PCLatencyAccumulation_DoubleType)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Initially 0.0
            Assert::AreEqual(0.0, core.accumulatedInput2FrameStartTime);
            
            // Set value
            core.accumulatedInput2FrameStartTime = 16.7;
            Assert::AreEqual(16.7, core.accumulatedInput2FrameStartTime, 0.001);
            
            // Accumulate more
            core.accumulatedInput2FrameStartTime += 8.3;
            Assert::AreEqual(25.0, core.accumulatedInput2FrameStartTime, 0.001);
            
            // Reset
            core.accumulatedInput2FrameStartTime = 0.0;
            Assert::AreEqual(0.0, core.accumulatedInput2FrameStartTime);
        }

        TEST_METHOD(AnimationErrorSource_EnumAssignment)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Default is CpuStart
            Assert::IsTrue(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Change to AppProvider
            core.animationErrorSource = pmon::util::metrics::AnimationErrorSource::AppProvider;
            Assert::IsTrue(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::AppProvider);
            Assert::IsFalse(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Change to PCLatency
            core.animationErrorSource = pmon::util::metrics::AnimationErrorSource::PCLatency;
            Assert::IsTrue(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::PCLatency);
            Assert::IsFalse(core.animationErrorSource == pmon::util::metrics::AnimationErrorSource::AppProvider);
        }

        TEST_METHOD(NvidiaFlipDelay_AssignmentAndRetrieval)
        {
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Default is 0
            Assert::AreEqual(uint64_t(0), core.lastDisplayedFlipDelay);
            
            // Set value
            core.lastDisplayedFlipDelay = 8888;
            Assert::AreEqual(uint64_t(8888), core.lastDisplayedFlipDelay);
        }

        TEST_METHOD(ComplexType_SharedPtrWithRealData)
        {
            // More realistic scenario with complex type
            struct ComplexEvent {
                uint64_t qpcTime;
                std::vector<int> displayData;
                double metric;
            };
            
            using ComplexCore = pmon::util::metrics::SwapChainCoreState<std::shared_ptr<ComplexEvent>>;
            ComplexCore core;
            
            // Create event with data
            auto event = std::make_shared<ComplexEvent>();
            event->qpcTime = 123456789;
            event->displayData = {1, 2, 3, 4, 5};
            event->metric = 16.7;
            
            // Store in core state
            core.pendingPresents.push_back(event);
            core.lastPresent = event;
            core.lastSimStartTime = event->qpcTime;
            
            // Verify access
            Assert::IsTrue(core.lastPresent.has_value());
            Assert::AreEqual(uint64_t(123456789), (*core.lastPresent)->qpcTime);
            Assert::AreEqual(size_t(5), (*core.lastPresent)->displayData.size());
            Assert::AreEqual(16.7, (*core.lastPresent)->metric, 0.001);
            Assert::AreEqual(uint64_t(123456789), core.lastSimStartTime);
        }

        TEST_METHOD(StateTransitions_SimulateFrameProcessing)
        {
            // Simulate a realistic frame processing scenario
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core;
            
            // Frame 1: First frame received
            int frame1 = 1000;
            core.pendingPresents.push_back(frame1);
            core.lastPresent = frame1;
            core.lastSimStartTime = 1000;
            
            Assert::AreEqual(size_t(1), core.pendingPresents.size());
            Assert::AreEqual(1000, *core.lastPresent);
            
            // Frame 2: App frame received
            int frame2 = 2000;
            core.pendingPresents.push_back(frame2);
            core.lastPresent = frame2;
            core.lastAppPresent = frame2;
            core.lastSimStartTime = 2000;
            core.firstAppSimStartTime = 2000;
            
            Assert::AreEqual(size_t(2), core.pendingPresents.size());
            Assert::AreEqual(2000, *core.lastAppPresent);
            
            // Frame 2 displayed: Update display state
            core.lastDisplayedSimStartTime = 2000;
            core.lastDisplayedScreenTime = 2016;  // +16ms latency
            core.lastDisplayedAppScreenTime = 2016;
            
            // Clear pending presents (they've been processed)
            core.pendingPresents.clear();
            
            Assert::AreEqual(size_t(0), core.pendingPresents.size());
            Assert::AreEqual(uint64_t(2016), core.lastDisplayedScreenTime);
            
            // Frame 3: Dropped frame (not displayed)
            int frame3 = 3000;
            core.pendingPresents.push_back(frame3);
            core.lastPresent = frame3;
            core.lastReceivedNotDisplayedAllInputTime = 2990;  // Had input
            
            Assert::AreEqual(size_t(1), core.pendingPresents.size());
            Assert::AreEqual(uint64_t(2990), core.lastReceivedNotDisplayedAllInputTime);
        }

        TEST_METHOD(CopySemantics_Independent)
        {
            // Verify that copies are independent (important for value types)
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core1;
            
            // Set state in core1
            core1.lastSimStartTime = 1234;
            core1.pendingPresents.push_back(1);
            core1.pendingPresents.push_back(2);
            core1.lastPresent = 42;
            core1.accumulatedInput2FrameStartTime = 16.7;
            
            // Copy to core2
            TestCore core2 = core1;
            
            // Verify core2 has same values
            Assert::AreEqual(uint64_t(1234), core2.lastSimStartTime);
            Assert::AreEqual(size_t(2), core2.pendingPresents.size());
            Assert::AreEqual(42, *core2.lastPresent);
            Assert::AreEqual(16.7, core2.accumulatedInput2FrameStartTime, 0.001);
            
            // Modify core2
            core2.lastSimStartTime = 5678;
            core2.pendingPresents.push_back(3);
            core2.lastPresent = 99;
            
            // Verify core1 is unchanged
            Assert::AreEqual(uint64_t(1234), core1.lastSimStartTime);
            Assert::AreEqual(size_t(2), core1.pendingPresents.size());
            Assert::AreEqual(42, *core1.lastPresent);
        }

        TEST_METHOD(MoveSemantics_Efficient)
        {
            // Verify move semantics work correctly (important for efficiency)
            using TestCore = pmon::util::metrics::SwapChainCoreState<int>;
            TestCore core1;
            
            // Set state with large vector
            for (int i = 0; i < 100; ++i) {
                core1.pendingPresents.push_back(i);
            }
            core1.lastSimStartTime = 9999;
            core1.lastPresent = 42;
            
            // Move to core2
            TestCore core2 = std::move(core1);
            
            // Verify core2 has the data
            Assert::AreEqual(size_t(100), core2.pendingPresents.size());
            Assert::AreEqual(uint64_t(9999), core2.lastSimStartTime);
            Assert::AreEqual(42, *core2.lastPresent);
            
            // Note: core1 is in moved-from state, don't test its values
        }
    };
}
