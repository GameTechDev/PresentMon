// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "CppUnitTest.h"
#include <cstdint>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pmon::tests::machine
{
    struct MetricExpectation
    {
        bool expectedAvailable = true;
        std::optional<double> minValue;
        std::optional<double> maxValue;
        std::optional<double> numericValue;
        std::optional<std::string> stringValue;
    };

    struct MetricMeasurement
    {
        std::string section;
        uint32_t gpuDeviceId = 0;
        std::string metric;
        bool available = true;
        bool introspectionAvailable = true;
        bool identityOnly = false;
        std::optional<double> numericValue;
        std::optional<std::string> stringValue;
    };

    class MeasurementSet
    {
    public:
        explicit MeasurementSet(std::string testCase);

        void AddSystem(std::string metric, double value);
        void AddSystem(std::string metric, std::string value);
        void AddSystemUnavailable(std::string metric);
        void AddSystemIntrospectionUnavailable(std::string metric);
        void AddGpu(uint32_t deviceId, std::string metric, double value);
        void AddGpu(uint32_t deviceId, std::string metric, std::string value);
        void AddGpuIdentity(uint32_t deviceId, std::string name);
        void AddGpuUnavailable(uint32_t deviceId, std::string metric);
        void AddGpuIntrospectionUnavailable(uint32_t deviceId, std::string metric);

        const std::string& GetTestCase() const;
        const std::vector<MetricMeasurement>& GetMeasurements() const;
        void AppendToSharedFile() const;

    private:
        void Add_(MetricMeasurement measurement);

        std::string testCase_;
        std::vector<MetricMeasurement> measurements_;
    };

    class ExpectationStore
    {
    public:
        static ExpectationStore Load();

        bool HasExpectationFile() const;
        std::string GetExpectationPath() const;
        std::string GetMeasurementPath() const;
        double GetWaitMultiplier() const;
        bool AssertMeasurements(const MeasurementSet& measurements) const;

    private:
        std::optional<MetricExpectation> Find_(const MetricMeasurement& measurement) const;

        bool hasExpectationFile_ = false;
        std::string expectationPath_;
        std::string measurementPath_;
        double waitMultiplier_ = 1.;
        std::map<std::string, MetricExpectation> system_;
        std::map<uint32_t, std::map<std::string, MetricExpectation>> gpus_;
    };

    double GetWaitMultiplier();
    std::chrono::milliseconds ScaleWait(std::chrono::milliseconds wait);
}
