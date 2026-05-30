// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "MachineExpectations.h"
#include "../str/String.h"
#include "CppUnitTest.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <format>
#include <set>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <variant>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace pmon::util::test
{
    namespace fs = std::filesystem;

    namespace
    {
        struct JsonValue
        {
            using Object = std::map<std::string, JsonValue>;
            using Array = std::vector<JsonValue>;
            std::variant<std::nullptr_t, bool, double, std::string, Object, Array> value;
        };

        class JsonParser
        {
        public:
            explicit JsonParser(std::string text) : text_{ std::move(text) } {}

            JsonValue Parse()
            {
                auto value = ParseValue_();
                SkipWs_();
                if (pos_ != text_.size()) {
                    Throw_("Unexpected trailing JSON content");
                }
                return value;
            }

        private:
            [[noreturn]] void Throw_(const char* message) const
            {
                throw std::runtime_error(std::format("{} at byte {}", message, pos_));
            }

            void SkipWs_()
            {
                while (pos_ < text_.size() && std::isspace((unsigned char)text_[pos_])) {
                    ++pos_;
                }
            }

            bool TryConsume_(char c)
            {
                SkipWs_();
                if (pos_ < text_.size() && text_[pos_] == c) {
                    ++pos_;
                    return true;
                }
                return false;
            }

            void Consume_(char c)
            {
                if (!TryConsume_(c)) {
                    Throw_("Unexpected JSON token");
                }
            }

            JsonValue ParseValue_()
            {
                SkipWs_();
                if (pos_ >= text_.size()) {
                    Throw_("Unexpected end of JSON");
                }
                switch (text_[pos_]) {
                case '{': return JsonValue{ ParseObject_() };
                case '[': return JsonValue{ ParseArray_() };
                case '"': return JsonValue{ ParseString_() };
                case 'n': return ParseLiteral_("null", JsonValue{ nullptr });
                case 't': return ParseLiteral_("true", JsonValue{ true });
                case 'f': return ParseLiteral_("false", JsonValue{ false });
                default:
                    return JsonValue{ ParseNumber_() };
                }
            }

            JsonValue ParseLiteral_(const char* literal, JsonValue value)
            {
                const auto len = strlen(literal);
                if (text_.compare(pos_, len, literal) != 0) {
                    Throw_("Unexpected JSON literal");
                }
                pos_ += len;
                return value;
            }

            JsonValue::Object ParseObject_()
            {
                JsonValue::Object object;
                Consume_('{');
                if (TryConsume_('}')) {
                    return object;
                }
                for (;;) {
                    auto key = ParseString_();
                    Consume_(':');
                    object.emplace(std::move(key), ParseValue_());
                    if (TryConsume_('}')) {
                        break;
                    }
                    Consume_(',');
                }
                return object;
            }

            JsonValue::Array ParseArray_()
            {
                JsonValue::Array array;
                Consume_('[');
                if (TryConsume_(']')) {
                    return array;
                }
                for (;;) {
                    array.push_back(ParseValue_());
                    if (TryConsume_(']')) {
                        break;
                    }
                    Consume_(',');
                }
                return array;
            }

            std::string ParseString_()
            {
                Consume_('"');
                std::string s;
                while (pos_ < text_.size()) {
                    const auto c = text_[pos_++];
                    if (c == '"') {
                        return s;
                    }
                    if (c != '\\') {
                        s.push_back(c);
                        continue;
                    }
                    if (pos_ >= text_.size()) {
                        Throw_("Unterminated JSON escape");
                    }
                    const auto e = text_[pos_++];
                    switch (e) {
                    case '"': s.push_back('"'); break;
                    case '\\': s.push_back('\\'); break;
                    case '/': s.push_back('/'); break;
                    case 'b': s.push_back('\b'); break;
                    case 'f': s.push_back('\f'); break;
                    case 'n': s.push_back('\n'); break;
                    case 'r': s.push_back('\r'); break;
                    case 't': s.push_back('\t'); break;
                    default:
                        Throw_("Unsupported JSON escape");
                    }
                }
                Throw_("Unterminated JSON string");
            }

            double ParseNumber_()
            {
                SkipWs_();
                const auto begin = pos_;
                if (pos_ < text_.size() && text_[pos_] == '-') {
                    ++pos_;
                }
                while (pos_ < text_.size() && std::isdigit((unsigned char)text_[pos_])) {
                    ++pos_;
                }
                if (pos_ < text_.size() && text_[pos_] == '.') {
                    ++pos_;
                    while (pos_ < text_.size() && std::isdigit((unsigned char)text_[pos_])) {
                        ++pos_;
                    }
                }
                if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
                    ++pos_;
                    if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) {
                        ++pos_;
                    }
                    while (pos_ < text_.size() && std::isdigit((unsigned char)text_[pos_])) {
                        ++pos_;
                    }
                }
                if (begin == pos_) {
                    Throw_("Expected JSON number");
                }
                return std::stod(text_.substr(begin, pos_ - begin));
            }

            std::string text_;
            size_t pos_ = 0;
        };

        const JsonValue::Object* AsObject_(const JsonValue& value)
        {
            return std::get_if<JsonValue::Object>(&value.value);
        }

        const JsonValue::Array* AsArray_(const JsonValue& value)
        {
            return std::get_if<JsonValue::Array>(&value.value);
        }

        const std::string* AsString_(const JsonValue& value)
        {
            return std::get_if<std::string>(&value.value);
        }

        const double* AsNumber_(const JsonValue& value)
        {
            return std::get_if<double>(&value.value);
        }

        bool IsNull_(const JsonValue& value)
        {
            return std::holds_alternative<std::nullptr_t>(value.value);
        }

        std::optional<JsonValue> Get_(const JsonValue::Object& object, const char* key)
        {
            if (const auto it = object.find(key); it != object.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        std::string Escape_(const std::string& value)
        {
            std::string out;
            out.reserve(value.size() + 8);
            for (const auto ch : value) {
                switch (ch) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out.push_back(ch); break;
                }
            }
            return out;
        }

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

        MetricExpectation ParseExpectation_(const JsonValue& value)
        {
            MetricExpectation exp;
            if (IsNull_(value)) {
                exp.expectedAvailable = false;
                return exp;
            }
            if (const auto text = AsString_(value)) {
                exp.stringValue = *text;
                return exp;
            }
            if (const auto number = AsNumber_(value)) {
                exp.numericValue = *number;
                return exp;
            }
            const auto object = AsObject_(value);
            if (object == nullptr) {
                throw std::runtime_error("Metric expectation must be null, scalar, or object");
            }
            if (const auto v = Get_(*object, "value")) {
                if (const auto text = AsString_(*v)) {
                    exp.stringValue = *text;
                }
                else if (const auto number = AsNumber_(*v)) {
                    exp.numericValue = *number;
                }
                else if (IsNull_(*v)) {
                    exp.expectedAvailable = false;
                }
            }
            if (const auto v = Get_(*object, "min")) {
                if (const auto number = AsNumber_(*v)) {
                    exp.minValue = *number;
                }
            }
            if (const auto v = Get_(*object, "max")) {
                if (const auto number = AsNumber_(*v)) {
                    exp.maxValue = *number;
                }
            }
            return exp;
        }

        void LoadMetricMap_(std::map<std::string, MetricExpectation>& dst,
            const JsonValue::Object& object)
        {
            for (const auto& [name, value] : object) {
                dst[name] = ParseExpectation_(value);
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

    void MeasurementSet::Add_(MetricMeasurement measurement)
    {
        measurements_.push_back(std::move(measurement));
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
        file << "{\"test_case\":\"" << Escape_(testCase_) << "\",\"measurements\":[";
        for (size_t i = 0; i < measurements_.size(); ++i) {
            const auto& m = measurements_[i];
            if (i != 0) {
                file << ",";
            }
            file << "{\"section\":\"" << m.section << "\",\"metric\":\"" << Escape_(m.metric) << "\"";
            if (m.section == "gpu") {
                file << ",\"device_id\":" << m.gpuDeviceId;
            }
            if (m.identityOnly) {
                file << ",\"identity_only\":true";
            }
            if (!m.available) {
                file << ",\"available\":false,\"introspection_available\":"
                    << (m.introspectionAvailable ? "true" : "false") << ",\"value\":null}";
            }
            else if (m.numericValue) {
                file << ",\"value\":" << std::format("{}", *m.numericValue) << "}";
            }
            else {
                file << ",\"value\":\"" << Escape_(m.stringValue.value_or("")) << "\"}";
            }
        }
        file << "]}\n";
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
        std::stringstream buffer;
        buffer << file.rdbuf();
        const auto root = JsonParser{ buffer.str() }.Parse();
        const auto rootObject = AsObject_(root);
        if (rootObject == nullptr) {
            throw std::runtime_error("Machine expectation root must be an object");
        }

        if (const auto waitMultiplier = Get_(*rootObject, "wait_multiplier")) {
            if (const auto number = AsNumber_(*waitMultiplier)) {
                store.waitMultiplier_ = (std::max)(1., *number);
            }
        }
        else if (const auto waitMultiplier = Get_(*rootObject, "waitMultiplier")) {
            if (const auto number = AsNumber_(*waitMultiplier)) {
                store.waitMultiplier_ = (std::max)(1., *number);
            }
        }

        if (const auto system = Get_(*rootObject, "system")) {
            if (const auto systemObject = AsObject_(*system)) {
                LoadMetricMap_(store.system_, *systemObject);
            }
        }
        auto gpus = Get_(*rootObject, "gpu");
        if (!gpus) {
            gpus = Get_(*rootObject, "gpus");
        }
        if (gpus) {
            if (const auto gpuObject = AsObject_(*gpus)) {
                uint32_t id = 0;
                if (const auto idValue = Get_(*gpuObject, "device_id")) {
                    if (const auto number = AsNumber_(*idValue)) {
                        id = (uint32_t)*number;
                    }
                }
                if (const auto metrics = Get_(*gpuObject, "metrics")) {
                    if (const auto metricsObject = AsObject_(*metrics)) {
                        LoadMetricMap_(store.gpus_[id], *metricsObject);
                    }
                }
                else {
                    LoadMetricMap_(store.gpus_[id], *gpuObject);
                }
            }
            else if (const auto gpuArray = AsArray_(*gpus)) {
                for (const auto& gpuValue : *gpuArray) {
                    const auto gpuObject = AsObject_(gpuValue);
                    if (gpuObject == nullptr) {
                        continue;
                    }
                    uint32_t id = 0;
                    if (const auto idValue = Get_(*gpuObject, "device_id")) {
                        if (const auto number = AsNumber_(*idValue)) {
                            id = (uint32_t)*number;
                        }
                    }
                    if (const auto metrics = Get_(*gpuObject, "metrics")) {
                        if (const auto metricsObject = AsObject_(*metrics)) {
                            LoadMetricMap_(store.gpus_[id], *metricsObject);
                        }
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
