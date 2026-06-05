// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../../CommonUtilities/win/WinAPI.h"
#include "../../Interprocess/source/SystemDeviceId.h"
#include "../TelemetryProvider.h"
#include "UciSdk.h"
#if PMON_HAS_UCI_SDK
#include "UciWrapper.h"
#endif

#include <atomic>
#include <cstdint>
#include <memory>
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

        static uint32_t CountPhysicalCores_();
        static std::optional<double> ComputeAverageCpuTemperature_(const DeviceState_& device);
        static void ValidateMetricIndex_(const DeviceState_& device, PM_METRIC metricId, uint32_t arrayIndex);
        static TelemetryDeviceFingerprint BuildFingerprint_();
        static ipc::MetricCapabilities BuildCaps_(
            const std::unordered_set<std::string>& enumeratedMetricNames,
            uint32_t physicalCoreCount);

#if PMON_HAS_UCI_SDK
        static void StaticDataCallback_(uciDataBundle* dataBundle);
        void OnDataCallback_(uciDataBundle* dataBundle);
        void ApplyMetricRecord_(DeviceState_& device, void* recordHandle);
        std::unordered_set<std::string> EnumerateMetrics_();
        void DumpCapabilitiesJson_(const std::string& capabilitiesJson) const;
        void ReconfigureCollection_();
        void StopCollection_() noexcept;
#endif

    private:
        static constexpr ProviderDeviceId kProviderDeviceId_ = 1;
#if PMON_HAS_UCI_SDK
        std::unique_ptr<UciWrapper> pUci_{};
#endif
        DeviceState_ systemDevice_{};
        std::mutex configMutex_{};
        std::mutex dataMutex_{};
        uint32_t pollRateMs_ = 1000;
        bool collectionStarted_ = false;
        bool wantsCpuPower_ = false;
        bool wantsCpuTemperature_ = false;
        bool wantsCpuCoreTemperature_ = false;

#if PMON_HAS_UCI_SDK
        static std::atomic<UciTelemetryProvider*> activeProvider_;
#endif
    };
}
