// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include <string>
#include <ranges>
#include <fstream>
#include "Folders.h"
#include "JobManager.h"

#include "../PresentMonMiddleware/ActionClient.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
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
    static std::string DumpRing_(const ipc::SampleHistoryRing<double>& ring, size_t maxSamples = 8)
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
                .shmNamePrefix = "pm_intbroad_test",
                .logLevel = "verbose",
                .logVerboseModules = "ipc_sto met_use",
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
            Assert::AreEqual(0ull, status.trackedPids.size());
            Assert::AreEqual(0ull, status.frameStorePids.size());
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
            Assert::AreEqual(3ull, pIntro->pDevices->size);
            auto pDevice = static_cast<const PM_INTROSPECTION_DEVICE*>(pIntro->pDevices->pData[1]);
            Assert::AreEqual("NVIDIA GeForce RTX 2080 Ti", pDevice->pName->pData);
        }
    };

    TEST_CLASS(SystemStoreTests)
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
        // trying to use a store without reporting use
        TEST_METHOD(NoReport)
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

            // get the store containing system-wide telemetry (cpu etc.)
            auto& sys = pComms->GetSystemDataStore();
            for (auto&& [met, r] : sys.telemetryData.Rings()) {
                Logger::WriteMessage(std::format(" TeleRing@{}\n", pMetricMap->at(met).narrowName).c_str());
                // TODO: understand the disconnect between CPU Core Utility showing up here
                // and not showing up in the UI (update: it is blacklisted manually in UI introspection)
            }
            Assert::AreEqual(2ull, (size_t)rn::distance(sys.telemetryData.Rings()));

            // allow warm-up period
            std::this_thread::sleep_for(650ms);

            // we expect 0 data point in the rings for the system since it does not populate on init
            Assert::AreEqual(0ull, sys.telemetryData.FindRing<double>(PM_METRIC_CPU_UTILIZATION).at(0).Size());
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

            // get the store containing system-wide telemetry (cpu etc.)
            auto& sys = pComms->GetSystemDataStore();
            for (auto&& [met, r] : sys.telemetryData.Rings()) {
                Logger::WriteMessage(std::format(" TeleRing@{}\n", pMetricMap->at(met).narrowName).c_str());
                // TODO: understand the disconnect between CPU Core Utility showing up here
                // and not showing up in the UI (update: it is blacklisted manually in UI introspection)
            }
            Assert::AreEqual(2ull, (size_t)rn::distance(sys.telemetryData.Rings()));

            // update server with metric/device usage information
            // this will trigger system telemetry collection
            client.DispatchSync(svc::acts::ReportMetricUse::Params{ {
                { PM_METRIC_CPU_UTILIZATION, ipc::kSystemDeviceId, 0 },
                { PM_METRIC_CPU_FREQUENCY, ipc::kSystemDeviceId, 0 },
            } });

            // allow warm-up period
            std::this_thread::sleep_for(150ms);

            // check that we have data for frequency and utilization
            std::vector<ipc::TelemetrySample<double>> utilizSamples;
            std::vector<ipc::TelemetrySample<double>> freqSamples;
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
        // full 1:1 correspondence between ring creation, ring population, and introspection availability
        TEST_METHOD(RingUtilization)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // acquire introspection with enhanced wrapper interface
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };
            pmapi::EnumMap::Refresh(intro);
            auto pMetricMap = pmapi::EnumMap::GetKeyMap(PM_ENUM_METRIC);

            // set telemetry period so we have a known baseline
            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 40 });

            // get the store containing adapter telemetry
            auto& sys = pComms->GetSystemDataStore();

            // allow a short warmup
            std::this_thread::sleep_for(500ms);

            // build the set of expected rings from the store, and cross-check against introspection
            Logger::WriteMessage("Store Metrics\n=============\n");
            std::map<PM_METRIC, size_t> storeRings;
            for (auto&& [met, r] : sys.telemetryData.Rings()) {
                const auto storeArraySize = sys.telemetryData.ArraySize(met);
                storeRings[met] = storeArraySize;

                // dump for review in output pane
                Logger::WriteMessage(std::format("[{}] {}\n", storeArraySize,
                    pMetricMap->at(met).narrowName).c_str());

                // validate introspection says the metric is available for the system device
                auto&& m = intro.FindMetric(met);
                bool matchedDevice = false;
                size_t introArraySize = 0;
                for (auto&& di : m.GetDeviceMetricInfo()) {
                    if (di.GetDevice().GetId() != ipc::kSystemDeviceId) {
                        // skip over non-matching devices
                        continue;
                    }
                    matchedDevice = true;
                    if (di.GetAvailability() == PM_METRIC_AVAILABILITY_AVAILABLE) {
                        introArraySize = di.GetArraySize();
                    }
                    // either way, if we get here, device matched so no need to continue
                    break;
                }
                Assert::IsTrue(matchedDevice, pMetricMap->at(met).wideName.c_str());
                Assert::AreEqual(storeArraySize, introArraySize, pMetricMap->at(met).wideName.c_str());
            }
            Logger::WriteMessage(std::format("Total: {}\n", storeRings.size()).c_str());

            // validate that the expected number of rings sets are present in the store
            Assert::AreEqual(storeRings.size(), (size_t)rn::distance(sys.telemetryData.Rings()));

            {
                // build metric use set from above store results
                std::unordered_set<svc::acts::MetricUse> uses;
                for (auto&& [met, siz] : storeRings) {
                    if (siz > 0) {
                        uses.insert({ met, ipc::kSystemDeviceId, 0 });
                    }
                }
                // update server with metric/device usage information
                // this will trigger system telemetry collection
                client.DispatchSync(svc::acts::ReportMetricUse::Params{ std::move(uses) });
            }

            // allow a short warmup
            std::this_thread::sleep_for(150ms);

            // validate that exepected rings are populated with samples and have correct dimensions
            for (auto&& [met, size] : storeRings) {
                // array sizes should match
                Assert::AreEqual(size, sys.telemetryData.ArraySize(met),
                    pMetricMap->at(met).wideName.c_str());
                std::visit([&](auto const& rings) {
                    // for each history ring in set, make sure it has at least more than one sample in it
                    for (size_t i = 0; i < size; i++) {
                        auto& name = pMetricMap->at(met).wideName;
                        Assert::IsTrue(rings[i].Size() > 1,
                            std::format(L"{}[{}]", name, i).c_str());
                        auto& sample = rings[i].Newest();
                        Logger::WriteMessage(std::format(L"{}[{}]: {}@{}\n", name, i,
                            sample.value, sample.timestamp).c_str());
                    }
                }, sys.telemetryData.FindRingVariant(met));
            }
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
        // trying to use a store without reporting use
        TEST_METHOD(NoReport)
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

            // target gpu device 1 (hardcoded for test)
            const uint32_t TargetDeviceID = 1;

            // get the store containing adapter telemetry
            auto& gpu = pComms->GetGpuDataStore(TargetDeviceID);
            for (auto&& [met, r] : gpu.telemetryData.Rings()) {
                Logger::WriteMessage(std::format(" TeleRing@{}\n", pMetricMap->at(met).narrowName).c_str());
            }
            Assert::IsTrue((size_t)rn::distance(gpu.telemetryData.Rings()) > 0);

            // allow warm-up period
            std::this_thread::sleep_for(650ms);

            // we expect 0 data points in the rings for the gpu since it does not populate on init
            Assert::AreEqual(0ull, gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_TEMPERATURE).at(0).Size());
        }
        // static store
        TEST_METHOD(StaticData)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            // get the store containing gpu telemetry
            auto& gpu = pComms->GetGpuDataStore(1);
            Assert::AreEqual((int)PM_DEVICE_VENDOR_NVIDIA, (int)gpu.statics.vendor);
            Assert::AreEqual("NVIDIA GeForce RTX 2080 Ti", gpu.statics.name.c_str());
            Assert::AreEqual(260., gpu.statics.sustainedPowerLimit);
            Assert::AreEqual(11811160064ull, gpu.statics.memSize);
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

            // target gpu device 1 (hardcoded for test)
            const uint32_t TargetDeviceID = 1;

            // update server with metric/device usage information
            // this will trigger gpu telemetry collection
            client.DispatchSync(svc::acts::ReportMetricUse::Params{ {
                { PM_METRIC_GPU_TEMPERATURE, TargetDeviceID, 0 },
                { PM_METRIC_GPU_POWER, TargetDeviceID, 0 },
            } });

            // get the store containing adapter telemetry
            auto& gpu = pComms->GetGpuDataStore(TargetDeviceID);

            // allow a short warmup
            std::this_thread::sleep_for(150ms);

            std::vector<ipc::TelemetrySample<double>> tempSamples;
            std::vector<ipc::TelemetrySample<double>> powerSamples;

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
        // full 1:1 correspondence between ring creation, ring population, and introspection availability
        TEST_METHOD(RingUtilization)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // acquire introspection with enhanced wrapper interface
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };
            pmapi::EnumMap::Refresh(intro);
            auto pMetricMap = pmapi::EnumMap::GetKeyMap(PM_ENUM_METRIC);

            // set telemetry period so we have a known baseline
            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 40 });

            // target gpu device 1 (hardcoded for test)
            const uint32_t TargetDeviceID = 1;

            // get the store containing adapter telemetry
            auto& gpu = pComms->GetGpuDataStore(TargetDeviceID);

            // build the set of expected rings from introspection
            Logger::WriteMessage("Introspection Metrics\n=====================\n");
            std::map<PM_METRIC, size_t> introspectionAvailability;
            for (auto&& m : intro.GetMetrics()) {
                // only consider metrics that are polled
                if (m.GetType() != PM_METRIC_TYPE_DYNAMIC &&
                    m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
                    continue;
                }
                // check availability for target gpu
                size_t arraySize = 0;
                for (auto&& di : m.GetDeviceMetricInfo()) {
                    if (di.GetDevice().GetId() != TargetDeviceID) {
                        // skip over non-matching devices
                        continue;
                    }
                    if (di.GetAvailability() == PM_METRIC_AVAILABILITY_AVAILABLE) {
                        // if available get size (otherwise leave at 0 default)
                        arraySize = di.GetArraySize();
                    }
                    // either way, if we get here, device matched so no need to continue
                    break;
                }
                // only consider metrics associated with and available for target gpu
                if (arraySize > 0) {
                    introspectionAvailability[m.GetId()] = arraySize;
                    // dump for review in output pane
                    Logger::WriteMessage(std::format("[{}] {}\n", arraySize,
                        pMetricMap->at(m.GetId()).narrowName).c_str());
                }
            }
            Logger::WriteMessage(std::format("Total: {}\n", introspectionAvailability.size()).c_str());

            // validate that the expected number of rings sets are present in the store
            Assert::AreEqual(introspectionAvailability.size(), (size_t)rn::distance(gpu.telemetryData.Rings()));
                        
            {
                // build metric use set from above introspection results
                std::unordered_set<svc::acts::MetricUse> uses;
                for (auto&& [met, siz] : introspectionAvailability) {
                    if (siz > 0) {
                        uses.insert({ met, TargetDeviceID, 0 });
                    }
                }
                // update server with metric/device usage information
                // this will trigger gpu telemetry collection
                client.DispatchSync(svc::acts::ReportMetricUse::Params{ std::move(uses) });
            }

            // allow a short warmup
            std::this_thread::sleep_for(150ms);

            // validate that exepected rings are populated with samples and have correct dimensions
            for (auto&& [met, size] : introspectionAvailability) {
                // array sizes should match
                Assert::AreEqual(size, gpu.telemetryData.ArraySize(met),
                    pMetricMap->at(met).wideName.c_str());
                std::visit([&](auto const& rings) {
                    // for each history ring in set, make sure it has at least more than one sample in it
                    for (size_t i = 0; i < size; i++) {
                        auto& name = pMetricMap->at(met).wideName;
                        Assert::IsTrue(rings[i].Size() > 1,
                            std::format(L"{}[{}]", name, i).c_str());
                        auto& sample = rings[i].Newest();
                        Logger::WriteMessage(std::format(L"{}[{}]: {}@{}\n", name, i,
                            sample.value, sample.timestamp).c_str());
                    }
                }, gpu.telemetryData.FindRingVariant(met));
            }
        }
    };

    TEST_CLASS(NewActivationIsolationTests)
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
        TEST_METHOD(SystemOnlyLeavesGpuEmpty)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 350 });
            Logger::WriteMessage("SystemOnlyLeavesGpuEmpty: telemetry period set to 350ms\n");

            const uint32_t TargetDeviceID = 1;

            client.DispatchSync(svc::acts::ReportMetricUse::Params{ {
                { PM_METRIC_CPU_UTILIZATION, ipc::kSystemDeviceId, 0 },
                { PM_METRIC_CPU_FREQUENCY, ipc::kSystemDeviceId, 0 },
            } });
            Logger::WriteMessage("SystemOnlyLeavesGpuEmpty: reported CPU utilization/frequency usage\n");

            auto& sys = pComms->GetSystemDataStore();
            auto& gpu = pComms->GetGpuDataStore(TargetDeviceID);

            auto& sysRing = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_UTILIZATION).at(0);
            auto& sysFreqRing = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_FREQUENCY).at(0);
            auto& gpuRing = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_TEMPERATURE).at(0);
            std::this_thread::sleep_for(1500ms);
            const auto logRing = [](const char* label, const ipc::SampleHistoryRing<double>& ring) {
                const auto range = ring.GetSerialRange();
                Logger::WriteMessage(std::format(
                    "{}: serial [{}, {}) count={}\n",
                    label, range.first, range.second, range.second - range.first).c_str());
                for (size_t s = range.first; s < range.second; ++s) {
                    const auto& sample = ring.At(s);
                    Logger::WriteMessage(std::format(
                        "{}[{}]: val={} ts={}\n",
                        label, s, sample.value, sample.timestamp).c_str());
                }
            };
            Logger::WriteMessage(std::format(
                "SystemOnlyLeavesGpuEmpty: sizes cpu_util={} cpu_freq={} gpu_temp={}\n",
                sysRing.Size(), sysFreqRing.Size(), gpuRing.Size()).c_str());
            logRing("cpu_util", sysRing);
            logRing("cpu_freq", sysFreqRing);
            logRing("gpu_temp", gpuRing);
            Assert::IsTrue(sysRing.Size() >= 3,
                std::format(L"Expected cpu utilization ring to have >= 3 samples, got {}",
                    sysRing.Size()).c_str());
            Assert::IsTrue(sysFreqRing.Size() >= 3,
                std::format(L"Expected cpu frequency ring to have >= 3 samples, got {}",
                    sysFreqRing.Size()).c_str());
            const auto sysRange = sysRing.GetSerialRange();
            const auto sysFreqRange = sysFreqRing.GetSerialRange();
            const auto sysSample = sysRing.At(sysRange.second - 1);
            const auto sysFreqSample = sysFreqRing.At(sysFreqRange.second - 1);
            Logger::WriteMessage(std::format(
                "SystemOnlyLeavesGpuEmpty: cpu_util val={} ts={} cpu_freq val={} ts={}\n",
                sysSample.value, sysSample.timestamp, sysFreqSample.value, sysFreqSample.timestamp).c_str());
            Assert::IsTrue(sysSample.value > 1.,
                std::format(L"Expected cpu utilization > 1, got {}", sysSample.value).c_str());
            Assert::IsTrue(sysSample.value < 100.,
                std::format(L"Expected cpu utilization < 100, got {}", sysSample.value).c_str());
            Assert::IsTrue(sysFreqSample.value > 1500.,
                std::format(L"Expected cpu frequency > 1500, got {}", sysFreqSample.value).c_str());
            Assert::IsTrue(sysFreqSample.value < 6000.,
                std::format(L"Expected cpu frequency < 6000, got {}", sysFreqSample.value).c_str());
            Assert::AreEqual(0ull, gpuRing.Size(),
                std::format(L"Expected gpu temperature ring size == 0, got {}", gpuRing.Size()).c_str());
        }
        TEST_METHOD(GpuOnlyLeavesSystemEmpty)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            client.DispatchSync(svc::acts::SetTelemetryPeriod::Params{ .telemetrySamplePeriodMs = 350 });
            Logger::WriteMessage("GpuOnlyLeavesSystemEmpty: telemetry period set to 350ms\n");

            const uint32_t TargetDeviceID = 1;

            client.DispatchSync(svc::acts::ReportMetricUse::Params{ {
                { PM_METRIC_GPU_TEMPERATURE, TargetDeviceID, 0 },
                { PM_METRIC_GPU_POWER, TargetDeviceID, 0 },
            } });
            Logger::WriteMessage("GpuOnlyLeavesSystemEmpty: reported GPU temperature/power usage\n");

            auto& sys = pComms->GetSystemDataStore();
            auto& gpu = pComms->GetGpuDataStore(TargetDeviceID);

            auto& gpuRing = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_TEMPERATURE).at(0);
            auto& gpuPowerRing = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_POWER).at(0);
            auto& sysRing = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_UTILIZATION).at(0);
            std::this_thread::sleep_for(1500ms);
            const auto logRing = [](const char* label, const ipc::SampleHistoryRing<double>& ring) {
                const auto range = ring.GetSerialRange();
                Logger::WriteMessage(std::format(
                    "{}: serial [{}, {}) count={}\n",
                    label, range.first, range.second, range.second - range.first).c_str());
                for (size_t s = range.first; s < range.second; ++s) {
                    const auto& sample = ring.At(s);
                    Logger::WriteMessage(std::format(
                        "{}[{}]: val={} ts={}\n",
                        label, s, sample.value, sample.timestamp).c_str());
                }
            };
            Logger::WriteMessage(std::format(
                "GpuOnlyLeavesSystemEmpty: sizes gpu_temp={} gpu_power={} cpu_util={}\n",
                gpuRing.Size(), gpuPowerRing.Size(), sysRing.Size()).c_str());
            logRing("gpu_temp", gpuRing);
            logRing("gpu_power", gpuPowerRing);
            logRing("cpu_util", sysRing);
            Assert::IsTrue(gpuRing.Size() >= 3,
                std::format(L"Expected gpu temperature ring to have >= 3 samples, got {}",
                    gpuRing.Size()).c_str());
            Assert::IsTrue(gpuPowerRing.Size() >= 3,
                std::format(L"Expected gpu power ring to have >= 3 samples, got {}",
                    gpuPowerRing.Size()).c_str());
            const auto gpuRange = gpuRing.GetSerialRange();
            const auto gpuPowerRange = gpuPowerRing.GetSerialRange();
            const auto gpuSample = gpuRing.At(gpuRange.second - 1);
            const auto gpuPowerSample = gpuPowerRing.At(gpuPowerRange.second - 1);
            Logger::WriteMessage(std::format(
                "GpuOnlyLeavesSystemEmpty: gpu_temp val={} ts={} gpu_power val={} ts={}\n",
                gpuSample.value, gpuSample.timestamp, gpuPowerSample.value, gpuPowerSample.timestamp).c_str());
            Assert::IsTrue(gpuSample.value > 10.,
                std::format(L"Expected gpu temperature > 10, got {}", gpuSample.value).c_str());
            Assert::IsTrue(gpuSample.value < 120.,
                std::format(L"Expected gpu temperature < 120, got {}", gpuSample.value).c_str());
            Assert::IsTrue(gpuPowerSample.value > 1.,
                std::format(L"Expected gpu power > 1, got {}", gpuPowerSample.value).c_str());
            Assert::IsTrue(gpuPowerSample.value < 600.,
                std::format(L"Expected gpu power < 600, got {}", gpuPowerSample.value).c_str());
            Assert::AreEqual(0ull, sysRing.Size(),
                std::format(L"Expected cpu utilization ring size == 0, got {}", sysRing.Size()).c_str());
        }
    };

    TEST_CLASS(FrameStoreRealtimeTests)
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

            // launch target and track it
            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            // open the store
            pComms->OpenFrameDataStore(pres.GetId());

            // verify static data
            auto& store = pComms->GetFrameDataStore(pres.GetId());
            Assert::AreEqual(pres.GetId(), store.bookkeeping.processId);
            const std::string staticAppName = store.statics.applicationName.c_str();
            Assert::AreEqual("PresentBench.exe"s, staticAppName);
        }
        TEST_METHOD(TrackUntrack)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // launch target and track it
            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            // verify the store exists
            pComms->OpenFrameDataStore(pres.GetId());

            // verify the service tracking, as expected
            {
                const auto sta = fixture_.service->QueryStatus();
                Assert::AreEqual(1ull, sta.trackedPids.size());
                Assert::IsTrue(sta.trackedPids.contains(pres.GetId()));
                Assert::AreEqual(1ull, sta.frameStorePids.size());
                Assert::IsTrue(sta.frameStorePids.contains(pres.GetId()));
            }

            // stop tracking
            client.DispatchSync(svc::acts::StopTracking::Params{ .targetPid = pres.GetId() });

            // close the segment
            pComms->CloseFrameDataStore(pres.GetId());

            // verify the service not tracking, as expected
            {
                const auto sta = fixture_.service->QueryStatus();
                Assert::AreEqual(0ull, sta.trackedPids.size());
                Assert::AreEqual(0ull, sta.frameStorePids.size());
            }

            // verify segment can no longer be opened
            Assert::ExpectException<std::exception>([&] {pComms->OpenFrameDataStore(pres.GetId()); });
        }
        // make sure we get frames over time
        TEST_METHOD(ReadFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // launch target and track it
            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            // make sure the flush period propagates to the flusher thread
            std::this_thread::sleep_for(1ms);
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            // open the store
            pComms->OpenFrameDataStore(pres.GetId());
            auto& frames = pComms->GetFrameDataStore(pres.GetId()).frameData;

            // sleep here to let the presenter init, etw system warm up, and frames propagate
            std::this_thread::sleep_for(550ms);

            // verify that frames are added over time
            const auto range1 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range1.first, range1.second).c_str());
            std::this_thread::sleep_for(100ms);
            const auto range2 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range2.first, range2.second).c_str());
            std::this_thread::sleep_for(100ms);
            const auto range3 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range3.first, range3.second).c_str());

            Assert::IsTrue(range1.second - range1.first < range2.second - range2.first);
            Assert::IsTrue(range2.second - range2.first < range3.second - range3.first);
        }
    };

    TEST_CLASS(FrameStoreRealtimeWrapTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--frame-ring-samples"s, "16"s,
            });
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }
        TEST_METHOD(WrapNoMissingFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            std::this_thread::sleep_for(1ms);
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            pComms->OpenFrameDataStore(pres.GetId());
            auto& ring = pComms->GetFrameDataStore(pres.GetId()).frameData;

            std::this_thread::sleep_for(200ms);

            size_t lastProcessed = 0;
            bool missed = false;
            bool sawWrap = false;
            bool hasTimestamp = false;
            uint64_t lastTimestamp = 0;

            for (size_t i = 0; i < 60; ++i) {
                std::this_thread::sleep_for(25ms);
                const auto range = ring.GetSerialRange();
                Logger::WriteMessage(std::format(
                    "rt-wrap-no-miss: range [{}, {}), lastProcessed={}\n",
                    range.first, range.second, lastProcessed).c_str());
                if (range.first > 0) {
                    sawWrap = true;
                }
                if (range.first > lastProcessed) {
                    missed = true;
                }
                const size_t start = (std::max)(lastProcessed, range.first);
                for (size_t s = start; s < range.second; ++s) {
                    const auto& frame = ring.At(s);
                    const uint64_t stamp = frame.presentStartTime + frame.timeInPresent;
                    if (hasTimestamp) {
                        Assert::IsTrue(stamp >= lastTimestamp);
                    }
                    lastTimestamp = stamp;
                    hasTimestamp = true;
                }
                lastProcessed = range.second;
            }

            Assert::IsTrue(sawWrap, L"Expected ring to wrap");
            Assert::IsFalse(missed, L"Expected no missing frames with frequent reads");
            Assert::IsTrue(lastProcessed > 0);
        }
        TEST_METHOD(WrapMissingFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            std::this_thread::sleep_for(1ms);
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });

            pComms->OpenFrameDataStore(pres.GetId());
            auto& ring = pComms->GetFrameDataStore(pres.GetId()).frameData;

            auto range = ring.GetSerialRange();
            for (size_t i = 0; i < 20 && range.first == 0; ++i) {
                std::this_thread::sleep_for(100ms);
                range = ring.GetSerialRange();
            }
            Logger::WriteMessage(std::format(
                "rt-wrap-miss: range [{}, {})\n", range.first, range.second).c_str());

            Assert::IsTrue(range.first > 0, L"Expected missing frames after delay");
            Assert::IsTrue(range.second > range.first);
        }
    };

    TEST_CLASS(FrameStorePacedPlaybackTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P00HeaWin2080.etl)",
                "--pace-playback"s,
            });
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

            // track known target
            const uint32_t pid = 12820;
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pid, .isPlayback = true });

            // open the store
            pComms->OpenFrameDataStore(pid);

            // wait for population of frame data-initialized statics
            std::this_thread::sleep_for(500ms);

            // verify static data
            auto& store = pComms->GetFrameDataStore(pid);
            Assert::AreEqual(pid, store.bookkeeping.processId);
            const std::string staticAppName = store.statics.applicationName.c_str();
            Assert::AreEqual("Heaven.exe"s, staticAppName);
        }
        // make sure we get frames over time
        TEST_METHOD(ReadFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // set up a fast flush
            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            // make sure the flush period propagates to the flusher thread
            std::this_thread::sleep_for(1ms);
            // we know the pid of interest in this etl file, track it
            const uint32_t pid = 12820;
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pid, .isPlayback = true });

            // open the store
            pComms->OpenFrameDataStore(pid);
            auto& frames = pComms->GetFrameDataStore(pid).frameData;

            // sleep here to let the etw system warm up, and frames propagate
            std::this_thread::sleep_for(450ms);

            // verify that frames are added over time
            const auto range1 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range1.first, range1.second).c_str());
            std::this_thread::sleep_for(100ms);
            const auto range2 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range2.first, range2.second).c_str());
            std::this_thread::sleep_for(100ms);
            const auto range3 = frames.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range3.first, range3.second).c_str());

            Assert::IsTrue(range1.second - range1.first < range2.second - range2.first);
            Assert::IsTrue(range2.second - range2.first < range3.second - range3.first);
        }
    };
    
    TEST_CLASS(FrameStoreBackpressuredPlaybackTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P00HeaWin2080.etl)"s
            });
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

            // track known target
            const uint32_t pid = 12820;
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pid, .isPlayback = true });

            // open the store
            pComms->OpenFrameDataStore(pid);

            // wait for population of frame data-initialized statics
            std::this_thread::sleep_for(500ms);

            // verify static data
            auto& store = pComms->GetFrameDataStore(pid);
            Assert::AreEqual(pid, store.bookkeeping.processId);
            const std::string staticAppName = store.statics.applicationName.c_str();
            Assert::AreEqual("Heaven.exe"s, staticAppName);
        }
        // make sure we get frames over time
        TEST_METHOD(ReadFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            // set up a fast flush
            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            // make sure the flush period propagates to the flusher thread
            std::this_thread::sleep_for(1ms);

            // we know the pid of interest in this etl file, track it
            const uint32_t pid = 12820;
            client.DispatchSync(svc::acts::StartTracking::Params{
                .targetPid = pid, .isPlayback = true, .isBackpressured = true });

            // open the store
            pComms->OpenFrameDataStore(pid);
            auto& ring = pComms->GetFrameDataStore(pid).frameData;

            // sleep here to let the etw system warm up, and frames propagate
            std::this_thread::sleep_for(300ms);

            struct Row { uint64_t timestamp; uint64_t timeInPresent; };
            std::vector<Row> frames;
            uint64_t lastProcessed = 0;

            const auto appendRange = [&](std::pair<uint64_t, uint64_t> range) {
                for (uint64_t s = (std::max)(lastProcessed, range.first); s < range.second; ++s) {
                    auto& p = ring.At(s);
                    frames.push_back(Row{
                        .timestamp = p.presentStartTime + p.timeInPresent,
                        .timeInPresent = p.timeInPresent,
                        });
                }
                lastProcessed = range.second;
            };

            const auto range1 = ring.GetSerialRange();
            ring.MarkNextRead(range1.second);
            Logger::WriteMessage(std::format("range [{},{})\n", range1.first, range1.second).c_str());
            appendRange(range1);

            std::this_thread::sleep_for(300ms);

            const auto range2 = ring.GetSerialRange();
            ring.MarkNextRead(range2.second);
            Logger::WriteMessage(std::format("range [{},{})\n", range2.first, range2.second).c_str());
            appendRange(range2);

            std::this_thread::sleep_for(500ms);

            const auto range3 = ring.GetSerialRange();
            ring.MarkNextRead(range3.second);
            Logger::WriteMessage(std::format("range [{},{})\n", range3.first, range3.second).c_str());
            appendRange(range3);

            // output timestamp of each frame
            const auto outpath = fs::path{ outFolder_ } /
                std::format("broadcaster-frames-{:%Y%m%d-%H%M%S}.csv", std::chrono::system_clock::now());
            Logger::WriteMessage(std::format("Writing output to: {}\n",
                fs::absolute(outpath).string()).c_str());

            std::ofstream frameFile{ outpath };
            frameFile << "timestamp,timeInPresent\n";
            for (const auto& r : frames) {
                frameFile << r.timestamp << ',' << r.timeInPresent << "\n";
            }

            Assert::AreEqual(0ull, range1.first);
            Assert::IsTrue(range2.first <= range1.second);
            Assert::IsTrue(range3.first <= range2.second);
            // known issue with PresentData is that it sometimes outputs 24 rogue frames at
            // the end for P00; we can ignore these for the time being, issue added to board
            Assert::IsTrue(range3.second == 1905ull || range3.second == 1929ull);
        }
    };

    TEST_CLASS(FrameStorePlaybackBackpressureWrapTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P01TimeSpyDemoFS2080.etl)"s,
                "--frame-ring-samples"s, "32"s,
            });
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }
        TEST_METHOD(BackpressurePreventsMissingFrames)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());

            client.DispatchSync(svc::acts::SetEtwFlushPeriod::Params{ .etwFlushPeriodMs = 8 });
            std::this_thread::sleep_for(1ms);

            const uint32_t pid = 19736;
            client.DispatchSync(svc::acts::StartTracking::Params{
                .targetPid = pid, .isPlayback = true, .isBackpressured = true });

            pComms->OpenFrameDataStore(pid);
            auto& ring = pComms->GetFrameDataStore(pid).frameData;

            size_t lastProcessed = 0;
            bool missed = false;
            bool sawWrap = false;

            for (size_t i = 0; i < 10; ++i) {
                std::this_thread::sleep_for(300ms);
                const auto range = ring.GetSerialRange();
                Logger::WriteMessage(std::format(
                    "pb-wrap-backpressure: range [{}, {}), lastProcessed={}\n",
                    range.first, range.second, lastProcessed).c_str());
                if (range.first > 0) {
                    sawWrap = true;
                }
                if (range.first > lastProcessed) {
                    missed = true;
                }
                const size_t start = (std::max)(lastProcessed, range.first);
                for (size_t s = start; s < range.second; ++s) {
                    (void)ring.At(s);
                }
                ring.MarkNextRead(range.second);
                lastProcessed = range.second;
            }

            Assert::IsTrue(sawWrap, L"Expected ring to wrap during playback");
            Assert::IsFalse(missed, L"Expected backpressure to prevent missing frames");
            Assert::IsTrue(lastProcessed > 0);
        }
    };

    TEST_CLASS(FrameStoreBackpressuredPlayback3DMTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P01TimeSpyDemoFS2080.etl)"s
                });
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }
        TEST_METHOD(ReadFrames)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };

            // set up a fast flush
            session.SetEtwFlushPeriod(8);
            // make sure the flush period propagates to the flusher thread
            std::this_thread::sleep_for(1ms);

            // setup query
            PM_BEGIN_FIXED_FRAME_QUERY(FQ)
                pmapi::FixedQueryElement timestamp{ this, PM_METRIC_CPU_START_QPC };
            pmapi::FixedQueryElement timeInPres{ this, PM_METRIC_IN_PRESENT_API };
            PM_END_FIXED_QUERY query{ session, 1'000 };

            struct Row { uint64_t timestamp; double timeInPresent; };
            std::vector<Row> frames;

            // we know the pid of interest in this etl file, track it
            const uint32_t pid = 19736;
            auto tracker = query.TrackProcess(pid, true, true);

            // sleep here to let the etw system warm up, and frames propagate
            std::this_thread::sleep_for(300ms);

            const auto consume = [&] {
                query.ForEachConsume(tracker, [&] {
                    frames.push_back(Row{
                        .timestamp = query.timestamp,
                        .timeInPresent = query.timeInPres,
                        });
                });
            };

            // verify that backpressure works correctly to ensure no frames are lost
            consume();
            const auto count1 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count1).c_str());

            std::this_thread::sleep_for(300ms);

            consume();
            const auto count2 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count2).c_str());

            std::this_thread::sleep_for(500ms);

            consume();
            const auto count3 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count3).c_str());

            // output timestamp of each frame
            const auto outpath = fs::path{ outFolder_ } /
                std::format("legacy-frames-32m-{:%Y%m%d-%H%M%S}.csv", std::chrono::system_clock::now());
            Logger::WriteMessage(std::format("Writing output to: {}\n",
                fs::absolute(outpath).string()).c_str());

            std::ofstream frameFile{ outpath };
            frameFile << "timestamp,timeInPresent\n";
            for (const auto& r : frames) {
                frameFile << r.timestamp << ',' << r.timeInPresent << "\n";
            }

            Assert::AreEqual(2037u, count1 + count2 + count3);
        }
    };
    
    TEST_CLASS(LegacyBackpressuredPlaybackTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P00HeaWin2080.etl)"s,
                });
        }
        TEST_METHOD_CLEANUP(Cleanup)
        {
            fixture_.Cleanup();
        }
        // make sure we get frames over time
        TEST_METHOD(ReadFrames)
        {
            pmapi::Session session{ fixture_.GetCommonArgs().ctrlPipe };

            // set up a fast flush
            session.SetEtwFlushPeriod(8);
            // make sure the flush period propagates to the flusher thread
            std::this_thread::sleep_for(1ms);

            // setup query
            PM_BEGIN_FIXED_FRAME_QUERY(FQ)
                pmapi::FixedQueryElement timestamp{ this, PM_METRIC_CPU_START_QPC };
                pmapi::FixedQueryElement timeInPres{ this, PM_METRIC_IN_PRESENT_API };
            PM_END_FIXED_QUERY query{ session, 1'000 };

            struct Row { uint64_t timestamp; double timeInPresent; };
            std::vector<Row> frames;

            // we know the pid of interest in this etl file, track it
            const uint32_t pid = 12820;
            auto tracker = query.TrackProcess(pid, true, true);

            // sleep here to let the etw system warm up, and frames propagate
            std::this_thread::sleep_for(300ms);

            const auto consume = [&] {
                query.ForEachConsume(tracker, [&] {
                    frames.push_back(Row{
                        .timestamp = query.timestamp,
                        .timeInPresent = query.timeInPres,
                        });
                });
            };

            // verify that backpressure works correctly to ensure no frames are lost
            consume();
            const auto count1 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count1).c_str());

            std::this_thread::sleep_for(300ms);

            consume();
            const auto count2 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count2).c_str());

            std::this_thread::sleep_for(500ms);

            consume();
            const auto count3 = query.PeekBlobContainer().GetNumBlobsPopulated();
            Logger::WriteMessage(std::format("count [{}]\n", count3).c_str());

            // output timestamp of each frame
            const auto outpath = fs::path{ outFolder_ } /
                std::format("legacy-frames-{:%Y%m%d-%H%M%S}.csv", std::chrono::system_clock::now());
            Logger::WriteMessage(std::format("Writing output to: {}\n",
                fs::absolute(outpath).string()).c_str());

            std::ofstream frameFile{ outpath };
            frameFile << "timestamp,timeInPresent\n";
            for (const auto& r : frames) {
                frameFile << r.timestamp << ',' << r.timeInPresent << "\n";
            }

            const auto total = count1 + count2 + count3;
            // known issue with PresentData is that it sometimes outputs 24 rogue frames at
            // the end for P00; we can ignore these for the time being, issue added to board
            Logger::WriteMessage(std::format("Total frames: {}\n", total).c_str());
            Assert::IsTrue(total == 1902u);
        }
    };
}
