// Copyright (C) 2022-2025 Intel Corporation
// SPDX-License-Identifier: MIT

#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include "JobManager.h"

#include "../Interprocess/source/ViewedDataSegment.h"
#include "../Interprocess/source/DataStores.h"

#include "../PresentMonAPI2/PresentMonAPI.h"

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace IpcComponentTests
{
    namespace ipc = pmon::ipc;

    // Must match the server submode constant.
    static constexpr const char* kSystemSegName = "pm_ipc_system_store_test_seg";

    static constexpr PM_METRIC kScalarMetric = PM_METRIC_CPU_FREQUENCY;
    static constexpr PM_METRIC kArrayMetric = PM_METRIC_CPU_UTILIZATION;

    // Expected test child patterns
    static constexpr uint64_t kBaseTs = 10'000ull;
    static constexpr size_t kSampleCount = 12;

    class TestFixture : public CommonTestFixture
    {
    protected:
        const CommonProcessArgs& GetCommonArgs() const override
        {
            static CommonProcessArgs args{
                .ctrlPipe = R"(\\.\pipe\pm-ipc-sys-store-test-ctrl)",
                .shmNamePrefix = "pm_ipc_sys_store_unused_prefix",
                .logLevel = "debug",
                .logFolder = logFolder_,
                .sampleClientMode = "IpcComponentServer",
            };
            return args;
        }
    };

    static std::string DumpRing_(const ipc::HistoryRing<double>& ring, size_t maxSamples = 8)
    {
        std::ostringstream oss;
        const auto [first, last] = ring.GetSerialRange();
        const size_t count = last - first;

        oss << "serial range [" << first << ", " << last << "), count=" << count << "\n";

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

        return oss.str();
    }

    static void LogRing_(const char* label, const ipc::HistoryRing<double>& ring)
    {
        std::ostringstream oss;
        oss << label << "\n" << DumpRing_(ring);
        Logger::WriteMessage(oss.str().c_str());
    }

    static double ExpectedScalarValue_(uint64_t timestamp)
    {
        const size_t i = static_cast<size_t>(timestamp - kBaseTs);
        return 3000.0 + 10.0 * static_cast<double>(i);
    }

    static double ExpectedArray0Value_(uint64_t timestamp)
    {
        const size_t i = static_cast<size_t>(timestamp - kBaseTs);
        return 5.0 + static_cast<double>(i);
    }

    static double ExpectedArray1Value_(uint64_t timestamp)
    {
        const size_t i = static_cast<size_t>(timestamp - kBaseTs);
        return 50.0 + 2.0 * static_cast<double>(i);
    }

    TEST_CLASS(SystemDataStoreHistoryRingInterfaceTests)
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

        TEST_METHOD(RingsArePresentAndSized)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& scalarVect = store.telemetryData.FindRing<double>(kScalarMetric);
            const auto& arrayVect = store.telemetryData.FindRing<double>(kArrayMetric);

            Logger::WriteMessage("Checking ring vector sizes...\n");

            Assert::AreEqual<size_t>(1, scalarVect.size(), L"Scalar metric should have 1 ring");
            Assert::AreEqual<size_t>(2, arrayVect.size(), L"Array metric should have 2 rings");
        }

        TEST_METHOD(EmptyRangeAndFrontWorkForScalar)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& ring = store.telemetryData.FindRing<double>(kScalarMetric).at(0);

            Logger::WriteMessage("Validating Empty/GetSerialRange/Front for scalar ring\n");
            LogRing_("Scalar ring dump:", ring);

            Assert::IsFalse(ring.Empty(), L"Ring should not be empty after server push");

            const auto [first, last] = ring.GetSerialRange();
            Assert::IsTrue(last >= first, L"Serial range should be valid");
            Assert::IsTrue((last - first) >= kSampleCount, L"Expected at least 12 samples");

            const auto& front = ring.Front();
            const auto& atFirst = ring.At(first);

            Assert::AreEqual(front.timestamp, atFirst.timestamp);
            Assert::AreEqual(front.value, atFirst.value, 1e-9);

            Assert::AreEqual<uint64_t>(kBaseTs, front.timestamp);
            Assert::AreEqual(3000.0, front.value, 1e-9);
        }

        TEST_METHOD(AtReadsExpectedValuesForArrayElements)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& arrVect = store.telemetryData.FindRing<double>(kArrayMetric);

            const auto& ring0 = arrVect.at(0);
            const auto& ring1 = arrVect.at(1);

            Logger::WriteMessage("Validating At() value mapping for array rings\n");
            LogRing_("Array ring[0] dump:", ring0);
            LogRing_("Array ring[1] dump:", ring1);

            const auto [f0, l0] = ring0.GetSerialRange();
            const auto [f1, l1] = ring1.GetSerialRange();

            Assert::IsTrue((l0 - f0) >= kSampleCount);
            Assert::IsTrue((l1 - f1) >= kSampleCount);

            // Check a few specific timestamps
            for (uint64_t ts : { kBaseTs, kBaseTs + 5, kBaseTs + 11 }) {
                const size_t i = static_cast<size_t>(ts - kBaseTs);

                const auto& s0 = ring0.At(f0 + i);
                const auto& s1 = ring1.At(f1 + i);

                Assert::AreEqual(ts, s0.timestamp);
                Assert::AreEqual(ts, s1.timestamp);

                Assert::AreEqual(ExpectedArray0Value_(ts), s0.value, 1e-9);
                Assert::AreEqual(ExpectedArray1Value_(ts), s1.value, 1e-9);
            }
        }

        TEST_METHOD(LowerBoundSerialEdgeAndExactCases)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& ring = store.telemetryData.FindRing<double>(kScalarMetric).at(0);

            Logger::WriteMessage("Validating LowerBoundSerial cases\n");

            const auto [first, last] = ring.GetSerialRange();

            // Before first timestamp -> should return first
            {
                const size_t s = ring.LowerBoundSerial(kBaseTs - 1);
                Assert::AreEqual(first, s);
            }

            // Exact timestamp match
            {
                const uint64_t ts = kBaseTs + 5;
                const size_t s = ring.LowerBoundSerial(ts);
                const auto& sample = ring.At(s);

                Assert::AreEqual(ts, sample.timestamp);
                Assert::AreEqual(ExpectedScalarValue_(ts), sample.value, 1e-9);
            }

            // After last timestamp -> should return last (one past end)
            {
                const size_t s = ring.LowerBoundSerial(kBaseTs + static_cast<uint64_t>(kSampleCount));
                Assert::AreEqual(last, s);
            }
        }

        TEST_METHOD(UpperBoundSerialEdgeAndExactCases)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& ring = store.telemetryData.FindRing<double>(kScalarMetric).at(0);

            Logger::WriteMessage("Validating UpperBoundSerial cases\n");

            const auto [first, last] = ring.GetSerialRange();

            // Before first timestamp -> should return first
            {
                const size_t s = ring.UpperBoundSerial(kBaseTs - 1);
                Assert::AreEqual(first, s);
            }

            // Upper bound of first sample timestamp -> should point to second sample
            {
                const size_t s = ring.UpperBoundSerial(kBaseTs);
                Assert::IsTrue(s > first);
                const auto& sample = ring.At(s);
                Assert::AreEqual<uint64_t>(kBaseTs + 1, sample.timestamp);
            }

            // Upper bound of last sample timestamp -> should return last
            {
                const uint64_t lastTs = kBaseTs + static_cast<uint64_t>(kSampleCount - 1);
                const size_t s = ring.UpperBoundSerial(lastTs);
                Assert::AreEqual(last, s);
            }
        }

        TEST_METHOD(NearestSerialClampsAndExact)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& ring = store.telemetryData.FindRing<double>(kScalarMetric).at(0);

            Logger::WriteMessage("Validating NearestSerial cases\n");

            const auto [first, last] = ring.GetSerialRange();

            // Before first -> clamp to first
            {
                const size_t s = ring.NearestSerial(kBaseTs - 500);
                Assert::AreEqual(first, s);
                Assert::AreEqual<uint64_t>(kBaseTs, ring.At(s).timestamp);
            }

            // After last -> clamp to last-1
            {
                const size_t s = ring.NearestSerial(kBaseTs + 500);
                Assert::AreEqual(last - 1, s);
                Assert::AreEqual<uint64_t>(kBaseTs + static_cast<uint64_t>(kSampleCount - 1),
                    ring.At(s).timestamp);
            }

            // Exact timestamp -> should return that sample
            {
                const uint64_t ts = kBaseTs + 7;
                const size_t s = ring.NearestSerial(ts);
                const auto& sample = ring.At(s);

                Assert::AreEqual(ts, sample.timestamp);
                Assert::AreEqual(ExpectedScalarValue_(ts), sample.value, 1e-9);
            }
        }

        TEST_METHOD(ForEachInTimestampRangeVisitsExpectedSamples)
        {
            auto server = fixture_.LaunchClient();
            std::this_thread::sleep_for(25ms);

            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            const auto& ring = store.telemetryData.FindRing<double>(kScalarMetric).at(0);

            Logger::WriteMessage("Validating ForEachInTimestampRange\n");
            LogRing_("Scalar ring dump:", ring);

            const uint64_t start = kBaseTs + 3;
            const uint64_t end = kBaseTs + 6;

            size_t visited = 0;
            double sum = 0.0;

            const size_t count = ring.ForEachInTimestampRange(start, end, [&](const auto& s) {
                ++visited;
                sum += s.value;
            });

            // Timestamps are contiguous and inclusive
            // Expected: 10003, 10004, 10005, 10006 -> 4 samples
            Assert::AreEqual<size_t>(4, count);
            Assert::AreEqual<size_t>(4, visited);

            const double expectedSum =
                ExpectedScalarValue_(start) +
                ExpectedScalarValue_(start + 1) +
                ExpectedScalarValue_(start + 2) +
                ExpectedScalarValue_(end);

            Logger::WriteMessage(std::format("ForEach visited={}, sum={}\n", visited, sum).c_str());

            Assert::AreEqual(expectedSum, sum, 1e-9);
        }
    };
}
