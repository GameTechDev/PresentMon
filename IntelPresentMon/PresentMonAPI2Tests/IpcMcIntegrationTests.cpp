// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"

#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../Interprocess/source/SystemDeviceId.h"

#include <chrono>
#include <format>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;

namespace IpcMcIntegrationTests
{
    namespace ipc = pmon::ipc;

    class TestFixture : public CommonTestFixture
    {
    public:
        const CommonProcessArgs& GetCommonArgs() const override
        {
            static CommonProcessArgs args{
                .ctrlPipe = R"(\\.\pipe\pm-ipcmc-int-ctrl)",
                .shmNamePrefix = "pm_ipcmc_int",
                .logLevel = "verbose",
                .logVerboseModules = "ipc_sto met_use",
                .logFolder = logFolder_,
                .sampleClientMode = "NONE",
            };
            return args;
        }
    };

    static std::vector<PM_QUERY_ELEMENT> BuildUniversalFrameQueryElements_(const pmapi::intro::Root& intro)
    {
        std::vector<PM_QUERY_ELEMENT> elements;
        for (auto metric : intro.GetMetrics()) {
            if (!pmapi::intro::MetricTypeIsFrameEvent(metric.GetType())) {
                continue;
            }
            for (auto info : metric.GetDeviceMetricInfo()) {
                if (info.GetDevice().GetId() != ipc::kUniversalDeviceId) {
                    continue;
                }
                if (!info.IsAvailable()) {
                    break;
                }
                const uint32_t arraySize = info.GetArraySize();
                for (uint32_t index = 0; index < arraySize; ++index) {
                    elements.push_back(PM_QUERY_ELEMENT{
                        .metric = metric.GetId(),
                        .stat = PM_STAT_NONE,
                        .deviceId = ipc::kUniversalDeviceId,
                        .arrayIndex = index,
                        .dataOffset = 0,
                        .dataSize = 0,
                    });
                }
                break;
            }
        }
        return elements;
    }

    TEST_CLASS(IpcMcIntegrationTests)
    {
        TestFixture fixture_;

    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup();
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }

        TEST_METHOD(UniversalFrameQueryConsumesPresenterFrames)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro = session.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro);

            auto elements = BuildUniversalFrameQueryElements_(*intro);
            Logger::WriteMessage(std::format("Universal frame metrics: {}\n", elements.size()).c_str());
            Assert::IsTrue(!elements.empty(), L"No universal frame metrics found");

            auto query = session.RegisterFrameQuery(elements);

            auto presenter = fixture_.LaunchPresenter();
            session.SetEtwFlushPeriod(8);
            std::this_thread::sleep_for(1ms);
            auto tracker = session.TrackProcess(presenter.GetId());

            auto blobs = query.MakeBlobContainer(16);
            bool gotFrames = false;
            const auto deadline = std::chrono::steady_clock::now() + 2s;
            while (std::chrono::steady_clock::now() < deadline) {
                query.Consume(tracker, blobs);
                if (blobs.GetNumBlobsPopulated() > 0) {
                    gotFrames = true;
                    break;
                }
                std::this_thread::sleep_for(25ms);
            }
            Assert::IsTrue(gotFrames, L"Expected frame query to consume frames");
        }

        TEST_METHOD(SecondFrameQueryRegistrationFails)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro = session.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro);

            auto elements = BuildUniversalFrameQueryElements_(*intro);
            Assert::IsTrue(!elements.empty(), L"No universal frame metrics found");

            // first registration succeeds
            std::vector<PM_QUERY_ELEMENT> single{ elements.front() };
            auto query = session.RegisterFrameQuery(single);
            Assert::IsTrue((bool)query);

            // second registration fails
            Assert::ExpectException<pmapi::ApiErrorException>([&] {
                session.RegisterFrameQuery(single);
            });
        }
    };
}
