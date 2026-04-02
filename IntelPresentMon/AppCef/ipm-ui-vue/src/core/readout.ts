// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Widget, WidgetType, generateKey } from './widget'
import { makeDefaultWidgetMetric } from './widget-metric';
import { type QualifiedMetric } from './qualified-metric';
import { type RgbaColor } from './color';

export interface Readout extends Widget {
    showLabel: boolean,
    fontSize: number,
    fontColor: RgbaColor,
    backgroundColor: RgbaColor,
}

export function makeDefaultReadout(metric: QualifiedMetric|null = null): Readout {
    return {
        key: generateKey(),
        metrics: [makeDefaultWidgetMetric(metric)],
        widgetType: WidgetType.Readout,
        showLabel: true,
        fontSize: 12,
        fontColor: {
            r: 205,
            g: 211,
            b: 233,
            a: 1
        },
        backgroundColor: {
            r: 45,
            g: 50,
            b: 96,
            a: 0.4
        },
    };
}
