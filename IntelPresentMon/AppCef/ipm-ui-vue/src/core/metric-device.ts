// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { MetricDeviceType, type Metric } from './metric';
import type { LoadoutFile } from './loadout';
import type { QualifiedMetric } from './qualified-metric';

// Matches pmon::ipc::kSystemDeviceId in Interprocess/source/SystemDeviceId.h
export const SYSTEM_DEVICE_ID = 65536;

export interface MetricQueryContext {
    systemDeviceId: number;
    preferenceDefaultAdapterId: number;
    enablePerMetricDeviceSelection: boolean;
}

export function makeMetricQueryContext(
    systemDeviceId: number,
    adapterId: number,
    enablePerMetricDeviceSelection: boolean,
): MetricQueryContext {
    return {
        systemDeviceId,
        preferenceDefaultAdapterId: adapterId,
        enablePerMetricDeviceSelection,
    };
}

export function systemDeviceIdEffective(systemDeviceId: number): number {
    return systemDeviceId !== 0 ? systemDeviceId : SYSTEM_DEVICE_ID;
}

export function isFrameMetric(metric: Metric): boolean {
    return metric.deviceType === MetricDeviceType.Independent;
}

export function isSystemMetric(metric: Metric): boolean {
    return metric.deviceType === MetricDeviceType.System;
}

export function isGpuMetric(metric: Metric): boolean {
    return metric.deviceType === MetricDeviceType.GraphicsAdapter;
}

export function arraySizeForDevice(metric: Metric, deviceId: number): number {
    const entry = metric.deviceAvailability.find((d) => d.deviceId === deviceId);
    return entry?.arraySize ?? 0;
}

export function effectiveGpuDeviceId(
    deviceId: number | null,
    preferenceDefaultAdapterId: number,
): number {
    if (deviceId !== null && deviceId !== 0) {
        return deviceId;
    }
    return preferenceDefaultAdapterId;
}

export function storedGpuDeviceIdForMetricQuery(
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): number | null {
    if (!ctx.enablePerMetricDeviceSelection) {
        return null;
    }
    if (qm.deviceId === 0) {
        return null;
    }
    return qm.deviceId;
}

export function effectiveGpuDeviceIdForQualifiedMetric(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): number {
    return effectiveGpuDeviceId(
        storedGpuDeviceIdForMetricQuery(qm, ctx),
        ctx.preferenceDefaultAdapterId,
    );
}

export function clampArrayIndex(metric: Metric, deviceId: number, arrayIndex: number): number {
    const size = arraySizeForDevice(metric, deviceId);
    if (size <= 0) {
        return 0;
    }
    if (arrayIndex < 0) {
        return 0;
    }
    if (arrayIndex >= size) {
        return size - 1;
    }
    return arrayIndex;
}

function normalizePersistedQualifiedMetric(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): boolean {
    const deviceIdBefore = qm.deviceId;
    const arrayIndexBefore = qm.arrayIndex;

    if (isFrameMetric(metric)) {
        qm.deviceId = 0;
        qm.arrayIndex = clampArrayIndex(metric, 0, qm.arrayIndex);
    } else if (isSystemMetric(metric)) {
        qm.deviceId = systemDeviceIdEffective(ctx.systemDeviceId);
        qm.arrayIndex = clampArrayIndex(metric, qm.deviceId, qm.arrayIndex);
    } else if (isGpuMetric(metric)) {
        if (qm.deviceId === 0) {
            qm.deviceId = null;
        }
        const effectiveId = effectiveGpuDeviceIdForQualifiedMetric(metric, qm, ctx);
        qm.arrayIndex = clampArrayIndex(metric, effectiveId, qm.arrayIndex);
    }

    return qm.deviceId !== deviceIdBefore || qm.arrayIndex !== arrayIndexBefore;
}

/** Loadout 1.0.0: persisted GPU lines used deviceId 0 for "default"; store null instead. */
export function migratePersistedGpuDeviceIdsZeroToNull(
    file: LoadoutFile,
    metrics: Metric[],
): number {
    let lineFixCount = 0;
    for (const widget of file.widgets) {
        for (const widgetMetric of widget.metrics) {
            const metric = metrics.find((m) => m.id === widgetMetric.metric.metricId);
            if (metric === undefined || !isGpuMetric(metric)) {
                continue;
            }
            if (widgetMetric.metric.deviceId === 0) {
                widgetMetric.metric.deviceId = null;
                lineFixCount++;
            }
        }
    }
    return lineFixCount;
}

export function normalizeLoadoutPersistedMetrics(
    file: LoadoutFile,
    metrics: Metric[],
    ctx: MetricQueryContext,
): number {
    let lineFixCount = 0;
    for (const widget of file.widgets) {
        for (const widgetMetric of widget.metrics) {
            const metric = metrics.find((m) => m.id === widgetMetric.metric.metricId);
            if (metric === undefined) {
                continue;
            }
            if (normalizePersistedQualifiedMetric(metric, widgetMetric.metric, ctx)) {
                lineFixCount++;
            }
        }
    }
    return lineFixCount;
}

export function applyRuntimeQualifiedMetricFields(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): void {
    normalizePersistedQualifiedMetric(metric, qm, ctx);
    qm.desiredUnitId = metric.preferredUnitId;
}

export function repairQualifiedMetric(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): void {
    const deviceIdBefore = qm.deviceId;
    const arrayIndexBefore = qm.arrayIndex;
    applyRuntimeQualifiedMetricFields(metric, qm, ctx);
    if (qm.deviceId !== deviceIdBefore || qm.arrayIndex !== arrayIndexBefore) {
        console.warn(
            `repairQualifiedMetric: unexpected persisted field change for metric ${qm.metricId} `
            + '(loadout introspection migration should have normalized this already)',
        );
    }
}

export function prepareWidgetMetricForPush(
    metric: Metric,
    qm: QualifiedMetric,
    ctx: MetricQueryContext,
): boolean {
    applyRuntimeQualifiedMetricFields(metric, qm, ctx);
    let deviceId = qm.deviceId ?? 0;
    if (isGpuMetric(metric)) {
        deviceId = effectiveGpuDeviceIdForQualifiedMetric(metric, qm, ctx);
        if (deviceId === 0) {
            return false;
        }
        qm.deviceId = deviceId;
    }
    const arraySize = arraySizeForDevice(metric, deviceId);
    if (arraySize > 0) {
        if (qm.arrayIndex >= arraySize) {
            qm.arrayIndex = arraySize - 1;
        }
    } else {
        qm.arrayIndex = 0;
    }
    return true;
}

export function buildQualifiedMetricForMetric(
    metric: Metric,
    ctx: MetricQueryContext,
    statId?: number,
): QualifiedMetric {
    const qm: QualifiedMetric = {
        metricId: metric.id,
        arrayIndex: 0,
        statId: statId ?? metric.availableStatIds[0],
        deviceId: isGpuMetric(metric) ? null : 0,
        desiredUnitId: metric.preferredUnitId,
    };
    applyRuntimeQualifiedMetricFields(metric, qm, ctx);
    return qm;
}
