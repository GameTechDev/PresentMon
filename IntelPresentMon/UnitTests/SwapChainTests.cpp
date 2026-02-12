// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"
#include "../CommonUtilities/mc/SwapChainState.h"
#include "../CommonUtilities/mc/MetricsTypes.h"
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MetricsCoreTests
{
    TEST_CLASS(SwapChainStateTests)
    {
    public:
        
        TEST_METHOD(DefaultConstruction_AllFieldsInitialized)
        {
            // Test with a simple type to verify default initialization
            pmon::util::metrics::SwapChainCoreState swapChain;
            
            // Verify timing state defaults to 0
            Assert::AreEqual(uint64_t(0), swapChain.lastSimStartTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastDisplayedAppScreenTime);
            Assert::AreEqual(uint64_t(0), swapChain.firstAppSimStartTime);
            
            // Verify dropped frame tracking defaults to 0
            Assert::AreEqual(uint64_t(0), swapChain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastReceivedNotDisplayedAppProviderInputTime);
            Assert::AreEqual(uint64_t(0), swapChain.lastReceivedNotDisplayedPclSimStart);
            Assert::AreEqual(uint64_t(0), swapChain.lastReceivedNotDisplayedPclInputTime);
            
            // Verify PC Latency accumulation defaults to 0.0
            Assert::AreEqual(0.0, swapChain.accumulatedInput2FrameStartTime);
            
            // Verify NVIDIA-specific defaults to 0
            Assert::AreEqual(uint64_t(0), swapChain.lastDisplayedFlipDelay);
            
            // Verify animation error source defaults to CpuStart
            Assert::IsTrue(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Verify optional presents are empty
            Assert::IsFalse(swapChain.lastPresent.has_value());
            Assert::IsFalse(swapChain.lastAppPresent.has_value());
        }

        TEST_METHOD(OptionalPresents_HasValue)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Initially empty
            Assert::IsFalse(swapChain.lastPresent.has_value());
            Assert::IsFalse(swapChain.lastAppPresent.has_value());

            pmon::util::metrics::FrameData present[2]{};
            present[0].appFrameId = 1;
            present[1].appFrameId = 2;

            // Set lastPresent
            swapChain.lastPresent = present[0];
            Assert::IsTrue(swapChain.lastPresent.has_value());
            Assert::AreEqual((uint32_t)1, swapChain.lastPresent.value().appFrameId);
            Assert::IsFalse(swapChain.lastAppPresent.has_value());
            
            // Set lastAppPresent
            swapChain.lastAppPresent = present[1];
            Assert::IsTrue(swapChain.lastPresent.has_value());
            Assert::IsTrue(swapChain.lastAppPresent.has_value());
            Assert::AreEqual((uint32_t)1, swapChain.lastPresent.value().appFrameId);
            Assert::AreEqual((uint32_t)2, swapChain.lastAppPresent.value().appFrameId);
            
            // Reset lastPresent
            swapChain.lastPresent.reset();
            Assert::IsFalse(swapChain.lastPresent.has_value());
            Assert::IsTrue(swapChain.lastAppPresent.has_value());
        }

        TEST_METHOD(TimingState_AssignmentAndRetrieval)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};

            // Set timing values
            swapChain.lastSimStartTime = 1000;
            swapChain.lastDisplayedSimStartTime = 2000;
            swapChain.lastDisplayedScreenTime = 3000;
            swapChain.lastDisplayedAppScreenTime = 4000;
            swapChain.firstAppSimStartTime = 5000;
            
            // Verify retrieval
            Assert::AreEqual(uint64_t(1000), swapChain.lastSimStartTime);
            Assert::AreEqual(uint64_t(2000), swapChain.lastDisplayedSimStartTime);
            Assert::AreEqual(uint64_t(3000), swapChain.lastDisplayedScreenTime);
            Assert::AreEqual(uint64_t(4000), swapChain.lastDisplayedAppScreenTime);
            Assert::AreEqual(uint64_t(5000), swapChain.firstAppSimStartTime);
        }

        TEST_METHOD(DroppedFrameTracking_AssignmentAndRetrieval)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Set dropped frame tracking values
            swapChain.lastReceivedNotDisplayedAllInputTime = 1111;
            swapChain.lastReceivedNotDisplayedMouseClickTime = 2222;
            swapChain.lastReceivedNotDisplayedAppProviderInputTime = 3333;
            swapChain.lastReceivedNotDisplayedPclSimStart = 4444;
            swapChain.lastReceivedNotDisplayedPclInputTime = 5555;
            
            // Verify retrieval
            Assert::AreEqual(uint64_t(1111), swapChain.lastReceivedNotDisplayedAllInputTime);
            Assert::AreEqual(uint64_t(2222), swapChain.lastReceivedNotDisplayedMouseClickTime);
            Assert::AreEqual(uint64_t(3333), swapChain.lastReceivedNotDisplayedAppProviderInputTime);
            Assert::AreEqual(uint64_t(4444), swapChain.lastReceivedNotDisplayedPclSimStart);
            Assert::AreEqual(uint64_t(5555), swapChain.lastReceivedNotDisplayedPclInputTime);
        }

        TEST_METHOD(PCLatencyAccumulation_DoubleType)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Initially 0.0
            Assert::AreEqual(0.0, swapChain.accumulatedInput2FrameStartTime);
            
            // Set value
            swapChain.accumulatedInput2FrameStartTime = 16.7;
            Assert::AreEqual(16.7, swapChain.accumulatedInput2FrameStartTime, 0.001);
            
            // Accumulate more
            swapChain.accumulatedInput2FrameStartTime += 8.3;
            Assert::AreEqual(25.0, swapChain.accumulatedInput2FrameStartTime, 0.001);
            
            // Reset
            swapChain.accumulatedInput2FrameStartTime = 0.0;
            Assert::AreEqual(0.0, swapChain.accumulatedInput2FrameStartTime);
        }

        TEST_METHOD(AnimationErrorSource_EnumAssignment)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Default is CpuStart
            Assert::IsTrue(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Change to AppProvider
            swapChain.animationErrorSource = pmon::util::metrics::AnimationErrorSource::AppProvider;
            Assert::IsTrue(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::AppProvider);
            Assert::IsFalse(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::CpuStart);
            
            // Change to PCLatency
            swapChain.animationErrorSource = pmon::util::metrics::AnimationErrorSource::PCLatency;
            Assert::IsTrue(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::PCLatency);
            Assert::IsFalse(swapChain.animationErrorSource == pmon::util::metrics::AnimationErrorSource::AppProvider);
        }

        TEST_METHOD(NvidiaFlipDelay_AssignmentAndRetrieval)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Default is 0
            Assert::AreEqual(uint64_t(0), swapChain.lastDisplayedFlipDelay);
            
            // Set value
            swapChain.lastDisplayedFlipDelay = 8888;
            Assert::AreEqual(uint64_t(8888), swapChain.lastDisplayedFlipDelay);
        }

        TEST_METHOD(MultipleFrameDataFields)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            pmon::util::metrics::FrameData present;

            present.appFrameId = 7777;
            present.presentStartTime = 5555;
            present.timeInPresent = 2000;

            // Store in core state
            swapChain.lastPresent = present;
            
            // Verify access
            Assert::IsTrue(swapChain.lastPresent.has_value());
            Assert::AreEqual(uint32_t(7777), swapChain.lastPresent.value().appFrameId);
            Assert::AreEqual(uint64_t(5555), swapChain.lastPresent.value().presentStartTime);
            Assert::AreEqual(uint64_t(2000), swapChain.lastPresent.value().timeInPresent);
        }

        TEST_METHOD(StateTransitions_SimulateFrameProcessing)
        {
            pmon::util::metrics::SwapChainCoreState swapChain{};
            
            // Frame 1: First frame received
            pmon::util::metrics::FrameData presentOne;

            presentOne.presentStartTime = 1000;
            presentOne.appFrameId = 1;
            swapChain.lastPresent = presentOne;
            swapChain.lastSimStartTime = 1000;
            Assert::AreEqual(true, swapChain.lastPresent.has_value());

            // Frame 2: Next frame received
            pmon::util::metrics::FrameData presentTwo;

            int frame2 = 2000;
            swapChain.lastPresent = presentTwo;
            swapChain.lastSimStartTime = 2000;

            // Frame 2 displayed: Update display state
            swapChain.lastDisplayedSimStartTime = 2000;
            swapChain.lastDisplayedScreenTime = 2016;  // +16ms latency
            swapChain.lastDisplayedAppScreenTime = 2016;
            Assert::AreEqual(uint64_t(2016), swapChain.lastDisplayedScreenTime);

            // Frame 3
            pmon::util::metrics::FrameData presentThree;
            swapChain.lastPresent = presentThree;
            swapChain.lastReceivedNotDisplayedAllInputTime = 2990;
            Assert::AreEqual(uint64_t(2990), swapChain.lastReceivedNotDisplayedAllInputTime);
        }

        TEST_METHOD(CopySemantics_Independent)
        {
            // Verify that copies are independent (important for value types)
            pmon::util::metrics::SwapChainCoreState swapChainOne{};
            pmon::util::metrics::FrameData presents[3];
            
            // Set state in core1
            swapChainOne.lastSimStartTime = 1234;
            swapChainOne.lastPresent = presents[1];
            swapChainOne.accumulatedInput2FrameStartTime = 16.7;
            
            pmon::util::metrics::SwapChainCoreState swapChainTwo{};
            
            // SwapChainOne to SwapChainTwo
            swapChainTwo = swapChainOne;

            // Verify core2 has same values
            Assert::AreEqual(uint64_t(1234), swapChainTwo.lastSimStartTime);
            //Assert::AreEqual(presents[1], *swapChainTwo.lastPresent);
            Assert::AreEqual(16.7, swapChainTwo.accumulatedInput2FrameStartTime, 0.001);
            
            // Modify SwapChainTwo
            swapChainTwo.lastSimStartTime = 5678;
            swapChainTwo.lastPresent = presents[2];
            
            // Verify core1 is unchanged
            Assert::AreEqual(uint64_t(1234), swapChainOne.lastSimStartTime);
            //Assert::AreEqual(presents[1], *swapChainOne.lastPresent);
        }
    };
}
