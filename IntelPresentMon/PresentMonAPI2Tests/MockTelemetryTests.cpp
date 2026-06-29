// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/Qpc.h"
#include "CppUnitTest.h"
#include "Folders.h"
#include "TestProcess.h"
#include "../PresentMonMiddleware/ActionClient.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonService/AllActions.h"
#include "../ControlLib/mock/MockTelemetryProvider.h"

#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;
using namespace pmon;

namespace MockTelemetryTests
{
    namespace
    {
        inline constexpr double kTwoPi = 6.283185307179586;
        inline constexpr double kValueTolerance = 1.0;

        double ExpectedMockValue_(int64_t requestQpc, uint32_t arrayIndex)
        {
            const double qpcFrequency = util::GetTimestampFrequencyDouble();
            const double tSec = (double)requestQpc / qpcFrequency;
            const double phase = (double)arrayIndex * tel::mock::kMockSinePhasePerIndex;
            return 50.0 + 50.0 * std::sin(kTwoPi * (tSec / tel::mock::kMockSinePeriodSec + phase));
        }

        std::optional<uint32_t> FindMockGpuDeviceId_(const pmapi::intro::Root& intro)
        {
            for (auto device : intro.GetDevices()) {
                if (device.GetType() != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
                    continue;
                }
                const auto name = device.GetName();
                if (name.find(tel::mock::kMockGpuDeviceName) != std::string::npos) {
                    return device.GetId();
                }
            }
            return std::nullopt;
        }

        uint32_t GetMetricArraySizeForDevice_(const pmapi::intro::Root& intro,
            PM_METRIC metricId, uint32_t deviceId)
        {
            const auto metric = intro.FindMetric(metricId);
            for (auto info : metric.GetDeviceMetricInfo()) {
                if (info.GetDevice().GetId() == deviceId && info.IsAvailable()) {
                    return info.GetArraySize();
                }
            }
            return 0;
        }

        void AssertSampleMatchesMock_(const ipc::TelemetrySample<double>& sample, uint32_t arrayIndex)
        {
            Assert::IsTrue(sample.value >= 0.0 && sample.value <= 100.0);
            const double expected = ExpectedMockValue_((int64_t)sample.timestamp, arrayIndex);
            Assert::IsTrue(std::fabs(sample.value - expected) <= kValueTolerance,
                std::format(L"array[{}] value {} expected ~{}", arrayIndex, sample.value, expected).c_str());
        }
    }

    class TestFixture : public CommonTestFixture
    {
    public:
        const CommonProcessArgs& GetCommonArgs() const override
        {
            static CommonProcessArgs args{
                .ctrlPipe = R"(\\.\pipe\pm-mock-telemetry-test-ctrl)",
                .shmNamePrefix = "pm_mock_telemetry_test",
                .logLevel = "verbose",
                .logVerboseModules = "ipc_sto met_use",
                .logFolder = logFolder_,
                .sampleClientMode = "NONE",
            };
            return args;
        }
    };

    TEST_CLASS(MockTelemetryProviderTests)
    {
        TestFixture fixture_;

    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({ "--enable-mock-telemetry"s });
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }

        TEST_METHOD(MockGpuFanSpeedArrayMetrics)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };
            pmapi::EnumMap::Refresh(intro);

            const auto mockGpuId = FindMockGpuDeviceId_(intro);
            Assert::IsTrue(mockGpuId.has_value(), L"PresentMon Mock GPU device not found in introspection");

            const auto fanArraySize = GetMetricArraySizeForDevice_(
                intro, PM_METRIC_GPU_FAN_SPEED, *mockGpuId);
            Assert::AreEqual(tel::mock::kMockGpuFanArraySize, fanArraySize);

            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 100 });

            std::unordered_set<svc::MetricUse> uses;
            for (uint32_t i = 0; i < tel::mock::kMockGpuFanArraySize; ++i) {
                uses.insert({ PM_METRIC_GPU_FAN_SPEED, *mockGpuId, i });
            }
            client.DispatchSync(svc::acts::ReportMetricUse::Params{ std::move(uses) });

            std::this_thread::sleep_for(250ms);

            auto& gpu = pComms->GetGpuDataStore(*mockGpuId);
            auto& fanRings = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_FAN_SPEED);
            Assert::AreEqual((size_t)tel::mock::kMockGpuFanArraySize, fanRings.size());

            std::array<ipc::TelemetrySample<double>, tel::mock::kMockGpuFanArraySize> newest{};
            for (uint32_t i = 0; i < tel::mock::kMockGpuFanArraySize; ++i) {
                Assert::IsFalse(fanRings[i].Empty());
                newest[i] = fanRings[i].Newest();
                AssertSampleMatchesMock_(newest[i], i);
            }

            Assert::AreNotEqual(newest[0].value, newest[1].value);
        }

        TEST_METHOD(MockSystemCoreTemperatureArrayMetrics)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };
            pmapi::EnumMap::Refresh(intro);

            const auto coreTempArraySize = GetMetricArraySizeForDevice_(
                intro, PM_METRIC_CPU_CORE_TEMPERATURE, ipc::kSystemDeviceId);
            if (coreTempArraySize != tel::mock::kMockCoreTempArraySize) {
                Logger::WriteMessage(std::format(
                    "Skipping: mock CPU core temperature route not active (array size {}, expected {})\n",
                    coreTempArraySize, tel::mock::kMockCoreTempArraySize).c_str());
                return;
            }

            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 100 });

            std::unordered_set<svc::MetricUse> uses;
            for (uint32_t i = 0; i < tel::mock::kMockCoreTempArraySize; ++i) {
                uses.insert({ PM_METRIC_CPU_CORE_TEMPERATURE, ipc::kSystemDeviceId, i });
            }
            client.DispatchSync(svc::acts::ReportMetricUse::Params{ std::move(uses) });

            std::this_thread::sleep_for(250ms);

            auto& sys = pComms->GetSystemDataStore();
            auto& tempRings = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_CORE_TEMPERATURE);
            Assert::AreEqual((size_t)tel::mock::kMockCoreTempArraySize, tempRings.size());

            for (uint32_t i = 0; i < tel::mock::kMockCoreTempArraySize; ++i) {
                Assert::IsFalse(tempRings[i].Empty());
                AssertSampleMatchesMock_(tempRings[i].Newest(), i);
            }
        }
    };
}
