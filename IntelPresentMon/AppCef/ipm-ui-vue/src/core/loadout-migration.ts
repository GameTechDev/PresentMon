// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { compareVersions } from "./signature";
import { type LoadoutFile, signature } from "./loadout";
import { WidgetType } from "./widget";
import { type Readout, migrateReadout } from "./readout";
import { type Graph, migrateGraph } from "./graph";

interface Migration {
    version: string;
    migrate: (file: LoadoutFile) => void;
}

const PM_STAT_NONE = 0;
const PM_STAT_NEWEST_POINT = 12;
const PM_DEVICE_ID_SYSTEM = 65536;
const PM_METRIC_CPU_POWER_LIMIT = 57;

const cpuSystemMetrics = new Set<number>([
    4,  // PM_METRIC_CPU_VENDOR
    5,  // PM_METRIC_CPU_NAME
    56, // PM_METRIC_CPU_UTILIZATION
    57, // PM_METRIC_CPU_POWER_LIMIT
    58, // PM_METRIC_CPU_POWER
    59, // PM_METRIC_CPU_TEMPERATURE
    60, // PM_METRIC_CPU_FREQUENCY
    61, // PM_METRIC_CPU_CORE_UTILITY
]);

const newestPointMetrics = new Set<number>([
    1,  // PM_METRIC_SWAP_CHAIN_ADDRESS
    18, // PM_METRIC_SYNC_INTERVAL
    19, // PM_METRIC_PRESENT_FLAGS
    20, // PM_METRIC_PRESENT_MODE
    21, // PM_METRIC_PRESENT_RUNTIME
    63, // PM_METRIC_FRAME_TYPE
]);

function migrateLoadout_0_14_0(file: LoadoutFile): void {
    let statFixCount = 0;
    let deviceFixCount = 0;

    for (const widget of file.widgets) {
        for (const widgetMetric of widget.metrics) {
            const qmet = widgetMetric.metric;

            if (cpuSystemMetrics.has(qmet.metricId) && qmet.deviceId !== PM_DEVICE_ID_SYSTEM) {
                qmet.deviceId = PM_DEVICE_ID_SYSTEM;
                deviceFixCount++;
            }

            if (newestPointMetrics.has(qmet.metricId) && qmet.statId !== PM_STAT_NEWEST_POINT) {
                qmet.statId = PM_STAT_NEWEST_POINT;
                statFixCount++;
            }

            if (qmet.metricId === PM_METRIC_CPU_POWER_LIMIT && qmet.statId !== PM_STAT_NONE) {
                qmet.statId = PM_STAT_NONE;
                statFixCount++;
            }
        }
    }

    console.info(`Migrated loadout to 0.14.0; stat fixes: ${statFixCount}, device fixes: ${deviceFixCount}`);
}

const migrations: Migration[] = [
    {
        version: "0.14.0",
        migrate: migrateLoadout_0_14_0,
    },
];

migrations.sort((a, b) => compareVersions(a.version, b.version));

export function migrateLoadout(file: LoadoutFile): void {
    const sourceVersion = file.signature.version;

    if (file.signature.code !== signature.code) {
        throw new Error(`wrong signature code in loadout migration: ${file.signature.code}`);
    }
    if (compareVersions(sourceVersion, signature.version) > 0) {
        throw new Error(`error attempted migration from newer to older version: ${sourceVersion} => ${signature.version}`);
    }
    if (compareVersions(sourceVersion, signature.version) === 0) {
        console.warn(`migrateLoadout called but version up to date ${sourceVersion}`);
        return;
    }

    for (const widget of file.widgets) {
        if (widget.widgetType === WidgetType.Graph) {
            migrateGraph(widget as Graph, sourceVersion);
        }
        else if (widget.widgetType === WidgetType.Readout) {
            migrateReadout(widget as Readout, sourceVersion);
        }
        else {
            console.error("Unrecognized widget type in migrateLoadout");
        }
    }

    for (const migration of migrations) {
        if (compareVersions(migration.version, sourceVersion) > 0) {
            migration.migrate(file);
        }
    }

    file.signature.version = signature.version;
}
