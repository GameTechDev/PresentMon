// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <Interprocess/source/MetricCapabilities.h>
#include <PresentMonAPI2/PresentMonAPI.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MetricCapabilitiesTests
{
    TEST_CLASS(MetricCapabilitiesMergeTests)
    {
    public:
        TEST_METHOD(AvailableFromEitherSideWinsWithLargerArraySize)
        {
            pmon::ipc::MetricCapabilities base;
            base.Set(PM_METRIC_GPU_POWER, 1, PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE);

            pmon::ipc::MetricCapabilities incoming;
            incoming.Set(PM_METRIC_GPU_POWER, 3, PM_METRIC_AVAILABILITY_AVAILABLE);

            base.Merge(incoming);

            Assert::AreEqual((size_t)3, base.Check(PM_METRIC_GPU_POWER));
            const auto entry = base.Lookup(PM_METRIC_GPU_POWER);
            Assert::IsTrue(entry.has_value());
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)entry->availability);
        }

        TEST_METHOD(NonAvailableFillsMissingMetric)
        {
            pmon::ipc::MetricCapabilities base;

            pmon::ipc::MetricCapabilities incoming;
            incoming.Set(PM_METRIC_GPU_VOLTAGE, 0, PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE);

            base.Merge(incoming);

            const auto entry = base.Lookup(PM_METRIC_GPU_VOLTAGE);
            Assert::IsTrue(entry.has_value());
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE, (int)entry->availability);
            Assert::AreEqual((size_t)0, base.Check(PM_METRIC_GPU_VOLTAGE));
        }

        TEST_METHOD(ExistingAvailableNotDowngradedByNonAvailable)
        {
            pmon::ipc::MetricCapabilities base;
            base.Set(PM_METRIC_GPU_FREQUENCY, 2, PM_METRIC_AVAILABILITY_AVAILABLE);

            pmon::ipc::MetricCapabilities incoming;
            incoming.Set(PM_METRIC_GPU_FREQUENCY, 0, PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE);

            base.Merge(incoming);

            Assert::AreEqual((size_t)2, base.Check(PM_METRIC_GPU_FREQUENCY));
            const auto entry = base.Lookup(PM_METRIC_GPU_FREQUENCY);
            Assert::IsTrue(entry.has_value());
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)entry->availability);
        }

        TEST_METHOD(NonAvailableReplacesOtherNonAvailable)
        {
            pmon::ipc::MetricCapabilities base;
            base.Set(PM_METRIC_GPU_TEMPERATURE, 0, PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE);

            pmon::ipc::MetricCapabilities incoming;
            incoming.Set(PM_METRIC_GPU_TEMPERATURE, 0, PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE);

            base.Merge(incoming);

            const auto entry = base.Lookup(PM_METRIC_GPU_TEMPERATURE);
            Assert::IsTrue(entry.has_value());
            Assert::AreEqual((int)PM_METRIC_AVAILABILITY_NOT_SUPPORTED_BY_DEVICE, (int)entry->availability);
        }
    };
}
