// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"

#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"

#include <chrono>
#include <format>
#include <string>
#include <type_traits>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;

namespace IpcMcIntegrationTests
{
    namespace ipc = pmon::ipc;
    namespace util = pmon::util;

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

    static bool IsMetricAvailableForDevice_(const pmapi::intro::Root& intro, PM_METRIC metricId, uint32_t deviceId, uint32_t arrayIndex = 0)
    {
        const auto metric = intro.FindMetric(metricId);
        for (auto info : metric.GetDeviceMetricInfo()) {
            if (info.GetDevice().GetId() != deviceId) {
                continue;
            }
            return info.IsAvailable() && info.GetArraySize() > arrayIndex;
        }
        return false;
    }

    static std::optional<PM_METRIC> FindGpuDynamicMetric_(const pmapi::intro::Root& intro, uint32_t gpuDeviceId)
    {
        static constexpr PM_METRIC preferredMetrics[] = {
            PM_METRIC_GPU_UTILIZATION,
            PM_METRIC_GPU_TEMPERATURE,
            PM_METRIC_GPU_POWER,
        };
        for (const auto metric : preferredMetrics) {
            if (IsMetricAvailableForDevice_(intro, metric, gpuDeviceId, 0)) {
                return metric;
            }
        }
        for (auto metric : intro.GetMetrics()) {
            if (metric.GetType() != PM_METRIC_TYPE_DYNAMIC &&
                metric.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
                continue;
            }
            for (auto info : metric.GetDeviceMetricInfo()) {
                if (info.GetDevice().GetId() != gpuDeviceId) {
                    continue;
                }
                if (info.IsAvailable() && info.GetArraySize() > 0) {
                    return metric.GetId();
                }
                break;
            }
        }
        return std::nullopt;
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

    struct DynamicPairPollSummary_
    {
        uint32_t gpuPollsWithData = 0;
        uint32_t cpuPollsWithData = 0;
    };

    static DynamicPairPollSummary_ PollDynamicQueryPairAndLog_(
        const pmapi::intro::Root& intro,
        const char* phaseLabel,
        pmapi::DynamicQuery& gpuQuery,
        const std::vector<PM_QUERY_ELEMENT>& gpuElements,
        pmapi::DynamicQuery& cpuQuery,
        const std::vector<PM_QUERY_ELEMENT>& cpuElements,
        uint32_t pollCount,
        std::chrono::milliseconds interval)
    {
        auto gpuBlobs = gpuQuery.MakeBlobContainer(8);
        auto cpuBlobs = cpuQuery.MakeBlobContainer(8);
        DynamicPairPollSummary_ summary{};

        for (uint32_t i = 0; i < pollCount; ++i) {
            gpuQuery.Poll(gpuBlobs);
            const auto gpuCount = gpuBlobs.GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("{} poll {} gpu blobs={}\n", phaseLabel, i, gpuCount).c_str());
            if (gpuCount > 0) {
                ++summary.gpuPollsWithData;
                Logger::WriteMessage(std::format("{} poll {} gpu values:\n", phaseLabel, i).c_str());
                LogFrameQueryResults_(intro, gpuElements, gpuBlobs[0]);
            }

            cpuQuery.Poll(cpuBlobs);
            const auto cpuCount = cpuBlobs.GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("{} poll {} cpu blobs={}\n", phaseLabel, i, cpuCount).c_str());
            if (cpuCount > 0) {
                ++summary.cpuPollsWithData;
                Logger::WriteMessage(std::format("{} poll {} cpu values:\n", phaseLabel, i).c_str());
                LogFrameQueryResults_(intro, cpuElements, cpuBlobs[0]);
            }

            std::this_thread::sleep_for(interval);
        }

        return summary;
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

        TEST_METHOD(UniversalNonStaticMetricsMapToFrameMetrics)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro = session.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro);

            std::vector<PM_METRIC> metricsToCheck;
            for (auto metricView : intro->GetMetrics()) {
                if (metricView.GetType() == PM_METRIC_TYPE_STATIC) {
                    continue;
                }

                bool hasUniversalDevice = false;
                for (auto info : metricView.GetDeviceMetricInfo()) {
                    if (info.GetDevice().GetId() == ipc::kUniversalDeviceId) {
                        hasUniversalDevice = true;
                        break;
                    }
                }

                if (hasUniversalDevice) {
                    metricsToCheck.push_back(metricView.GetId());
                }
            }

            Logger::WriteMessage(std::format("Universal non-static metrics to map: {}\n", metricsToCheck.size()).c_str());
            Assert::IsTrue(!metricsToCheck.empty(), L"No universal non-static metrics found");

            size_t failedMappings = 0;
            for (auto metricId : metricsToCheck) {
                const auto metricView = intro->FindMetric(metricId);
                const auto symbol = metricView.Introspect().GetSymbol();
                const bool mapped = util::DispatchEnumValue<PM_METRIC, int(PM_METRIC_COUNT_)>(
                    metricId,
                    [&]<PM_METRIC Metric>() -> bool {
                        if constexpr (util::metrics::HasFrameMetricMember<Metric>) {
                            constexpr auto memberPtr = util::metrics::FrameMetricMember<Metric>::member;
                            using MemberInfo = util::MemberPointerInfo<decltype(memberPtr)>;
                            return std::is_same_v<typename MemberInfo::StructType, pmon::util::metrics::FrameMetrics>;
                        }
                        return false;
                    },
                    false);
                Logger::WriteMessage(std::format("Metric {}: {}\n", symbol, mapped ? "ok" : "*FAIL !! MISSING").c_str());
                if (!mapped) {
                    ++failedMappings;
                }
            }

            if (failedMappings > 0) {
                Logger::WriteMessage(std::format("Unmapped universal non-static metrics: {}\n", failedMappings).c_str());
            }

            Assert::IsTrue(failedMappings == 0, L"FrameMetricsMemberMap missing universal non-static metrics");
        }
    };

    TEST_CLASS(MultiSessionDynamicQueryTests)
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

        TEST_METHOD(RegisterAndPollDynamicQueriesAcrossSessionTeardown)
        {
            pmapi::Session session1{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro1 = session1.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro1);

            const auto gpuDeviceId = FindFirstGpuDeviceId_(*intro1);
            Assert::IsTrue(gpuDeviceId.has_value(), L"No GPU device found");
            const auto gpuMetric = FindGpuDynamicMetric_(*intro1, *gpuDeviceId);
            Assert::IsTrue(gpuMetric.has_value(), L"No dynamic GPU metric available");
            Assert::IsTrue(IsMetricAvailableForDevice_(*intro1, PM_METRIC_CPU_UTILIZATION, ipc::kSystemDeviceId, 0),
                L"CPU utilization metric unavailable for system device");

            Logger::WriteMessage(std::format("Using gpu metric {} on device {}\n",
                intro1->FindMetric(*gpuMetric).Introspect().GetSymbol(),
                *gpuDeviceId).c_str());

            std::vector<PM_QUERY_ELEMENT> gpuElements{
                PM_QUERY_ELEMENT{
                    .metric = *gpuMetric,
                    .stat = PM_STAT_AVG,
                    .deviceId = *gpuDeviceId,
                    .arrayIndex = 0,
                    .dataOffset = 0,
                    .dataSize = 0,
                },
            };
            std::vector<PM_QUERY_ELEMENT> cpuElements{
                PM_QUERY_ELEMENT{
                    .metric = PM_METRIC_CPU_UTILIZATION,
                    .stat = PM_STAT_AVG,
                    .deviceId = ipc::kSystemDeviceId,
                    .arrayIndex = 0,
                    .dataOffset = 0,
                    .dataSize = 0,
                },
            };

            session1.SetTelemetryPollingPeriod(*gpuDeviceId, 100);
            session1.SetTelemetryPollingPeriod(ipc::kSystemDeviceId, 100);

            auto gpuQuery1 = session1.RegisterDynamicQuery(gpuElements);
            auto cpuQuery1 = session1.RegisterDynamicQuery(cpuElements);

            std::this_thread::sleep_for(150ms);
            const auto firstPass = PollDynamicQueryPairAndLog_(*intro1, "session1", gpuQuery1, gpuElements, cpuQuery1, cpuElements, 4, 100ms);
            Logger::WriteMessage(std::format("session1 summary: gpu_with_data={}, cpu_with_data={}\n",
                firstPass.gpuPollsWithData, firstPass.cpuPollsWithData).c_str());

            pmapi::Session session2{ fixture_.GetCommonArgs().ctrlPipe };
            auto intro2 = session2.GetIntrospectionRoot();
            Assert::IsTrue((bool)intro2);

            gpuQuery1.Reset();
            cpuQuery1.Reset();
            session1.Reset();
            Logger::WriteMessage("Session1 destroyed after session2 creation\n");

            session2.SetTelemetryPollingPeriod(*gpuDeviceId, 100);
            session2.SetTelemetryPollingPeriod(ipc::kSystemDeviceId, 100);

            auto gpuQuery2 = session2.RegisterDynamicQuery(gpuElements);
            auto cpuQuery2 = session2.RegisterDynamicQuery(cpuElements);

            std::this_thread::sleep_for(150ms);
            const auto secondPass = PollDynamicQueryPairAndLog_(*intro2, "session2", gpuQuery2, gpuElements, cpuQuery2, cpuElements, 4, 100ms);
            Logger::WriteMessage(std::format("session2 summary: gpu_with_data={}, cpu_with_data={}\n",
                secondPass.gpuPollsWithData, secondPass.cpuPollsWithData).c_str());

            Assert::IsTrue(secondPass.gpuPollsWithData > 0, L"Expected GPU dynamic query to return data in second session");
            Assert::IsTrue(secondPass.cpuPollsWithData > 0, L"Expected CPU dynamic query to return data in second session");
        }
    };
}
