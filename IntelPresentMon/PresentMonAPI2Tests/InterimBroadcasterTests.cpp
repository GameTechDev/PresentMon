// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include <string>
#include <ranges>
#include "Folders.h"
#include "JobManager.h"

#include "../PresentMonMiddleware/ActionClient.h"
#include "../Interprocess/source/Interprocess.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "../PresentMonService/AllActions.h"

#include <format>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace vi = std::views;
namespace rn = std::ranges;
using namespace std::literals;
using namespace pmon;

namespace InterimBroadcasterTests
{
    static std::string DumpRing_(const ipc::HistoryRing<double>& ring, size_t maxSamples = 8)
    {
        std::ostringstream oss;
        const auto [first, last] = ring.GetSerialRange();
        const size_t count = last - first;

        oss << "serial range [" << first << ", " << last << "), count=" << count << "\n";

        if (count == 0) {
            return oss.str();
        }

        const size_t n = (count < maxSamples) ? count : maxSamples;
        for (size_t i = 0; i < n; ++i) {
            const auto& s = ring.At(first + i);
            oss << "  [" << (first + i) << "] ts=" << s.timestamp << " val=" << s.value << "\n";
        }

        if (count > n) {
            oss << "  ...\n";
            const auto& sLast = ring.At(last - 1);
            oss << "  [" << (last - 1) << "] ts=" << sLast.timestamp << " val=" << sLast.value << "\n";
        }

        // Try to include Newest() summary for debugging
        try {
            const auto& newest = ring.Newest();
            oss << "newest: ts=" << newest.timestamp << " val=" << newest.value << "\n";
        }
        catch (...) {
            // If Newest() throws on empty in some impls, ignore here.
        }

        return oss.str();
    }

    class TestFixture : public CommonTestFixture
    {
    public:
        const CommonProcessArgs& GetCommonArgs() const override
        {
            static CommonProcessArgs args{
                .ctrlPipe = R"(\\.\pipe\pm-intbroad-test-ctrl)",
                .shmNamePrefix = "pm_intborad_test_intro",
                .logLevel = "debug",
                .logFolder = logFolder_,
                .sampleClientMode = "NONE",
            };
            return args;
        }
    };

    TEST_CLASS(CommonFixtureTests)
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
        // verify service lifetime and status command functionality
        TEST_METHOD(ServiceStatusTest)
        {
            // verify initial status
            const auto status = fixture_.service->QueryStatus();
            Assert::AreEqual(0ull, status.nsmStreamedPids.size());
            Assert::AreEqual(16u, status.telemetryPeriodMs);
            Assert::IsTrue((bool)status.etwFlushPeriodMs);
            Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
        }
        // verify action system can connect
        TEST_METHOD(ActionConnect)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            Assert::IsFalse(client.GetShmPrefix().empty());
            // there is a bit of a race condition on creating a service, immediately connecting
            // and then immediately terminating it via the test control module
            // not a concern for normal operation and is entirely synthetic; don't waste
            // effort on trying to rework this, just add a little wait for odd tests like this
            std::this_thread::sleep_for(150ms);
        }
        // verify comms work with introspection (no wrapper)
        TEST_METHOD(IntrospectionConnect)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            auto pIntro = pComms->GetIntrospectionRoot();
            Assert::AreEqual(2ull, pIntro->pDevices->size);
            auto pDevice = static_cast<const PM_INTROSPECTION_DEVICE*>(pIntro->pDevices->pData[1]);
            Assert::AreEqual("NVIDIA GeForce RTX 2080 Ti", pDevice->pName->pData);
        }
    };

    TEST_CLASS(CpuStoreTests)
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
        // static store
        TEST_METHOD(StaticData)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            // get the store containing system-wide telemetry (cpu etc.)
            auto& sys = pComms->GetSystemDataStore();
            Assert::AreEqual((int)PM_DEVICE_VENDOR_AMD, (int)sys.statics.cpuVendor);
            Assert::AreEqual("AMD Ryzen 7 5800X 8-Core Processor", sys.statics.cpuName.c_str());
            Assert::AreEqual(0., sys.statics.cpuPowerLimit);
        }
        // polled store
        TEST_METHOD(PolledData)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            // acquire introspection with enhanced wrapper interface
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) {delete p; } };
            pmapi::EnumMap::Refresh(intro);
            auto pMetricMap = pmapi::EnumMap::GetKeyMap(PM_ENUM_METRIC);
            // set telemetry period so we have a known baseline
            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 100 });
            // as a stopgap we target a process in order to trigger service-side telemetry collection
            // TODO: remove this when we enable service-side query awareness of connected clients
            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });
            // get the store containing system-wide telemetry (cpu etc.)
            auto& sys = pComms->GetSystemDataStore();
            for (auto&& [met, r] : sys.telemetryData.Rings()) {
                Logger::WriteMessage(std::format(" TeleRing@{}\n", pMetricMap->at(met).narrowName).c_str());
                // TODO: understand the disconnect between CPU Core Utility showing up here
                // and now showing up in the UI
            }
            Assert::AreEqual(3ull, (size_t)rn::distance(sys.telemetryData.Rings()));
            std::this_thread::sleep_for(500ms);
            // check that we have data for frequency and utilization
            std::vector<ipc::HistoryRing<double>::Sample> utilizSamples;
            std::vector<ipc::HistoryRing<double>::Sample> freqSamples;
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(250ms);
                {
                    constexpr auto m = PM_METRIC_CPU_UTILIZATION;
                    auto& r = sys.telemetryData.FindRing<double>(m).at(0);
                    Assert::IsFalse(r.Empty());
                    if (i == 0 || i == 9) {
                        Logger::WriteMessage(DumpRing_(r).c_str());
                    }
                    auto sample = r.Newest();
                    utilizSamples.push_back(sample);
                    Logger::WriteMessage(std::format("({}) {}: {}\n",
                        i, pMetricMap->at(m).narrowName, sample.value).c_str());
                    Assert::IsTrue(sample.value > 1.);
                }
                {
                    constexpr auto m = PM_METRIC_CPU_FREQUENCY;
                    auto& r = sys.telemetryData.FindRing<double>(m).at(0);
                    Assert::IsFalse(r.Empty());
                    if (i == 0 || i == 9) {
                        Logger::WriteMessage(DumpRing_(r).c_str());
                    }
                    auto sample = r.Newest();
                    freqSamples.push_back(sample);
                    Logger::WriteMessage(std::format("({}) {}: {}\n",
                        i, pMetricMap->at(m).narrowName, sample.value).c_str());
                    Assert::IsTrue(sample.value > 1500.);
                }
            }
            // make sure samples actually change over time
            Assert::AreNotEqual(utilizSamples.front().timestamp, utilizSamples.back().timestamp);
            Assert::AreNotEqual(utilizSamples.front().value, utilizSamples.back().value);
            Assert::AreNotEqual(freqSamples.front().timestamp, freqSamples.back().timestamp);
            Assert::AreNotEqual(freqSamples.front().value, freqSamples.back().value);
        }
    };

    TEST_CLASS(GpuStoreTests)
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
        // polled store
        TEST_METHOD(PolledData)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // acquire introspection with enhanced wrapper interface
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };
            pmapi::EnumMap::Refresh(intro);
            auto pMetricMap = pmapi::EnumMap::GetKeyMap(PM_ENUM_METRIC);

            // set telemetry period so we have a known baseline
            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 100 });

            // as a stopgap we target a process in order to trigger service-side telemetry collection
            // TODO: remove this when we enable service-side query awareness of connected clients
            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            // get the store containing adapter telemetry
            auto& gpu = pComms->GetGpuDataStore(1);

            // allow a short warmup
            std::this_thread::sleep_for(500ms);

            std::vector<ipc::HistoryRing<double>::Sample> tempSamples;
            std::vector<ipc::HistoryRing<double>::Sample> powerSamples;

            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(250ms);

                {
                    constexpr auto m = PM_METRIC_GPU_TEMPERATURE;
                    auto& r = gpu.telemetryData.FindRing<double>(m).at(0);
                    Assert::IsFalse(r.Empty());

                    if (i == 0 || i == 9) {
                        Logger::WriteMessage(DumpRing_(r).c_str());
                    }

                    auto sample = r.Newest();
                    tempSamples.push_back(sample);

                    Logger::WriteMessage(std::format("({}) {}: {}\n",
                        i, pMetricMap->at(m).narrowName, sample.value).c_str());

                    // loose sanity check to avoid flakiness
                    Assert::IsTrue(sample.value > 10.);
                }

                {
                    constexpr auto m = PM_METRIC_GPU_POWER;
                    auto& r = gpu.telemetryData.FindRing<double>(m).at(0);
                    Assert::IsFalse(r.Empty());

                    if (i == 0 || i == 9) {
                        Logger::WriteMessage(DumpRing_(r).c_str());
                    }

                    auto sample = r.Newest();
                    powerSamples.push_back(sample);

                    Logger::WriteMessage(std::format("({}) {}: {}\n",
                        i, pMetricMap->at(m).narrowName, sample.value).c_str());

                    // loose sanity check to avoid flakiness
                    Assert::IsTrue(sample.value > 1.);
                }
            }

            // make sure samples actually change over time
            Assert::AreNotEqual(tempSamples.front().timestamp, tempSamples.back().timestamp);
            Assert::AreNotEqual(powerSamples.front().timestamp, powerSamples.back().timestamp);
            Assert::AreNotEqual(powerSamples.front().value, powerSamples.back().value);
        }
    };
}
