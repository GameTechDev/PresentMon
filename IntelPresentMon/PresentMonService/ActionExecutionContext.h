#pragma once
#include "../Interprocess/source/act/SessionEndReason.h"
#include "../Interprocess/source/ShmNamer.h"
#include "../CommonUtilities/win/Handle.h"
#include <atomic>
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
    struct ActionSessionContext
    {
        // required by the transport, see TransportSessionContext
        uint32_t remotePid = 0;

        // custom items
        struct TrackedTarget
        {
            std::shared_ptr<FrameBroadcaster::Segment> pSegment;
            util::win::Handle processHandle;
            // Backpressured playback rings are SPSC: one producer in the service and
            // one client-owned reader cursor reported via ReportFrameReadProgress.
            std::optional<uint64_t> backpressureReadSerial;
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
        void EnterFinalTeardown()
        {
            cleanupMode_->store(ipc::act::SessionCleanupMode::FinalTeardown, std::memory_order_release);
        }
        ipc::act::SessionCleanupMode GetSessionCleanupMode() const
        {
            return cleanupMode_->load(std::memory_order_acquire);
        }
        void Dispose(SessionContextType& stx, ipc::act::SessionDisposition disposition);

        std::shared_ptr<std::atomic<ipc::act::SessionCleanupMode>> cleanupMode_ =
            std::make_shared<std::atomic<ipc::act::SessionCleanupMode>>(ipc::act::SessionCleanupMode::NormalOperation);

        // TODO: refactor so that these functions need not be const
        void UpdateTelemetryPeriod() const;
        void UpdateEtwFlushPeriod() const;
        void UpdateMetricUsage() const;
        void UpdatePeriodicLogFlushing() const;
        std::unordered_set<uint32_t> GetTrackedPidSet() const;
        void ReleaseBackpressure(uint32_t pid) const;
    };
}
