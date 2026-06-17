// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PsoCompileTelemetryAggregator.h"
#include "../CommonUtilities/Qpc.h"
#include "../CommonUtilities/log/Log.h"
#include "../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::svc
{
    namespace
    {
        constexpr double kEtwProcessingLatencyMs = 40.0;

        uint64_t MsToQpc_(double ms, double qpcPeriodSeconds)
        {
            if (ms <= 0. || qpcPeriodSeconds <= 0.) {
                return 0;
            }
            return (uint64_t)((ms / 1000.) / qpcPeriodSeconds);
        }
        void PushDoubleTelemetrySample_(
            ipc::TelemetryMap& telemetryMap,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            double value,
            uint64_t qpc)
        {
            auto& rings = telemetryMap.FindRing<double>(metricId);
            if (arrayIndex >= rings.size()) {
                pmlog_error("Index out of bounds in process telemetry ring")
                    .pmwatch((int)metricId)
                    .pmwatch(rings.size())
                    .pmwatch(arrayIndex);
                return;
            }
            if (!rings[arrayIndex].Push(value, qpc)) {
                pmlog_error("Failed pushing process telemetry sample to ring")
                    .pmwatch((int)metricId)
                    .pmwatch(arrayIndex);
            }
        }

        void PushSliceAggregate_(
            ipc::ProcessDataStore& store,
            const util::metrics::PsoCompileSliceAggregate& aggregate,
            uint64_t tickQpc)
        {
            PushDoubleTelemetrySample_(store.telemetryData,
                PM_METRIC_D3D12_PSO_COMPILE_COUNT, 0, aggregate.compileCountHz, tickQpc);
            PushDoubleTelemetrySample_(store.telemetryData,
                PM_METRIC_D3D12_PSO_COMPILE_TIME, 0, aggregate.compileTimeMsPerSecond, tickQpc);
            PushDoubleTelemetrySample_(store.telemetryData,
                PM_METRIC_D3D12_PSO_COMPILE_BUSY_PERCENT, 0, aggregate.compileBusyPercent, tickQpc);
        }
    }

    void PsoCompileTelemetryAggregator::Append(
        uint32_t processId,
        double durationMs,
        uint64_t eventCompleteQpc)
    {
        std::lock_guard lk{ mtx_ };
        auto& state = stateByPid_[processId];
        state.pendingSamples.push_back(util::metrics::PsoCompileCompletedSample{
            durationMs,
            eventCompleteQpc,
        });
    }

    size_t PsoCompileTelemetryAggregator::PollToIpc(ipc::ServiceComms& comms, uint64_t gridTargetQpc)
    {
        const double qpcPeriodSeconds = util::GetTimestampPeriodSeconds();
        const uint64_t windowOffsetQpc = MsToQpc_(kEtwProcessingLatencyMs, qpcPeriodSeconds);
        const uint64_t sliceEndQpc = gridTargetQpc > windowOffsetQpc ? gridTargetQpc - windowOffsetQpc : 0;
        const auto trackedPids = comms.GetFramePids();
        size_t samplesWritten = 0;

        for (uint32_t pid : trackedPids) {
            auto pSegment = comms.GetProcessDataSegment(pid);
            if (!pSegment) {
                continue;
            }

            std::vector<util::metrics::PsoCompileCompletedSample> events;
            uint64_t sliceStartQpc = 0;
            {
                std::lock_guard lk{ mtx_ };
                auto& state = stateByPid_[pid];
                events.swap(state.pendingSamples);

                if (!state.lastSliceEndQpc.has_value()) {
                    const int64_t startQpc = pSegment->GetStore().bookkeeping.startQpc;
                    const uint64_t anchorQpc = startQpc > 0 ? (uint64_t)startQpc : gridTargetQpc;
                    state.lastSliceEndQpc = anchorQpc > windowOffsetQpc ? anchorQpc - windowOffsetQpc : 0;
                }

                sliceStartQpc = *state.lastSliceEndQpc;
                state.lastSliceEndQpc = sliceEndQpc;
            }

            const auto aggregate = util::metrics::AggregatePsoCompileSlice(
                events, sliceStartQpc, sliceEndQpc, qpcPeriodSeconds);
            const uint64_t sampleTimestampQpc = sliceEndQpc > sliceStartQpc
                ? sliceStartQpc + (sliceEndQpc - sliceStartQpc) / 2
                : sliceEndQpc;
            PushSliceAggregate_(pSegment->GetStore(), aggregate, sampleTimestampQpc);
            samplesWritten += 3;
        }

        return samplesWritten;
    }
}
