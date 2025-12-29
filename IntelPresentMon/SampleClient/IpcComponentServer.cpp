// Copyright (C) 2022-2025 Intel Corporation
// SPDX-License-Identifier: MIT

#include "../Interprocess/source/OwnedDataSegment.h"
#include "../Interprocess/source/DataStores.h"

#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/log/Log.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

using namespace std::literals;

namespace ipc = pmon::ipc;

// Hardcoded segment name shared with the test.
static constexpr const char* kSystemSegName = "pm_ipc_system_store_test_seg";

// We only create two metrics:
// 1) A scalar metric with 1 element (count = 1)
// 2) An "array-like" metric with 2 elements (count = 2)
//
// The test goal is ring push/read plumbing, not capability validation.
static constexpr PM_METRIC kScalarMetric = PM_METRIC_CPU_FREQUENCY;
static constexpr PM_METRIC kArrayMetric = PM_METRIC_CPU_UTILIZATION;
static constexpr size_t kSystemRingCapacity = 32;
static constexpr size_t kSystemSegmentBytes = 512 * 1024;

static void BuildRings_(ipc::SystemDataStore& store)
{
    // Scalar metric
    store.telemetryData.AddRing(kScalarMetric, kSystemRingCapacity, 1, PM_DATA_TYPE_DOUBLE);

    // Array metric with 2 elements
    store.telemetryData.AddRing(kArrayMetric, kSystemRingCapacity, 2, PM_DATA_TYPE_DOUBLE);
}

static void PushDeterministicSamples_(ipc::SystemDataStore& store)
{
    auto& scalar = store.telemetryData.FindRing<double>(kScalarMetric);

    auto& array = store.telemetryData.FindRing<double>(kArrayMetric);

    // Expect sizes: scalar = 1 ring, array = 2 rings
    if (scalar.size() != 1 || array.size() != 2) {
        throw std::logic_error("IpcSystemServer: ring vectors not sized as expected");
    }

    auto& scalarRing = scalar.at(0);
    auto& arr0 = array.at(0);
    auto& arr1 = array.at(1);

    constexpr size_t sampleCount = 12;

    for (size_t i = 0; i < sampleCount; ++i) {
        const uint64_t ts = 10'000ull + static_cast<uint64_t>(i);

        // Scalar sequence
        const double freq = 3000.0 + 10.0 * static_cast<double>(i);
        scalarRing.Push(freq, ts);

        // Array element 0 sequence
        const double util0 = 5.0 + static_cast<double>(i);
        arr0.Push(util0, ts);

        // Array element 1 sequence (offset so we can tell them apart)
        const double util1 = 50.0 + static_cast<double>(i) * 2.0;
        arr1.Push(util1, ts);
    }
}

// Submode entry point.
int IpcComponentServer()
{
    ipc::DataStoreSizingInfo sizing{};
    sizing.overrideBytes = kSystemSegmentBytes;

    // Create the shared memory segment hosting SystemDataStore.
    ipc::OwnedDataSegment<ipc::SystemDataStore> seg{
        kSystemSegName,
        sizing
    };
    auto& store = seg.GetStore();

    // Only build the two test rings.
    BuildRings_(store);

    // Ping gate to sync "server ready" with the test harness.
    std::string line;
    std::getline(std::cin, line);
    if (line != "%ping") {
        std::cout << "%%{ping-error}%%" << std::endl;
        return -1;
    }
    std::cout << "%%{ping-ok}%%" << std::endl;

    // Push a deterministic batch after ping.
    PushDeterministicSamples_(store);

    // Command loop.
    while (std::getline(std::cin, line)) {
        if (line == "%quit") {
            std::cout << "%%{quit-ok}%%" << std::endl;
            std::this_thread::sleep_for(25ms);
            return 0;
        }
        else if (line == "%push-more") {
            PushDeterministicSamples_(store);
            std::cout << "%%{push-more-ok}%%" << std::endl;
        }
        else {
            std::cout << "%%{err-bad-command}%%" << std::endl;
        }
    }

    return -1;
}

