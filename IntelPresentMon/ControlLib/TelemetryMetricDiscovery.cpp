// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TelemetryMetricDiscovery.h"
#include "../Interprocess/source/metadata/MetricList.h"

namespace pmon::tel
{
    PM_METRIC_AVAILABILITY DefaultMissingTelemetryMetricAvailability(PM_DEVICE_TYPE deviceType)
    {
        if (deviceType == PM_DEVICE_TYPE_SYSTEM) {
            return PM_METRIC_AVAILABILITY_NOT_IMPLEMENTED_BY_PRESENTMON;
        }
        return PM_METRIC_AVAILABILITY_NOT_EXPORTED_BY_SOURCE;
    }

    void ForEachTelemetryMetricForDeviceType(
        PM_DEVICE_TYPE deviceType,
        const std::function<void(PM_METRIC)>& fn)
    {
#define X_(id, metric_type, unit, polled, frame, enum_id, dev_type, ...) \
        if (dev_type == deviceType) { fn(id); }
        METRIC_LIST(X_)
#undef X_
    }

    void ApplyNotExportedForUnlistedTelemetryMetrics(
        ipc::MetricCapabilities& caps,
        PM_DEVICE_TYPE deviceType)
    {
        ForEachTelemetryMetricForDeviceType(deviceType, [&](PM_METRIC metricId) {
            if (!caps.Lookup(metricId)) {
                caps.Set(metricId, 0, DefaultMissingTelemetryMetricAvailability(deviceType));
            }
        });
    }
}