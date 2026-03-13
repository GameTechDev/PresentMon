#pragma once
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include "../Interprocess/source/ShmNamer.h"
#include "../CommonUtilities/win/Handle.h"
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
#include <cereal/cereal.hpp>

#include "PresentMon.h"
#include "Service.h"
#include "FrameBroadcaster.h"
#include "MetricUse.h"

namespace pmon::svc::acts
{
    struct ActionExecutionContext;
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
        struct TrackedTarget
        {
            std::shared_ptr<FrameBroadcaster::Segment> pSegment;
            util::win::Handle processHandle;
        };
        std::map<uint32_t, TrackedTarget> trackedPids;
        // etl recording functionality support
        std::set<uint32_t> etwLogSessionIds;
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
        std::unordered_set<uint32_t> GetTrackedPidSet() const;
    };
}
