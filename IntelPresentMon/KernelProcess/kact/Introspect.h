#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include <format>
#include "../../Core/source/kernel/Kernel.h"
#include "../../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include <ranges>
#include <array>

// cereal JSON dump + NVP macro
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

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

        struct Metric
        {
            PM_METRIC id;
            std::string name;
            std::string description;
            std::vector<uint32_t> availableDeviceIds;
            PM_UNIT preferredUnitId;
            int arraySize;
            std::vector<PM_STAT> availableStatIds;
            bool numeric;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(id),
                    CEREAL_NVP(name),
                    CEREAL_NVP(description),
                    CEREAL_NVP(availableDeviceIds),
                    CEREAL_NVP(preferredUnitId),
                    CEREAL_NVP(arraySize),
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

        struct Response
        {
            std::vector<Metric> metrics;
            std::vector<Stat> stats;
            std::vector<Unit> units;

            template<class A> void serialize(A& ar) {
                ar(CEREAL_NVP(metrics),
                    CEREAL_NVP(stats),
                    CEREAL_NVP(units));
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

            auto& intro = (*ctx.ppKernel)->GetIntrospectionRoot();

            // --- metrics ---
            // set of types that are numeric, used to generate numeric flag that the frontend uses
            const std::array numericTypes{ PM_DATA_TYPE_DOUBLE, PM_DATA_TYPE_UINT32, PM_DATA_TYPE_INT32, PM_DATA_TYPE_UINT64 };

            // filter predicate to only pick up metrics usable in dynamic queries (plus hardcoded blacklist)
            const auto filterPred = [](const pmapi::intro::MetricView& m) {
                const auto type = m.GetType();
                return
                    (m.GetId() != PM_METRIC_GPU_LATENCY)
                    &&
                    (
                        type == PM_METRIC_TYPE_DYNAMIC ||
                        type == PM_METRIC_TYPE_DYNAMIC_FRAME ||
                        type == PM_METRIC_TYPE_STATIC
                        );
            };

            // generate the response
            Response res;

            // reserve space for the actual number of metrics
            res.metrics.reserve(rn::distance(intro.GetMetrics() | vi::filter(filterPred)));

            // now process each applicable metric, filtering ones not usable for dynamic queries
            for (auto&& [i, m] : intro.GetMetrics() | vi::filter(filterPred) | vi::enumerate) {
                // array size: stopgap measure to use largest among all available devices
                // will replace this with per-device size when loadout per-line device selection
                // and per-line array index selection is implemented
                int arraySize = 0;

                // generate device list
                std::vector<uint32_t> devices;
                for (auto&& d : m.GetDeviceMetricInfo()) {
                    if (!d.IsAvailable()) continue;
                    arraySize = std::max(arraySize, (int)d.GetArraySize());
                    devices.push_back(d.GetDevice().GetId());
                }

                // generate stat list
                auto stats = m.GetStatInfo()
                    | vi::transform([](auto&& s) { return s.GetStat(); })
                    | rn::to<std::vector>();

                // add metric
                res.metrics.push_back(Metric{
                    .id = m.GetId(),
                    .name = m.Introspect().GetName(),
                    .description = m.Introspect().GetDescription(),
                    .availableDeviceIds = std::move(devices),
                    // TODO: populate this, currently not used so setting to 0
                    .preferredUnitId = PM_UNIT(0),
                    .arraySize = arraySize,
                    .availableStatIds = std::move(stats),
                    .numeric = rn::contains(numericTypes, m.GetDataTypeInfo().GetPolledType()),
                    });
            }

            // --- stats ---
            auto&& statRange = intro.FindEnum(PM_ENUM_STAT).GetKeys();
            for (auto&& s : statRange) {
                res.stats.push_back(Stat{
                    .id = (PM_STAT)s.GetId(),
                    .name = s.GetName(),
                    .shortName = s.GetShortName(),
                    .description = s.GetDescription(),
                    });
            }

            // --- dump response as JSON to log (best-effort; never fail the action due to logging) ---
            std::string responseJson;
            try
            {
                std::ostringstream os;

                // Pretty JSON by default; use NoIndent() if you want compact output.
                auto opts = cereal::JSONOutputArchive::Options::Default();
                cereal::JSONOutputArchive ar(os, opts);

                // Use macro-driven naming for the root object too
                auto& introspect = res;
                ar(CEREAL_NVP(introspect));

                responseJson = os.str();
            }
            catch (const std::exception& e)
            {
                responseJson = std::string("Introspect JSON serialization failed: ") + e.what();
            }

            pmlog_verb(v::kact)("Introspect action").pmwatch(responseJson);

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
