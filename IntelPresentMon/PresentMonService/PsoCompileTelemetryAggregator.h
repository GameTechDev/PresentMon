// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../Interprocess/source/Interprocess.h"
#include "../CommonUtilities/metrics/ProcessDataRate.h"
#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace pmon::svc
{
    class PsoCompileTelemetryAggregator
    {
    public:
        void Append(uint32_t processId, double durationMs, uint64_t eventCompleteQpc);
        size_t PollToIpc(ipc::ServiceComms& comms);

    private:
        struct PidState_
        {
            std::optional<uint64_t> lastSliceEndQpc;
            std::vector<util::metrics::PsoCompileCompletedSample> pendingSamples;
        };

        std::mutex mtx_;
        std::unordered_map<uint32_t, PidState_> stateByPid_;
    };
}
