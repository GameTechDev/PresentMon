#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include <format>
#include "../../Core/source/kernel/Kernel.h"
#include "../../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../../Interprocess/source/SystemDeviceId.h"
#include "../../Interprocess/source/IntrospectionPopulators.h"
#include <ranges>
#include <array>

// cereal JSON dump + NVP macro
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

#define ACT_NAME Introspect
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS kproc::kact

namespace ACT_NS
{
    using namespace ::pmon::ipc::act;

    class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
    {
    public:
        static constexpr const char* Identifier = STRINGIFY(ACT_NAME);

        struct Params
        {
            template<class A> void serialize(A& ar) {
                // no params
            }
        };

        struct MetricDeviceAvailability
        {
            uint32_t deviceId;
            int arraySize;
            int availabilityId;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(deviceId),
                    CEREAL_NVP(arraySize),
                    CEREAL_NVP(availabilityId));
            }
        };

        struct MetricAvailabilityReason
        {
            int id;
            std::string description;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(id),
                    CEREAL_NVP(description));
            }
        };

        struct Metric
        {
            PM_METRIC id;
            std::string name;
            std::string description;
            PM_DEVICE_TYPE deviceType;
            PM_UNIT preferredUnitId;
            std::vector<MetricDeviceAvailability> deviceAvailability;
            std::vector<PM_STAT> availableStatIds;
            bool numeric;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(id),
                    CEREAL_NVP(name),
                    CEREAL_NVP(description),
                    CEREAL_NVP(deviceType),
                    CEREAL_NVP(preferredUnitId),
                    CEREAL_NVP(deviceAvailability),
                    CEREAL_NVP(availableStatIds),
                    CEREAL_NVP(numeric));
            }
        };

        struct Stat
        {
            PM_STAT id;
            std::string name;
            std::string shortName;
            std::string description;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(id),
                    CEREAL_NVP(name),
                    CEREAL_NVP(shortName),
                    CEREAL_NVP(description));
            }
        };

        struct Unit
        {
            template<class A> void serialize(A& ar) {
                // TODO: populate when units are implemented
            }
        };

        struct Adapter
        {
            uint32_t id;
            std::string vendor;
            std::string name;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(id),
                    CEREAL_NVP(vendor),
                    CEREAL_NVP(name));
            }
        };

        struct Response
        {
            std::vector<Metric> metrics;
            std::vector<Stat> stats;
            std::vector<Unit> units;
            std::vector<Adapter> adapters;
            uint32_t systemDeviceId;
            uint32_t defaultAdapterId;
            std::vector<MetricAvailabilityReason> metricAvailabilityReasons;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metrics),
                    CEREAL_NVP(stats),
                    CEREAL_NVP(units),
                    CEREAL_NVP(adapters),
                    CEREAL_NVP(systemDeviceId),
                    CEREAL_NVP(defaultAdapterId),
                    CEREAL_NVP(metricAvailabilityReasons));
            }
        };

    private:
        friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;

        static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
        {
            using namespace std::string_literals;
            namespace vi = std::views;
            namespace rn = std::ranges;
            using v = pmon::util::log::V;
            const auto includeStat = [](PM_STAT stat) {
                return stat != PM_STAT_MID_POINT;
            };

            auto& intro = (*ctx.ppKernel)->GetIntrospectionRoot();

            // --- metrics ---
            // set of types that are numeric, used to generate numeric flag that the frontend uses
            const std::array numericTypes{ PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_UINT32, PM_DATA_TYPE_INT32, PM_DATA_TYPE_UINT64 };

            // filter predicate to only pick up metrics usable in dynamic queries (plus hardcoded blacklist)
            const auto filterPred = [](const pmapi::intro::MetricView& m) {
                const auto id = m.GetId();
                const auto type = m.GetType();
                return
                    (id != PM_METRIC_COUNT_) &&
                    (id != PM_METRIC_GPU_LATENCY) &&
                    (id != PM_METRIC_SESSION_START_QPC) &&
                    (id != PM_METRIC_SWAP_CHAIN_ADDRESS)
                    &&
                    (
                        type == PM_METRIC_TYPE_DYNAMIC ||
                        type == PM_METRIC_TYPE_DYNAMIC_FRAME ||
                        type == PM_METRIC_TYPE_STATIC
                        );
            };

            // generate the response
            Response res;
            res.systemDeviceId = ::pmon::ipc::kSystemDeviceId;
            res.defaultAdapterId = (*ctx.ppKernel)->GetDefaultGpuDeviceId();

            for (auto&& key : intro.FindEnum(PM_ENUM_METRIC_AVAILABILITY).GetKeys()) {
                res.metricAvailabilityReasons.push_back(MetricAvailabilityReason{
                    .id = key.GetId(),
                    .description = key.GetDescription(),
                });
            }

            for (const auto& device : intro.GetDevices()) {
                if (device.GetType() != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
                    continue;
                }
                res.adapters.push_back(Adapter{
                    .id = device.GetId(),
                    .vendor = device.IntrospectVendor().GetName(),
                    .name = device.GetName(),
                });
            }

            // reserve space for the actual number of metrics
            res.metrics.reserve(rn::distance(intro.GetMetrics() | vi::filter(filterPred)));

            // now process each applicable metric, filtering ones not usable for dynamic queries
            for (auto&& [i, m] : intro.GetMetrics() | vi::filter(filterPred) | vi::enumerate) {
                std::vector<MetricDeviceAvailability> deviceAvailability;
                for (auto&& d : m.GetDeviceMetricInfo()) {
                    const auto deviceId = d.GetDevice().GetId();
                    deviceAvailability.push_back(MetricDeviceAvailability{
                        .deviceId = deviceId,
                        .arraySize = (int)d.GetArraySize(),
                        .availabilityId = (int)d.GetAvailability(),
                    });
                }

                // generate stat list
                auto stats = m.GetStatInfo()
                    | vi::transform([](auto&& s) { return s.GetStat(); })
                    | vi::filter(includeStat)
                    | rn::to<std::vector>();

                // add metric
                res.metrics.push_back(Metric{
                    .id = m.GetId(),
                    .name = m.Introspect().GetName(),
                    .description = m.Introspect().GetDescription(),
                    .deviceType = ::pmon::ipc::intro::GetMetricRegisteredDeviceType(m.GetId()),
                    .preferredUnitId = m.GetPreferredUnitHint(),
                    .deviceAvailability = std::move(deviceAvailability),
                    .availableStatIds = std::move(stats),
                    .numeric = rn::contains(numericTypes, m.GetDataTypeInfo().GetPolledType()),
                    });
            }

            // --- stats ---
            auto&& statRange = intro.FindEnum(PM_ENUM_STAT).GetKeys();
            for (auto&& s : statRange) {
                if (!includeStat((PM_STAT)s.GetId())) continue;
                res.stats.push_back(Stat{
                    .id = (PM_STAT)s.GetId(),
                    .name = s.GetName(),
                    .shortName = s.GetShortName(),
                    .description = s.GetDescription(),
                    });
            }

            pmlog_verb(v::kact)("Introspect action")
                .serialize("introspect", res);

            return res;
        }
    };

    ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
