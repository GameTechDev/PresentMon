// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

export interface AppInfo {
    productName: string;
    productVersion: string;
    fileVersion: string;
    apiVersion: string;
    middlewareApiVersion: string;
    buildId: string;
    buildHash: string;
    buildHashShort: string;
    buildDateTime: string;
    buildConfig: string;
    buildDirty: boolean;
    isDebugBuild: boolean;
    compileDate: string;
    compileTime: string;
    serviceBuildId: string;
    serviceBuildTime: string;
    serviceVersion: string;
    cefVersion: string;
    cefVersionMajor: number;
    cefVersionMinor: number;
    cefVersionPatch: number;
    chromeVersion: string;
    chromeVersionMajor: number;
    chromeVersionMinor: number;
    chromeVersionBuild: number;
    chromeVersionPatch: number;
    cefProcessType: string;
    msvcVersion: string;
    winSdkVersion: string;
    crtVersion: string;
    crtRuntime: string;
    logLevel: string;
    verboseModules: string;
    devModeEnabled: boolean;
    debugBlocklistEnabled: boolean;
    chromiumDebugEnabled: boolean;
}
