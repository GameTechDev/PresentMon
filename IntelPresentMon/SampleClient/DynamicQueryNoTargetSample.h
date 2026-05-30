#pragma once
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../CommonUtilities/IntervalWaiter.h"
#include "CliOptions.h"
#include <unordered_set>
#include <optional>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include <format>

namespace
{
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

    struct QueryItem_
    {
        size_t elementIndex = 0;
        PM_DATA_TYPE outputType = PM_DATA_TYPE_VOID;
        PM_ENUM enumId = PM_ENUM_NULL_ENUM;
        std::string label;
        uint32_t arrayIndex = 0;
        uint32_t arraySize = 1;
    };

    bool HasStat_(const pmapi::intro::MetricView& metric, PM_STAT stat)
    {
        for (auto s : metric.GetStatInfo()) {
            if (s.GetStat() == stat) {
                return true;
            }
        }
        return false;
    }

    PM_STAT ChooseDynamicStat_(const pmapi::intro::MetricView& metric)
    {
        if (HasStat_(metric, PM_STAT_AVG)) {
            return PM_STAT_AVG;
        }
        if (HasStat_(metric, PM_STAT_NON_ZERO_AVG)) {
            return PM_STAT_NON_ZERO_AVG;
        }
        if (HasStat_(metric, PM_STAT_NEWEST_POINT)) {
            return PM_STAT_NEWEST_POINT;
        }
        if (HasStat_(metric, PM_STAT_MID_POINT)) {
            return PM_STAT_MID_POINT;
        }
        if (HasStat_(metric, PM_STAT_OLDEST_POINT)) {
            return PM_STAT_OLDEST_POINT;
        }
        return PM_STAT_NONE;
    }

    PM_DATA_TYPE SelectDynamicOutputType_(PM_STAT stat, PM_DATA_TYPE polledType)
    {
        if (stat == PM_STAT_AVG || stat == PM_STAT_NON_ZERO_AVG) {
            return PM_DATA_TYPE_DOUBLE;
        }
        return polledType;
    }

    std::optional<pmapi::intro::DeviceMetricInfoView> FindDeviceMetricInfo_(
        const pmapi::intro::MetricView& metric, uint32_t deviceId)
    {
        for (auto info : metric.GetDeviceMetricInfo()) {
            if (info.GetDevice().GetId() == deviceId) {
                return info;
            }
        }
        return {};
    }

    std::optional<uint32_t> SelectGpuDeviceId_(
        const pmapi::intro::Root& intro, const clio::Options& opt, std::string& outName, std::string& outError)
    {
        if (opt.defaultAdapterId) {
            const uint32_t requestedId = *opt.defaultAdapterId;
            if (requestedId == 0 || requestedId == pmon::ipc::kSystemDeviceId) {
                outError = "Invalid --default-adapter-id (must be a GPU device id).";
                return {};
            }
            try {
                const auto dev = intro.FindDevice(requestedId);
                if (dev.GetType() != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
                    outError = "Requested adapter id is not a graphics adapter.";
                    return {};
                }
                outName = dev.GetName();
                return requestedId;
            }
            catch (...) {
                outError = "Requested adapter id not found in introspection.";
                return {};
            }
        }

        for (auto dev : intro.GetDevices()) {
            if (dev.GetType() == PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
                outName = dev.GetName();
                return dev.GetId();
            }
        }
        return {};
    }

    std::string FormatValue_(
        const QueryItem_& item,
        const std::vector<PM_QUERY_ELEMENT>& elements,
        const uint8_t* pBlob,
        const pmapi::intro::Root& intro)
    {
        if (item.elementIndex >= elements.size()) {
            return "n/a";
        }
        const auto& el = elements[item.elementIndex];
        const uint8_t* pData = pBlob + el.dataOffset;
        switch (item.outputType) {
        case PM_DATA_TYPE_DOUBLE:
            return std::format("{}", *reinterpret_cast<const double*>(pData));
        case PM_DATA_TYPE_INT32:
            return std::format("{}", *reinterpret_cast<const int32_t*>(pData));
        case PM_DATA_TYPE_UINT32:
            return std::format("{}", *reinterpret_cast<const uint32_t*>(pData));
        case PM_DATA_TYPE_UINT64:
            return std::format("{}", *reinterpret_cast<const uint64_t*>(pData));
        case PM_DATA_TYPE_BOOL:
            return *reinterpret_cast<const bool*>(pData) ? "true" : "false";
        case PM_DATA_TYPE_ENUM:
        {
            const int32_t value = *reinterpret_cast<const int32_t*>(pData);
            if (item.enumId != PM_ENUM_NULL_ENUM) {
                try {
                    return intro.FindEnumKey(item.enumId, value).GetSymbol();
                }
                catch (...) {
                    return std::format("{}", value);
                }
            }
            return std::format("{}", value);
        }
        case PM_DATA_TYPE_STRING:
            return std::string{ reinterpret_cast<const char*>(pData) };
        default:
            return "n/a";
        }
    }
}

int DynamicQueryNoTargetSample(std::unique_ptr<pmapi::Session>&& pSession, double windowSize, double metricOffset)
{
    using Clock = std::chrono::steady_clock;

    try {
        auto& opt = clio::Options::Get();

        if (opt.telemetryPeriodMs) {
            pSession->SetTelemetryPollingPeriod(0, *opt.telemetryPeriodMs);
        }
        if (opt.etwFlushPeriodMs) {
            pSession->SetEtwFlushPeriod(*opt.etwFlushPeriodMs);
        }

        auto pIntro = pSession->GetIntrospectionRoot();
        const auto& intro = *pIntro;

        std::string gpuName;
        std::string gpuError;
        std::optional<uint32_t> gpuDeviceId = SelectGpuDeviceId_(intro, opt, gpuName, gpuError);
        if (opt.defaultAdapterId && !gpuDeviceId.has_value()) {
            std::cout << "Error: " << gpuError << std::endl;
            return -1;
        }
        if (gpuDeviceId.has_value()) {
            std::cout << "Using GPU device id " << *gpuDeviceId << " (" << gpuName << ")" << std::endl;
        }
        else {
            std::cout << "No GPU device found, using system device only." << std::endl;
        }

        std::vector<PM_QUERY_ELEMENT> elements;
        std::vector<QueryItem_> items;
        std::unordered_set<QueryKey_, QueryKeyHasher_, QueryKeyEqual_> seen;

        for (auto metric : intro.GetMetrics()) {
            const auto metricType = metric.GetType();
            const bool isStatic = metricType == PM_METRIC_TYPE_STATIC;
            if (!isStatic && !pmapi::intro::MetricTypeIsDynamic(metricType)) {
                continue;
            }

            PM_STAT stat = isStatic ? PM_STAT_NONE : ChooseDynamicStat_(metric);
            if (!isStatic && stat == PM_STAT_NONE) {
                continue;
            }

            std::optional<pmapi::intro::DeviceMetricInfoView> chosenInfo;
            const auto systemInfo = FindDeviceMetricInfo_(metric, pmon::ipc::kSystemDeviceId);
            if (systemInfo.has_value() && systemInfo->IsAvailable()) {
                chosenInfo = systemInfo;
            }
            else if (gpuDeviceId.has_value()) {
                const auto gpuInfo = FindDeviceMetricInfo_(metric, *gpuDeviceId);
                if (gpuInfo.has_value() && gpuInfo->IsAvailable()) {
                    chosenInfo = gpuInfo;
                }
            }

            if (!chosenInfo.has_value()) {
                continue;
            }

            const uint32_t deviceId = chosenInfo->GetDevice().GetId();
            if (deviceId == 0) {
                continue;
            }

            const uint32_t arraySize = chosenInfo->GetArraySize();
            if (arraySize == 0) {
                continue;
            }

            const auto typeInfo = metric.GetDataTypeInfo();
            const PM_DATA_TYPE outputType = isStatic
                ? typeInfo.GetPolledType()
                : SelectDynamicOutputType_(stat, typeInfo.GetPolledType());

            std::string statSymbol;
            try {
                statSymbol = intro.FindEnumKey(PM_ENUM_STAT, (int)stat).GetSymbol();
            }
            catch (...) {
                statSymbol = "PM_STAT_UNKNOWN";
            }

            const std::string metricSymbol = metric.Introspect().GetSymbol();

            for (uint32_t arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex) {
                const QueryKey_ key{
                    .metric = metric.GetId(),
                    .arrayIndex = arrayIndex,
                    .stat = stat,
                };
                if (seen.find(key) != seen.end()) {
                    continue;
                }
                seen.insert(key);

                PM_QUERY_ELEMENT element{
                    .metric = metric.GetId(),
                    .stat = stat,
                    .deviceId = deviceId,
                    .arrayIndex = arrayIndex,
                    .dataOffset = 0,
                    .dataSize = 0,
                };
                const size_t elementIndex = elements.size();
                elements.push_back(element);

                std::string label = metricSymbol;
                if (arraySize > 1) {
                    label += "#";
                    label += std::to_string(arrayIndex);
                }
                label += "[";
                label += statSymbol;
                label += "]";

                items.push_back(QueryItem_{
                    .elementIndex = elementIndex,
                    .outputType = outputType,
                    .enumId = typeInfo.GetEnumId(),
                    .label = std::move(label),
                    .arrayIndex = arrayIndex,
                    .arraySize = arraySize,
                });
            }
        }

        if (elements.empty()) {
            std::cout << "No eligible metrics found for system or GPU devices." << std::endl;
            return -1;
        }

        auto query = pSession->RegisterDynamicQuery(elements, windowSize, metricOffset);
        auto blobs = query.MakeBlobContainer(1u);

        if (*opt.runStart > 0.0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(*opt.runStart));
        }

        const double pollPeriod = *opt.pollPeriod > 0.0 ? *opt.pollPeriod : 0.1;
        pmon::util::IntervalWaiter waiter{ pollPeriod };

        auto PrintPoll_ = [&](size_t pollIndex) {
            std::cout << "======= poll " << pollIndex << " =======" << std::endl;
            const uint8_t* pBlob = blobs.GetFirst();
            for (const auto& item : items) {
                std::cout << item.label << ":" << FormatValue_(item, elements, pBlob, intro) << std::endl;
            }
        };

        const double runTime = *opt.runTime;
        if (runTime <= 0.0) {
            query.Poll(blobs);
            PrintPoll_(1);
            return 0;
        }

        const auto endTime = Clock::now() + std::chrono::duration<double>(runTime);
        size_t pollIndex = 1;
        while (Clock::now() < endTime) {
            query.Poll(blobs);
            PrintPoll_(pollIndex++);
            waiter.Wait();
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cout << "Unknown Error" << std::endl;
        return -1;
    }

    return 0;
}
