// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "TelemetryDeviceFingerprint.h"
#include "../Interprocess/source/MetricCapabilities.h"
#include "../Interprocess/source/TelemetryMap.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace pmon::tel
{
    using ProviderDeviceId = uint32_t;
    using ProviderCapabilityMap = std::unordered_map<ProviderDeviceId, ipc::MetricCapabilities>;
    using TelemetryMetricValue = ipc::TelemetryMap::ScalarValueType;

    class TelemetryProvider
    {
    public:
        virtual ~TelemetryProvider() = default;
        virtual ProviderCapabilityMap GetCaps() = 0;
        virtual const TelemetryDeviceFingerprint& GetFingerPrint(ProviderDeviceId providerDeviceId) const = 0;
        virtual TelemetryMetricValue PollMetric(
            ProviderDeviceId providerDeviceId,
            PM_METRIC metricId,
            uint32_t arrayIndex,
            int64_t requestQpc) = 0;
    };
}
