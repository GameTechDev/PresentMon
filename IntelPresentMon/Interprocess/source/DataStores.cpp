#include "DataStores.h"
#include "MetricCapabilities.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionDataTypeMapping.h"
#include "../../CommonUtilities/Memory.h"
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace pmon::ipc
{
    namespace
    {
        constexpr size_t kSegmentAlignmentBytes = 64 * 1024;
        constexpr size_t kFrameLeewayPerRingBytes = 2 * 1024 * 1024;
        constexpr size_t kFrameLeewayBaseBytes = 8 * 1024 * 1024;
        constexpr size_t kTelemetryLeewayPerRingBytes = 64 * 1024;
        constexpr size_t kTelemetryLeewayBaseBytes = 2 * 1024 * 1024;

        size_t PadToAlignment(size_t bytes, size_t alignment)
        {
            return bytes + util::GetPadding(bytes, alignment);
        }

        template<PM_DATA_TYPE T>
        struct DataTypeSizeBridger
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

        size_t EstimateSampleBytes(PM_DATA_TYPE type)
        {
            const size_t valueBytes = intro::BridgeDataType<DataTypeSizeBridger>(type);
            const size_t safeValueBytes = valueBytes > 0 ? valueBytes : sizeof(uint32_t);
            const size_t pad = util::GetPadding(safeValueBytes, alignof(uint64_t));
            return safeValueBytes + pad + sizeof(uint64_t);
        }

        bool ShouldAllocateTelemetryRing(PM_METRIC metricId,
            const intro::IntrospectionMetric& metric)
        {
            if (metric.GetMetricType() == PM_METRIC_TYPE_STATIC) {
                return false;
            }
            if (metricId == PM_METRIC_GPU_FAN_SPEED_PERCENT ||
                metricId == PM_METRIC_GPU_MEM_UTILIZATION) {
                return false;
            }
            return true;
        }

        size_t TelemetrySegmentBytes(const DataStoreSizingInfo& sizing, PM_DEVICE_TYPE deviceType)
        {
            if (sizing.overrideBytes) {
                return *sizing.overrideBytes;
            }
            if (!sizing.pRoot || !sizing.pCaps) {
                throw std::logic_error("DataStoreSizingInfo requires introspection root and caps");
            }

            std::unordered_map<uint32_t, PM_DEVICE_TYPE> deviceTypeById;
            for (const auto& pDevice : sizing.pRoot->GetDevices()) {
                deviceTypeById.emplace(pDevice->GetId(), pDevice->GetType());
            }

            size_t ringCount = 0;
            size_t payloadBytes = 0;
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
                if (!ShouldAllocateTelemetryRing(metricId, metric)) {
                    continue;
                }
                payloadBytes += count * sizing.ringSamples *
                    EstimateSampleBytes(metric.GetDataTypeInfo().GetFrameType());
                ringCount += count;
            }

            const size_t leewayBytes =
                ringCount * kTelemetryLeewayPerRingBytes + kTelemetryLeewayBaseBytes;
            return PadToAlignment(payloadBytes + leewayBytes, kSegmentAlignmentBytes);
        }
    }

    size_t FrameDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        const size_t payloadBytes = sizing.ringSamples * sizeof(FrameData);
        const size_t leewayBytes = kFrameLeewayPerRingBytes + kFrameLeewayBaseBytes;
        return PadToAlignment(payloadBytes + leewayBytes, kSegmentAlignmentBytes);
    }

    size_t GpuDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        return TelemetrySegmentBytes(sizing, PM_DEVICE_TYPE_GRAPHICS_ADAPTER);
    }

    size_t SystemDataStore::CalculateSegmentBytes(const DataStoreSizingInfo& sizing)
    {
        return TelemetrySegmentBytes(sizing, PM_DEVICE_TYPE_SYSTEM);
    }
}

