// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Metric } from '@/core/metric'
import { Process } from '@/core/process'
import { Adapter } from './adapter'
import { Spec } from '@/core/spec'
import { Binding, KeyOption, ModifierOption, Action } from '@/core/hotkey'
import { WidgetType } from './widget'
import { Graph } from './graph'

/* eslint-disable no-explicit-any */

export enum FileLocation {
    Install,
    Data,
    Documents,
}

type AsyncCallback = (arg: any) => void;
type SignalCallback = (...args: any[]) => void;

interface Core {
    invokeEndpoint(key: string, payload: any, resolve: AsyncCallback, reject: AsyncCallback): void;
    registerSignalHandler(key: string, callback: SignalCallback): void;
}

export class Api {
    private static get core(): Core {
        return (window as unknown as {core: Core}).core;
    }
    private static invokeEndpointFuture(key: string, payload: any): Promise<any> {
        return new Promise((resolve: AsyncCallback, reject: AsyncCallback) => {
            this.core.invokeEndpoint(key, payload, resolve, reject);
        });
    }

    // async endpoints
    static async enumerateModifiers(): Promise<ModifierOption[]> {
        const {mods} = await this.invokeEndpointFuture('enumerateModifiers', {});
        if (!Array.isArray(mods)) {
            throw new Error('Bad (non-array) type returned from enumerateModifiers');
        }
        return mods;
    }
    static async enumerateKeys(): Promise<KeyOption[]> {
        const {keys} = await this.invokeEndpointFuture('enumerateKeys', {});
        if (!Array.isArray(keys)) {
            throw new Error('Bad (non-array) type returned from enumerateKeys');
        }
        return keys;
    }
    static async enumerateMetrics(): Promise<Metric[]> {
        const {metrics} = await this.invokeEndpointFuture('enumerateMetrics', {});
        if (!Array.isArray(metrics)) {
            throw new Error('Bad (non-array) type returned from enumerateMetrics');
        }
        return metrics;
    }
    static async enumerateProcesses(): Promise<Process[]> {
        const {processes} = await this.invokeEndpointFuture('enumerateProcesses', {});
        if (!Array.isArray(processes)) {
            throw new Error('Bad (non-array) type returned from enumerateProcesses');
        }
        return processes;
    }
    static async getTopGpuProcess(blacklist: string[]): Promise<Process|null> {
        const {top} = await this.invokeEndpointFuture('getTopGpuProcess', {blacklist});
        return top;
    }
    static async enumerateAdapters(): Promise<Adapter[]> {
        const {adapters} = await this.invokeEndpointFuture('enumerateAdapters', {});
        if (!Array.isArray(adapters)) {
            throw new Error('Bad (non-array) type returned from enumerateAdapters');
        }
        return adapters;
    }
    static async bindHotkey(binding: Binding): Promise<void> {
        await this.invokeEndpointFuture('bindHotkey', binding);
    }
    static async clearHotkey(action: Action): Promise<void> {
        await this.invokeEndpointFuture('clearHotkey', {action});
    }
    static async setAdapter(id: number): Promise<void> {
        await this.invokeEndpointFuture('setAdapter', {id});
    }
    static async launchKernel(): Promise<void> {
        await this.invokeEndpointFuture('launchKernel', {});
    }
    static async pushSpecification(spec: Spec): Promise<void> {
        await this.invokeEndpointFuture('pushSpecification', spec);
    }
    static async setCapture(active: boolean): Promise<void> {
        await this.invokeEndpointFuture('setCapture', {active});
    }

    /////// file access-related /////////
    // base file endpoints
    static async loadFile(location: FileLocation, path: string): Promise<{payload: string}> {
        return await this.invokeEndpointFuture('loadFile', {location, path});
    }
    static async storeFile(payload: string, location: FileLocation, path: string): Promise<void> {
        await this.invokeEndpointFuture('storeFile', {payload, location, path});
    }
    static async browseStoreSpec(payload: string): Promise<void> {
        await this.invokeEndpointFuture('browseStoreSpec', {payload});
    }
    static async browseReadSpec(): Promise<{payload: string}> {
        return await this.invokeEndpointFuture('browseReadSpec', {});
    }
    static async exploreCaptures(): Promise<void> {
        await this.invokeEndpointFuture('exploreCaptures', {});
    }
    // derived file endpoints
    static async loadPreset(path: string): Promise<{payload: string}> {
        return await this.loadFile(FileLocation.Install, `Presets\\${path}`);
    }
    static async loadConfig(path: string): Promise<{payload: string}> {
        return await this.loadFile(FileLocation.Documents, `Loadouts\\${path}`);
    }
    static async storeConfig(payload: string, path: string): Promise<void> {
        await this.storeFile(payload, FileLocation.Documents, `Loadouts\\${path}`);
    }
    static async loadPreferences(): Promise<{payload: string}> {
        return await this.loadFile(FileLocation.Documents, 'preferences.json');
    }
    static async storePreferences(payload: string): Promise<void> {
        await this.storeFile(payload, FileLocation.Documents, 'preferences.json');
    }

    /////// signal handlers ///////
    static registerHotkeyHandler(callback: (action: number) => void) {
        this.core.registerSignalHandler('hotkeyFired', callback);
    }
    static registerPresentmonInitFailedHandler(callback: () => void) {
        this.core.registerSignalHandler('presentmonInitFailed', callback);
    }
    static registerOverlayDiedHandler(callback: () => void) {
        this.core.registerSignalHandler('overlayDied', callback);
    }
    static registerTargetLostHandler(callback: (pid: number) => void) {
        this.core.registerSignalHandler('targetLost', callback);
    }
    static registerStalePidHandler(callback: () => void) {
        this.core.registerSignalHandler('stalePid', callback);
    }
}