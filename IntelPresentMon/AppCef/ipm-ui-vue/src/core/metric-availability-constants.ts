// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

// Matches PM_METRIC_AVAILABILITY in PresentMonAPI2/PresentMonAPI.h
export const MetricAvailability = {
    Available: 0,
    Unavailable: 1,
    NotExportedBySource: 2,
    NotSupportedByDevice: 3,
    NotImplementedByPresentMon: 4,
} as const;

export type MetricAvailabilityValue = (typeof MetricAvailability)[keyof typeof MetricAvailability];

export interface MetricAvailabilityReason {
    id: number,
    description: string,
}

export const METRIC_UNAVAILABLE_ON_ANY_DEVICE = 'Not available on any device';

/** Used when introspection does not supply a description for an availability id. */
export const METRIC_AVAILABILITY_REASON_FALLBACK = 'Metric is unavailable on the indicated device';
