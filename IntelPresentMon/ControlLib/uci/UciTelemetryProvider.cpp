// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "UciTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/GlobalPolicy.h"
#include "../../CommonUtilities/str/String.h"

#include "inc/uci/uci-versions.h"

#include <algorithm>
#include <exception>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace pmon;

namespace pmon::tel::uci
{
    namespace
    {
        using v = util::log::V;

        constexpr std::string_view kCpuPowerMetricName_ = "pkg-pwr";
        constexpr std::string_view kCpuTemperatureMetricName_ = "core-temp";
        constexpr std::string_view kCpuPowerRecordMetricName_ = "PKG-PWR";
        constexpr std::string_view kCpuTemperatureRecordMetricName_ = "TEMP";
        constexpr std::string_view kCoreEntityPrefix_ = "Core_";

        struct MetricContainerGuard_
        {
            uciMetricContainerHandle handle = nullptr;

            ~MetricContainerGuard_()
            {
                if (handle != nullptr) {
                    uciFreeMetricContainer(handle);
                }
            }
        };

        [[noreturn]] void ThrowUciError_(uc_result_t result, const char* call, bool subsystemAbsent)
        {
            const auto message = std::format("UCI call [{}] failed with code [{}]", call, (int)result);
            if (subsystemAbsent) {
                throw Except<TelemetrySubsystemAbsent>(message);
            }
            throw Except<>(message);
        }

        void CheckUciCall_(uc_result_t result, const char* call, bool subsystemAbsent = false)
        {
            if (result != UC_SUCCESS) {
                ThrowUciError_(result, call, subsystemAbsent);
            }
        }

        bool TryGetString_(auto getter, auto handle, std::string& value)
        {
            char* pText = nullptr;
            if (getter(handle, &pText) != UC_SUCCESS || pText == nullptr) {
                value.clear();
                return false;
            }
            value = pText;
            return true;
        }

        bool TryGetSample_(auto getter, uciMetricRecordHandle record, double& sample)
        {
            return getter(record, &sample) == UC_SUCCESS;
        }

        bool TryGetDuration_(auto getter, uciMetricRecordHandle record, uint64_t& duration)
        {
            return getter(record, &duration) == UC_SUCCESS;
        }

        void DumpDataCallback_(uciDataBundle* dataBundle)
        {
            std::ostringstream oss;
            if (dataBundle == nullptr) {
                oss << "UCI callback received null data bundle";
                pmlog_verb(v::uci)(oss.str());
                return;
            }

            oss << "UCI callback received numRecords=" << dataBundle->numRecords << '\n';
            if (dataBundle->records == nullptr) {
                oss << "records pointer is null";
                pmlog_verb(v::uci)(oss.str());
                return;
            }

            for (uint16_t i = 0; i < dataBundle->numRecords; ++i) {
                const auto record = dataBundle->records[i];
                oss << "record[" << i << "] handle=" << record << '\n';
                if (record == nullptr) {
                    continue;
                }

                std::string metricName;
                std::string entity;
                std::string descriptor;
                std::string unit;
                double sample = 0.0;
                uint64_t timestamp = 0;
                uint64_t duration = 0;

                if (TryGetString_(uciMetricRecordGetMetricName, record, metricName)) {
                    oss << "record[" << i << "].metricName=\"" << metricName << "\"\n";
                }
                if (TryGetString_(uciMetricRecordGetEntity, record, entity)) {
                    oss << "record[" << i << "].entity=\"" << entity << "\"\n";
                }
                if (TryGetString_(uciMetricRecordGetDescriptor, record, descriptor)) {
                    oss << "record[" << i << "].descriptor=\"" << descriptor << "\"\n";
                }
                if (TryGetString_(uciMetricRecordGetUnit, record, unit)) {
                    oss << "record[" << i << "].unit=\"" << unit << "\"\n";
                }
                if (TryGetSample_(uciMetricRecordGetSample, record, sample)) {
                    oss << "record[" << i << "].sample=" << sample << '\n';
                }
                if (TryGetDuration_(uciMetricRecordGetRecordTimestamp, record, timestamp)) {
                    oss << "record[" << i << "].timestamp=" << timestamp << '\n';
                }
                if (TryGetDuration_(uciMetricRecordGetRecordDuration, record, duration)) {
                    oss << "record[" << i << "].duration=" << duration << '\n';
                }
            }

            pmlog_verb(v::uci)(oss.str());
        }

        std::optional<uint32_t> TryParseCoreIndex_(std::string_view entity)
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

        bool IsCpuTemperatureRecord_(
            std::string_view metricName,
            std::string_view entity)
        {
            return metricName == kCpuTemperatureRecordMetricName_ &&
                TryParseCoreIndex_(entity).has_value();
        }

        void AppendQuotedMetricName_(std::string& metricsJson, std::string_view metricName)
        {
            if (!metricsJson.empty()) {
                metricsJson += ", ";
            }
            metricsJson += std::format("\"{}\"", metricName);
        }
    }

    std::atomic<UciTelemetryProvider*> UciTelemetryProvider::activeProvider_{ nullptr };

    UciTelemetryProvider::UciTelemetryProvider()
    {
        CheckUciCall_(
            uciGetCollectorFromIdentifier(SoCWatchIdentifier, &collector_),
            "uciGetCollectorFromIdentifier",
            true);
        CheckUciCall_(uciInitialize(collector_), "uciInitialize", true);

        device_.providerDeviceId = kProviderDeviceId_;
        device_.physicalCoreCount = CountPhysicalCores_();
        device_.fingerprint = BuildFingerprint_();
        enumeratedMetricNames_ = EnumerateMetrics_();
        device_.caps = BuildCaps_(enumeratedMetricNames_, device_.physicalCoreCount);
        device_.samples.cpuTemperatures.resize(device_.physicalCoreCount, 0.0);
        device_.samples.hasCpuTemperature.resize(device_.physicalCoreCount, false);

        pmlog_info(std::format(
            "UCI telemetry provider initialized successfully; physicalCores={} enumeratedMetrics={}",
            device_.physicalCoreCount,
            enumeratedMetricNames_.size()));
    }

    UciTelemetryProvider::~UciTelemetryProvider()
    {
        StopCollection_();
        auto* expected = this;
        activeProvider_.compare_exchange_strong(expected, nullptr);
        if (collector_) {
            uciDestroy(collector_);
        }
    }

    ProviderCapabilityMap UciTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        capsByDeviceId.emplace(kProviderDeviceId_, device_.caps);
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& UciTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        if (providerDeviceId == kProviderDeviceId_) {
            return device_.fingerprint;
        }
        throw Except<>("UCI provider device not found");
    }

    TelemetryMetricValue UciTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        (void)requestQpc;

        if (providerDeviceId != kProviderDeviceId_) {
            throw Except<>("UCI provider device not found");
        }

        std::lock_guard dataLock{ dataMutex_ };
        ValidateMetricIndex_(device_, metricId, arrayIndex);

        switch (metricId) {
        case PM_METRIC_CPU_POWER:
            return device_.samples.hasCpuPower ? device_.samples.cpuPower : 0.0;
        case PM_METRIC_CPU_TEMPERATURE:
            return device_.samples.hasCpuTemperature[arrayIndex] ?
                device_.samples.cpuTemperatures[arrayIndex] : 0.0;
        default:
            throw Except<>("Unsupported metric for UCI provider");
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
        std::unordered_set<svc::MetricUse> systemMetricUse;
        if (const auto it = metricUse.find(ipc::kSystemDeviceId); it != metricUse.end()) {
            systemMetricUse = it->second;
        }

        if (metricUse_ == systemMetricUse) {
            return;
        }

        metricUse_ = std::move(systemMetricUse);
        ReconfigureCollection_();
    }

    void UciTelemetryProvider::StaticDataCallback_(uciDataBundle* dataBundle)
    {
        if (auto* pProvider = activeProvider_.load(std::memory_order_acquire)) {
            pProvider->OnDataCallback_(dataBundle);
        }
    }

    uint32_t UciTelemetryProvider::CountPhysicalCores_()
    {
        DWORD bytes = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0) {
            return 1;
        }

        std::vector<uint8_t> buffer(bytes);
        if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data(),
            &bytes)) {
            return 1;
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

        return coreCount != 0 ? coreCount : 1;
    }

    void UciTelemetryProvider::ValidateMetricIndex_(
        const DeviceState_& device,
        PM_METRIC metricId,
        uint32_t arrayIndex)
    {
        switch (metricId) {
        case PM_METRIC_CPU_POWER:
            if (arrayIndex != 0) {
                throw Except<>("UCI scalar metric queried with nonzero array index");
            }
            return;
        case PM_METRIC_CPU_TEMPERATURE:
            if (arrayIndex >= device.samples.cpuTemperatures.size()) {
                throw Except<>("UCI CPU temperature queried with out-of-range array index");
            }
            return;
        default:
            throw Except<>("Unsupported UCI metric queried");
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
        if (enumeratedMetricNames.contains(std::string{ kCpuTemperatureMetricName_ })) {
            caps.Set(PM_METRIC_CPU_TEMPERATURE, physicalCoreCount != 0 ? physicalCoreCount : 1);
        }
        return caps;
    }

    std::unordered_set<std::string> UciTelemetryProvider::EnumerateMetrics_()
    {
        MetricContainerGuard_ metricContainer{};
        CheckUciCall_(uciEnumerateMetrics(collector_, &metricContainer.handle), "uciEnumerateMetrics");

        if (util::log::GlobalPolicy::VCheck(v::uci)) {
            DumpMetricEnumeration_(metricContainer.handle);
        }

        uint32_t metricCount = 0;
        uciMetricHandle* pMetrics = nullptr;
        CheckUciCall_(
            uciGetMetricContainerMetrics(metricContainer.handle, &metricCount, &pMetrics),
            "uciGetMetricContainerMetrics");

        std::unordered_set<std::string> enumeratedMetricNames{};
        for (uint32_t i = 0; i < metricCount; ++i) {
            std::string metricName;
            if (!TryGetString_(uciGetMetricName, pMetrics[i], metricName) || metricName.empty()) {
                continue;
            }

            enumeratedMetricNames.emplace(util::str::ToLower(metricName));
        }

        return enumeratedMetricNames;
    }

    void UciTelemetryProvider::DumpMetricEnumeration_(uciMetricContainerHandle metricContainer) const
    {
        std::ostringstream oss;
        oss << "UCI metric enumeration dump\n";

        uint32_t metricCount = 0;
        uciMetricHandle* pMetrics = nullptr;
        auto result = uciGetMetricContainerMetrics(metricContainer, &metricCount, &pMetrics);
        oss << "metrics.result=" << (int)result << " metrics.count=" << metricCount << '\n';
        if (result == UC_SUCCESS && pMetrics != nullptr) {
            for (uint32_t i = 0; i < metricCount; ++i) {
                uint32_t metricId = 0;
                std::string metricName;
                std::string metricDescription;
                auto metricIdResult = uciGetMetricId(pMetrics[i], &metricId);
                TryGetString_(uciGetMetricName, pMetrics[i], metricName);
                TryGetString_(uciGetMetricDescription, pMetrics[i], metricDescription);
                oss << std::format(
                    "metric[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                    i,
                    (int)metricIdResult,
                    metricId,
                    metricName,
                    metricDescription);

                uint32_t eventCount = 0;
                uciMetricEventHandle* pEvents = nullptr;
                auto eventResult = uciGetMetricEvents(pMetrics[i], &eventCount, &pEvents);
                oss << std::format(
                    "metric[{}].events.result={} metric[{}].events.count={}\n",
                    i,
                    (int)eventResult,
                    i,
                    eventCount);
                if (eventResult == UC_SUCCESS && pEvents != nullptr) {
                    for (uint32_t j = 0; j < eventCount; ++j) {
                        uint32_t eventId = 0;
                        std::string eventName;
                        std::string eventDescription;
                        auto eventIdResult = uciGetEventId(pEvents[j], &eventId);
                        TryGetString_(uciGetMetricEventName, pEvents[j], eventName);
                        TryGetString_(uciGetMetricEventDescription, pEvents[j], eventDescription);
                        oss << std::format(
                            "metric[{}].event[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                            i,
                            j,
                            (int)eventIdResult,
                            eventId,
                            eventName,
                            eventDescription);
                    }
                }
            }
        }

        uint32_t groupCount = 0;
        uciMetricGroupHandle* pGroups = nullptr;
        result = uciGetMetricContainerGroups(metricContainer, &groupCount, &pGroups);
        oss << "groups.result=" << (int)result << " groups.count=" << groupCount << '\n';
        if (result == UC_SUCCESS && pGroups != nullptr) {
            for (uint32_t i = 0; i < groupCount; ++i) {
                uint32_t groupId = 0;
                std::string groupName;
                std::string groupDescription;
                auto groupIdResult = uciGetMetricGroupId(pGroups[i], &groupId);
                TryGetString_(uciGetMetricGroupName, pGroups[i], groupName);
                TryGetString_(uciGetMetricGroupDescription, pGroups[i], groupDescription);
                oss << std::format(
                    "group[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                    i,
                    (int)groupIdResult,
                    groupId,
                    groupName,
                    groupDescription);

                uint32_t groupMetricCount = 0;
                uciMetricHandle* pGroupMetrics = nullptr;
                auto groupMetricResult =
                    uciGetMetricsInGroup(pGroups[i], &groupMetricCount, &pGroupMetrics);
                oss << std::format(
                    "group[{}].metrics.result={} group[{}].metrics.count={}\n",
                    i,
                    (int)groupMetricResult,
                    i,
                    groupMetricCount);
                if (groupMetricResult == UC_SUCCESS && pGroupMetrics != nullptr) {
                    for (uint32_t j = 0; j < groupMetricCount; ++j) {
                        std::string groupMetricName;
                        std::string groupMetricDescription;
                        TryGetString_(uciGetMetricName, pGroupMetrics[j], groupMetricName);
                        TryGetString_(uciGetMetricDescription, pGroupMetrics[j], groupMetricDescription);
                        oss << std::format(
                            "group[{}].metric[{}] name=\"{}\" description=\"{}\"\n",
                            i,
                            j,
                            groupMetricName,
                            groupMetricDescription);
                    }
                }
            }
        }

        uint32_t containerEventCount = 0;
        uciMetricEventHandle* pContainerEvents = nullptr;
        result = uciGetMetricContainerEvents(
            metricContainer,
            &containerEventCount,
            &pContainerEvents);
        oss << "container.events.result=" << (int)result
            << " container.events.count=" << containerEventCount << '\n';
        if (result == UC_SUCCESS && pContainerEvents != nullptr) {
            for (uint32_t i = 0; i < containerEventCount; ++i) {
                uint32_t eventId = 0;
                std::string eventName;
                std::string eventDescription;
                auto eventIdResult = uciGetEventId(pContainerEvents[i], &eventId);
                TryGetString_(uciGetMetricEventName, pContainerEvents[i], eventName);
                TryGetString_(uciGetMetricEventDescription, pContainerEvents[i], eventDescription);
                oss << std::format(
                    "container.event[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                    i,
                    (int)eventIdResult,
                    eventId,
                    eventName,
                    eventDescription);
            }
        }

        pmlog_verb(v::uci)(oss.str());
    }

    void UciTelemetryProvider::OnDataCallback_(uciDataBundle* dataBundle)
    {
        if (util::log::GlobalPolicy::VCheck(v::uci)) {
            DumpDataCallback_(dataBundle);
        }

        if (dataBundle == nullptr || dataBundle->records == nullptr || dataBundle->numRecords == 0) {
            return;
        }

        std::lock_guard dataLock{ dataMutex_ };
        for (uint16_t i = 0; i < dataBundle->numRecords; ++i) {
            ApplyMetricRecord_(device_, dataBundle->records[i]);
        }
    }

    void UciTelemetryProvider::ApplyMetricRecord_(DeviceState_& device, void* recordHandle)
    {
        const auto record = (uciMetricRecordHandle)recordHandle;
        std::string metricName;
        if (!TryGetString_(uciMetricRecordGetMetricName, record, metricName) || metricName.empty()) {
            pmlog_dbg("Could not get record metric name");
            return;
        }

        double sample = 0.0;
        if (!TryGetSample_(uciMetricRecordGetSample, record, sample)) {
            pmlog_dbg("Could not get record sample");
            return;
        }

        if (metricName == kCpuPowerRecordMetricName_) {
            uint64_t duration = 0;
            if (!TryGetDuration_(uciMetricRecordGetRecordDuration, record, duration) || duration == 0) {
                pmlog_dbg("Could not get record duration");
                return;
            }

            const auto durationSeconds = double(duration) / 1'000'000'000.0;
            if (durationSeconds <= 0.0) {
                pmlog_dbg("Negative duration").pmwatch(durationSeconds);
                return;
            }

            device.samples.cpuPower = (sample / durationSeconds) / 1000.0;
            device.samples.hasCpuPower = true;
            return;
        }

        std::string entity;
        TryGetString_(uciMetricRecordGetEntity, record, entity);

        if (!IsCpuTemperatureRecord_(metricName, entity)) {
            return;
        }

        if (const auto coreIndex = TryParseCoreIndex_(entity)) {
            if (*coreIndex < device.samples.cpuTemperatures.size()) {
                device.samples.cpuTemperatures[*coreIndex] = sample;
                device.samples.hasCpuTemperature[*coreIndex] = true;
            }
            return;
        }

    }

    void UciTelemetryProvider::ReconfigureCollection_()
    {
        if (!collector_) {
            return;
        }

        bool wantsCpuPower = false;
        bool wantsCpuTemperature = false;
        for (const auto& use : metricUse_) {
            if (use.metricId == PM_METRIC_CPU_POWER &&
                enumeratedMetricNames_.contains(std::string{ kCpuPowerMetricName_ })) {
                wantsCpuPower = true;
            }
            else if (use.metricId == PM_METRIC_CPU_TEMPERATURE &&
                enumeratedMetricNames_.contains(std::string{ kCpuTemperatureMetricName_ })) {
                wantsCpuTemperature = true;
            }
        }

        const bool wantsCollection = wantsCpuPower || wantsCpuTemperature;
        pmlog_dbg(std::format(
            "Configuring UCI collection; pollRateMs={} wantsCpuPower={} wantsCpuTemperature={} collectionStarted={}",
            pollRateMs_,
            wantsCpuPower,
            wantsCpuTemperature,
            collectionStarted_));

        if (!wantsCollection) {
            pmlog_dbg("Stopping UCI collection because no supported UCI metrics are requested");
            StopCollection_();
            std::lock_guard dataLock{ dataMutex_ };
            device_.samples.cpuPower = 0.0;
            device_.samples.hasCpuPower = false;
            std::fill(device_.samples.cpuTemperatures.begin(), device_.samples.cpuTemperatures.end(), 0.0);
            std::fill(device_.samples.hasCpuTemperature.begin(), device_.samples.hasCpuTemperature.end(), false);
            return;
        }

        StopCollection_();

        std::string metricsJson;
        if (wantsCpuPower) {
            AppendQuotedMetricName_(metricsJson, kCpuPowerMetricName_);
        }
        if (wantsCpuTemperature) {
            AppendQuotedMetricName_(metricsJson, kCpuTemperatureMetricName_);
        }

        const auto configJson = std::format(
            R"json(
{{
  "collectorIdentifier": {{
    "name": "socwatch",
    "guid": "54B46B56-5439-4D29-8B07-15863D1F6EC6",
    "version": "0.1.0"
  }},
  "collectionParams": {{
    "samplingInterval": {},
    "isContinuous": true,
    "maxDetail": true
  }},
  "metrics": [ {} ],
  "outputParams": {{
    "outputFormats": [ "data-callback" ],
    "outputPath": "UCI_metrics"
  }}
}}
)json",
            pollRateMs_,
            metricsJson);

        pmlog_dbg(std::format(
            "Applying UCI collection configuration; pollRateMs={} metrics=[{}]",
            pollRateMs_,
            metricsJson));
        pmlog_verb(v::uci)(std::format("UCI collection config JSON:\n{}", configJson));

        CheckUciCall_(uciConfigureCollection(collector_, configJson.c_str()), "uciConfigureCollection");
        CheckUciCall_(uciSetDataCallback(collector_, &StaticDataCallback_), "uciSetDataCallback");

        auto* expected = (UciTelemetryProvider*)nullptr;
        if (!activeProvider_.compare_exchange_strong(expected, this, std::memory_order_acq_rel)) {
            activeProvider_.store(this, std::memory_order_release);
        }

        pmlog_dbg(std::format(
            "Starting UCI collection; pollRateMs={} wantsCpuPower={} wantsCpuTemperature={}",
            pollRateMs_,
            wantsCpuPower,
            wantsCpuTemperature));
        CheckUciCall_(uciStartCollection(collector_), "uciStartCollection");
        collectionStarted_ = true;
    }

    void UciTelemetryProvider::StopCollection_() noexcept
    {
        if (!collectionStarted_ || !collector_) {
            return;
        }

        pmlog_dbg("Stopping UCI collection");
        const auto result = uciStopCollection(collector_);
        if (result != UC_SUCCESS &&
            result != UC_ERROR_COLLECTION_NOT_RUNNING) {
            pmlog_warn("uciStopCollection failed").pmwatch((int)result);
        }
        collectionStarted_ = false;
    }
}
