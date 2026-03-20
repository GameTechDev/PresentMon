// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../../CommonUtilities/win/WinAPI.h"
#include "../../Interprocess/source/SystemDeviceId.h"
#include "../TelemetryProvider.h"
#include "inc/uci/uci-data-records.h"
#include "inc/uci/uci.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace pmon::tel::uci
{
    class UciTelemetryProvider : public TelemetryProvider
    {
    public:
        UciTelemetryProvider();
        ~UciTelemetryProvider() override;
        ProviderCapabilityMap GetCaps() override;
        const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const override;
        TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) override;
        void SetPollRate(uint32_t pollRateMs) override;
        void SetMetricUse(const svc::DeviceMetricUse& metricUse) override;

    private:
        struct DeviceState_
        {
            ProviderDeviceId providerDeviceId = 0;
            TelemetryDeviceFingerprint fingerprint{};
            ipc::MetricCapabilities caps{};
            uint32_t physicalCoreCount = 1;
            std::optional<double> cpuPowerSample{};
            std::vector<std::optional<double>> cpuCoreTemperaturesSample{};
        };

        static void StaticDataCallback_(uciDataBundle* dataBundle);
        static uint32_t CountPhysicalCores_();
        static void ValidateMetricIndex_(const DeviceState_& device, PM_METRIC metricId, uint32_t arrayIndex);
        static TelemetryDeviceFingerprint BuildFingerprint_();
        static ipc::MetricCapabilities BuildCaps_(
            const std::unordered_set<std::string>& enumeratedMetricNames,
            uint32_t physicalCoreCount);

        void OnDataCallback_(uciDataBundle* dataBundle);
        void ApplyMetricRecord_(DeviceState_& device, void* recordHandle);
        std::unordered_set<std::string> EnumerateMetrics_();
        void DumpMetricEnumeration_(uciMetricContainerHandle metricContainer) const;
        void ReconfigureCollection_();
        void StopCollection_() noexcept;

    private:
        static constexpr ProviderDeviceId kProviderDeviceId_ = 1;
        uciCollectorHandle collector_ = nullptr;
        DeviceState_ systemDevice_{};
        std::mutex configMutex_{};
        std::mutex dataMutex_{};
        uint32_t pollRateMs_ = 1000;
        bool collectionStarted_ = false;
        bool wantsCpuPower_ = false;
        bool wantsCpuTemperature_ = false;

        static std::atomic<UciTelemetryProvider*> activeProvider_;
    };
}
