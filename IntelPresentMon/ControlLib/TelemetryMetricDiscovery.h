// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include "../Interprocess/source/MetricCapabilities.h"
#include "../PresentMonAPI2/PresentMonAPI.h"
#include <functional>
#include <span>

namespace pmon::tel
{
    struct DiscoverOutcome
    {
        size_t arraySize = 0;
        PM_METRIC_AVAILABILITY availability = PM_METRIC_AVAILABILITY_UNAVAILABLE;
    };

    using MetricDiscoverFn = std::function<DiscoverOutcome()>;

    struct MetricDiscoverSpec
    {
        PM_METRIC metricId = PM_METRIC_COUNT_;
        MetricDiscoverFn discover;
    };

    void ForEachTelemetryMetricForDeviceType(
        PM_DEVICE_TYPE deviceType,
        const std::function<void(PM_METRIC)>& fn);

    PM_METRIC_AVAILABILITY DefaultMissingTelemetryMetricAvailability(PM_DEVICE_TYPE deviceType);

    void ApplyNotExportedForUnlistedTelemetryMetrics(
        ipc::MetricCapabilities& caps,
        PM_DEVICE_TYPE deviceType);

    inline void DiscoverMetricsFromSpecs(
        std::span<const MetricDiscoverSpec> specs,
        ipc::MetricCapabilities& caps,
        PM_DEVICE_TYPE deviceType)
    {
        for (const auto& spec : specs) {
            if (!spec.discover) {
                continue;
            }
            const auto outcome = spec.discover();
            caps.Set(spec.metricId, outcome.arraySize, outcome.availability);
        }
        ApplyNotExportedForUnlistedTelemetryMetrics(caps, deviceType);
    }
}