// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"

#include "../PresentData/PresentMonTraceConsumer.hpp"

#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PcLatencyFrameGenerationTests
{
    namespace
    {
        constexpr uint32_t ProcessId = 1;

        void Configure_(PMTraceConsumer& consumer)
        {
            consumer.mTrackFrameType = true;
            consumer.mTrackPcLatency = true;
            consumer.mDeferralTimeLimit = 1000000;
            consumer.mHasCompletedAPresent = true;
        }

        void AddPclFrame_(PMTraceConsumer& consumer, uint32_t pclFrameId, uint64_t simStartTime, uint64_t presentStartTime)
        {
            AppTimingData data;
            data.ProcessId = ProcessId;
            data.FrameId = pclFrameId;
            data.PclSimStartTime = simStartTime;
            data.PclPresentStartTime = presentStartTime;
            consumer.mPclTimingDataByPclFrameId.emplace(std::make_pair(pclFrameId, ProcessId), data);
        }

        std::shared_ptr<PresentEvent> MakePresent_(PMTraceConsumer& consumer, uint64_t presentStartTime,
            FrameType frameType, uint32_t frameId)
        {
            auto present = std::make_shared<PresentEvent>();
            present->ProcessId = ProcessId;
            present->ThreadId = ProcessId;
            present->PresentStartTime = presentStartTime;
            present->TimeInPresent = 10;
            auto& event = consumer.mPendingPresentFrameTypeEvents[present->ThreadId];
            event.FrameId = frameId;
            event.FrameType = frameType;
            consumer.ApplyPresentFrameType(present);
            return present;
        }
    }

    TEST_CLASS(PcLatencyFrameGenerationTests)
    {
    public:
        // With runtime frame generation (e.g., FSR FG) the generated frame is presented and
        // completed first, and both presents follow the PCL PresentStart marker of the
        // application frame.
        TEST_METHOD(GeneratedPresentDoesNotTakeApplicationPclData)
        {
            PMTraceConsumer consumer;
            Configure_(consumer);
            AddPclFrame_(consumer, 7, 500, 1000);

            auto generated = MakePresent_(consumer, 2000, FrameType::AMD_FSR_FG, 10);
            auto application = MakePresent_(consumer, 2100, FrameType::Application, 10);

            consumer.CompletePresent(generated);
            Assert::AreEqual((uint64_t)500, generated->PclSimStartTime);

            consumer.CompletePresent(application);
            Assert::AreEqual((uint64_t)500, application->PclSimStartTime);
            Assert::AreEqual((uint32_t)7, application->PclFrameId);
        }

        // With XeSS-FG the application frame is presented first and the generated frame has no
        // PCL data of its own; it must not use older data that no application present took.
        TEST_METHOD(GeneratedPresentIgnoresStalePclData)
        {
            PMTraceConsumer consumer;
            Configure_(consumer);
            AddPclFrame_(consumer, 5, 300, 500);
            AddPclFrame_(consumer, 7, 900, 1000);

            auto application = MakePresent_(consumer, 2000, FrameType::Application, 10);
            auto generated = MakePresent_(consumer, 2100, FrameType::Intel_XEFG, 11);

            consumer.CompletePresent(application);
            Assert::AreEqual((uint64_t)900, application->PclSimStartTime);

            consumer.CompletePresent(generated);
            Assert::AreEqual((uint64_t)0, generated->PclSimStartTime);
        }

        TEST_METHOD(ApplicationPresentAssignsPclData)
        {
            PMTraceConsumer consumer;
            Configure_(consumer);
            AddPclFrame_(consumer, 7, 500, 1000);

            auto first = MakePresent_(consumer, 2000, FrameType::Application, 10);
            auto second = MakePresent_(consumer, 3000, FrameType::Application, 11);

            consumer.CompletePresent(first);
            consumer.CompletePresent(second);
            Assert::AreEqual((uint64_t)500, first->PclSimStartTime);
            Assert::AreEqual((uint64_t)0, second->PclSimStartTime);
        }
    };
}
