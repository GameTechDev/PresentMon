// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <ControlLib/TelemetryAvailabilityMerge.h>
#include <PresentMonAPI2/PresentMonAPI.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TelemetryAvailabilityMergeTests
{
    TEST_CLASS(MergeProviderAvailabilityEntriesTests)
    {
    public:
        TEST_METHOD(AnyAvailableWinsWithMaxArraySize)
        {
            const std::vector<pmon::ipc::MetricCapabilityEntry> entries{
                { .arraySize = 0, .availability = PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE },
                { .arraySize = 4, .availability = PM_METRIC_AVAILABILITY_AVAILABLE },
                { .arraySize = 2, .availability = PM_METRIC_AVAILABILITY_AVAILABLE },
            };

            const auto merged = pmon::tel::MergeProviderAvailabilityEntries(
                PM_DEVICE_TYPE_GRAPHICS_ADAPTER, entries);

            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)merged.availability);
            Assert::AreEqual((size_t)4, merged.arraySize);
        }

        TEST_METHOD(NotSupportedBeatsNotExported)
        {
            const std::vector<pmon::ipc::MetricCapabilityEntry> entries{
                { .arraySize = 0, .availability = PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE },
                { .arraySize = 0, .availability = PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE },
            };

            const auto merged = pmon::tel::MergeProviderAvailabilityEntries(
                PM_DEVICE_TYPE_GRAPHICS_ADAPTER, entries);

            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE, (int)merged.availability);
        }

        TEST_METHOD(NotExportedBeatsNotImplemented)
        {
            const std::vector<pmon::ipc::MetricCapabilityEntry> entries{
                { .arraySize = 0, .availability = PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON },
                { .arraySize = 0, .availability = PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE },
            };

            const auto merged = pmon::tel::MergeProviderAvailabilityEntries(
                PM_DEVICE_TYPE_GRAPHICS_ADAPTER, entries);

            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE, (int)merged.availability);
        }

        TEST_METHOD(EmptyProviderListUsesDefaultMissingForDeviceType)
        {
            const auto gpuDefault = pmon::tel::MergeProviderAvailabilityEntries(
                PM_DEVICE_TYPE_GRAPHICS_ADAPTER, {});
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE, (int)gpuDefault.availability);

            const auto systemDefault = pmon::tel::MergeProviderAvailabilityEntries(
                PM_DEVICE_TYPE_SYSTEM, {});
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON, (int)systemDefault.availability);
        }
    };
}
