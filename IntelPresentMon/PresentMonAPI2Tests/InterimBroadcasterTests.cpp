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

    TEST_CLASS(SystemStoreTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({ "--new-telemetry-activation"s });
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

            // get the store containing system-wide telemetry (cpu etc.)
            auto& sys = pComms->GetSystemDataStore();
            for (auto&& [met, r] : sys.telemetryData.Rings()) {
                Logger::WriteMessage(std::format(" TeleRing@{}\n", pMetricMap->at(met).narrowName).c_str());
                // TODO: understand the disconnect between CPU Core Utility showing up here
                // and not showing up in the UI (update: it is blacklisted manually in UI introspection)
            }
            Assert::AreEqual(2ull, (size_t)rn::distance(sys.telemetryData.Rings()));

            // system device id constant
            const uint32_t SystemDeviceID = 65536;

            // update server with metric/device usage information
            // this will trigger system telemetry collection
            client.DispatchSync(svc::acts::ReportMetricUse::Params{ {
                { PM_METRIC_CPU_UTILIZATION, SystemDeviceID, 0 },
                { PM_METRIC_CPU_FREQUENCY, SystemDeviceID, 0 },
            } });

            // allow warm-up period
            std::this_thread::sleep_for(150ms);

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

            // build the set of expected rings from introspection
            Logger::WriteMessage("Introspection Metrics\n=====================\n");
            std::map<PM_METRIC, size_t> introspectionAvailability;
            const auto TryAddMetric = [&](PM_METRIC metric) {
                auto&& m = intro.FindMetric(metric);
                if (m.GetDeviceMetricInfo().empty()) {
                    return;
                }
                auto&& dmi = m.GetDeviceMetricInfo().front();
                if (dmi.IsAvailable()) {
                    const auto arraySize = dmi.GetArraySize();
                    introspectionAvailability[metric] = arraySize;
                    // dump for review in output pane
                    Logger::WriteMessage(std::format("[{}] {}\n", arraySize,
                        pMetricMap->at(m.GetId()).narrowName).c_str());
                }
            };
            // TODO: replace this with code that iterates over all metrics and automatically
            // evaluates all cpu telemetry metrics
            TryAddMetric(PM_METRIC_CPU_POWER);
            TryAddMetric(PM_METRIC_CPU_TEMPERATURE);
            TryAddMetric(PM_METRIC_CPU_UTILIZATION);
            TryAddMetric(PM_METRIC_CPU_FREQUENCY);
            TryAddMetric(PM_METRIC_CPU_CORE_UTILITY);
            Logger::WriteMessage(std::format("Total: {}\n", introspectionAvailability.size()).c_str());

            // validate that the expected number of rings sets are present in the store
            Assert::AreEqual(introspectionAvailability.size(), (size_t)rn::distance(sys.telemetryData.Rings()));

            // validate that exepected rings are present and are populated with samples
            for (auto&& [met, size] : introspectionAvailability) {
                // array sizes should match
                Assert::AreEqual(size, sys.telemetryData.ArraySize(met),
                    pMetricMap->at(met).wideName.c_str());
                std::visit([&](auto const& rings) {
                    // for each history ring in set, make sure it has at least one sample in it
                    for (size_t i = 0; i < size; i++) {
                        auto& name = pMetricMap->at(met).wideName;
                        Assert::IsFalse(rings[i].Empty(),
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
            fixture_.Setup({"--new-telemetry-activation"s});
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
                // some polled metrics are derived in middleware thus not present in shm
                if (m.GetId() == PM_METRIC_GPU_MEM_UTILIZATION ||
                    m.GetId() == PM_METRIC_GPU_FAN_SPEED_PERCENT) {
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
            Assert::AreEqual(pres.GetId(), store.statics.processId);
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
            }

            // stop tracking
            client.DispatchSync(svc::acts::StopTracking::Params{ .targetPid = pres.GetId() });

            // close the segment
            pComms->CloseFrameDataStore(pres.GetId());

            // verify the service not tracking, as expected
            Assert::AreEqual(0ull, fixture_.service->QueryStatus().trackedPids.size());

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
            Assert::AreEqual(pid, store.statics.processId);
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
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P00HeaWin2080.etl)"s,
                "--disable-legacy-backpressure"s
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
            Assert::AreEqual(pid, store.statics.processId);
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
            Assert::AreEqual(1905ull, range3.second);
            // known issue with PresentData is that it sometimes outputs 24 rogue frames at
            // the end for P00; we can ignore these for the time being, issue added to board
            Assert::IsTrue(range3.second == 1905ull || range3.second == 1929ull);
        }
    };

    TEST_CLASS(FrameStoreBackpressuredPlayback3DMTests)
    {
        TestFixture fixture_;
    public:
        TEST_METHOD_INITIALIZE(Setup)
        {
            fixture_.Setup({
                "--etl-test-file"s, R"(..\..\Tests\AuxData\Data\P01TimeSpyDemoFS2080.etl)"s,
                "--disable-legacy-backpressure"s
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
            auto tracker = session.TrackProcess(pid);

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
            auto tracker = session.TrackProcess(pid);

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
            Assert::IsTrue(total == 1903u || total == 1927u);
        }
    };
}
