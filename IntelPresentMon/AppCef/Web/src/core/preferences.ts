// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Binding } from "./hotkey";
import { Signature } from "./signature";
import { OverlayPosition } from "./overlay-position";
import { RgbaColor } from "./color";


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
    samplingPeriodMs: number;
    samplesPerFrame: number;
    telemetrySamplingPeriodMs: number;
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
        samplingPeriodMs: 100, 
        samplesPerFrame: 1, 
        telemetrySamplingPeriodMs: 100, 
        metricsOffset: 1020, 
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
        enableAutotargetting: true,
        upscaleFactor: 2,        
        adapterId: null,
    };
}

export const signature: Signature = {
    code: "p2c-cap-pref",
    version: "0.16.0",
};

export interface PreferenceFile {
    signature: Signature;
    preferences: Preferences;
    hotkeyBindings: {[key: string]: Binding};
}

