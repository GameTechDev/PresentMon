// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
//
// Availability rules align with p2c::pmon::IsQualifiedMetricAvailable in
// Core/source/pmon/QualifiedMetricAvailability.h (kernel overlay push path).
import type { Metric } from './metric';
import type { QualifiedMetric } from './qualified-metric';
import {
    applyRuntimeQualifiedMetricFields,
    effectiveGpuDeviceIdForQualifiedMetric,
    isFrameMetric,
    isGpuMetric,
    isSystemMetric,
    systemDeviceIdEffective,
    type MetricQueryContext,
} from './metric-device';
import {
    MetricAvailability,
    METRIC_AVAILABILITY_REASON_FALLBACK,
    METRIC_UNAVAILABLE_ON_ANY_DEVICE,
    type MetricAvailabilityReason,
} from './metric-availability-constants';
import type { Adapter } from './adapter';

function resolveQueryDeviceId(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): number {
    if (isFrameMetric(metric)) {
        return 0;
    }
    if (isSystemMetric(metric)) {
        return systemDeviceIdEffective(ctx.systemDeviceId);
    }
    if (isGpuMetric(metric)) {
        return effectiveGpuDeviceIdForQualifiedMetric(metric, qm, ctx);
    }
    return qm.deviceId ?? 0;
}

function deviceEntryForQuery(metric: Metric, queryDeviceId: number) {
    return metric.deviceAvailability.find((d) => d.deviceId === queryDeviceId);
}

function isDeviceEntryAvailable(entry: { availabilityId: number } | undefined): boolean {
    return entry !== undefined && entry.availabilityId === MetricAvailability.Available;
}

/** Device-level availability for UI (device picker); ignores qualified array index. */
function isMetricAvailableOnDeviceIgnoringArrayIndex(
    metric: Metric,
    queryDeviceId: number,
): boolean {
    return isDeviceEntryAvailable(deviceEntryForQuery(metric, queryDeviceId));
}

/** Normalize line qualified-metric fields for availability checks against another metric in the picker. */
export function qualifiedMetricForAvailabilityProbe(
    metric: Metric,
    lineQm: QualifiedMetric,
    ctx: MetricQueryContext,
): QualifiedMetric {
    const probe: QualifiedMetric = { ...lineQm, metricId: metric.id };
    applyRuntimeQualifiedMetricFields(metric, probe, ctx);
    return probe;
}

function reasonTextForAvailabilityId(
    availabilityId: number,
    reasons: readonly MetricAvailabilityReason[],
): string {
    const match = reasons.find((r) => r.id === availabilityId);
    if (match !== undefined) {
        return match.description;
    }
    return METRIC_AVAILABILITY_REASON_FALLBACK;
}

export function isMetricAvailable(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): boolean {
    const queryDeviceId = resolveQueryDeviceId(metric, qm, ctx);
    const entry = deviceEntryForQuery(metric, queryDeviceId);
    if (!entry) {
        return false;
    }
    if (entry.availabilityId !== MetricAvailability.Available) {
        return false;
    }
    if (entry.arraySize <= 0) {
        return qm.arrayIndex === 0;
    }
    return qm.arrayIndex < entry.arraySize;
}

export function isMetricAvailableForGpuDeviceId(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    deviceId: number | null,
): boolean {
    const probe: QualifiedMetric = { ...qm, deviceId };
    const queryDeviceId = resolveQueryDeviceId(metric, probe, ctx);
    return isMetricAvailableOnDeviceIgnoringArrayIndex(metric, queryDeviceId);
}

export function metricAvailabilityReasonForGpuDeviceId(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    deviceId: number | null,
    reasons: readonly MetricAvailabilityReason[],
): string | null {
    const probe: QualifiedMetric = { ...qm, deviceId };
    const queryDeviceId = resolveQueryDeviceId(metric, probe, ctx);
    if (isMetricAvailableOnDeviceIgnoringArrayIndex(metric, queryDeviceId)) {
        return null;
    }
    const entry = deviceEntryForQuery(metric, queryDeviceId);
    if (entry !== undefined) {
        return reasonTextForAvailabilityId(entry.availabilityId, reasons);
    }
    return reasonTextForAvailabilityId(MetricAvailability.Unavailable, reasons);
}

export function getMetricAvailabilityReasonText(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    reasons: readonly MetricAvailabilityReason[],
): string | null {
    if (isMetricAvailable(metric, qm, ctx)) {
        return null;
    }
    const queryDeviceId = resolveQueryDeviceId(metric, qm, ctx);
    const entry = deviceEntryForQuery(metric, queryDeviceId);
    if (entry !== undefined) {
        return reasonTextForAvailabilityId(entry.availabilityId, reasons);
    }
    return reasonTextForAvailabilityId(MetricAvailability.Unavailable, reasons);
}

export function isMetricUnavailableOnAllAdapters(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    adapters: readonly Adapter[],
): boolean {
    if (!isGpuMetric(metric) || adapters.length === 0) {
        return false;
    }
    return adapters.every((adapter) =>
        !isMetricAvailableForGpuDeviceId(metric, qm, ctx, adapter.id),
    );
}

export function isMetricUnavailableInAutocomplete(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    adapters: readonly Adapter[],
): boolean {
    if (!ctx.enablePerMetricDeviceSelection || !isGpuMetric(metric)) {
        return !isMetricAvailable(metric, qm, ctx);
    }
    return isMetricUnavailableOnAllAdapters(metric, qm, ctx, adapters);
}

export function metricTooltipAvailabilityReason(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
    reasons: readonly MetricAvailabilityReason[],
    adapters: readonly Adapter[],
): string | null {
    if (isMetricUnavailableInAutocomplete(metric, qm, ctx, adapters)) {
        if (ctx.enablePerMetricDeviceSelection && isGpuMetric(metric)) {
            return METRIC_UNAVAILABLE_ON_ANY_DEVICE;
        }
        return getMetricAvailabilityReasonText(metric, qm, ctx, reasons);
    }
    return getMetricAvailabilityReasonText(metric, qm, ctx, reasons);
}
