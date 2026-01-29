// Copyright (C) 2017-2024 Intel Corporation
#include "QueryValidation.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../PresentMonAPIWrapperCommon/Exception.h"
#include "../CommonUtilities/Exception.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/mc/FrameMetricsMemberMap.h"
#include "../CommonUtilities/log/Log.h"
#include "../Interprocess/source/Interprocess.h"
#include "../Interprocess/source/IntrospectionHelpers.h"
#include "../Interprocess/source/IntrospectionDataTypeMapping.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include <optional>
#include <string>
#include <unordered_set>
#include <type_traits>

namespace ipc = pmon::ipc;
namespace util = pmon::util;

namespace
{
    void ThrowMalformed_(const char* msg)
    {
        throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED, msg);
    }

    bool IsStatSupported_(PM_STAT stat, const pmapi::intro::MetricView& metricView)
    {
        for (auto statInfo : metricView.GetStatInfo()) {
            if (statInfo.GetStat() == stat) {
                return true;
            }
        }
        return false;
    }

    bool IsDynamicStatSupported_(PM_STAT stat)
    {
        switch (stat) {
        case PM_STAT_AVG:
        case PM_STAT_NON_ZERO_AVG:
        case PM_STAT_PERCENTILE_99:
        case PM_STAT_PERCENTILE_95:
        case PM_STAT_PERCENTILE_90:
        case PM_STAT_PERCENTILE_01:
        case PM_STAT_PERCENTILE_05:
        case PM_STAT_PERCENTILE_10:
        case PM_STAT_MAX:
        case PM_STAT_MIN:
        case PM_STAT_MID_POINT:
        case PM_STAT_NEWEST_POINT:
        case PM_STAT_OLDEST_POINT:
            return true;
        case PM_STAT_NONE:
        case PM_STAT_MID_LERP:
        case PM_STAT_COUNT:
        default:
            return false;
        }
    }

    bool IsAvgStat_(PM_STAT stat)
    {
        return stat == PM_STAT_AVG || stat == PM_STAT_NON_ZERO_AVG;
    }

    bool IsSupportedDynamicInputType_(PM_DATA_TYPE inType)
    {
        switch (inType) {
        case PM_DATA_TYPE_DOUBLE:
        case PM_DATA_TYPE_INT32:
        case PM_DATA_TYPE_ENUM:
        case PM_DATA_TYPE_UINT32:
        case PM_DATA_TYPE_UINT64:
        case PM_DATA_TYPE_BOOL:
            return true;
        default:
            return false;
        }
    }

    bool IsSupportedDynamicOutputType_(PM_DATA_TYPE outType, bool allowBool, bool allowUint32, bool allowUint64)
    {
        switch (outType) {
        case PM_DATA_TYPE_DOUBLE:
        case PM_DATA_TYPE_INT32:
        case PM_DATA_TYPE_ENUM:
            return true;
        case PM_DATA_TYPE_BOOL:
            return allowBool;
        case PM_DATA_TYPE_UINT32:
            return allowUint32;
        case PM_DATA_TYPE_UINT64:
            return allowUint64;
        default:
            return false;
        }
    }

    const char* ValidateDynamicStatTypes_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType)
    {
        if (IsAvgStat_(stat)) {
            if (outType != PM_DATA_TYPE_DOUBLE) {
                return "Dynamic stat average expects double output value";
            }
            return nullptr;
        }

        const bool allowBool = inType == PM_DATA_TYPE_BOOL;
        const bool allowUint32 = inType == PM_DATA_TYPE_UINT32;
        const bool allowUint64 = inType == PM_DATA_TYPE_UINT64;
        if (!IsSupportedDynamicOutputType_(outType, allowBool, allowUint32, allowUint64)) {
            return "Unsupported dynamic stat output data type";
        }
        return nullptr;
    }

    PM_DATA_TYPE SelectDynamicOutputType_(PM_STAT stat, PM_DATA_TYPE metricOutType)
    {
        if (IsAvgStat_(stat)) {
            return PM_DATA_TYPE_DOUBLE;
        }
        return metricOutType;
    }

    bool IsFrameMetricMapped_(PM_METRIC metric)
    {
        return util::DispatchEnumValue<PM_METRIC, int(PM_METRIC_COUNT)>(
            metric,
            [&]<PM_METRIC Metric>() -> bool {
                return util::metrics::HasFrameMetricMember<Metric>;
            },
            false);
    }

    template<PM_DATA_TYPE dt, PM_ENUM enumValue>
    struct TelemetryRingValueChecker_
    {
        static bool Invoke(PM_ENUM)
        {
            using ValueType = typename ipc::intro::DataTypeToStaticType<dt, enumValue>::type;
            return std::is_same_v<ValueType, double> ||
                std::is_same_v<ValueType, uint32_t> ||
                std::is_same_v<ValueType, uint64_t> ||
                std::is_same_v<ValueType, bool> ||
                std::is_same_v<ValueType, int>;
        }
        static bool Default()
        {
            return false;
        }
    };

    struct QueryKey_
    {
        PM_METRIC metric;
        uint32_t arrayIndex;
        PM_STAT stat;
    };

    struct QueryKeyHasher_
    {
        size_t operator()(const QueryKey_& key) const noexcept
        {
            uint64_t h = (uint64_t)key.metric;
            h = (h * 1315423911u) ^ (uint64_t)key.arrayIndex;
            h = (h * 1315423911u) ^ (uint64_t)key.stat;
            return (size_t)h;
        }
    };

    struct QueryKeyEqual_
    {
        bool operator()(const QueryKey_& lhs, const QueryKey_& rhs) const noexcept
        {
            return lhs.metric == rhs.metric &&
                lhs.arrayIndex == rhs.arrayIndex &&
                lhs.stat == rhs.stat;
        }
    };

    std::string GetStatSymbol_(const pmapi::intro::Root& introRoot, PM_STAT stat)
    {
        try {
            return introRoot.FindEnumKey(PM_ENUM_STAT, (int)stat).GetSymbol();
        }
        catch (...) {
            return "UnknownStat";
        }
    }

    bool IsSupportedTelemetryRingType_(PM_DATA_TYPE dataType, PM_ENUM enumId)
    {
        return ipc::intro::BridgeDataTypeWithEnum<TelemetryRingValueChecker_>(dataType, enumId);
    }
}

namespace pmon::mid
{
    void ValidateQueryElements(std::span<PM_QUERY_ELEMENT> queryElements, PM_METRIC_TYPE queryType,
        const pmapi::intro::Root& introRoot, const ipc::MiddlewareComms& comms)
    {
        const bool isDynamicQuery = queryType == PM_METRIC_TYPE_DYNAMIC;
        const bool isFrameQuery = queryType == PM_METRIC_TYPE_FRAME_EVENT;
        if (!isDynamicQuery && !isFrameQuery) {
            ThrowMalformed_("Invalid query type for validation");
        }

        if (queryElements.empty()) {
            pmlog_error("Query requires at least one query element").diag();
            ThrowMalformed_("Empty query");
        }

        std::unordered_map<QueryKey_, size_t, QueryKeyHasher_, QueryKeyEqual_> seenKeys;
        for (size_t elementIndex = 0; elementIndex < queryElements.size(); ++elementIndex) {
            const auto& q = queryElements[elementIndex];
            const auto metricView = introRoot.FindMetric(q.metric);
            const auto statSymbol = GetStatSymbol_(introRoot, q.stat);
            const auto LogAndThrow = [&](const char* msg) {
                pmlog_error(msg)
                    .pmwatch(metricView.Introspect().GetSymbol())
                    .pmwatch(statSymbol)
                    .pmwatch((int)q.stat)
                    .pmwatch(q.arrayIndex)
                    .pmwatch(q.deviceId)
                    .pmwatch((uint64_t)elementIndex).diag();
                ThrowMalformed_(msg);
            };

            const QueryKey_ key{
                .metric = q.metric,
                .arrayIndex = q.arrayIndex,
                .stat = q.stat,
            };
            if (auto it = seenKeys.find(key); it != seenKeys.end()) {
                pmlog_error("Duplicate query element")
                    .pmwatch(metricView.Introspect().GetSymbol())
                    .pmwatch(statSymbol)
                    .pmwatch((int)q.stat)
                    .pmwatch(q.arrayIndex)
                    .pmwatch(q.deviceId)
                    .pmwatch((uint64_t)it->second)
                    .pmwatch((uint64_t)elementIndex).diag();
                ThrowMalformed_("Duplicate query element");
            }
            else {
                seenKeys.emplace(key, elementIndex);
            }

            const auto metricType = metricView.GetType();
            const bool isStaticMetric = metricType == PM_METRIC_TYPE_STATIC;

            const bool metricTypeOk = isStaticMetric || (isDynamicQuery ?
                pmapi::intro::MetricTypeIsDynamic(metricType) :
                pmapi::intro::MetricTypeIsFrameEvent(metricType));
            if (!metricTypeOk) {
                if (isDynamicQuery) {
                    LogAndThrow("Dynamic query contains non-dynamic metric");
                }
                LogAndThrow("Frame query contains non-frame metric");
            }

            if (isFrameQuery) {
                if (q.stat != PM_STAT_NONE) {
                    pmlog_warn("Frame query stat should be NONE")
                        .pmwatch(metricView.Introspect().GetSymbol())
                        .pmwatch((int)q.stat).diag();
                }
            }
            else {
                if (isStaticMetric) {
                    if (q.stat != PM_STAT_NONE) {
                        LogAndThrow("Static metric in dynamic query requires NONE stat");
                    }
                }
                else {
                    if (!IsStatSupported_(q.stat, metricView)) {
                        LogAndThrow("Dynamic metric stat not supported by metric");
                    }
                    if (!IsDynamicStatSupported_(q.stat)) {
                        LogAndThrow("Dynamic metric stat not supported by implementation");
                    }
                }
            }

            const auto typeInfo = metricView.GetDataTypeInfo();
            const auto frameType = typeInfo.GetFrameType();
            const auto polledType = typeInfo.GetPolledType();
            const auto queryDataType = isFrameQuery ? frameType : polledType;
            if (ipc::intro::GetDataTypeSize(queryDataType) == 0) {
                LogAndThrow("Unsupported query data type");
            }

            if (q.deviceId != ipc::kUniversalDeviceId) {
                try {
                    introRoot.FindDevice(q.deviceId);
                }
                catch (const pmapi::LookupException&) {
                    LogAndThrow("Invalid device ID");
                }
            }

            const auto deviceMetricInfo = [&]() -> std::optional<pmapi::intro::DeviceMetricInfoView> {
                for (auto info : metricView.GetDeviceMetricInfo()) {
                    if (info.GetDevice().GetId() == q.deviceId) {
                        return info;
                    }
                }
                return std::nullopt;
            }();

            if (!deviceMetricInfo.has_value()) {
                if (!isStaticMetric || q.deviceId != ipc::kSystemDeviceId) {
                    LogAndThrow("Metric not supported by device in query");
                }
                if (q.arrayIndex != 0) {
                    pmlog_error("Query array index out of bounds")
                        .pmwatch(metricView.Introspect().GetSymbol())
                        .pmwatch(statSymbol)
                        .pmwatch((int)q.stat)
                        .pmwatch(q.arrayIndex)
                        .pmwatch(q.deviceId)
                        .pmwatch((uint64_t)elementIndex)
                        .pmwatch(1).diag();
                    ThrowMalformed_("Query array index out of bounds");
                }
            }
            else {
                if (!deviceMetricInfo->IsAvailable()) {
                    LogAndThrow("Metric not supported by device in query");
                }

                const auto arraySize = deviceMetricInfo->GetArraySize();
                if (q.arrayIndex >= arraySize) {
                    pmlog_error("Query array index out of bounds")
                        .pmwatch(metricView.Introspect().GetSymbol())
                        .pmwatch(statSymbol)
                        .pmwatch((int)q.stat)
                        .pmwatch(q.arrayIndex)
                        .pmwatch(q.deviceId)
                        .pmwatch((uint64_t)elementIndex)
                        .pmwatch(arraySize).diag();
                    ThrowMalformed_("Query array index out of bounds");
                }
            }

            if (isFrameQuery && !isStaticMetric && q.deviceId == ipc::kUniversalDeviceId) {
                if (!IsFrameMetricMapped_(q.metric)) {
                    LogAndThrow("Unexpected frame metric in frame query");
                }
            }

            if (!isStaticMetric && q.deviceId != ipc::kUniversalDeviceId) {
                if (q.deviceId > 0 && q.deviceId <= ipc::kSystemDeviceId) {
                    const auto& teleMap = q.deviceId == ipc::kSystemDeviceId ?
                        comms.GetSystemDataStore().telemetryData :
                        comms.GetGpuDataStore(q.deviceId).telemetryData;
                    if (teleMap.ArraySize(q.metric) == 0) {
                        LogAndThrow("Telemetry ring missing for metric in query");
                    }
                }
                else {
                    LogAndThrow("Invalid device id in query");
                }
            }

            if (isDynamicQuery && !isStaticMetric) {
                if (!IsSupportedDynamicInputType_(frameType)) {
                    LogAndThrow("Unsupported dynamic stat input data type");
                }

                const auto outType = SelectDynamicOutputType_(q.stat, polledType);
                if (const auto err = ValidateDynamicStatTypes_(q.stat, frameType, outType)) {
                    LogAndThrow(err);
                }

                if (q.deviceId != ipc::kUniversalDeviceId) {
                    if (!IsSupportedTelemetryRingType_(frameType, typeInfo.GetEnumId())) {
                        LogAndThrow("Unsupported telemetry ring data type for dynamic query");
                    }
                }
            }
        }
    }
}
