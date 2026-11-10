// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

export enum LogLevel {
    None = 0,
    Fatal = 10,
    Error = 20,
    Warning = 30,
    Info = 40,
    Performance = 50,
    Debug = 60,
    Verbose = 70,
    Verbose2 = 80,
}

export enum LoggingConfigSource {
    Default = 0,
    Persisted = 1,
    Cli = 2,
    Ui = 3,
}

export enum LoggingConfigField {
    LogLevel = 0,
    TraceLevel = 1,
    VerboseModules = 2,
    Folder = 3,
    ExceptionTrace = 4,
}

export enum DiagnosticLoggingMode {
    Patch = 0,
    RestoreDefaults = 1,
    SaveAsDefault = 2,
}

export interface DiagnosticLoggingValues {
    logLevel: LogLevel;
    traceLevel: LogLevel;
    verboseModules: number;
    folder: string;
    exceptionTrace: boolean;
}

export interface LoggingConfigFieldStatus {
    source: LoggingConfigSource;
    cliOverride: boolean;
}

export interface LoggingConfigurationSnapshot {
    persisted: DiagnosticLoggingValues;
    effective: DiagnosticLoggingValues;
    cli: DiagnosticLoggingValues;
    fields: LoggingConfigFieldStatus[];
    clientLogPath: string;
    middlewareLogPath: string;
    storeLoaded: boolean;
}

export interface DiagnosticLoggingPatch {
    mode?: DiagnosticLoggingMode;
    logLevel?: LogLevel;
    traceLevel?: LogLevel;
    verboseModules?: number;
    folder?: string;
    exceptionTrace?: boolean;
    persist?: boolean;
}

export interface LogLevelOption {
    title: string;
    value: LogLevel;
}

export interface VerboseModuleOption {
    title: string;
    value: number;
}

export const logLevelOptions: LogLevelOption[] = [
    { title: 'Disabled', value: LogLevel.None },
    { title: 'Fatal', value: LogLevel.Fatal },
    { title: 'Error', value: LogLevel.Error },
    { title: 'Warning', value: LogLevel.Warning },
    { title: 'Info', value: LogLevel.Info },
    { title: 'Performance', value: LogLevel.Performance },
    { title: 'Debug', value: LogLevel.Debug },
    { title: 'Verbose', value: LogLevel.Verbose },
    { title: 'Verbose 2', value: LogLevel.Verbose2 },
];

const verboseModuleNames = [
    'V8 Async',
    'Process Watch',
    'GPU Telemetry',
    'UI',
    'Core Metrics',
    'Core Hotkeys',
    'Core Window',
    'ETW Query',
    'Kernel Actions',
    'IPC Storage',
    'IPC Ring',
    'Metric Use',
    'Middleware',
    'Dynamic Integration',
    'Chromium',
];

// TODO: Replace JS bitwise masks before the module count exceeds 31; bitwise
// operations truncate to 32 bits, but the current list has only 15 modules.
export const verboseModuleOptions: VerboseModuleOption[] = verboseModuleNames.map(
    (title, index) => ({ title, value: 1 << index }),
);

export function makeDefaultDiagnosticLoggingValues(): DiagnosticLoggingValues {
    return {
        logLevel: LogLevel.Error,
        traceLevel: LogLevel.Error,
        verboseModules: 0,
        folder: '',
        exceptionTrace: false,
    };
}

export function makeEmptyLoggingConfigurationSnapshot(): LoggingConfigurationSnapshot {
    const defaults = makeDefaultDiagnosticLoggingValues();
    return {
        persisted: { ...defaults },
        effective: { ...defaults },
        cli: { ...defaults },
        fields: Object.values(LoggingConfigField)
            .filter((value): value is LoggingConfigField => typeof value === 'number')
            .map(() => ({
                source: LoggingConfigSource.Default,
                cliOverride: false,
            })),
        clientLogPath: '',
        middlewareLogPath: '',
        storeLoaded: false,
    };
}

export function logLevelTitle(level: LogLevel): string {
    return logLevelOptions.find(option => option.value === level)?.title ?? String(level);
}
