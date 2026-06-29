// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

export interface QualifiedMetric {
    metricId: number,
    arrayIndex: number,
    statId: number,
    // null on GPU metrics means use preferences default adapter (resolved at push)
    deviceId: number | null,
    // omitted in loadout files >= 0.15.0; filled from introspection on repair/push
    desiredUnitId?: number,
}
