// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { compareVersions } from "./signature";
import { type LoadoutFile, signature } from "./loadout";

interface Migration {
    version: string;
    migrate: (file: LoadoutFile) => void;
}

const migrations: Migration[] = [
    {
        version: "0.13.0",
        migrate: (_file: LoadoutFile): void => {
            const e = new Error("Loadout file version too old to migrate (<0.13.0).");
            (e as { noticeOverride?: boolean }).noticeOverride = true;
            throw e;
        },
    },
    {
        version: "0.14.0",
        migrate: (file: LoadoutFile): void => {
            const PM_STAT_NONE = 0;
            const PM_STAT_MID_POINT = 10;
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

            let statFixCount = 0;
            let deviceFixCount = 0;

            for (const widget of file.widgets) {
                for (const widgetMetric of widget.metrics) {
                    const qmet = widgetMetric.metric;

                    if (cpuSystemMetrics.has(qmet.metricId) && qmet.deviceId !== PM_DEVICE_ID_SYSTEM) {
                        qmet.deviceId = PM_DEVICE_ID_SYSTEM;
                        deviceFixCount++;
                    }

                    if (qmet.statId === PM_STAT_MID_POINT) {
                        qmet.statId = PM_STAT_NEWEST_POINT;
                        statFixCount++;
                    }

                    if (qmet.metricId === PM_METRIC_CPU_POWER_LIMIT) {
                        qmet.statId = PM_STAT_NONE;
                        statFixCount++;
                    }
                }
            }

            console.info(`Migrated loadout to 0.14.0; stat fixes: ${statFixCount}, device fixes: ${deviceFixCount}`);
        },
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

    for (const migration of migrations) {
        if (compareVersions(migration.version, sourceVersion) > 0) {
            migration.migrate(file);
        }
    }

    file.signature.version = signature.version;
}
