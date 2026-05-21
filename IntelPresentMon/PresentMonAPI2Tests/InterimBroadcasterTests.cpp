// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/Env.h"
#include "../CommonUtilities/PrecisionWaiter.h"
#include "CppUnitTest.h"
#include "FirstFrameWait.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include <string>
#include <ranges>
#include <fstream>
#include "Folders.h"
#include "JobManager.h"
#include "MachineExpectations.h"

#include "../PresentMonMiddleware/ActionClient.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapper/FixedQuery.h"
#include "../PresentMonService/AllActions.h"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>

#include <format>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace vi = std::views;
namespace rn = std::ranges;
using namespace std::literals;
using namespace pmon;
namespace machine = pmon::tests::machine;

namespace InterimBroadcasterTests
{
    static void AssertSegmentRejectsWrite_(const std::string& segmentName)
    {
        ipc::bip::windows_shared_memory readOnlyShm{
            ipc::bip::open_only,
            segmentName.c_str(),
            ipc::bip::read_only
        };
        ipc::bip::mapped_region readOnlyRegion{ readOnlyShm, ipc::bip::read_only };
        Assert::IsTrue(readOnlyRegion.get_address() != nullptr);

        Assert::ExpectException<std::exception>([&] {
            ipc::bip::windows_shared_memory readWriteShm{
                ipc::bip::open_only,
                segmentName.c_str(),
                ipc::bip::read_write
            };
            ipc::bip::mapped_region readWriteRegion{ readWriteShm, ipc::bip::read_write };
            auto pData = static_cast<volatile char*>(readWriteRegion.get_address());
            *pData = *pData;
        });
    }

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

    static bool IsMetricAvailableForDevice_(const pmapi::intro::Root& intro,
        PM_METRIC metricId, uint32_t deviceId, uint32_t arrayIndex = 0)
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
            Assert::IsFalse((bool)status.etwFlushPeriodMs);
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
            Assert::AreEqual(13ull, pIntro->pEnums->size);
            auto pEnum = static_cast<const PM_INTROSPECTION_ENUM*>(pIntro->pEnums->pData[0]);
            Assert::AreEqual("PM_STATUS", pEnum->pSymbol->pData);
        }
        TEST_METHOD(IntrospectionSegmentRejectsWrite)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            const ipc::ShmNamer namer{ client.GetShmPrefix(), client.GetShmSalt() };
            AssertSegmentRejectsWrite_(namer.MakeIntrospectionName());
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
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.SystemStoreTests.StaticData" };
            measurements.AddSystem("PM_METRIC_CPU_VENDOR", (double)sys.statics.cpuVendor);
            measurements.AddSystem("PM_METRIC_CPU_NAME", sys.statics.cpuName.c_str());
            measurements.AddSystem("PM_METRIC_CPU_POWER_LIMIT", sys.statics.cpuPowerLimit);
            measurements.AppendToSharedFile();

            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);
        }
        TEST_METHOD(SystemStoreRejectsWrite)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            const ipc::ShmNamer namer{ client.GetShmPrefix(), client.GetShmSalt() };
            (void)pComms->GetSystemDataStore();
            AssertSegmentRejectsWrite_(namer.MakeSystemName());
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
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.SystemStoreTests.PolledData" };
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(250ms);
                {
                    constexpr auto m = PM_METRIC_CPU_UTILIZATION;
                    auto& rings = sys.telemetryData.FindRing<double>(m);
                    if (!IsMetricAvailableForDevice_(intro, m, ipc::kSystemDeviceId)) {
                        measurements.AddSystemIntrospectionUnavailable("PM_METRIC_CPU_UTILIZATION");
                    }
                    else {
                        auto& r = rings[0];
                        if (r.Empty()) {
                            measurements.AddSystemUnavailable("PM_METRIC_CPU_UTILIZATION");
                        }
                        else {
                            if (i == 0 || i == 9) {
                                Logger::WriteMessage(DumpRing_(r).c_str());
                            }
                            auto sample = r.Newest();
                            utilizSamples.push_back(sample);
                            measurements.AddSystem("PM_METRIC_CPU_UTILIZATION", sample.value);
                            Logger::WriteMessage(std::format("({}) {}: {}\n",
                                i, pMetricMap->at(m).narrowName, sample.value).c_str());
                        }
                    }
                }
                {
                    constexpr auto m = PM_METRIC_CPU_FREQUENCY;
                    auto& rings = sys.telemetryData.FindRing<double>(m);
                    if (!IsMetricAvailableForDevice_(intro, m, ipc::kSystemDeviceId)) {
                        measurements.AddSystemIntrospectionUnavailable("PM_METRIC_CPU_FREQUENCY");
                    }
                    else {
                        auto& r = rings[0];
                        if (r.Empty()) {
                            measurements.AddSystemUnavailable("PM_METRIC_CPU_FREQUENCY");
                        }
                        else {
                            if (i == 0 || i == 9) {
                                Logger::WriteMessage(DumpRing_(r).c_str());
                            }
                            auto sample = r.Newest();
                            freqSamples.push_back(sample);
                            measurements.AddSystem("PM_METRIC_CPU_FREQUENCY", sample.value);
                            Logger::WriteMessage(std::format("({}) {}: {}\n",
                                i, pMetricMap->at(m).narrowName, sample.value).c_str());
                        }
                    }
                }
            }

            measurements.AppendToSharedFile();
            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);

            // make sure samples actually change over time
            if (!utilizSamples.empty()) {
                Assert::AreNotEqual(utilizSamples.front().timestamp, utilizSamples.back().timestamp);
                Assert::AreNotEqual(utilizSamples.front().value, utilizSamples.back().value);
            }
            if (!freqSamples.empty()) {
                Assert::AreNotEqual(freqSamples.front().timestamp, freqSamples.back().timestamp);
                Assert::AreNotEqual(freqSamples.front().value, freqSamples.back().value);
            }
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
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.GpuStoreTests.StaticData" };
            measurements.AddGpu(1, "PM_METRIC_GPU_VENDOR", (double)gpu.statics.vendor);
            measurements.AddGpu(1, "PM_METRIC_GPU_NAME", gpu.statics.name.c_str());
            measurements.AddGpu(1, "PM_METRIC_GPU_POWER_LIMIT", gpu.statics.sustainedPowerLimit);
            measurements.AddGpu(1, "PM_METRIC_GPU_MEM_SIZE", (double)gpu.statics.memSize);
            measurements.AppendToSharedFile();

            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);
        }
        TEST_METHOD(GpuStoreRejectsWrite)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            const ipc::ShmNamer namer{ client.GetShmPrefix(), client.GetShmSalt() };
            constexpr uint32_t TargetDeviceID = 1;
            (void)pComms->GetGpuDataStore(TargetDeviceID);
            AssertSegmentRejectsWrite_(namer.MakeGpuName(TargetDeviceID));
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
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.GpuStoreTests.PolledData" };

            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(250ms);

                {
                    constexpr auto m = PM_METRIC_GPU_TEMPERATURE;
                    auto& rings = gpu.telemetryData.FindRing<double>(m);
                    if (!IsMetricAvailableForDevice_(intro, m, TargetDeviceID)) {
                        measurements.AddGpuIntrospectionUnavailable(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE");
                    }
                    else {
                        auto& r = rings[0];
                        if (r.Empty()) {
                            measurements.AddGpuUnavailable(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE");
                        }
                        else {
                            if (i == 0 || i == 9) {
                                Logger::WriteMessage(DumpRing_(r).c_str());
                            }

                            auto sample = r.Newest();
                            tempSamples.push_back(sample);
                            measurements.AddGpu(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE", sample.value);

                            Logger::WriteMessage(std::format("({}) {}: {}\n",
                                i, pMetricMap->at(m).narrowName, sample.value).c_str());
                        }
                    }
                }

                {
                    constexpr auto m = PM_METRIC_GPU_POWER;
                    auto& rings = gpu.telemetryData.FindRing<double>(m);
                    if (!IsMetricAvailableForDevice_(intro, m, TargetDeviceID)) {
                        measurements.AddGpuIntrospectionUnavailable(TargetDeviceID, "PM_METRIC_GPU_POWER");
                    }
                    else {
                        auto& r = rings[0];
                        if (r.Empty()) {
                            measurements.AddGpuUnavailable(TargetDeviceID, "PM_METRIC_GPU_POWER");
                        }
                        else {
                            if (i == 0 || i == 9) {
                                Logger::WriteMessage(DumpRing_(r).c_str());
                            }

                            auto sample = r.Newest();
                            powerSamples.push_back(sample);
                            measurements.AddGpu(TargetDeviceID, "PM_METRIC_GPU_POWER", sample.value);

                            Logger::WriteMessage(std::format("({}) {}: {}\n",
                                i, pMetricMap->at(m).narrowName, sample.value).c_str());
                        }
                    }
                }
            }

            measurements.AppendToSharedFile();
            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);

            // make sure samples actually change over time
            if (!tempSamples.empty()) {
                Assert::AreNotEqual(tempSamples.front().timestamp, tempSamples.back().timestamp);
            }
            if (!powerSamples.empty()) {
                Assert::AreNotEqual(powerSamples.front().timestamp, powerSamples.back().timestamp);
                Assert::AreNotEqual(powerSamples.front().value, powerSamples.back().value);
            }
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
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };

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
            auto& sysRings = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_UTILIZATION);
            auto& sysFreqRings = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_FREQUENCY);
            auto& gpuRings = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_TEMPERATURE);
            const auto sysSize = sysRings[0].Size();
            const auto sysFreqSize = sysFreqRings[0].Size();
            const auto gpuSize = gpuRings[0].Size();
            Logger::WriteMessage(std::format(
                "SystemOnlyLeavesGpuEmpty: sizes cpu_util={} cpu_freq={} gpu_temp={}\n",
                sysSize, sysFreqSize, gpuSize).c_str());
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.NewActivationIsolationTests.SystemOnlyLeavesGpuEmpty" };
            if (!IsMetricAvailableForDevice_(intro, PM_METRIC_CPU_UTILIZATION, ipc::kSystemDeviceId)) {
                measurements.AddSystemIntrospectionUnavailable("PM_METRIC_CPU_UTILIZATION");
            }
            else if (sysSize == 0) {
                measurements.AddSystemUnavailable("PM_METRIC_CPU_UTILIZATION");
            }
            else {
                logRing("cpu_util", sysRings[0]);
                const auto range = sysRings[0].GetSerialRange();
                measurements.AddSystem("PM_METRIC_CPU_UTILIZATION", sysRings[0].At(range.second - 1).value);
            }
            if (!IsMetricAvailableForDevice_(intro, PM_METRIC_CPU_FREQUENCY, ipc::kSystemDeviceId)) {
                measurements.AddSystemIntrospectionUnavailable("PM_METRIC_CPU_FREQUENCY");
            }
            else if (sysFreqSize == 0) {
                measurements.AddSystemUnavailable("PM_METRIC_CPU_FREQUENCY");
            }
            else {
                logRing("cpu_freq", sysFreqRings[0]);
                const auto range = sysFreqRings[0].GetSerialRange();
                measurements.AddSystem("PM_METRIC_CPU_FREQUENCY", sysFreqRings[0].At(range.second - 1).value);
            }
            if (gpuSize != 0) {
                logRing("gpu_temp", gpuRings[0]);
            }
            measurements.AppendToSharedFile();
            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);
            Assert::AreEqual(0ull, gpuSize,
                std::format(L"Expected gpu temperature ring size == 0, got {}", gpuSize).c_str());
        }
        TEST_METHOD(GpuOnlyLeavesSystemEmpty)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            auto pIntro = pComms->GetIntrospectionRoot();
            pmapi::intro::Root intro{ pIntro, [](auto* p) { delete p; } };

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
            auto& gpuRings = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_TEMPERATURE);
            auto& gpuPowerRings = gpu.telemetryData.FindRing<double>(PM_METRIC_GPU_POWER);
            auto& sysRings = sys.telemetryData.FindRing<double>(PM_METRIC_CPU_UTILIZATION);
            const auto gpuSize = gpuRings[0].Size();
            const auto gpuPowerSize = gpuPowerRings[0].Size();
            const auto sysSize = sysRings[0].Size();
            Logger::WriteMessage(std::format(
                "GpuOnlyLeavesSystemEmpty: sizes gpu_temp={} gpu_power={} cpu_util={}\n",
                gpuSize, gpuPowerSize, sysSize).c_str());
            machine::MeasurementSet measurements{ "InterimBroadcasterTests.NewActivationIsolationTests.GpuOnlyLeavesSystemEmpty" };
            if (!IsMetricAvailableForDevice_(intro, PM_METRIC_GPU_TEMPERATURE, TargetDeviceID)) {
                measurements.AddGpuIntrospectionUnavailable(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE");
            }
            else if (gpuSize == 0) {
                measurements.AddGpuUnavailable(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE");
            }
            else {
                logRing("gpu_temp", gpuRings[0]);
                const auto range = gpuRings[0].GetSerialRange();
                measurements.AddGpu(TargetDeviceID, "PM_METRIC_GPU_TEMPERATURE", gpuRings[0].At(range.second - 1).value);
            }
            if (!IsMetricAvailableForDevice_(intro, PM_METRIC_GPU_POWER, TargetDeviceID)) {
                measurements.AddGpuIntrospectionUnavailable(TargetDeviceID, "PM_METRIC_GPU_POWER");
            }
            else if (gpuPowerSize == 0) {
                measurements.AddGpuUnavailable(TargetDeviceID, "PM_METRIC_GPU_POWER");
            }
            else {
                logRing("gpu_power", gpuPowerRings[0]);
                const auto range = gpuPowerRings[0].GetSerialRange();
                measurements.AddGpu(TargetDeviceID, "PM_METRIC_GPU_POWER", gpuPowerRings[0].At(range.second - 1).value);
            }
            if (sysSize != 0) {
                logRing("cpu_util", sysRings[0]);
            }
            measurements.AppendToSharedFile();
            const auto expectations = machine::ExpectationStore::Load();
            if (expectations.HasUnavailableExpectation(measurements)) {
                return;
            }
            expectations.AssertMeasurements(measurements);
            Assert::AreEqual(0ull, sysSize,
                std::format(L"Expected cpu utilization ring size == 0, got {}", sysSize).c_str());
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
        TEST_METHOD(FrameStoreRejectsWrite)
        {
            mid::ActionClient client{ fixture_.GetCommonArgs().ctrlPipe };
            auto pComms = ipc::MakeMiddlewareComms(client.GetShmPrefix(), client.GetShmSalt());
            const ipc::ShmNamer namer{ client.GetShmPrefix(), client.GetShmSalt() };

            auto pres = fixture_.LaunchPresenter();
            client.DispatchSync(svc::acts::StartTracking::Params{ .targetPid = pres.GetId() });
            pComms->OpenFrameDataStore(pres.GetId());

            AssertSegmentRejectsWrite_(namer.MakeFrameName(pres.GetId()));
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
                "--frame-ring-samples"s, "64"s,
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
            bool hasPreviousFrame = false;
            ipc::FrameData previousFrame{};
            uint64_t previousStamp = 0;
            size_t previousSerial = 0;
            util::PrecisionWaiter waiter;

            for (size_t i = 0; i < 229; ++i) {
                waiter.Wait(0.008);
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
                    const auto frame = ring.At(s);
                    const uint64_t stamp = frame.presentStartTime + frame.timeInPresent;
                    if (hasPreviousFrame) {
                        const bool frameIdInOrder = frame.frameId >= previousFrame.frameId;
                        const bool timestampInOrder = stamp >= previousStamp;
                        const auto assertMessage = std::format(
                            "Frame order assert failed: expected current frameId >= previous frameId and current stamp >= previous stamp.\n"
                            "Checks: frameId {} >= {} is {}; stamp {} >= {} is {}.\n"
                            "Previous: serial={} frameId={} appFrameId={} pid={} tid={} swapChain=0x{:X} presentStartTime={} timeInPresent={} stamp={}\n"
                            "Current:  serial={} frameId={} appFrameId={} pid={} tid={} swapChain=0x{:X} presentStartTime={} timeInPresent={} stamp={}",
                            frame.frameId, previousFrame.frameId, frameIdInOrder,
                            stamp, previousStamp, timestampInOrder,
                            previousSerial, previousFrame.frameId, previousFrame.appFrameId,
                            previousFrame.processId, previousFrame.threadId, previousFrame.swapChainAddress,
                            previousFrame.presentStartTime, previousFrame.timeInPresent, previousStamp,
                            s, frame.frameId, frame.appFrameId, frame.processId, frame.threadId,
                            frame.swapChainAddress, frame.presentStartTime, frame.timeInPresent, stamp);
                        const std::wstring wideAssertMessage{ assertMessage.begin(), assertMessage.end() };
                        Assert::IsTrue(frameIdInOrder && timestampInOrder, wideAssertMessage.c_str());
                    }
                    previousFrame = frame;
                    previousStamp = stamp;
                    previousSerial = s;
                    hasPreviousFrame = true;
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

            pmon::tests::WaitForFirstFrame(ring, "backpressured-playback");

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
            Logger::WriteMessage(std::format("range [{},{})\n", range1.first, range1.second).c_str());
            appendRange(range1);
            client.DispatchDetached(svc::acts::ReportFrameReadProgress::Params{
                .targetPid = pid,
                .nextReadSerial = range1.second,
            });

            std::this_thread::sleep_for(300ms);

            const auto range2 = ring.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range2.first, range2.second).c_str());
            appendRange(range2);
            client.DispatchDetached(svc::acts::ReportFrameReadProgress::Params{
                .targetPid = pid,
                .nextReadSerial = range2.second,
            });

            std::this_thread::sleep_for(500ms);

            const auto range3 = ring.GetSerialRange();
            Logger::WriteMessage(std::format("range [{},{})\n", range3.first, range3.second).c_str());
            appendRange(range3);
            client.DispatchDetached(svc::acts::ReportFrameReadProgress::Params{
                .targetPid = pid,
                .nextReadSerial = range3.second,
            });

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

            pmon::tests::WaitForFirstFrame(ring, "pb-wrap-backpressure");

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
                client.DispatchDetached(svc::acts::ReportFrameReadProgress::Params{
                    .targetPid = pid,
                    .nextReadSerial = range.second,
                });
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
            pmon::tests::WaitForFirstFrame(fixture_.GetCommonArgs().ctrlPipe, pid, "backpressured-playback-3dm");

            const auto consume = [&] {
                return uint32_t(query.ForEachConsume(tracker, [&] {
                    frames.push_back(Row{
                        .timestamp = query.timestamp,
                        .timeInPresent = query.timeInPres,
                        });
                }));
            };

            // verify that backpressure works correctly to ensure no frames are lost
            const auto count1 = consume();
            Logger::WriteMessage(std::format("count [{}]\n", count1).c_str());

            std::this_thread::sleep_for(300ms);

            const auto count2 = consume();
            Logger::WriteMessage(std::format("count [{}]\n", count2).c_str());

            std::this_thread::sleep_for(500ms);

            const auto count3 = consume();
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
            pmon::tests::WaitForFirstFrame(fixture_.GetCommonArgs().ctrlPipe, pid, "legacy-backpressured-playback");

            const auto consume = [&] {
                return uint32_t(query.ForEachConsume(tracker, [&] {
                    frames.push_back(Row{
                        .timestamp = query.timestamp,
                        .timeInPresent = query.timeInPres,
                        });
                }));
            };

            // verify that backpressure works correctly to ensure no frames are lost
            const auto count1 = consume();
            Logger::WriteMessage(std::format("count [{}]\n", count1).c_str());

            std::this_thread::sleep_for(300ms);

            const auto count2 = consume();
            Logger::WriteMessage(std::format("count [{}]\n", count2).c_str());

            std::this_thread::sleep_for(500ms);

            const auto count3 = consume();
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
