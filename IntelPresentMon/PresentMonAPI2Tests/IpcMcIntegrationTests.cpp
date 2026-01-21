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
#include <string>
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

    static std::optional<uint32_t> FindFirstGpuDeviceId_(const pmapi::intro::Root& intro)
    {
        for (auto device : intro.GetDevices()) {
            if (device.GetType() == PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
                return device.GetId();
            }
        }
        return std::nullopt;
    }

    static std::string GetVendorName_(const pmapi::intro::Root& intro, PM_DEVICE_VENDOR vendor)
    {
        return intro.FindEnumKey(PM_ENUM_DEVICE_VENDOR, static_cast<int>(vendor)).GetName();
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

        TEST_METHOD(StaticQueryReturnsExpectedValues)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro = session.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro);

            const auto gpuDeviceId = FindFirstGpuDeviceId_(*intro);
            Assert::IsTrue(gpuDeviceId.has_value(), L"No GPU device found");

            auto presenter = fixture_.LaunchPresenter();
            auto tracker = session.TrackProcess(presenter.GetId());

            const auto cpuName = pmapi::PollStatic(session, tracker, PM_METRIC_CPU_NAME, ipc::kSystemDeviceId)
                .As<std::string>();
            Logger::WriteMessage(std::format("CPU name: {}\n", cpuName).c_str());
            Assert::IsTrue(!cpuName.empty(), L"CPU name empty");

            const auto cpuVendor = pmapi::PollStatic(session, tracker, PM_METRIC_CPU_VENDOR, ipc::kSystemDeviceId)
                .As<PM_DEVICE_VENDOR>();
            const auto cpuVendorName = GetVendorName_(*intro, cpuVendor);
            Logger::WriteMessage(std::format("CPU vendor: {}\n", cpuVendorName).c_str());
            Assert::IsTrue(!cpuVendorName.empty(), L"CPU vendor name empty");

            const auto gpuName = pmapi::PollStatic(session, tracker, PM_METRIC_GPU_NAME, *gpuDeviceId)
                .As<std::string>();
            const auto gpuVendor = pmapi::PollStatic(session, tracker, PM_METRIC_GPU_VENDOR, *gpuDeviceId)
                .As<PM_DEVICE_VENDOR>();
            const auto gpuMemSize = pmapi::PollStatic(session, tracker, PM_METRIC_GPU_MEM_SIZE, *gpuDeviceId)
                .As<uint64_t>();
            Logger::WriteMessage(std::format("GPU name: {}\n", gpuName).c_str());
            Assert::IsTrue(!gpuName.empty(), L"GPU name empty");
            const auto gpuVendorName = GetVendorName_(*intro, gpuVendor);
            Logger::WriteMessage(std::format("GPU vendor: {}\n", gpuVendorName).c_str());
            Assert::IsTrue(!gpuVendorName.empty(), L"GPU vendor name empty");
            Logger::WriteMessage(std::format("GPU memory size: {}\n", gpuMemSize).c_str());
            Assert::IsTrue(gpuMemSize > 0, L"GPU memory size not available");

            const auto appName = pmapi::PollStatic(session, tracker, PM_METRIC_APPLICATION).As<std::string>();
            Logger::WriteMessage(std::format("Application name: {}\n", appName).c_str());
            Assert::IsTrue(appName == "PresentBench.exe", L"Unexpected application name");
        }
    };
}
