// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface Metric {
    id: number,
    name: string,
    description: string,
    availableStatIds: number[],
    // not used yet in frontend -> fully controlled on backend
    preferredUnitId: number,
    // not independently controlled in frontend -> represented as package with id in MetricOption
    arraySize: number,
    // not used yet in frontend -> backend fills in selected adapter id IF metric is not universal
    availableDeviceIds: number[],
    numeric: boolean,
}