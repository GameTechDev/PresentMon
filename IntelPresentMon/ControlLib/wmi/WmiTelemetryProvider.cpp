// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "WmiTelemetryProvider.h"

#include "../Exceptions.h"
#include "../Logging.h"
#include "../../CommonUtilities/Qpc.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/str/String.h"
#include "../../CommonUtilities/win/com/ComPtr.h"
#include "../../CommonUtilities/win/com/Comdef.h"
#include "../../CommonUtilities/win/com/WbemConnection.h"

#include <Wbemidl.h>

#include <chrono>
#include <string>
#include <string_view>

using namespace pmon;
using namespace util;

namespace pmon::tel::wmi
{
    namespace
    {
        constexpr wchar_t kCpuIdentityQuery_[] = L"SELECT Name, Manufacturer FROM Win32_Processor";
        constexpr wchar_t kProcessorFrequencyCounter_[] =
            L"\\Processor Information(_Total)\\Processor Frequency";
        constexpr wchar_t kProcessorPerformanceCounter_[] =
            L"\\Processor Information(_Total)\\% Processor Performance";
        constexpr wchar_t kProcessorIdleTimeCounter_[] =
            L"\\Processor(_Total)\\% Idle Time";

        std::string ReadWmiStringProperty_(IWbemClassObject& object, const wchar_t* propertyName)
        {
            VARIANT variant{};
            VariantInit(&variant);

            const auto result = object.Get(propertyName, 0, &variant, nullptr, nullptr);
            if (FAILED(result)) {
                pmlog_warn("IWbemClassObject::Get failed").hr(result)
                    .pmwatch(str::ToNarrow(std::wstring{ propertyName }));
                VariantClear(&variant);
                return {};
            }

            std::string value{};
            if (variant.vt == VT_BSTR && variant.bstrVal != nullptr) {
                value = str::TrimWhitespace(
                    str::ToNarrow(std::wstring{ variant.bstrVal, SysStringLen(variant.bstrVal) }));
            }
            else if (variant.vt != VT_NULL && variant.vt != VT_EMPTY) {
                pmlog_warn("Unexpected VARIANT type for WMI string property")
                    .pmwatch(str::ToNarrow(std::wstring{ propertyName }))
                    .pmwatch((int)variant.vt);
            }

            VariantClear(&variant);
            return value;
        }

        PM_DEVICE_VENDOR ResolveCpuVendor_(std::string_view manufacturer, std::string_view cpuName)
        {
            const auto matchVendor = [](std::string_view value) {
                const auto lowerValue = str::ToLower(std::string{ value });
                if (lowerValue.contains("intel")) {
                    return PM_DEVICE_VENDOR_INTEL;
                }
                if (lowerValue.contains("amd")) {
                    return PM_DEVICE_VENDOR_AMD;
                }
                return PM_DEVICE_VENDOR_UNKNOWN;
            };

            const auto manufacturerVendor = matchVendor(manufacturer);
            if (manufacturerVendor != PM_DEVICE_VENDOR_UNKNOWN) {
                return manufacturerVendor;
            }

            return matchVendor(cpuName);
        }
    }

    void WmiTelemetryProvider::QueryDeleter_::operator()(PDH_HQUERY query) const noexcept
    {
        if (query != nullptr) {
            PdhCloseQuery(query);
        }
    }

    WmiTelemetryProvider::WmiTelemetryProvider()
    {
        const auto providerDeviceId = nextProviderDeviceId_;
        const auto emplaceResult = devicesById_.try_emplace(providerDeviceId);
        if (!emplaceResult.second) {
            throw Except<>("Duplicate WMI provider device id encountered");
        }

        auto& device = emplaceResult.first->second;
        device.providerDeviceId = providerDeviceId;
        device.fingerprint.deviceType = PM_DEVICE_TYPE_SYSTEM;
        device.fingerprint.vendor = PM_DEVICE_VENDOR_UNKNOWN;
        device.fingerprint.deviceName = "UNKNOWN_CPU";

        InitializeDevice_(device);
        device.caps = BuildCapsForDevice_(device);
        ++nextProviderDeviceId_;
    }

    ProviderCapabilityMap WmiTelemetryProvider::GetCaps()
    {
        ProviderCapabilityMap capsByDeviceId{};
        for (const auto& entry : devicesById_) {
            capsByDeviceId.emplace(entry.first, entry.second.caps);
        }
        return capsByDeviceId;
    }

    const TelemetryDeviceFingerprint& WmiTelemetryProvider::GetFingerPrint(
        ProviderDeviceId providerDeviceId) const
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("WMI provider device not found");
        }
        return iDevice->second.fingerprint;
    }

    TelemetryMetricValue WmiTelemetryProvider::PollMetric(
        ProviderDeviceId providerDeviceId,
        PM_METRIC metricId,
        uint32_t arrayIndex,
        int64_t requestQpc)
    {
        const auto iDevice = devicesById_.find(providerDeviceId);
        if (iDevice == devicesById_.end()) {
            throw Except<>("WMI provider device not found");
        }

        auto& device = iDevice->second;

        switch (metricId) {
        case PM_METRIC_CPU_VENDOR:
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            return (int)device.fingerprint.vendor;
        case PM_METRIC_CPU_NAME:
            throw Except<>("PM_METRIC_CPU_NAME is static-only and is not served by poll path");
        case PM_METRIC_CPU_FREQUENCY:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pSample = PollCounterSampleEndpoint_(device, requestQpc);
            if (pSample == nullptr || !pSample->hasFrequency) {
                return 0.0;
            }
            return pSample->frequencyMhz;
        }
        case PM_METRIC_CPU_UTILIZATION:
        {
            ValidateScalarMetricIndex_(metricId, arrayIndex);
            const auto* pSample = PollCounterSampleEndpoint_(device, requestQpc);
            if (pSample == nullptr || !pSample->hasUtilization) {
                return 0.0;
            }
            return pSample->utilizationPercent;
        }
        default:
            throw Except<>("Unsupported metric for WMI provider");
        }
    }

    void WmiTelemetryProvider::ValidateScalarMetricIndex_(PM_METRIC metricId, uint32_t arrayIndex)
    {
        if (arrayIndex != 0) {
            throw Except<>("WMI scalar metric queried with nonzero array index");
        }
        (void)metricId;
    }

    void WmiTelemetryProvider::InitializeDevice_(DeviceState_& device) const
    {
        PopulateFingerprintFromWmi_(device);

        PDH_HQUERY tempQuery = nullptr;
        const auto openResult = PdhOpenQueryW(nullptr, 0, &tempQuery);
        if (openResult != ERROR_SUCCESS) {
            pmlog_error("PdhOpenQueryW failed").code(openResult);
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize WMI telemetry provider");
        }
        device.query.reset(tempQuery);

        const auto addCounter = [&](const wchar_t* path, HCOUNTER& counter, const char* label) {
            const auto addResult = PdhAddEnglishCounterW(device.query.get(), path, 0, &counter);
            if (addResult != ERROR_SUCCESS) {
                pmlog_warn(label).code(addResult)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
                counter = nullptr;
            }
        };

        addCounter(
            kProcessorFrequencyCounter_,
            device.processorFrequencyCounter,
            "PdhAddEnglishCounterW failed for processor frequency");
        addCounter(
            kProcessorPerformanceCounter_,
            device.processorPerformanceCounter,
            "PdhAddEnglishCounterW failed for processor performance");
        addCounter(
            kProcessorIdleTimeCounter_,
            device.processorIdleTimeCounter,
            "PdhAddEnglishCounterW failed for processor idle time");

        const auto collectResult = PdhCollectQueryData(device.query.get());
        if (collectResult != ERROR_SUCCESS) {
            pmlog_error("PdhCollectQueryData initial collect failed").code(collectResult)
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            throw Except<TelemetrySubsystemAbsent>("Unable to initialize WMI telemetry provider");
        }

        device.qpcFrequency = GetTimestampFrequencyUint64();
        // Seed the gate to "now" so the first post-init probe performs the
        // second PDH collection needed for formatted CPU counters.
        device.nextSampleQpc = GetCurrentTimestamp();
    }

    void WmiTelemetryProvider::PopulateFingerprintFromWmi_(DeviceState_& device) const
    {
        try {
            win::com::WbemConnection connection;
            auto* pServices = connection.GetServices();
            if (pServices == nullptr) {
                pmlog_warn("WMI services connection was null")
                    .pmwatch(device.providerDeviceId);
                return;
            }

            win::com::ComPtr<IEnumWbemClassObject> pEnumerator;
            const auto queryResult = pServices->ExecQuery(
                bstr_t(L"WQL"),
                bstr_t(kCpuIdentityQuery_),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                nullptr,
                pEnumerator.ReleaseAndGetAddressOf());
            if (FAILED(queryResult)) {
                pmlog_warn("IWbemServices::ExecQuery failed").hr(queryResult)
                    .pmwatch(device.providerDeviceId);
                return;
            }

            DWORD returned = 0;
            win::com::ComPtr<IWbemClassObject> pClassObject;
            const auto nextResult = pEnumerator->Next(
                WBEM_INFINITE,
                1,
                pClassObject.ReleaseAndGetAddressOf(),
                &returned);
            if (FAILED(nextResult) || returned == 0) {
                pmlog_warn("IEnumWbemClassObject::Next failed").hr(nextResult)
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(returned);
                return;
            }

            if (const auto cpuName = ReadWmiStringProperty_(*pClassObject.Get(), L"Name");
                !cpuName.empty()) {
                device.fingerprint.deviceName = cpuName;
            }
            else {
                pmlog_warn("WMI CPU name query returned empty result")
                    .pmwatch(device.providerDeviceId);
            }

            const auto manufacturer = ReadWmiStringProperty_(*pClassObject.Get(), L"Manufacturer");
            device.fingerprint.vendor = ResolveCpuVendor_(manufacturer, device.fingerprint.deviceName);
            if (device.fingerprint.vendor == PM_DEVICE_VENDOR_UNKNOWN) {
                pmlog_warn("Unable to resolve CPU vendor from WMI")
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(manufacturer)
                    .pmwatch(device.fingerprint.deviceName);
            }
        }
        catch (...) {
            pmlog_warn(util::ReportException("WMI CPU identity query failed"))
                .pmwatch(device.providerDeviceId);
        }
    }

    ipc::MetricCapabilities WmiTelemetryProvider::BuildCapsForDevice_(DeviceState_& device) const
    {
        ipc::MetricCapabilities caps{};
        const auto requestQpc = GetCurrentTimestamp();

        caps.Set(PM_METRIC_CPU_VENDOR, 1);
        caps.Set(PM_METRIC_CPU_NAME, 1);

        const auto* pSample = PollCounterSampleEndpoint_(device, requestQpc);
        if (pSample != nullptr && pSample->hasFrequency) {
            caps.Set(PM_METRIC_CPU_FREQUENCY, 1);
        }
        if (pSample != nullptr && pSample->hasUtilization) {
            caps.Set(PM_METRIC_CPU_UTILIZATION, 1);
        }

        return caps;
    }

    const WmiTelemetryProvider::CounterSample_* WmiTelemetryProvider::PollCounterSampleEndpoint_(
        DeviceState_& device,
        int64_t requestQpc) const
    {
        auto& cache = device.counterSampleEndpointCache;
        if (cache.Matches(requestQpc)) {
            return &cache.output;
        }

        cache.output = {};
        cache.requestQpc = requestQpc;

        const auto sampleQpc = requestQpc != 0 ? requestQpc : GetCurrentTimestamp();
        if (device.query &&
            (device.nextSampleQpc == 0 || sampleQpc >= device.nextSampleQpc)) {
            const auto collectResult = PdhCollectQueryData(device.query.get());
            if (collectResult != ERROR_SUCCESS) {
                pmlog_warn("PdhCollectQueryData failed").code(collectResult).every(std::chrono::seconds{ 60 })
                    .pmwatch(device.providerDeviceId)
                    .pmwatch(device.fingerprint.deviceName);
            }
            else if (device.qpcFrequency != 0) {
                device.nextSampleQpc = sampleQpc + (int64_t)device.qpcFrequency;
            }
            else {
                device.nextSampleQpc = sampleQpc;
            }
        }

        double value = 0.0;
        if (TryReadCounterValue_(
            device.processorFrequencyCounter,
            "PdhGetFormattedCounterValue failed for processor frequency",
            device,
            value)) {
            double performancePercent = 0.0;
            if (TryReadCounterValue_(
                device.processorPerformanceCounter,
                "PdhGetFormattedCounterValue failed for processor performance",
                device,
                performancePercent)) {
                cache.output.frequencyMhz = value * (performancePercent / 100.0);
                cache.output.hasFrequency = true;
            }
        }

        if (TryReadCounterValue_(
            device.processorIdleTimeCounter,
            "PdhGetFormattedCounterValue failed for processor idle time",
            device,
            value)) {
            cache.output.utilizationPercent = 100.0 - value;
            cache.output.hasUtilization = true;
        }

        return &cache.output;
    }

    bool WmiTelemetryProvider::TryReadCounterValue_(
        HCOUNTER counter,
        const char* label,
        const DeviceState_& device,
        double& value)
    {
        if (counter == nullptr) {
            value = 0.0;
            return false;
        }

        PDH_FMT_COUNTERVALUE counterValue{};
        DWORD counterType = 0;
        const auto result = PdhGetFormattedCounterValue(
            counter,
            PDH_FMT_DOUBLE,
            &counterType,
            &counterValue);
        if (result != ERROR_SUCCESS) {
            pmlog_warn(label).code(result).every(std::chrono::seconds{ 60 })
                .pmwatch(device.providerDeviceId)
                .pmwatch(device.fingerprint.deviceName);
            value = 0.0;
            return false;
        }

        value = counterValue.doubleValue;
        return true;
    }
}
