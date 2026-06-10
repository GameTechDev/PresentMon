// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface MetricDeviceAvailability {
    deviceId: number,
    arraySize: number,
    availabilityId: number,
}

// Matches PM_DEVICE_TYPE in PresentMonAPI2/PresentMonAPI.h
export const MetricDeviceType = {
    Independent: 0,
    GraphicsAdapter: 1,
    System: 2,
} as const;

export type MetricDeviceTypeValue = (typeof MetricDeviceType)[keyof typeof MetricDeviceType];

export interface Metric {
    id: number,
    name: string,
    description: string,
    availableStatIds: number[],
    preferredUnitId: number,
    deviceType: MetricDeviceTypeValue,
    deviceAvailability: MetricDeviceAvailability[],
    numeric: boolean,
}
