// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MachineExpectations.h"
#include "../str/String.h"
#include "CppUnitTest.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>
#include <set>
#include <stdexcept>
#include <chrono>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace pmon::util::test
{
    namespace fs = std::filesystem;

    namespace
    {
        using Json = nlohmann::json;

        std::optional<std::string> GetEnv_(const wchar_t* name)
        {
            wchar_t* raw = nullptr;
            size_t len = 0;
            if (_wdupenv_s(&raw, &len, name) != 0 || raw == nullptr) {
                return std::nullopt;
            }
            std::wstring value{ raw };
            free(raw);
            if (value.empty()) {
                return std::nullopt;
            }
            return pmon::util::str::ToNarrow(value);
        }

        fs::path GetOutputFolder_()
        {
            return fs::path{ MachineExpectationOutputFolder };
        }

        fs::path GetExpectationPath_()
        {
            if (const auto env = GetEnv_(L"PRESENTMON_MACHINE_EXPECTATIONS")) {
                return fs::path{ *env };
            }
            return fs::path{ ".." } / ".." / "IntelPresentMon" / "machine_expectations.json";
        }

        fs::path GetMeasurementPath_()
        {
            return GetOutputFolder_() / "measurements.jsonl";
        }

        std::string GetUpdateCommand_(const std::string& measurementPath, const std::string& expectationPath)
        {
            const auto scriptPath = fs::absolute(fs::path{ ".." } / ".." / "IntelPresentMon" /
                "PresentMonAPI2Tests" / "update_machine_expectations.py");
            const auto absoluteMeasurementPath = fs::absolute(measurementPath);
            const auto absoluteExpectationPath = fs::absolute(expectationPath);
            return std::format(
                R"(python "{}" --measurements "{}" --expectation "{}")",
                scriptPath.string(),
                absoluteMeasurementPath.string(),
                absoluteExpectationPath.string());
        }

        MetricExpectation ParseExpectation_(const Json& value)
        {
            MetricExpectation exp;
            if (value.is_null()) {
                exp.expectedAvailable = false;
                return exp;
            }
            if (value.is_string()) {
                exp.stringValue = value.get<std::string>();
                return exp;
            }
            if (value.is_number()) {
                exp.numericValue = value.get<double>();
                return exp;
            }
            if (!value.is_object()) {
                throw std::runtime_error("Metric expectation must be null, scalar, or object");
            }

            if (const auto v = value.find("value"); v != value.end()) {
                if (v->is_string()) {
                    exp.stringValue = v->get<std::string>();
                }
                else if (v->is_number()) {
                    exp.numericValue = v->get<double>();
                }
                else if (v->is_null()) {
                    exp.expectedAvailable = false;
                }
            }
            if (const auto v = value.find("min"); v != value.end() && v->is_number()) {
                exp.minValue = v->get<double>();
            }
            if (const auto v = value.find("max"); v != value.end() && v->is_number()) {
                exp.maxValue = v->get<double>();
            }
            return exp;
        }

        void LoadMetricMap_(std::map<std::string, MetricExpectation>& dst,
            const Json& object)
        {
            for (const auto& entry : object.items()) {
                dst[entry.key()] = ParseExpectation_(entry.value());
            }
        }

        void LogMissingExpectation_(const ExpectationStore& store, const MetricMeasurement& measurement)
        {
            Logger::WriteMessage(std::format(
                "No machine expectation for {}{}{}; recorded measurement to [{}]\n",
                measurement.section,
                measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                measurement.metric,
                store.GetMeasurementPath()).c_str());
        }

        std::string FormatMeasurementKey_(const MetricMeasurement& measurement)
        {
            return std::format("{}{}{}",
                measurement.section,
                measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                measurement.metric);
        }

        bool NumericMatches_(double actual, const MetricExpectation& exp)
        {
            if (exp.numericValue) {
                return actual == *exp.numericValue;
            }
            if (exp.minValue && actual < *exp.minValue) {
                return false;
            }
            if (exp.maxValue && actual > *exp.maxValue) {
                return false;
            }
            return true;
        }

        bool HasNumericExpectation_(const MetricExpectation& exp)
        {
            return exp.numericValue || exp.minValue || exp.maxValue;
        }

        std::map<uint32_t, std::string> GetMeasuredGpuNames_(const MeasurementSet& measurements)
        {
            std::map<uint32_t, std::string> names;
            for (const auto& measurement : measurements.GetMeasurements()) {
                if (measurement.section == "gpu" &&
                    measurement.metric == "PM_METRIC_GPU_NAME" &&
                    measurement.available &&
                    measurement.identityOnly &&
                    measurement.stringValue) {
                    names[measurement.gpuDeviceId] = *measurement.stringValue;
                }
            }
            return names;
        }

        std::set<uint32_t> GetMeasuredGpuDeviceIds_(const MeasurementSet& measurements)
        {
            std::set<uint32_t> deviceIds;
            for (const auto& measurement : measurements.GetMeasurements()) {
                if (measurement.section == "gpu") {
                    deviceIds.insert(measurement.gpuDeviceId);
                }
            }
            return deviceIds;
        }
    }

    double GetWaitMultiplier()
    {
        static const double multiplier = ExpectationStore::Load().GetWaitMultiplier();
        return multiplier;
    }

    std::chrono::milliseconds ScaleWait(std::chrono::milliseconds wait)
    {
        const auto scaled = wait.count() * GetWaitMultiplier();
        return std::chrono::milliseconds{ (std::max)(int64_t{ 1 }, (int64_t)std::ceil(scaled)) };
    }

    MeasurementSet::MeasurementSet(std::string testCase)
        : testCase_{ std::move(testCase) }
    {}

    void MeasurementSet::Add_(const MetricMeasurement& measurement)
    {
        measurements_.push_back(measurement);
    }

    void MeasurementSet::AddSystem(std::string metric, double value)
    {
        Add_({ .section = "system", .metric = std::move(metric), .numericValue = value });
    }

    void MeasurementSet::AddSystem(std::string metric, std::string value)
    {
        Add_({ .section = "system", .metric = std::move(metric), .stringValue = std::move(value) });
    }

    void MeasurementSet::AddSystemUnavailable(std::string metric)
    {
        Add_({ .section = "system", .metric = std::move(metric), .available = false });
    }

    void MeasurementSet::AddSystemIntrospectionUnavailable(std::string metric)
    {
        Add_({ .section = "system", .metric = std::move(metric), .available = false, .introspectionAvailable = false });
    }

    void MeasurementSet::AddGpu(uint32_t deviceId, std::string metric, double value)
    {
        Add_({ .section = "gpu", .gpuDeviceId = deviceId, .metric = std::move(metric), .numericValue = value });
    }

    void MeasurementSet::AddGpu(uint32_t deviceId, std::string metric, std::string value)
    {
        Add_({ .section = "gpu", .gpuDeviceId = deviceId, .metric = std::move(metric), .stringValue = std::move(value) });
    }

    void MeasurementSet::AddGpuIdentity(uint32_t deviceId, std::string name)
    {
        Add_({
            .section = "gpu",
            .gpuDeviceId = deviceId,
            .metric = "PM_METRIC_GPU_NAME",
            .available = true,
            .introspectionAvailable = true,
            .identityOnly = true,
            .stringValue = std::move(name),
            });
    }

    void MeasurementSet::AddGpuUnavailable(uint32_t deviceId, std::string metric)
    {
        Add_({ .section = "gpu", .gpuDeviceId = deviceId, .metric = std::move(metric), .available = false });
    }

    void MeasurementSet::AddGpuIntrospectionUnavailable(uint32_t deviceId, std::string metric)
    {
        Add_({ .section = "gpu", .gpuDeviceId = deviceId, .metric = std::move(metric), .available = false, .introspectionAvailable = false });
    }

    const std::string& MeasurementSet::GetTestCase() const
    {
        return testCase_;
    }

    const std::vector<MetricMeasurement>& MeasurementSet::GetMeasurements() const
    {
        return measurements_;
    }

    void MeasurementSet::AppendToSharedFile() const
    {
        fs::create_directories(GetOutputFolder_());
        std::ofstream file{ GetMeasurementPath_(), std::ios::app };

        Json record;
        record["test_case"] = testCase_;
        record["measurements"] = Json::array();
        for (const auto& m : measurements_) {
            Json measurement;
            measurement["section"] = m.section;
            measurement["metric"] = m.metric;
            if (m.section == "gpu") {
                measurement["device_id"] = m.gpuDeviceId;
            }
            if (m.identityOnly) {
                measurement["identity_only"] = true;
            }
            if (!m.available) {
                measurement["available"] = false;
                measurement["introspection_available"] = m.introspectionAvailable;
                measurement["value"] = nullptr;
            }
            else if (m.numericValue) {
                measurement["value"] = *m.numericValue;
            }
            else {
                measurement["value"] = m.stringValue.value_or("");
            }
            record["measurements"].push_back(std::move(measurement));
        }
        file << record.dump() << "\n";
    }

    ExpectationStore ExpectationStore::Load()
    {
        ExpectationStore store;
        store.expectationPath_ = GetExpectationPath_().string();
        store.measurementPath_ = GetMeasurementPath_().string();
        store.hasExpectationFile_ = fs::exists(store.expectationPath_);
        if (!store.hasExpectationFile_) {
            Logger::WriteMessage(std::format(
                "Machine expectation file not found: [{}]. Platform-dependent tests will fail after recording measurements.\n",
                store.expectationPath_).c_str());
            Logger::WriteMessage(std::format("Generate or update with: {}\n",
                GetUpdateCommand_(store.measurementPath_, store.expectationPath_)).c_str());
            return store;
        }

        std::ifstream file{ store.expectationPath_ };
        const auto root = Json::parse(file);
        if (!root.is_object()) {
            throw std::runtime_error("Machine expectation root must be an object");
        }

        if (const auto waitMultiplier = root.find("wait_multiplier"); waitMultiplier != root.end() && waitMultiplier->is_number()) {
            store.waitMultiplier_ = (std::max)(1., waitMultiplier->get<double>());
        }
        else if (const auto waitMultiplier = root.find("waitMultiplier"); waitMultiplier != root.end() && waitMultiplier->is_number()) {
            store.waitMultiplier_ = (std::max)(1., waitMultiplier->get<double>());
        }

        if (const auto system = root.find("system"); system != root.end() && system->is_object()) {
            LoadMetricMap_(store.system_, *system);
        }

        auto gpus = root.find("gpu");
        if (gpus == root.end()) {
            gpus = root.find("gpus");
        }
        if (gpus != root.end()) {
            if (gpus->is_object()) {
                uint32_t id = 0;
                if (const auto idValue = gpus->find("device_id"); idValue != gpus->end() && idValue->is_number()) {
                    id = (uint32_t)idValue->get<double>();
                }
                if (const auto metrics = gpus->find("metrics"); metrics != gpus->end() && metrics->is_object()) {
                    LoadMetricMap_(store.gpus_[id], *metrics);
                }
                else {
                    LoadMetricMap_(store.gpus_[id], *gpus);
                }
            }
            else if (gpus->is_array()) {
                for (const auto& gpuValue : *gpus) {
                    if (!gpuValue.is_object()) {
                        continue;
                    }
                    uint32_t id = 0;
                    if (const auto idValue = gpuValue.find("device_id"); idValue != gpuValue.end() && idValue->is_number()) {
                        id = (uint32_t)idValue->get<double>();
                    }
                    if (const auto metrics = gpuValue.find("metrics"); metrics != gpuValue.end() && metrics->is_object()) {
                        LoadMetricMap_(store.gpus_[id], *metrics);
                    }
                }
            }
        }
        return store;
    }

    bool ExpectationStore::HasExpectationFile() const
    {
        return hasExpectationFile_;
    }

    std::string ExpectationStore::GetExpectationPath() const
    {
        return expectationPath_;
    }

    std::string ExpectationStore::GetMeasurementPath() const
    {
        return measurementPath_;
    }

    double ExpectationStore::GetWaitMultiplier() const
    {
        return waitMultiplier_;
    }

    std::optional<MetricExpectation> ExpectationStore::Find_(const MetricMeasurement& measurement) const
    {
        if (measurement.section == "system") {
            if (const auto it = system_.find(measurement.metric); it != system_.end()) {
                return it->second;
            }
            return std::nullopt;
        }
        const auto gpu = gpus_.find(measurement.gpuDeviceId);
        if (gpu == gpus_.end()) {
            return std::nullopt;
        }
        if (const auto it = gpu->second.find(measurement.metric); it != gpu->second.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool ExpectationStore::AssertMeasurements(const MeasurementSet& measurements) const
    {
        if (!hasExpectationFile_) {
            Assert::Fail(pmon::util::str::ToWide(std::format(
                "Machine expectation file not found for platform-dependent test [{}]. Expected file: [{}]. "
                "Measurements were recorded to [{}] and can be used to generate expectations.\n{}",
                measurements.GetTestCase(),
                expectationPath_,
                measurementPath_,
                GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
            return false;
        }

        std::set<std::string> missingExpectations;
        const auto measuredGpuNames = GetMeasuredGpuNames_(measurements);
        for (const auto deviceId : GetMeasuredGpuDeviceIds_(measurements)) {
            const auto measuredName = measuredGpuNames.find(deviceId);
            if (measuredName == measuredGpuNames.end()) {
                Assert::Fail(pmon::util::str::ToWide(std::format(
                    "{} has GPU measurements for device id {}, but does not include AddGpuIdentity(...) data to verify the id matches the expectation.\n{}",
                    measurements.GetTestCase(),
                    deviceId,
                    GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                return false;
            }

            const MetricMeasurement nameMeasurement{
                .section = "gpu",
                .gpuDeviceId = deviceId,
                .metric = "PM_METRIC_GPU_NAME",
                .stringValue = measuredName->second,
            };
            const auto nameExp = Find_(nameMeasurement);
            if (!nameExp) {
                missingExpectations.insert(FormatMeasurementKey_(nameMeasurement));
                LogMissingExpectation_(*this, nameMeasurement);
                continue;
            }
            if (!nameExp->expectedAvailable || !nameExp->stringValue || measuredName->second != *nameExp->stringValue) {
                Assert::Fail(pmon::util::str::ToWide(std::format(
                    "GPU device id {} was expected to be [{}], but measured [{}]. Update expectations if device ordering changed.\n{}",
                    deviceId,
                    nameExp->stringValue.value_or("<unavailable>"),
                    measuredName->second,
                    GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                return false;
            }
        }

        for (const auto& measurement : measurements.GetMeasurements()) {
            if (measurement.identityOnly) {
                continue;
            }
            const auto exp = Find_(measurement);
            if (!exp) {
                if (missingExpectations.insert(FormatMeasurementKey_(measurement)).second) {
                    LogMissingExpectation_(*this, measurement);
                }
                continue;
            }
            if (!exp->expectedAvailable) {
                if (measurement.available || measurement.introspectionAvailable) {
                    Assert::Fail(pmon::util::str::ToWide(std::format(
                        "{}{}{} was expected unavailable, but the test/introspection reported it available.\n{}",
                        measurement.section,
                        measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                        measurement.metric,
                        GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                    return false;
                }
                continue;
            }
            if (!measurement.available) {
                Assert::Fail(pmon::util::str::ToWide(std::format(
                    "{}{}{} was unavailable but expectation requires a value.\n{}",
                    measurement.section,
                    measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                    measurement.metric,
                    GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                return false;
            }
            if (measurement.numericValue) {
                if (!HasNumericExpectation_(*exp)) {
                    Assert::Fail(pmon::util::str::ToWide(std::format(
                        "{} has a numeric measurement for {} but the machine expectation does not define a value, min, or max.\n{}",
                        measurements.GetTestCase(),
                        FormatMeasurementKey_(measurement),
                        GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                    return false;
                }
                if (!NumericMatches_(*measurement.numericValue, *exp)) {
                    Assert::Fail(pmon::util::str::ToWide(std::format(
                        "{}{}{} value {} outside expectation from [{}].\n{}",
                        measurement.section,
                        measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                        measurement.metric,
                        *measurement.numericValue,
                        expectationPath_,
                        GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                    return false;
                }
            }
            else {
                if (!exp->stringValue) {
                    Assert::Fail(pmon::util::str::ToWide(std::format(
                        "{} has a string measurement for {} but the machine expectation does not define a string value.\n{}",
                        measurements.GetTestCase(),
                        FormatMeasurementKey_(measurement),
                        GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                    return false;
                }
                if (measurement.stringValue.value_or("") != *exp->stringValue) {
                    Assert::Fail(pmon::util::str::ToWide(std::format(
                        "{}{}{} value [{}] did not match expected [{}].\n{}",
                        measurement.section,
                        measurement.section == "gpu" ? std::format("[{}].", measurement.gpuDeviceId) : ".",
                        measurement.metric,
                        measurement.stringValue.value_or(""),
                        *exp->stringValue,
                        GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
                    return false;
                }
            }
        }

        if (!missingExpectations.empty()) {
            std::string missingList;
            for (const auto& missing : missingExpectations) {
                missingList += "\n  ";
                missingList += missing;
            }
            Assert::Fail(pmon::util::str::ToWide(std::format(
                "Machine expectation file [{}] is missing {} expectation(s) for platform-dependent test [{}]."
                "{}\nMeasurements were recorded to [{}].\n{}",
                expectationPath_,
                missingExpectations.size(),
                measurements.GetTestCase(),
                missingList,
                measurementPath_,
                GetUpdateCommand_(measurementPath_, expectationPath_))).c_str());
            return false;
        }
        return true;
    }
}
