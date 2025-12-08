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
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace IpcComponentTests
{
    namespace ipc = pmon::ipc;

    // Must match the server submode constant.
    static constexpr const char* kSystemSegName = "pm_ipc_system_store_test_seg";

    static constexpr PM_METRIC kScalarMetric = PM_METRIC_CPU_FREQUENCY;
    static constexpr PM_METRIC kArrayMetric = PM_METRIC_CPU_UTILIZATION;

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

    TEST_CLASS(SystemDataStoreIpcTests)
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

        TEST_METHOD(ReadTwoMetricsWithOneArrayMetric)
        {
            // Launch SampleClient in IpcSystemServer submode.
            // The harness should handle the %ping gate.
            auto server = fixture_.LaunchClient();

            // Allow a brief moment for push after ping.
            std::this_thread::sleep_for(25ms);

            // Open the hardcoded segment in-process (client built into the test).
            ipc::ViewedDataSegment<ipc::SystemDataStore> view{ kSystemSegName };
            const auto& store = view.GetStore();

            // 1) Scalar metric expectations
            {
                const auto& scalarVect = store.telemetryData.FindRing<double>(kScalarMetric);
                Assert::AreEqual<size_t>(1, scalarVect.size());

                const auto& ring = scalarVect.at(0);

                const auto range = ring.GetSerialRange();
                const size_t count = range.second - range.first;

                Assert::IsTrue(count >= 12ull, L"Expected at least 12 scalar samples");

                const auto& first = ring.At(range.first);
                Assert::AreEqual<uint64_t>(10'000ull, first.timestamp);
                Assert::AreEqual(3000.0, first.value, 1e-9);

                const auto& last = ring.At(range.second - 1);
                Assert::AreEqual<uint64_t>(10'011ull, last.timestamp);
                Assert::AreEqual(3000.0 + 10.0 * 11.0, last.value, 1e-9);
            }

            // 2) Array metric with 2 elements expectations
            {
                const auto& arrVect = store.telemetryData.FindRing<double>(kArrayMetric);
                Assert::AreEqual<size_t>(2, arrVect.size());

                const auto& ring0 = arrVect.at(0);
                const auto& ring1 = arrVect.at(1);

                const auto r0 = ring0.GetSerialRange();
                const auto r1 = ring1.GetSerialRange();

                Assert::IsTrue((r0.second - r0.first) >= 12ull, L"Expected at least 12 samples in array[0]");
                Assert::IsTrue((r1.second - r1.first) >= 12ull, L"Expected at least 12 samples in array[1]");

                const auto& first0 = ring0.At(r0.first);
                const auto& first1 = ring1.At(r1.first);

                Assert::AreEqual<uint64_t>(10'000ull, first0.timestamp);
                Assert::AreEqual<uint64_t>(10'000ull, first1.timestamp);

                Assert::AreEqual(5.0, first0.value, 1e-9);
                Assert::AreEqual(50.0, first1.value, 1e-9);

                const auto& last0 = ring0.At(r0.second - 1);
                const auto& last1 = ring1.At(r1.second - 1);

                Assert::AreEqual<uint64_t>(10'011ull, last0.timestamp);
                Assert::AreEqual<uint64_t>(10'011ull, last1.timestamp);

                Assert::AreEqual(5.0 + 11.0, last0.value, 1e-9);
                Assert::AreEqual(50.0 + 2.0 * 11.0, last1.value, 1e-9);
            }
        }
    };
}
