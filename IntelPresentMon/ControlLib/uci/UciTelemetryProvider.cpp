// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "UciTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/GlobalPolicy.h"
#include "../../CommonUtilities/str/String.h"
#include "../../CommonUtilities/win/Privileges.h"

#include <algorithm>
#include <exception>
#include <format>
#include <optional>
#include <string>
#include <vector>

using namespace pmon;
using namespace std::literals;

#if PMON_HAS_UCI_SDK

namespace pmon::tel::uci
{
    namespace
    {
        using v = util::log::V;

        const std::string kCpuPowerMetricName_ = "pkg-pwr";
        const std::string kCpuTemperatureMetricName_ = "core-temp";
        const std::string kCpuPowerRecordMetricName_ = "PKG-PWR";
        const std::string kCpuTemperatureRecordMetricName_ = "TEMP";
        const std::string kCoreEntityPrefix_ = "Core_";

        [[noreturn]] void ThrowUciError_(uc_result_t result, const char* call, bool subsystemAbsent)
        {
            const auto message = std::format("UCI call [{}] failed with code [{}]", call, (int)result);
            if (subsystemAbsent) {
                throw Except<TelemetrySubsystemAbsent>(message);
            }
            throw Except<UciException>(message);
        }

        void CheckUciCall_(uc_result_t result, const char* call, bool subsystemAbsent = false)
        {
            if (result != UC_SUCCESS) {
                ThrowUciError_(result, call, subsystemAbsent);
            }
        }

        bool TryGetString_(
            const UciWrapper& wrapper,
            uc_result_t (UciWrapper::*getter)(uciMetricRecordHandle, char**) const noexcept,
            uciMetricRecordHandle handle,
            std::string& value)
        {
            char* pText = nullptr;
            if ((wrapper.*getter)(handle, &pText) != UC_SUCCESS || pText == nullptr) {
                value.clear();
                return false;
            }
            value = pText;
            return true;
        }

        void DumpDataCallback_(const UciWrapper& wrapper, uciDataBundle* dataBundle)
        {
            if (dataBundle == nullptr) {
                pmlog_verb(v::uci)("UCI callback received null data bundle");
                return;
            }

            std::string dump = std::format(
                "UCI callback received numRecords={}\n",
                dataBundle->num_records);
            if (dataBundle->records == nullptr) {
                pmlog_verb(v::uci)("records array pointer is null");
                return;
            }

            for (uint16_t i = 0; i < dataBundle->num_records; ++i) {
                const auto record = dataBundle->records[i];
                dump += std::format("record[{}]\n", i);
                if (record == nullptr) {
                    pmlog_verb(v::uci)("record pointer is null");
                    continue;
                }

                std::string metricName;
                std::string entity;
                std::string descriptor;
                std::string unit;
                double sample = 0.0;
                uint64_t timestamp = 0;
                uint64_t duration = 0;

                if (TryGetString_(wrapper, &UciWrapper::MetricRecordGetMetricName, record, metricName)) {
                    dump += std::format("record[{}].metricName=\"{}\"\n", i, metricName);
                }
                if (TryGetString_(wrapper, &UciWrapper::MetricRecordGetEntity, record, entity)) {
                    dump += std::format("record[{}].entity=\"{}\"\n", i, entity);
                }
                if (TryGetString_(wrapper, &UciWrapper::MetricRecordGetDescriptor, record, descriptor)) {
                    dump += std::format("record[{}].descriptor=\"{}\"\n", i, descriptor);
                }
                if (TryGetString_(wrapper, &UciWrapper::MetricRecordGetUnit, record, unit)) {
                    dump += std::format("record[{}].unit=\"{}\"\n", i, unit);
                }
                if (wrapper.MetricRecordGetSample(record, &sample) == UC_SUCCESS) {
                    dump += std::format("record[{}].sample={}\n", i, sample);
                }
                if (wrapper.MetricRecordGetRecordTimestamp(record, &timestamp) == UC_SUCCESS) {
                    dump += std::format("record[{}].timestamp={}\n", i, timestamp);
                }
                if (wrapper.MetricRecordGetRecordDuration(record, &duration) == UC_SUCCESS) {
                    dump += std::format("record[{}].duration={}\n", i, duration);
                }
            }

            pmlog_verb(v::uci)(dump);
        }

        std::optional<uint32_t> TryParseCoreIndex_(const std::string& entity)
        {
            if (!entity.starts_with(kCoreEntityPrefix_)) {
                pmlog_dbg("unexpected core entity").pmwatch(entity);
                return std::nullopt;
            }

            const auto suffix = entity.substr(kCoreEntityPrefix_.size());
            if (suffix.empty()) {
                pmlog_dbg("missing core index").pmwatch(entity);
                return std::nullopt;
            }

            try {
                const std::string suffixString{ suffix };
                size_t parsedChars = 0;
                const auto value = std::stoul(suffixString, &parsedChars);
                if (parsedChars != suffixString.size() || value > uint32_t(-1)) {
                    pmlog_dbg("unexpected parsed chars or value")
                        .pmwatch(entity).pmwatch(parsedChars).pmwatch(value);
                    return std::nullopt;
                }
                return (uint32_t)value;
            }
            catch (const std::exception&) {
                pmlog_dbg(util::ReportException("Parse failure"));
                return std::nullopt;
            }
        }

        void AppendQuotedMetricName_(std::string& capabilitiesJson, const std::string& metricName)
        {
            if (!capabilitiesJson.empty()) {
                capabilitiesJson += ", ";
            }
            capabilitiesJson += std::format(R"json({{ "name": "{}" }})json", metricName);
        }

        size_t SkipJsonWhitespace_(const std::string& json, size_t pos)
        {
            while (pos < json.size() &&
                (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
                ++pos;
            }
            return pos;
        }

        bool ParseJsonString_(const std::string& json, size_t& pos, std::string& value)
        {
            value.clear();
            if (pos >= json.size() || json[pos] != '"') {
                return false;
            }

            ++pos;
            while (pos < json.size()) {
                const auto c = json[pos++];
                if (c == '"') {
                    return true;
                }
                if (c != '\\') {
                    value.push_back(c);
                    continue;
                }
                if (pos >= json.size()) {
                    return false;
                }

                const auto escaped = json[pos++];
                switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    value.push_back(escaped);
                    break;
                case 'b':
                    value.push_back('\b');
                    break;
                case 'f':
                    value.push_back('\f');
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                case 'u':
                    if (pos + 4 > json.size()) {
                        return false;
                    }
                    pos += 4;
                    value.push_back('?');
                    break;
                default:
                    return false;
                }
            }
            return false;
        }

        size_t FindMatchingJsonArrayEnd_(const std::string& json, size_t arrayStart)
        {
            bool inString = false;
            bool escaped = false;
            uint32_t depth = 0;
            for (size_t pos = arrayStart; pos < json.size(); ++pos) {
                const auto c = json[pos];
                if (inString) {
                    if (escaped) {
                        escaped = false;
                    }
                    else if (c == '\\') {
                        escaped = true;
                    }
                    else if (c == '"') {
                        inString = false;
                    }
                    continue;
                }

                if (c == '"') {
                    inString = true;
                }
                else if (c == '[') {
                    ++depth;
                }
                else if (c == ']') {
                    if (depth == 0) {
                        return std::string::npos;
                    }
                    --depth;
                    if (depth == 0) {
                        return pos;
                    }
                }
            }
            return std::string::npos;
        }

        std::unordered_set<std::string> ExtractCapabilityNames_(const std::string& capabilitiesJson)
        {
            std::unordered_set<std::string> names;
            const auto capabilitiesKey = capabilitiesJson.find("\"capabilities\"");
            if (capabilitiesKey == std::string::npos) {
                return names;
            }

            const auto arrayStart = capabilitiesJson.find('[', capabilitiesKey);
            if (arrayStart == std::string::npos) {
                return names;
            }

            const auto arrayEnd = FindMatchingJsonArrayEnd_(capabilitiesJson, arrayStart);
            if (arrayEnd == std::string::npos) {
                return names;
            }

            size_t pos = arrayStart + 1;
            while (pos < arrayEnd) {
                const auto quote = capabilitiesJson.find('"', pos);
                if (quote == std::string::npos || quote >= arrayEnd) {
                    break;
                }

                pos = quote;
                std::string key;
                if (!ParseJsonString_(capabilitiesJson, pos, key)) {
                    break;
                }

                pos = SkipJsonWhitespace_(capabilitiesJson, pos);
                if (pos >= arrayEnd || capabilitiesJson[pos] != ':') {
                    continue;
                }
                ++pos;

                if (key != "name") {
                    continue;
                }

                pos = SkipJsonWhitespace_(capabilitiesJson, pos);
                std::string name;
                if (ParseJsonString_(capabilitiesJson, pos, name) && !name.empty()) {
                    names.emplace(util::str::ToLower(name));
                }
            }

            return names;
        }
    }

    std::atomic<UciTelemetryProvider*> UciTelemetryProvider::activeProvider_{ nullptr };

    UciTelemetryProvider::UciTelemetryProvider()
    {
        if (!util::win::WeAreElevated()) {
            throw Except<TelemetrySubsystemAbsent>("UCI telemetry provider disabled because service is not elevated");
        }
        pUci_ = std::make_unique<UciWrapper>();

        systemDevice_.providerDeviceId = kProviderDeviceId_;
        systemDevice_.physicalCoreCount = CountPhysicalCores_();
        systemDevice_.fingerprint = BuildFingerprint_();
        const auto enumeratedMetricNames = EnumerateMetrics_();
        systemDevice_.caps = BuildCaps_(enumeratedMetricNames, systemDevice_.physicalCoreCount);
        systemDevice_.cpuCoreTemperaturesSample.resize(systemDevice_.physicalCoreCount);

        pmlog_info(std::format(
            "UCI telemetry provider initialized successfully; physicalCores={} enumeratedMetrics={}",
            systemDevice_.physicalCoreCount,
            enumeratedMetricNames.size()));
    }

    UciTelemetryProvider::~UciTelemetryProvider()
    {
        StopCollection_();
        auto* expected = this;
        activeProvider_.compare_exchange_strong(expected, nullptr);
    }

    ProviderCapabilityMap UciTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        capsByDeviceId.emplace(kProviderDeviceId_, systemDevice_.caps);
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& UciTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        if (providerDeviceId == kProviderDeviceId_) {
            return systemDevice_.fingerprint;
        }
        throw Except<UciException>("UCI provider device not found");
    }

    TelemetryMetricValue UciTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        (void)requestQpc;

        if (providerDeviceId != kProviderDeviceId_) {
            throw Except<UciException>("UCI provider device not found");
        }

        std::lock_guard dataLock{ dataMutex_ };
        ValidateMetricIndex_(systemDevice_, metricId, arrayIndex);

        switch (metricId) {
        case PM_METRIC_CPU_POWER:
            return systemDevice_.cpuPowerSample.value_or(0.0);
        case PM_METRIC_CPU_TEMPERATURE:
            return ComputeAverageCpuTemperature_(systemDevice_).value_or(0.0);
        case PM_METRIC_CPU_CORE_TEMPERATURE:
            return systemDevice_.cpuCoreTemperaturesSample[arrayIndex].value_or(0.0);
        default:
            throw Except<UciException>("Unsupported metric for UCI provider");
        }
    }

    void UciTelemetryProvider::SetPollRate(uint32_t pollRateMs)
    {
        std::lock_guard configLock{ configMutex_ };
        const auto effectivePollRateMs = std::max(pollRateMs, 1u);
        if (pollRateMs_ == effectivePollRateMs) {
            return;
        }

        pollRateMs_ = effectivePollRateMs;
        ReconfigureCollection_();
    }

    void UciTelemetryProvider::SetMetricUse(const svc::DeviceMetricUse& metricUse)
    {
        std::lock_guard configLock{ configMutex_ };
        bool wantsCpuPower = false;
        bool wantsCpuTemperature = false;
        bool wantsCpuCoreTemperature = false;
        if (const auto it = metricUse.find(ipc::kSystemDeviceId); it != metricUse.end()) {
            for (const auto& use : it->second) {
                if (use.metricId == PM_METRIC_CPU_POWER &&
                    systemDevice_.caps.Check(PM_METRIC_CPU_POWER) != 0) {
                    wantsCpuPower = true;
                }
                else if (use.metricId == PM_METRIC_CPU_TEMPERATURE &&
                    systemDevice_.caps.Check(PM_METRIC_CPU_TEMPERATURE) != 0) {
                    wantsCpuTemperature = true;
                }
                else if (use.metricId == PM_METRIC_CPU_CORE_TEMPERATURE &&
                    systemDevice_.caps.Check(PM_METRIC_CPU_CORE_TEMPERATURE) != 0) {
                    wantsCpuCoreTemperature = true;
                }
            }
        }

        if (wantsCpuPower_ == wantsCpuPower &&
            wantsCpuTemperature_ == wantsCpuTemperature &&
            wantsCpuCoreTemperature_ == wantsCpuCoreTemperature) {
            return;
        }

        wantsCpuPower_ = wantsCpuPower;
        wantsCpuTemperature_ = wantsCpuTemperature;
        wantsCpuCoreTemperature_ = wantsCpuCoreTemperature;
        ReconfigureCollection_();
    }

    void UciTelemetryProvider::StaticDataCallback_(uciDataBundle* dataBundle)
    {
        if (auto* pProvider = activeProvider_.load(std::memory_order_acquire)) {
            pProvider->OnDataCallback_(dataBundle);
        }
        else {
            pmlog_warn("UCI callback fired with no provider set").every(10s);
        }
    }

    uint32_t UciTelemetryProvider::CountPhysicalCores_()
    {
        DWORD bytes = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0) {
            return 0;
        }

        std::vector<uint8_t> buffer(bytes);
        if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data(),
            &bytes)) {
            return 0;
        }

        uint32_t coreCount = 0;
        for (DWORD offset = 0; offset < bytes;) {
            const auto* pInfo =
                (const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(buffer.data() + offset);
            if (pInfo->Relationship == RelationProcessorCore) {
                ++coreCount;
            }
            offset += pInfo->Size;
        }

        return coreCount;
    }

    std::optional<double> UciTelemetryProvider::ComputeAverageCpuTemperature_(const DeviceState_& device)
    {
        double sum = 0.0;
        uint32_t sampleCount = 0;
        for (const auto& sample : device.cpuCoreTemperaturesSample) {
            if (!sample) {
                continue;
            }
            sum += *sample;
            ++sampleCount;
        }

        if (sampleCount == 0) {
            return std::nullopt;
        }

        return sum / (double)sampleCount;
    }

    void UciTelemetryProvider::ValidateMetricIndex_(
        const DeviceState_& device,
        PM_METRIC metricId,
        uint32_t arrayIndex)
    {
        switch (metricId) {
        case PM_METRIC_CPU_POWER:
        case PM_METRIC_CPU_TEMPERATURE:
            if (arrayIndex != 0) {
                throw Except<UciException>("UCI scalar metric queried with nonzero array index");
            }
            return;
        case PM_METRIC_CPU_CORE_TEMPERATURE:
            if (arrayIndex >= device.cpuCoreTemperaturesSample.size()) {
                throw Except<UciException>("UCI CPU core temperature queried with out-of-range array index");
            }
            return;
        default:
            throw Except<UciException>("Unsupported UCI metric queried");
        }
    }

    TelemetryDeviceFingerprint UciTelemetryProvider::BuildFingerprint_()
    {
        TelemetryDeviceFingerprint fingerprint{};
        fingerprint.deviceType = PM_DEVICE_TYPE_SYSTEM;
        return fingerprint;
    }

    ipc::MetricCapabilities UciTelemetryProvider::BuildCaps_(
        const std::unordered_set<std::string>& enumeratedMetricNames,
        uint32_t physicalCoreCount)
    {
        ipc::MetricCapabilities caps{};
        if (enumeratedMetricNames.contains(std::string{ kCpuPowerMetricName_ })) {
            caps.Set(PM_METRIC_CPU_POWER, 1);
        }
        if (physicalCoreCount != 0 &&
            enumeratedMetricNames.contains(std::string{ kCpuTemperatureMetricName_ })) {
            caps.Set(PM_METRIC_CPU_TEMPERATURE, 1);
            caps.Set(PM_METRIC_CPU_CORE_TEMPERATURE, physicalCoreCount);
        }
        return caps;
    }

    std::unordered_set<std::string> UciTelemetryProvider::EnumerateMetrics_()
    {
        const char* capabilitiesJson = nullptr;
        CheckUciCall_(
            pUci_->GetCapabilities(&capabilitiesJson),
            "uciGetCapabilities",
            true);
        if (capabilitiesJson == nullptr) {
            throw Except<TelemetrySubsystemAbsent>("UCI capabilities unavailable because uciGetCapabilities returned null JSON");
        }

        const std::string capabilities{ capabilitiesJson };
        if (util::log::GlobalPolicy::VCheck(v::uci)) {
            DumpCapabilitiesJson_(capabilities);
        }

        return ExtractCapabilityNames_(capabilities);
    }

    void UciTelemetryProvider::DumpCapabilitiesJson_(const std::string& capabilitiesJson) const
    {
        pmlog_verb(v::uci)(std::format("UCI capabilities JSON:\n{}", capabilitiesJson));
    }

    void UciTelemetryProvider::OnDataCallback_(uciDataBundle* dataBundle)
    {
        if (util::log::GlobalPolicy::VCheck(v::uci)) {
            DumpDataCallback_(*pUci_, dataBundle);
        }

        if (dataBundle == nullptr || dataBundle->records == nullptr || dataBundle->num_records == 0) {
            return;
        }

        std::lock_guard dataLock{ dataMutex_ };
        for (uint16_t i = 0; i < dataBundle->num_records; ++i) {
            ApplyMetricRecord_(systemDevice_, dataBundle->records[i]);
        }
    }

    void UciTelemetryProvider::ApplyMetricRecord_(DeviceState_& device, void* recordHandle)
    {
        const auto record = (uciMetricRecordHandle)recordHandle;
        std::string metricName;
        if (!TryGetString_(*pUci_, &UciWrapper::MetricRecordGetMetricName, record, metricName) || metricName.empty()) {
            pmlog_dbg("Could not get record metric name");
            return;
        }

        double sample = 0.0;
        if (pUci_->MetricRecordGetSample(record, &sample) != UC_SUCCESS) {
            pmlog_dbg("Could not get record sample");
            return;
        }

        if (metricName == kCpuPowerRecordMetricName_) {
            uint64_t duration = 0;
            if (pUci_->MetricRecordGetRecordDuration(record, &duration) != UC_SUCCESS || duration == 0) {
                pmlog_dbg("Could not get record duration");
                return;
            }

            const auto durationSeconds = double(duration) / 1'000'000'000.0;
            if (durationSeconds <= 0.0) {
                pmlog_dbg("Negative duration").pmwatch(durationSeconds);
                return;
            }

            device.cpuPowerSample = (sample / durationSeconds) / 1000.0;
            return;
        }

        std::string entity;
        TryGetString_(*pUci_, &UciWrapper::MetricRecordGetEntity, record, entity);

        if (metricName != kCpuTemperatureRecordMetricName_) {
            return;
        }

        if (const auto coreIndex = TryParseCoreIndex_(entity)) {
            if (*coreIndex < device.cpuCoreTemperaturesSample.size()) {
                device.cpuCoreTemperaturesSample[*coreIndex] = sample;
            }
        }
        return;
    }

    void UciTelemetryProvider::ReconfigureCollection_()
    {
        const auto wantsCpuPower = wantsCpuPower_;
        const auto wantsCpuTemperature = wantsCpuTemperature_;
        const auto wantsCpuCoreTemperature = wantsCpuCoreTemperature_;
        const bool wantsCollection = wantsCpuPower || wantsCpuTemperature || wantsCpuCoreTemperature;
        pmlog_dbg(std::format(
            "Configuring UCI collection; pollRateMs={} wantsCpuPower={} wantsCpuTemperature={} wantsCpuCoreTemperature={} collectionStarted={}",
            pollRateMs_,
            wantsCpuPower,
            wantsCpuTemperature,
            wantsCpuCoreTemperature,
            collectionStarted_));

        if (!wantsCollection) {
            pmlog_dbg("Stopping UCI collection because no supported UCI metrics are requested");
            StopCollection_();
            std::lock_guard dataLock{ dataMutex_ };
            systemDevice_.cpuPowerSample.reset();
            std::fill(
                systemDevice_.cpuCoreTemperaturesSample.begin(),
                systemDevice_.cpuCoreTemperaturesSample.end(),
                std::nullopt);
            return;
        }

        StopCollection_();

        std::string capabilitiesJson;
        if (wantsCpuPower) {
            AppendQuotedMetricName_(capabilitiesJson, kCpuPowerMetricName_);
        }
        if (wantsCpuTemperature || wantsCpuCoreTemperature) {
            AppendQuotedMetricName_(capabilitiesJson, kCpuTemperatureMetricName_);
        }

        const auto configJson = std::format(
            R"json(
{{
  "collection_params": {{
    "sampling_interval_in_ns": {},
    "is_continuous": true,
    "max_detail": true
  }},
  "capabilities": [ {} ]
}}
)json",
            (uint64_t)pollRateMs_ * 1'000'000ull,
            capabilitiesJson);

        pmlog_dbg(std::format(
            "Applying UCI collection configuration; pollRateMs={} capabilities=[{}]",
            pollRateMs_,
            capabilitiesJson));
        pmlog_verb(v::uci)(std::format("UCI collection config JSON:\n{}", configJson));

        CheckUciCall_(pUci_->Configure(configJson.c_str()), "uciConfigure");
        CheckUciCall_(pUci_->SetDataCallback(&StaticDataCallback_), "uciSetDataCallback");

        auto* expected = (UciTelemetryProvider*)nullptr;
        if (!activeProvider_.compare_exchange_strong(expected, this, std::memory_order_acq_rel)) {
            activeProvider_.store(this, std::memory_order_release);
        }

        pmlog_dbg(std::format(
            "Starting UCI collection; pollRateMs={} wantsCpuPower={} wantsCpuTemperature={} wantsCpuCoreTemperature={}",
            pollRateMs_,
            wantsCpuPower,
            wantsCpuTemperature,
            wantsCpuCoreTemperature));
        CheckUciCall_(pUci_->Start(), "uciStart");
        collectionStarted_ = true;
    }

    void UciTelemetryProvider::StopCollection_() noexcept
    {
        if (!collectionStarted_) {
            return;
        }

        pmlog_dbg("Stopping UCI collection");
        const auto result = pUci_->Stop();
        if (result != UC_SUCCESS &&
            result != UC_ERROR_COLLECTION_NOT_RUNNING) {
            pmlog_warn("uciStop failed").pmwatch((int)result);
        }
        collectionStarted_ = false;
    }
}

#else

namespace pmon::tel::uci
{
    UciTelemetryProvider::UciTelemetryProvider()
    {
        throw Except<TelemetrySubsystemAbsent>("UCI telemetry provider unavailable because service was built without UCI dependency present.");
    }

    UciTelemetryProvider::~UciTelemetryProvider() = default;

    ProviderCapabilityMap UciTelemetryProvider::GetCaps()
    {
        return {};
    }

    const TelemetryDeviceFingerprint& UciTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        (void)providerDeviceId;
        throw Except<UciException>("UCI provider device not found");
    }

    TelemetryMetricValue UciTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        (void)providerDeviceId;
        (void)metricId;
        (void)arrayIndex;
        (void)requestQpc;
        throw Except<UciException>("UCI provider unavailable");
    }

    void UciTelemetryProvider::SetPollRate(uint32_t pollRateMs)
    {
        (void)pollRateMs;
    }

    void UciTelemetryProvider::SetMetricUse(const svc::DeviceMetricUse& metricUse)
    {
        (void)metricUse;
    }
}

#endif
