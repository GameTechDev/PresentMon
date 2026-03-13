#pragma once
#include "../CommonUtilities/Hash.h"
#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#include <cereal/cereal.hpp>

namespace pmon::svc
{
    struct MetricUse
    {
        PM_METRIC metricId;
        uint32_t deviceId;
        uint32_t arrayIdx;

        template<class A>
        void serialize(A& ar)
        {
            ar(CEREAL_NVP(metricId),
                CEREAL_NVP(deviceId),
                CEREAL_NVP(arrayIdx));
        }

        bool operator==(const MetricUse& rhs) const
        {
            return metricId == rhs.metricId &&
                deviceId == rhs.deviceId &&
                arrayIdx == rhs.arrayIdx;
        }
    };

    using DeviceMetricUse = std::unordered_map<uint32_t, std::unordered_set<MetricUse>>;
}

// Must be visible before any std::unordered_set<MetricUse> instantiation
namespace std
{
    template<>
    struct hash<pmon::svc::MetricUse>
    {
        size_t operator()(const pmon::svc::MetricUse& mu) const noexcept
        {
            const uint64_t devIdx =
                (uint64_t(mu.deviceId) << 32) | uint64_t(mu.arrayIdx);

            // Avoid depending on std::hash<PM_METRIC> existing:
            using Under = std::underlying_type_t<PM_METRIC>;
            const Under mid = (Under)mu.metricId;

            return pmon::util::hash::DualHash(mid, devIdx);
        }
    };
}
