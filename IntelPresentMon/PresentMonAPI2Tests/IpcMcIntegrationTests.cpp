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
        return intro.FindEnumKey(PM_ENUM_DEVICE_VENDOR, (int)vendor).GetName();
    }

    static std::vector<PM_QUERY_ELEMENT> BuildStaticFrameQueryElements_(uint32_t gpuDeviceId)
    {
        return {
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_CPU_NAME,
                .stat = PM_STAT_NONE,
                .deviceId = ipc::kSystemDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_CPU_VENDOR,
                .stat = PM_STAT_NONE,
                .deviceId = ipc::kSystemDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_GPU_NAME,
                .stat = PM_STAT_NONE,
                .deviceId = gpuDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_GPU_VENDOR,
                .stat = PM_STAT_NONE,
                .deviceId = gpuDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_GPU_MEM_SIZE,
                .stat = PM_STAT_NONE,
                .deviceId = gpuDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
            PM_QUERY_ELEMENT{
                .metric = PM_METRIC_APPLICATION,
                .stat = PM_STAT_NONE,
                .deviceId = ipc::kUniversalDeviceId,
                .arrayIndex = 0,
                .dataOffset = 0,
                .dataSize = 0,
            },
        };
    }

    static const PM_QUERY_ELEMENT* FindQueryElement_(const std::vector<PM_QUERY_ELEMENT>& elements, PM_METRIC metric, uint32_t deviceId)
    {
        for (const auto& element : elements) {
            if (element.metric == metric && element.deviceId == deviceId) {
                return &element;
            }
        }
        return nullptr;
    }

    static std::string FormatQueryValue_(const pmapi::intro::Root& intro, const PM_QUERY_ELEMENT& element, const uint8_t* pBlob)
    {
        const auto metricView = intro.FindMetric(element.metric);
        const auto dataType = metricView.GetDataTypeInfo().GetFrameType();
        const uint8_t* pData = pBlob + (size_t)element.dataOffset;

        switch (dataType) {
        case PM_DATA_TYPE_UINT64:
            return std::format("{}", *reinterpret_cast<const uint64_t*>(pData));
        case PM_DATA_TYPE_INT32:
            return std::format("{}", *reinterpret_cast<const int32_t*>(pData));
        case PM_DATA_TYPE_UINT32:
            return std::format("{}", *reinterpret_cast<const uint32_t*>(pData));
        case PM_DATA_TYPE_DOUBLE:
            return std::format("{:.6f}", *reinterpret_cast<const double*>(pData));
        case PM_DATA_TYPE_ENUM:
        {
            const int enumValue = *reinterpret_cast<const int*>(pData);
            std::string enumName = "Unknown";
            try {
                enumName = intro.FindEnumKey(metricView.GetDataTypeInfo().GetEnumId(), enumValue).GetName();
            }
            catch (...) {
            }
            return std::format("{} ({})", enumName, enumValue);
        }
        case PM_DATA_TYPE_BOOL:
            return *reinterpret_cast<const bool*>(pData) ? "true" : "false";
        case PM_DATA_TYPE_STRING:
            return std::string(reinterpret_cast<const char*>(pData));
        case PM_DATA_TYPE_VOID:
        default:
            return "void";
        }
    }

    static void LogFrameQueryResults_(const pmapi::intro::Root& intro, const std::vector<PM_QUERY_ELEMENT>& elements, const uint8_t* pBlob)
    {
        for (const auto& element : elements) {
            const auto metricView = intro.FindMetric(element.metric);
            const auto value = FormatQueryValue_(intro, element, pBlob);
            Logger::WriteMessage(std::format("{}, {}\n",
                metricView.Introspect().GetSymbol(),
                value).c_str());
        }
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
                    Logger::WriteMessage("Universal frame query results:\n");
                    LogFrameQueryResults_(*intro, elements, blobs[0]);
                    break;
                }
                std::this_thread::sleep_for(25ms);
            }
            Assert::IsTrue(gotFrames, L"Expected frame query to consume frames");
        }

        TEST_METHOD(FrameQueryStaticMetricsAreFilled)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro = session.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro);

            const auto gpuDeviceId = FindFirstGpuDeviceId_(*intro);
            Assert::IsTrue(gpuDeviceId.has_value(), L"No GPU device found");

            auto elements = BuildUniversalFrameQueryElements_(*intro);
            auto staticElements = BuildStaticFrameQueryElements_(*gpuDeviceId);
            elements.insert(elements.end(), staticElements.begin(), staticElements.end());
            Logger::WriteMessage(std::format("Frame query metrics (with statics): {}\n", elements.size()).c_str());

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
                    Logger::WriteMessage("Frame query results with static metrics:\n");
                    LogFrameQueryResults_(*intro, elements, blobs[0]);
                    break;
                }
                std::this_thread::sleep_for(25ms);
            }
            Assert::IsTrue(gotFrames, L"Expected frame query to consume frames");

            const auto* cpuNameElement = FindQueryElement_(elements, PM_METRIC_CPU_NAME, ipc::kSystemDeviceId);
            Assert::IsTrue(cpuNameElement != nullptr, L"CPU name element missing");
            const auto* cpuVendorElement = FindQueryElement_(elements, PM_METRIC_CPU_VENDOR, ipc::kSystemDeviceId);
            Assert::IsTrue(cpuVendorElement != nullptr, L"CPU vendor element missing");
            const auto* gpuNameElement = FindQueryElement_(elements, PM_METRIC_GPU_NAME, *gpuDeviceId);
            Assert::IsTrue(gpuNameElement != nullptr, L"GPU name element missing");
            const auto* gpuVendorElement = FindQueryElement_(elements, PM_METRIC_GPU_VENDOR, *gpuDeviceId);
            Assert::IsTrue(gpuVendorElement != nullptr, L"GPU vendor element missing");
            const auto* gpuMemSizeElement = FindQueryElement_(elements, PM_METRIC_GPU_MEM_SIZE, *gpuDeviceId);
            Assert::IsTrue(gpuMemSizeElement != nullptr, L"GPU memory size element missing");
            const auto* appNameElement = FindQueryElement_(elements, PM_METRIC_APPLICATION, ipc::kUniversalDeviceId);
            Assert::IsTrue(appNameElement != nullptr, L"Application element missing");

            const uint8_t* firstBlob = blobs[0];
            const auto cpuName = std::string(reinterpret_cast<const char*>(firstBlob + (size_t)cpuNameElement->dataOffset));
            const int cpuVendorValue = *reinterpret_cast<const int*>(firstBlob + (size_t)cpuVendorElement->dataOffset);
            const auto cpuVendorName = GetVendorName_(*intro, (PM_DEVICE_VENDOR)cpuVendorValue);
            const auto gpuName = std::string(reinterpret_cast<const char*>(firstBlob + (size_t)gpuNameElement->dataOffset));
            const int gpuVendorValue = *reinterpret_cast<const int*>(firstBlob + (size_t)gpuVendorElement->dataOffset);
            const auto gpuVendorName = GetVendorName_(*intro, (PM_DEVICE_VENDOR)gpuVendorValue);
            const auto gpuMemSize = *reinterpret_cast<const uint64_t*>(firstBlob + (size_t)gpuMemSizeElement->dataOffset);
            const auto appName = std::string(reinterpret_cast<const char*>(firstBlob + (size_t)appNameElement->dataOffset));

            Assert::IsTrue(!cpuName.empty(), L"CPU name empty");
            Assert::IsTrue(!cpuVendorName.empty(), L"CPU vendor name empty");
            Assert::IsTrue(!gpuName.empty(), L"GPU name empty");
            Assert::IsTrue(!gpuVendorName.empty(), L"GPU vendor name empty");
            Assert::IsTrue(gpuMemSize > 0, L"GPU memory size not available");
            Assert::IsTrue(appName == "PresentBench.exe", L"Unexpected application name");
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
