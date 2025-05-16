// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Graph } from './graph'
import { type Readout } from './readout'
import { type WidgetMetric, regenerateKeys as regenerateMetricKeys } from './widget-metric';

export enum WidgetType {
    Graph,
    Readout,
}

export interface Widget {
    key: number;
    metrics: WidgetMetric[];
    widgetType: WidgetType;
}

let nextKey = 0;

export function generateKey(): number {
    return nextKey++;
}

export function resetKeySequence(): void {
    nextKey = 0;
}

export function regenerateKeys(w: Widget): void {
    for (const m of w.metrics) {
        regenerateMetricKeys(m);
    }
    w.key = generateKey();
}

export function asGraph(w: Widget): Graph {    
    if (w.widgetType !== WidgetType.Graph) {
        throw new Error(`Widget type [${WidgetType[w.widgetType]}] accessed as Graph`);
    }
    return w as Graph;
}

export function asReadout(w: Widget): Readout {    
    if (w.widgetType !== WidgetType.Readout) {
        throw new Error(`Widget type [${WidgetType[w.widgetType]}] accessed as Readout`);
    }
    return w as Readout;
}