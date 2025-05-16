// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Binding } from "./hotkey";
import { type Signature } from "./signature";
import { OverlayPosition } from "./overlay-position";
import { type RgbaColor } from "./color";
import { compareVersions } from "./signature";


export enum Preset {
    Slot1 = 0,
    Slot2 = 1,
    Slot3 = 2,
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
    adapterId:number|null;
    enableFlashInjection:boolean;
    flashInjectionSize:number;
    flashInjectionColor:RgbaColor;
    flashInjectionBackgroundEnable:boolean;
    flashInjectionBackgroundColor:RgbaColor;
    flashInjectionRightShift:number;
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
        independentWindow: false,
        metricPollRate: 40,
        overlayDrawRate: 10,
        telemetrySamplingPeriodMs: 100,
        etwFlushPeriod: 8,
        manualEtwFlush: true,
        metricsOffset: 32, 
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
        adapterId: null,
        enableFlashInjection: false,
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
    };
}

export const signature: Signature = {
    code: "p2c-cap-pref",
    version: "0.19.0",
};

export interface PreferenceFile {
    signature: Signature;
    preferences: Preferences;
    hotkeyBindings: {[key: string]: Binding};
}


interface Migration {
    version: string;
    migrate: (prefs: Preferences) => void;
}
  
const migrations: Migration[] = [
    {
        version: '0.16.0',
        migrate: (prefs: Preferences) => {
            let e = new Error('Preferences file version too old to migrate (<0.16.0).');
            (e as any).noticeOverride = true;
            throw e;
        }
    },
    {
        version: '0.17.0',
        migrate: (prefs: Preferences) => {
            const drawRate = Math.round(1000 / ((prefs as any).samplingPeriodMs * (prefs as any).samplesPerFrame));
            const pollRate = Math.round(1000 / (prefs as any).samplingPeriodMs);
            console.info(`Migrating preferences to 0.17.0; samplingPeriodMs:${(prefs as any).samplingPeriodMs} => metricPollRate:${pollRate}, samplesPerFrame:${(prefs as any).samplesPerFrame} => overlayDrawRate:${drawRate}`);
            prefs.metricPollRate = pollRate;
            prefs.overlayDrawRate = drawRate;
        }
    },
    {
        version: '0.18.0',
        migrate: (prefs: Preferences) => {
            console.info('Migrating preferences to 0.18.0 (manualEtwFlush enable/rate, lower offset)');
            const def = makeDefaultPreferences();
            prefs.manualEtwFlush = def.manualEtwFlush;
            prefs.etwFlushPeriod = def.etwFlushPeriod;
            prefs.metricsOffset = def.metricsOffset;
        }
    },
    {
        version: '0.19.0',
        migrate: (prefs: Preferences) => {
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
];

migrations.sort((a, b) => compareVersions(a.version, b.version));

export function migratePreferences(prefs: Preferences, sourceVersion: string): void {
    for (const mig of migrations) {
        if (compareVersions(mig.version, sourceVersion) > 0) {
            mig.migrate(prefs);
        }
    }
}
