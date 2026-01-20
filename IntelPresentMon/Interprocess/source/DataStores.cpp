#include "DataStores.h"
#include "MetricCapabilities.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionDataTypeMapping.h"
#include "../../CommonUtilities/Meta.h"
#include "../../CommonUtilities/Memory.h"
#include "../../CommonUtilities/log/Verbose.h"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <unordered_map>

namespace pmon::ipc
{
    namespace
    {
        constexpr size_t kSegmentAlignmentBytes_ = 64 * 1024;
        constexpr size_t kFrameScaleMul_ = 3;
        constexpr size_t kFrameScaleDiv_ = 2;
        constexpr size_t kTelemetryScaleGpuMul_ = 3;
        constexpr size_t kTelemetryScaleSystemMul_ = 2;
        constexpr size_t kTelemetryScaleDiv_ = 1;
        constexpr size_t kFixedLeewayBytes_ = 4 * 1024;

        size_t ScaleBytes_(size_t bytes, size_t numerator, size_t denominator)
        {
            return (bytes * numerator + denominator - 1) / denominator;
        }

        template<PM_DATA_TYPE T>
        struct DataTypeSizeBridger_
        {
            static size_t Invoke()
            {
                return intro::DataTypeToStaticType_sz<T>;
            }
            static size_t Default()
            {
                return 0ull;
            }
        };

        size_t EstimateSampleBytes_(PM_DATA_TYPE type)
        {
            const size_t valueBytes = intro::BridgeDataType<DataTypeSizeBridger_>(type);
            const size_t safeValueBytes = valueBytes > 0 ? valueBytes : sizeof(uint32_t);
            const size_t pad = util::GetPadding(safeValueBytes, alignof(uint64_t));
            return safeValueBytes + pad + sizeof(uint64_t);
        }

        bool ShouldAllocateTelemetryRing_(const intro::IntrospectionMetric& metric)
        {
            if (metric.GetMetricType() == PM_METRIC_TYPE_STATIC) {
                return false;
            }
            return true;
        }

        template<typename Func>
        void ForEachTelemetryRing_(const DataStoreSizingInfo& sizing,
            PM_DEVICE_TYPE deviceType,
            Func&& func)
        {
            if (!sizing.pRoot || !sizing.pCaps) {
                throw std::logic_error("DataStoreSizingInfo requires introspection root and caps");
            }

            std::unordered_map<uint32_t, PM_DEVICE_TYPE> deviceTypeById;
            for (const auto& pDevice : sizing.pRoot->GetDevices()) {
                deviceTypeById.emplace(pDevice->GetId(), pDevice->GetType());
            }

            for (auto&& [metricId, count] : *sizing.pCaps) {
                const auto& metric = sizing.pRoot->FindMetric(metricId);
                bool matchesDeviceType = false;
                for (const auto& pInfo : metric.GetDeviceMetricInfo()) {
                    const auto it = deviceTypeById.find(pInfo->GetDeviceId());
                    if (it != deviceTypeById.end() && it->second == deviceType) {
                        matchesDeviceType = true;
                        break;
                    }
                }
                if (!matchesDeviceType) {
                    throw std::logic_error(
                        "DataStoreSizingInfo caps contain a metric outside the expected device type");
                }
                if (!ShouldAllocateTelemetryRing_(metric)) {
                    continue;
                }
                const auto dataType = metric.GetDataTypeInfo().GetFrameType();
                func(metricId, count, dataType);
            }
        }

        size_t TelemetrySegmentBytes_(const DataStoreSizingInfo& sizing, PM_DEVICE_TYPE deviceType)
        {
            if (sizing.overrideBytes) {
                return *sizing.overrideBytes;
            }

            size_t ringCount = 0;
            size_t payloadBytes = 0;
            ForEachTelemetryRing_(sizing, deviceType,
                [&](PM_METRIC metricId, size_t count, PM_DATA_TYPE dataType) {
                    const size_t sampleBytes = EstimateSampleBytes_(dataType);
                    const size_t metricBytes = count * sizing.ringSamples * sampleBytes;
                    payloadBytes += metricBytes;
                    ringCount += count;
                    pmlog_verb(util::log::V::ipc_sto)(std::format(
                        "ipc telem metric sizing | metric:{} count:{} ring_samples:{} sample_bytes:{} payload_bytes:{}",
                        static_cast<int>(metricId), count, sizing.ringSamples, sampleBytes, metricBytes));
                });

            const size_t scaleMul = (deviceType == PM_DEVICE_TYPE_SYSTEM) ?
                kTelemetryScaleSystemMul_ : kTelemetryScaleGpuMul_;
            size_t scaledBytes =
                ScaleBytes_(payloadBytes, scaleMul, kTelemetryScaleDiv_);
            if (scaledBytes < payloadBytes + kFixedLeewayBytes_) {
                scaledBytes = payloadBytes + kFixedLeewayBytes_;
            }
            const size_t leewayBytes = scaledBytes - payloadBytes;
            const size_t totalBytes = util::PadToAlignment(scaledBytes, kSegmentAlignmentBytes_);
            pmlog_verb(util::log::V::ipc_sto)(std::format(
                "ipc telem sizing | ring_samples:{} ring_count:{} payload_bytes:{} scaled_bytes:{} fixed_leeway_bytes:{} leeway_bytes:{} alignment:{} total_bytes:{}",
                sizing.ringSamples, ringCount, payloadBytes, scaledBytes, kFixedLeewayBytes_,
                leewayBytes, kSegmentAlignmentBytes_, totalBytes));
            return totalBytes;
        }
    }

    size_t FrameDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        const size_t payloadBytes = sizing.ringSamples * sizeof(FrameData);
        size_t scaledBytes =
            ScaleBytes_(payloadBytes, kFrameScaleMul_, kFrameScaleDiv_);
        if (scaledBytes < payloadBytes + kFixedLeewayBytes_) {
            scaledBytes = payloadBytes + kFixedLeewayBytes_;
        }
        const size_t leewayBytes = scaledBytes - payloadBytes;
        const size_t totalBytes = util::PadToAlignment(scaledBytes, kSegmentAlignmentBytes_);
        pmlog_verb(util::log::V::ipc_sto)(std::format(
            "ipc frame sizing | ring_samples:{} payload_bytes:{} scaled_bytes:{} fixed_leeway_bytes:{} leeway_bytes:{} alignment:{} total_bytes:{}",
            sizing.ringSamples, payloadBytes, scaledBytes, kFixedLeewayBytes_,
            leewayBytes, kSegmentAlignmentBytes_, totalBytes));
        return totalBytes;
    }

    void PopulateTelemetryRings(TelemetryMap& telemetryData,
        const DataStoreSizingInfo& sizing,
        PM_DEVICE_TYPE deviceType)
    {
        ForEachTelemetryRing_(sizing, deviceType,
            [&](PM_METRIC metricId, size_t count, PM_DATA_TYPE dataType) {
                telemetryData.AddRing(metricId, sizing.ringSamples, count, dataType);
            });
    }

    size_t GpuDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        return TelemetrySegmentBytes_(sizing, PM_DEVICE_TYPE_GRAPHICS_ADAPTER);
    }

    size_t SystemDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        return TelemetrySegmentBytes_(sizing, PM_DEVICE_TYPE_SYSTEM);
    }
}

