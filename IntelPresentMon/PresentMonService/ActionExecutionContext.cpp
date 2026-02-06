#include "ActionExecutionContext.h"
#include <CommonUtilities/rng/MemberSlice.h>
#include <CommonUtilities/rng/OptionalMinMax.h>
#include "../Interprocess/source/act/ActionHelper.h"
#include <cereal/types/unordered_set.hpp>
#include <vector>

namespace pmon::svc::acts
{
    void ActionExecutionContext::Dispose(SessionContextType& stx)
    {
        // etw log trace cleanup
        auto& etw = pPmon->GetEtwLogger();
        for (auto id : stx.etwLogSessionIds) {
            if (etw.HasActiveSession(id)) {
                etw.CancelLogSession(id);
            }
        }
        // tracked pids cleanup
        stx.trackedPids.clear();
        pPmon->UpdateTracking(GetTrackedPidSet());
        // telemetry period cleanup
        stx.requestedTelemetryPeriodMs.reset();
        UpdateTelemetryPeriod();
        // etw flush cleanup
        stx.requestedEtwFlushPeriodMs.reset();
        UpdateEtwFlushPeriod();
        // metric use cleanup
        pmlog_verb(pmon::util::log::V::met_use)("Session closing, removing metric usage")
            .pmwatch(stx.remotePid)
            .serialize("sessionMetricUsage", stx.metricUsage);
        stx.metricUsage.clear();
        UpdateMetricUsage();
    }
    void ActionExecutionContext::UpdateTelemetryPeriod() const
    {
        // gather requests across all sessions
        auto&& reqPeriods = util::rng::MemberSlice(*pSessionMap, &SessionContextType::requestedTelemetryPeriodMs);
        // determine the prioritized setting among those
        const auto prioritizedPeriod = util::rng::OptionalMin(reqPeriods);
        // execute the setting on the service system
        if (auto sta = pPmon->SetGpuTelemetryPeriod(prioritizedPeriod); sta != PM_STATUS_SUCCESS) {
            pmlog_error("Set telemetry period failed").code(sta);
            throw util::Except<ipc::act::ActionExecutionError>(sta);
        }
    }
    void ActionExecutionContext::UpdateEtwFlushPeriod() const
    {
        // gather requests across all sessions
        auto&& reqPeriods = util::rng::MemberSlice(*pSessionMap, &SessionContextType::requestedEtwFlushPeriodMs);
        // determine the prioritized setting among those
        const auto prioritizedPeriod = util::rng::OptionalMin(reqPeriods);
        // execute the setting on the service system
        if (auto sta = pPmon->SetEtwFlushPeriod(prioritizedPeriod); sta != PM_STATUS_SUCCESS) {
            pmlog_error("Set telemetry period failed").code(sta);
            throw util::Except<ipc::act::ActionExecutionError>(sta);
        }
    }
    void ActionExecutionContext::UpdateMetricUsage() const
    {
        std::unordered_set<MetricUse> aggregateMetricUsage;
        std::unordered_set<uint32_t> deviceMetricUsage;
        auto&& allUsageSets = util::rng::MemberSlice(*pSessionMap, &SessionContextType::metricUsage);
        for (auto&& clientUsageSet : allUsageSets) {
            for (auto&& usage : clientUsageSet) {
                aggregateMetricUsage.insert(usage);
                deviceMetricUsage.insert(usage.deviceId);
            }
        }
        if (!hasLastAggregateMetricUsage || aggregateMetricUsage != lastAggregateMetricUsage) {
            pmlog_verb(pmon::util::log::V::met_use)("Aggregate metric usage updated")
                .serialize("aggregateMetricUsage", aggregateMetricUsage);
            lastAggregateMetricUsage = aggregateMetricUsage;
            hasLastAggregateMetricUsage = true;
        }
        pPmon->SetDeviceMetricUsage(std::move(deviceMetricUsage));
    }

    std::unordered_set<uint32_t> ActionExecutionContext::GetTrackedPidSet() const
    {
        std::unordered_set<uint32_t> trackedPids;
        if (pSessionMap == nullptr) {
            return trackedPids;
        }
        for (auto const& [sid, session] : *pSessionMap) {
            for (auto const& [pid, target] : session.trackedPids) {
                trackedPids.emplace(pid);
            }
        }
        return trackedPids;
    }
}
