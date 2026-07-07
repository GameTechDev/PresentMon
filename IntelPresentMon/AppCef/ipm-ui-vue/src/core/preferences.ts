// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Binding } from "./hotkey";
import { type Signature } from "./signature";
import { OverlayPosition } from "./overlay-position";
import { type RgbaColor } from "./color";
import { compareVersions } from "./signature";
import { lowestAdapterId, type Adapter } from "./adapter";


export enum Preset {
    Slot1 = 0,
    Slot2 = 1,
    Slot3 = 2,
    Slot4 = 3,
    Custom = 1000,
}

export interface Preferences {
    selectedPreset: Preset|null;
    capturePath: string;
    captureDelay: number,
    enableCaptureDelay: boolean,
    captureDuration: number,
    enableCaptureDuration: boolean,
    hideDuringCapture: boolean;
    hideAlways: boolean;
    enablePerMetricDeviceSelection: boolean;
    independentWindow: boolean;
    metricPollRate: number;
    overlayDrawRate: number;
    telemetrySamplingPeriodMs: number;
    etwFlushPeriod: number;
    manualEtwFlush: boolean;
    metricsOffset: number;
    metricsWindow: number;
    overlayPosition: OverlayPosition;
    timeRange: number;
    overlayWidth: number;
    upscale: boolean;
    upscaleFactor: number;
    generateStats: boolean;
    enableTargetBlocklist: boolean;
    enableAutotargetting: boolean;
    readonly overlayMargin: 0;
    readonly overlayBorder: 0;
    readonly overlayPadding: 10;
    readonly graphMargin: 2;
    readonly graphBorder: 0;
    readonly graphPadding: 5;
    readonly overlayBorderColor: RgbaColor;
    readonly overlayBackgroundColor: RgbaColor;
    readonly graphFont: {
        readonly name: 'Verdana';
        readonly axisSize: 10.0;
    };    
    adapterId: number;
    enableFlashInjection:boolean;
    flashInjectionEnableTargetOverride:boolean,
    flashInjectionTargetOverride:string;
    flashInjectionSize:number;
    flashInjectionColor:RgbaColor;
    flashInjectionBackgroundEnable:boolean;
    flashInjectionBackgroundColor:RgbaColor;
    flashInjectionRightShift:number;
    flashInjectionFlashDuration:number;
    flashInjectionUseRainbow:boolean;
    flashInjectionBackgroundSize:number;
};

export function makeDefaultPreferences(): Preferences {
    return {
        selectedPreset: null,
        capturePath: "", 
        captureDelay: 1,
        enableCaptureDelay: false,
        captureDuration: 10,
        enableCaptureDuration: false,
        hideDuringCapture: true, 
        hideAlways: false,
        enablePerMetricDeviceSelection: false,
        independentWindow: false,
        metricPollRate: 40,
        overlayDrawRate: 10,
        telemetrySamplingPeriodMs: 100,
        etwFlushPeriod: 8,
        manualEtwFlush: true,
        metricsOffset: 150, 
        metricsWindow: 1000, 
        overlayPosition: 0, 
        timeRange: 10, 
        overlayMargin: 0, 
        overlayBorder: 0, 
        overlayPadding: 10, 
        graphMargin: 2, 
        graphBorder: 0, 
        graphPadding: 5, 
        overlayBorderColor: { 
            r: 255, 
            g: 255, 
            b: 255, 
            a: 0.0, 
        }, 
        overlayBackgroundColor: { 
            r: 50, 
            g: 57, 
            b: 91, 
            a: 220 / 255, 
        }, 
        graphFont: { 
            name: 'Verdana', 
            axisSize: 10.0, 
        }, 
        overlayWidth: 400,         
        upscale: false,
        generateStats: true,
        enableTargetBlocklist: true,
        enableAutotargetting: false,
        upscaleFactor: 2,        
        adapterId: 0,
        enableFlashInjection: false,
        flashInjectionEnableTargetOverride: false,
        flashInjectionTargetOverride: '',
        flashInjectionSize: 0.25,
        flashInjectionColor: {
            r: 255, 
            g: 255, 
            b: 255, 
            a: 255, 
        },
        flashInjectionBackgroundEnable: false,
        flashInjectionBackgroundColor: {
            r: 0, 
            g: 0, 
            b: 0, 
            a: 255, 
        },
        flashInjectionRightShift: 0.5,
        flashInjectionFlashDuration: 0.05,
        flashInjectionUseRainbow: false,
        flashInjectionBackgroundSize:0.25,
    };
}

export const signature: Signature = {
    code: "p2c-cap-pref",
    version: "1.0.1",
};

export interface PreferenceFile {
    signature: Signature;
    preferences: Preferences;
    hotkeyBindings: {[key: string]: Binding};
}


export interface PreferenceMigrationIntrospection {
    adapters: readonly Adapter[];
}

interface Migration {
    version: string;
    migrate: (prefs: Preferences, intro: PreferenceMigrationIntrospection | null) => void;
}

const migrations: Migration[] = [
    {
        version: '0.16.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            let e = new Error('Preferences file version too old to migrate (<0.16.0).');
            (e as any).noticeOverride = true;
            throw e;
        }
    },
    {
        version: '0.17.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            const drawRate = Math.round(1000 / ((prefs as any).samplingPeriodMs * (prefs as any).samplesPerFrame));
            const pollRate = Math.round(1000 / (prefs as any).samplingPeriodMs);
            console.info(`Migrating preferences to 0.17.0; samplingPeriodMs:${(prefs as any).samplingPeriodMs} => metricPollRate:${pollRate}, samplesPerFrame:${(prefs as any).samplesPerFrame} => overlayDrawRate:${drawRate}`);
            prefs.metricPollRate = pollRate;
            prefs.overlayDrawRate = drawRate;
        }
    },
    {
        version: '0.18.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            console.info('Migrating preferences to 0.18.0 (manualEtwFlush enable/rate, lower offset)');
            const def = makeDefaultPreferences();
            prefs.manualEtwFlush = def.manualEtwFlush;
            prefs.etwFlushPeriod = def.etwFlushPeriod;
            prefs.metricsOffset = def.metricsOffset;
        }
    },
    {
        version: '0.19.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            console.info('Migrating preferences to 0.19.0 (flash injection)');
            const def = makeDefaultPreferences();
            prefs.enableFlashInjection = def.enableFlashInjection;
            prefs.flashInjectionSize = def.flashInjectionSize;
            prefs.flashInjectionColor = def.flashInjectionColor;
            prefs.flashInjectionBackgroundEnable = def.flashInjectionBackgroundEnable;
            prefs.flashInjectionBackgroundColor = def.flashInjectionBackgroundColor;
            prefs.flashInjectionRightShift = def.flashInjectionRightShift;
        }
    },
    {
        version: '0.20.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            if (prefs.metricsOffset <= 32) {
                const def = makeDefaultPreferences();
                console.info(`Migrating preferences to 0.20.0 (metricsOffset: ${prefs.metricsOffset} => ${def.metricsOffset})`);
                prefs.metricsOffset = def.metricsOffset;
            }
        }
    },
    {
        version: '0.21.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            if (prefs.metricsOffset === 80) {
                const def = makeDefaultPreferences();
                console.info(`Migrating preferences to 0.21.0 (metricsOffset: ${prefs.metricsOffset} => ${def.metricsOffset})`);
                prefs.metricsOffset = def.metricsOffset;
            }
        }
    },
    {
        version: '1.0.0',
        migrate: (prefs: Preferences, _intro: PreferenceMigrationIntrospection | null) => {
            const def = makeDefaultPreferences();
            if (prefs.enablePerMetricDeviceSelection === undefined) {
                prefs.enablePerMetricDeviceSelection = def.enablePerMetricDeviceSelection;
            }
            console.info('Migrating preferences to 1.0.0');
        },
    },
    {
        version: '1.0.1',
        migrate: (prefs: Preferences, intro: PreferenceMigrationIntrospection | null) => {
            if (intro === null) {
                throw new Error('Preferences migration to 1.0.1 requires introspection');
            }
            const legacy = prefs.adapterId as number | null | undefined;
            const missing = legacy == null || legacy === 0;
            const unknown = !missing
                && intro.adapters.length > 0
                && !intro.adapters.some((a) => a.id === legacy);
            if (missing || unknown) {
                const resolved = lowestAdapterId(intro.adapters);
                console.info(
                    `Migrating preferences to 1.0.1 (adapterId: ${legacy ?? 'null'} => ${resolved})`,
                );
                prefs.adapterId = resolved;
            } else {
                console.info('Migrating preferences to 1.0.1');
            }
        },
    },
];

migrations.sort((a, b) => compareVersions(a.version, b.version));

export function migratePreferences(
    prefs: Preferences,
    sourceVersion: string,
    intro: PreferenceMigrationIntrospection | null,
): void {
    for (const mig of migrations) {
        if (compareVersions(mig.version, sourceVersion) > 0) {
            mig.migrate(prefs, intro);
        }
    }
}
