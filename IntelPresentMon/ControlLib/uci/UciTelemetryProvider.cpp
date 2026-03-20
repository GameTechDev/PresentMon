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
#include <string>
#include <string_view>
#include <vector>

using namespace pmon;
using namespace std::literals;

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

            MetricContainerGuard_() = default;
            MetricContainerGuard_(const MetricContainerGuard_&) = delete;
            MetricContainerGuard_& operator=(const MetricContainerGuard_&) = delete;
            MetricContainerGuard_(MetricContainerGuard_&&) = delete;
            MetricContainerGuard_& operator=(MetricContainerGuard_&&) = delete;

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

        void DumpDataCallback_(uciDataBundle* dataBundle)
        {
            if (dataBundle == nullptr) {
                pmlog_verb(v::uci)("UCI callback received null data bundle");
                return;
            }

            std::string dump = std::format(
                "UCI callback received numRecords={}\n",
                dataBundle->numRecords);
            if (dataBundle->records == nullptr) {
                pmlog_verb(v::uci)("records array pointer is null");
                return;
            }

            for (uint16_t i = 0; i < dataBundle->numRecords; ++i) {
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

                if (TryGetString_(uciMetricRecordGetMetricName, record, metricName)) {
                    dump += std::format("record[{}].metricName=\"{}\"\n", i, metricName);
                }
                if (TryGetString_(uciMetricRecordGetEntity, record, entity)) {
                    dump += std::format("record[{}].entity=\"{}\"\n", i, entity);
                }
                if (TryGetString_(uciMetricRecordGetDescriptor, record, descriptor)) {
                    dump += std::format("record[{}].descriptor=\"{}\"\n", i, descriptor);
                }
                if (TryGetString_(uciMetricRecordGetUnit, record, unit)) {
                    dump += std::format("record[{}].unit=\"{}\"\n", i, unit);
                }
                if (uciMetricRecordGetSample(record, &sample) == UC_SUCCESS) {
                    dump += std::format("record[{}].sample={}\n", i, sample);
                }
                if (uciMetricRecordGetRecordTimestamp(record, &timestamp) == UC_SUCCESS) {
                    dump += std::format("record[{}].timestamp={}\n", i, timestamp);
                }
                if (uciMetricRecordGetRecordDuration(record, &duration) == UC_SUCCESS) {
                    dump += std::format("record[{}].duration={}\n", i, duration);
                }
            }

            pmlog_verb(v::uci)(dump);
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
        if (collector_) {
            uciDestroy(collector_);
        }
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
        ValidateMetricIndex_(systemDevice_, metricId, arrayIndex);

        switch (metricId) {
        case PM_METRIC_CPU_POWER:
            return systemDevice_.cpuPowerSample.value_or(0.0);
        case PM_METRIC_CPU_TEMPERATURE:
            return systemDevice_.cpuCoreTemperaturesSample[arrayIndex].value_or(0.0);
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
        bool wantsCpuPower = false;
        bool wantsCpuTemperature = false;
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
            }
        }

        if (wantsCpuPower_ == wantsCpuPower &&
            wantsCpuTemperature_ == wantsCpuTemperature) {
            return;
        }

        wantsCpuPower_ = wantsCpuPower;
        wantsCpuTemperature_ = wantsCpuTemperature;
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
            if (arrayIndex >= device.cpuCoreTemperaturesSample.size()) {
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
        if (physicalCoreCount != 0 &&
            enumeratedMetricNames.contains(std::string{ kCpuTemperatureMetricName_ })) {
            caps.Set(PM_METRIC_CPU_TEMPERATURE, physicalCoreCount);
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
        std::string dump = "UCI metric enumeration dump\n";

        uint32_t metricCount = 0;
        uciMetricHandle* pMetrics = nullptr;
        auto result = uciGetMetricContainerMetrics(metricContainer, &metricCount, &pMetrics);
        dump += std::format("metrics.result={} metrics.count={}\n", (int)result, metricCount);
        if (result == UC_SUCCESS && pMetrics != nullptr) {
            for (uint32_t i = 0; i < metricCount; ++i) {
                uint32_t metricId = 0;
                std::string metricName;
                std::string metricDescription;
                auto metricIdResult = uciGetMetricId(pMetrics[i], &metricId);
                TryGetString_(uciGetMetricName, pMetrics[i], metricName);
                TryGetString_(uciGetMetricDescription, pMetrics[i], metricDescription);
                dump += std::format(
                    "metric[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                    i,
                    (int)metricIdResult,
                    metricId,
                    metricName,
                    metricDescription);

                uint32_t eventCount = 0;
                uciMetricEventHandle* pEvents = nullptr;
                auto eventResult = uciGetMetricEvents(pMetrics[i], &eventCount, &pEvents);
                dump += std::format(
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
                        dump += std::format(
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
        dump += std::format("groups.result={} groups.count={}\n", (int)result, groupCount);
        if (result == UC_SUCCESS && pGroups != nullptr) {
            for (uint32_t i = 0; i < groupCount; ++i) {
                uint32_t groupId = 0;
                std::string groupName;
                std::string groupDescription;
                auto groupIdResult = uciGetMetricGroupId(pGroups[i], &groupId);
                TryGetString_(uciGetMetricGroupName, pGroups[i], groupName);
                TryGetString_(uciGetMetricGroupDescription, pGroups[i], groupDescription);
                dump += std::format(
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
                dump += std::format(
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
                        dump += std::format(
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
        dump += std::format(
            "container.events.result={} container.events.count={}\n",
            (int)result,
            containerEventCount);
        if (result == UC_SUCCESS && pContainerEvents != nullptr) {
            for (uint32_t i = 0; i < containerEventCount; ++i) {
                uint32_t eventId = 0;
                std::string eventName;
                std::string eventDescription;
                auto eventIdResult = uciGetEventId(pContainerEvents[i], &eventId);
                TryGetString_(uciGetMetricEventName, pContainerEvents[i], eventName);
                TryGetString_(uciGetMetricEventDescription, pContainerEvents[i], eventDescription);
                dump += std::format(
                    "container.event[{}] id.result={} id={} name=\"{}\" description=\"{}\"\n",
                    i,
                    (int)eventIdResult,
                    eventId,
                    eventName,
                    eventDescription);
            }
        }

        pmlog_verb(v::uci)(dump);
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
            ApplyMetricRecord_(systemDevice_, dataBundle->records[i]);
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
        if (uciMetricRecordGetSample(record, &sample) != UC_SUCCESS) {
            pmlog_dbg("Could not get record sample");
            return;
        }

        if (metricName == kCpuPowerRecordMetricName_) {
            uint64_t duration = 0;
            if (uciMetricRecordGetRecordDuration(record, &duration) != UC_SUCCESS || duration == 0) {
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
        TryGetString_(uciMetricRecordGetEntity, record, entity);

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
        if (!collector_) {
            pmlog_error("no collector");
            return;
        }

        const auto wantsCpuPower = wantsCpuPower_;
        const auto wantsCpuTemperature = wantsCpuTemperature_;
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
            systemDevice_.cpuPowerSample.reset();
            std::fill(
                systemDevice_.cpuCoreTemperaturesSample.begin(),
                systemDevice_.cpuCoreTemperaturesSample.end(),
                std::nullopt);
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
