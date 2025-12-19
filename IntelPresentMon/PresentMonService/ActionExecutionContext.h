#pragma once
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include "../Interprocess/source/ShmNamer.h"
#include "../CommonUtilities/Hash.h"
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <map>
#include <functional>
#include <type_traits>

#include "PresentMon.h"
#include "Service.h"
#include "FrameBroadcaster.h"

namespace pmon::svc::acts
{
    struct ActionExecutionContext;

    struct MetricUse
    {
        PM_METRIC metricId;
        uint32_t deviceId;
        uint32_t arrayIdx;

        template<class A>
        void serialize(A& ar)
        {
            ar(metricId, deviceId, arrayIdx);
        }

        bool operator==(const MetricUse& rhs) const
        {
            return metricId == rhs.metricId &&
                deviceId == rhs.deviceId &&
                arrayIdx == rhs.arrayIdx;
        }
    };
}

// Must be visible before any std::unordered_set<MetricUse> instantiation
namespace std
{
    template<>
    struct hash<pmon::svc::acts::MetricUse>
    {
        size_t operator()(const pmon::svc::acts::MetricUse& mu) const noexcept
        {
            const uint64_t devIdx =
                (uint64_t(mu.deviceId) << 32) | uint64_t(mu.arrayIdx);

            // Avoid depending on std::hash<PM_METRIC> existing:
            using Under = std::underlying_type_t<PM_METRIC>;
            const Under mid = static_cast<Under>(mu.metricId);

            return pmon::util::hash::DualHash(mid, devIdx);
        }
    };
}

namespace pmon::svc::acts
{
    struct ActionSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<ActionExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;

        // custom items
        std::map<uint32_t, std::shared_ptr<FrameBroadcaster::Segment>> trackedPids;
        std::set<uint32_t> etwLogSessionIds;
        std::optional<uint32_t> requestedAdapterId;
        std::optional<uint32_t> requestedTelemetryPeriodMs;
        std::optional<uint32_t> requestedEtwFlushPeriodMs;
        std::string clientBuildId;
        std::unordered_set<MetricUse> metricUsage;
    };

    struct ActionExecutionContext
    {
        using SessionContextType = ActionSessionContext;

        // data
        Service* pSvc = nullptr;
        PresentMon* pPmon = nullptr;
        const std::unordered_map<uint32_t, SessionContextType>* pSessionMap = nullptr;
        std::optional<uint32_t> responseWriteTimeoutMs;

        // functions
        void Dispose(SessionContextType& stx);

        // TODO: refactor so that these functions need not be const
        void UpdateTelemetryPeriod() const;
        void UpdateEtwFlushPeriod() const;
        void UpdateMetricUsage() const;
    };
}
