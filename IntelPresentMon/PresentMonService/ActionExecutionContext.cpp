#include "ActionExecutionContext.h"
#include <CommonUtilities/rng/MemberSlice.h>
#include <CommonUtilities/rng/OptionalMinMax.h>
#include "../Interprocess/source/act/ActionHelper.h"
#include "LogSetup.h"
#include <cereal/types/unordered_set.hpp>
#include <vector>

namespace pmon::svc::acts
{
    void ActionExecutionContext::Dispose(SessionContextType& stx)
    {
        for (auto const& [pid, target] : stx.trackedPids) {
            if (target.backpressureReadSerial) {
                ReleaseBackpressure(pid);
            }
        }
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
        UpdatePeriodicLogFlushing();
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
        auto deviceMetricUsage = std::make_shared<PresentMon::DeviceMetricUsage>();
        auto&& allUsageSets = util::rng::MemberSlice(*pSessionMap, &SessionContextType::metricUsage);
        for (auto&& clientUsageSet : allUsageSets) {
            for (auto&& usage : clientUsageSet) {
                (*deviceMetricUsage)[usage.deviceId].insert(usage);
            }
        }
        pPmon->SetDeviceMetricUsage(std::move(deviceMetricUsage));
        UpdatePeriodicLogFlushing();
    }
    void ActionExecutionContext::UpdatePeriodicLogFlushing() const
    {
        bool active = false;
        if (pSessionMap != nullptr) {
            for (const auto& sessionEntry : *pSessionMap) {
                const auto& session = sessionEntry.second;
                if (!session.trackedPids.empty() || !session.metricUsage.empty()) {
                    active = true;
                    break;
                }
            }
        }
        logsetup::SetPeriodicLogFlushingEnabled(active);
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

    void ActionExecutionContext::ReleaseBackpressure(uint32_t pid) const
    {
        // Backpressured playback is SPSC, so tearing down the owner simply advances the
        // single consumer cursor to the writer and releases any blocked producer.
        pPmon->GetBroadcaster().UpdateReadSerial(
            pid,
            pPmon->GetBroadcaster().GetCurrentWriteSerial(pid).value_or(0));
    }
}
