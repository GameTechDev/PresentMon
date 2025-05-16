// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Metric } from "./metric"

// fully-specified metric that can be used as a query element
export interface QualifiedMetric {
    metricId: number,
    arrayIndex: number,
    statId: number,
    // currently not directly filled by frontend -> active adapter is filled here in backend for non-universal metrics
    deviceId: number,
    // currently not touched by frontend and ignored by backend
    desiredUnitId: number,
}

export function makeDefaultQualifiedMetric(): QualifiedMetric {
    // TODO: hardcoded, make this dictated by backend OR at least derived from introspection options
    return {
        metricId: 8,
        arrayIndex: 0,
        statId: 1,
        deviceId: 0,
        desiredUnitId: 0,
    };
}