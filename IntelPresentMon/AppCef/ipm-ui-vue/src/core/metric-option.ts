// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

// metric id combined with array index
// necessary because metrics configuration not yet parameterized on index
// TODO: replace with ability to specify index independent of metric
export interface MetricOption {
    name: string,
    metricId: number,
    arrayIndex: number,
}