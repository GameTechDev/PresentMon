// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"

#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PresentFrameTypeDeferralTests
{
    namespace
    {
        void Configure_(PMTraceConsumer& consumer, uint64_t deferralTimeLimit)
        {
            consumer.mTrackFrameType = true;
            consumer.mDeferralTimeLimit = deferralTimeLimit;
            consumer.mHasCompletedAPresent = true;
        }

        std::shared_ptr<PresentEvent> MakePresent_(PMTraceConsumer& consumer, uint32_t processId,
            uint64_t presentStartTime, FrameType frameType = FrameType::NotSet, uint32_t frameId = 0)
        {
            auto present = std::make_shared<PresentEvent>();
            present->ProcessId = processId;
            present->ThreadId = processId;
            present->PresentStartTime = presentStartTime;
            if (frameType != FrameType::NotSet) {
                auto& event = consumer.mPendingPresentFrameTypeEvents[present->ThreadId];
                event.FrameId = frameId;
                event.FrameType = frameType;
            }
            consumer.ApplyPresentFrameType(present);
            return present;
        }

        std::vector<std::shared_ptr<PresentEvent>> Dequeue_(PMTraceConsumer& consumer)
        {
            std::vector<std::shared_ptr<PresentEvent>> presents;
            consumer.DequeuePresentEvents(presents);
            return presents;
        }
    }

    TEST_CLASS(PresentFrameTypeDeferralTests)
    {
    public:
        TEST_METHOD(XefgPartnerReceivesPropagatedData)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000000);

            auto generated = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            generated->TimeInPresent = 50;
            generated->GPUStartTime = 1100;
            generated->ReadyTime = 1200;
            generated->GPUDuration = 80;
            generated->GPUVideoDuration = 5;
            auto application = MakePresent_(consumer, 1, 2000, FrameType::Application, 10);

            consumer.CompletePresent(generated);
            Assert::IsTrue(generated->WaitingForFrameId);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());

            consumer.CompletePresent(application);
            Assert::IsFalse(generated->WaitingForFrameId);
            Assert::IsTrue(application->WaitingForFrameId);
            Assert::AreEqual((uint64_t)1000, application->AppPropagatedPresentStartTime);
            Assert::AreEqual((uint64_t)50, application->AppPropagatedTimeInPresent);
            Assert::AreEqual((uint64_t)1100, application->AppPropagatedGPUStartTime);
            Assert::AreEqual((uint64_t)1200, application->AppPropagatedReadyTime);
            Assert::AreEqual((uint64_t)80, application->AppPropagatedGPUDuration);
            Assert::AreEqual((uint64_t)5, application->AppPropagatedGPUVideoDuration);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)1, presents.size());
            Assert::IsTrue(presents[0] == generated);
        }

        TEST_METHOD(UntypedPresentReleasesWaitingPresent)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000000);

            auto typed = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            auto untyped = MakePresent_(consumer, 1, 2000);
            Assert::IsFalse(untyped->WaitingForFrameId);

            consumer.CompletePresent(typed);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());

            consumer.CompletePresent(untyped);
            Assert::IsFalse(typed->WaitingForFrameId);
            Assert::AreEqual((uint64_t)0, untyped->AppPropagatedPresentStartTime);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)2, presents.size());
            Assert::IsTrue(presents[0] == typed);
            Assert::IsTrue(presents[1] == untyped);
        }

        TEST_METHOD(OtherProcessDoesNotReleaseWaitingPresent)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000000);

            auto typed = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            auto other = MakePresent_(consumer, 2, 2000);

            consumer.CompletePresent(typed);
            consumer.CompletePresent(other);
            Assert::IsTrue(typed->WaitingForFrameId);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());
        }

        TEST_METHOD(EarlierStartedCompletionDoesNotRelease)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000000);

            auto older = MakePresent_(consumer, 1, 1000, FrameType::Application, 11);
            auto newer = MakePresent_(consumer, 1, 2000, FrameType::Intel_XEFG, 11);
            auto successor = MakePresent_(consumer, 1, 3000);

            consumer.CompletePresent(newer);
            consumer.CompletePresent(older);
            Assert::IsTrue(newer->WaitingForFrameId);
            Assert::AreEqual((uint64_t)0, older->AppPropagatedPresentStartTime);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());

            consumer.CompletePresent(successor);
            Assert::IsFalse(newer->WaitingForFrameId);
            Assert::IsFalse(older->WaitingForFrameId);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)3, presents.size());
            Assert::IsTrue(presents[0] == newer);
            Assert::IsTrue(presents[1] == older);
            Assert::IsTrue(presents[2] == successor);
        }

        TEST_METHOD(BackstopReleasesOnlyAfterDeferralTimeLimit)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000);

            auto typed = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            auto atLimit = MakePresent_(consumer, 2, 2000);
            auto afterLimit = MakePresent_(consumer, 2, 2001);

            consumer.CompletePresent(typed);
            consumer.CompletePresent(atLimit);
            Assert::IsTrue(typed->WaitingForFrameId);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());

            consumer.CompletePresent(afterLimit);
            Assert::IsFalse(typed->WaitingForFrameId);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)3, presents.size());
            Assert::IsTrue(presents[0] == typed);
            Assert::IsTrue(presents[1] == atLimit);
            Assert::IsTrue(presents[2] == afterLimit);
        }

        TEST_METHOD(BackstopReleasesWithUndequeuedReadyPresents)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 1000);

            auto ready = MakePresent_(consumer, 2, 500);
            auto typed = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            auto afterLimit = MakePresent_(consumer, 2, 2001);

            consumer.CompletePresent(ready);
            consumer.CompletePresent(typed);
            Assert::AreEqual((uint32_t)1, consumer.mReadyCount);

            consumer.CompletePresent(afterLimit);
            Assert::IsFalse(typed->WaitingForFrameId);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)3, presents.size());
            Assert::IsTrue(presents[0] == ready);
            Assert::IsTrue(presents[1] == typed);
            Assert::IsTrue(presents[2] == afterLimit);
        }

        TEST_METHOD(BackstopReleasesImmediatelyWithZeroDeferralTimeLimit)
        {
            PMTraceConsumer consumer;
            Configure_(consumer, 0);

            auto typed = MakePresent_(consumer, 1, 1000, FrameType::Intel_XEFG, 10);
            auto sameStart = MakePresent_(consumer, 2, 1000);
            auto later = MakePresent_(consumer, 2, 1001);

            consumer.CompletePresent(typed);
            consumer.CompletePresent(sameStart);
            Assert::IsTrue(typed->WaitingForFrameId);
            Assert::AreEqual((size_t)0, Dequeue_(consumer).size());

            consumer.CompletePresent(later);
            Assert::IsFalse(typed->WaitingForFrameId);

            auto presents = Dequeue_(consumer);
            Assert::AreEqual((size_t)3, presents.size());
            Assert::IsTrue(presents[0] == typed);
            Assert::IsTrue(presents[1] == sameStart);
            Assert::IsTrue(presents[2] == later);
        }
    };
}
