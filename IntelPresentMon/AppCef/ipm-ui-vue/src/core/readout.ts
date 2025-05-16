// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Widget, WidgetType, generateKey } from './widget'
import { makeDefaultWidgetMetric } from './widget-metric';
import { type QualifiedMetric } from './qualified-metric';
import { type RgbaColor } from './color';
import { compareVersions } from './signature';

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

interface Migration {
    version: string;
    migrate: (readout: Readout) => void;
}

const migrations: Migration[] = [
    {
        version: '0.13.0',
        migrate: (readout: Readout) => {
            let e = new Error('Loadout file version too old to migrate (<0.13.0).');
            (e as any).noticeOverride = true;
            throw e;
        }
    },
];

migrations.sort((a, b) => compareVersions(a.version, b.version));

export function migrateReadout(readout: Readout, sourceVersion: string): void {
    for (const mig of migrations) {
        if (compareVersions(mig.version, sourceVersion) > 0) {
            mig.migrate(readout);
        }
    }
}