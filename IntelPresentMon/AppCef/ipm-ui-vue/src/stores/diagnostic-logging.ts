// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
import { computed, ref } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import {
    DiagnosticLoggingMode,
    LoggingConfigField,
    LoggingConfigSource,
    makeDefaultDiagnosticLoggingValues,
    makeEmptyLoggingConfigurationSnapshot,
    type DiagnosticLoggingPatch,
    type DiagnosticLoggingValues,
    type LoggingConfigurationSnapshot,
} from '@/core/logging';

export const useDiagnosticLoggingStore = defineStore('diagnostic-logging', () => {
    const snapshot = ref<LoggingConfigurationSnapshot>(makeEmptyLoggingConfigurationSnapshot());
    const draft = ref<DiagnosticLoggingValues>(makeDefaultDiagnosticLoggingValues());
    const hydrated = ref(false);
    const applying = ref(false);
    const flushing = ref(false);
    const applyError = ref('');

    const effective = computed(() => snapshot.value.effective);
    const persisted = computed(() => snapshot.value.persisted);
    const activeLogPaths = computed(() => {
        if (!snapshot.value.clientLogPath && !snapshot.value.middlewareLogPath) {
            return null;
        }
        return {
            clientLogPath: snapshot.value.clientLogPath,
            middlewareLogPath: snapshot.value.middlewareLogPath,
        };
    });

    function fieldStatus(field: LoggingConfigField) {
        return snapshot.value.fields[field] ?? {
            source: LoggingConfigSource.Default,
            cliOverride: false,
        };
    }

    function isCliOverride(field: LoggingConfigField) {
        return fieldStatus(field).cliOverride && fieldStatus(field).source !== LoggingConfigSource.Ui;
    }

    // keep draft fields edited since base was captured (edits made while a request was in flight)
    function applySnapshot(next: LoggingConfigurationSnapshot, base?: DiagnosticLoggingValues) {
        const kept = base ? Object.fromEntries(Object.entries(draft.value)
            .filter(([key, value]) => value !== base[key as keyof DiagnosticLoggingValues])) : {};
        snapshot.value = next;
        draft.value = { ...next.effective, ...kept };
    }

    async function hydrate() {
        const next = await Api.getLoggingConfiguration();
        applySnapshot(next);
        hydrated.value = true;
        applyError.value = '';
        if (next.effective) {
            try {
                await Api.applyUiLoggingPolicy(next.effective);
            }
            catch (error) {
                console.warn('Failed to apply UI logging policy during hydration: ' + error);
            }
        }
    }

    async function run(request: DiagnosticLoggingPatch) {
        if (!hydrated.value) {
            return;
        }
        applying.value = true;
        applyError.value = '';
        const base = { ...draft.value };
        try {
            await Api.setDiagnosticLogging(request);
            const next = await Api.getLoggingConfiguration();
            applySnapshot(next, base);
            await Api.applyUiLoggingPolicy(next.effective);
        }
        catch (error) {
            applyError.value = error instanceof Error ? error.message : String(error);
            throw error;
        }
        finally {
            applying.value = false;
        }
    }

    const patch = (partial: DiagnosticLoggingPatch) =>
        run({ mode: DiagnosticLoggingMode.Patch, persist: true, ...partial });
    const restoreDefaults = () => run({ mode: DiagnosticLoggingMode.RestoreDefaults });

    async function saveFieldAsDefault(field: LoggingConfigField) {
        const payload: DiagnosticLoggingPatch = {
            mode: DiagnosticLoggingMode.SaveAsDefault,
        };
        switch (field) {
            case LoggingConfigField.LogLevel:
                payload.logLevel = draft.value.logLevel;
                break;
            case LoggingConfigField.TraceLevel:
                payload.traceLevel = draft.value.traceLevel;
                break;
            case LoggingConfigField.VerboseModules:
                payload.verboseModules = draft.value.verboseModules;
                break;
            case LoggingConfigField.Folder:
                payload.folder = draft.value.folder;
                break;
            case LoggingConfigField.ExceptionTrace:
                payload.exceptionTrace = draft.value.exceptionTrace;
                break;
        }
        await run(payload);
    }

    async function flush() {
        flushing.value = true;
        applyError.value = '';
        try {
            await Api.flushDiagnosticLogging();
        }
        catch (error) {
            applyError.value = error instanceof Error ? error.message : String(error);
            throw error;
        }
        finally {
            flushing.value = false;
        }
    }

    return {
        snapshot,
        draft,
        hydrated,
        applying,
        flushing,
        applyError,
        effective,
        persisted,
        activeLogPaths,
        fieldStatus,
        isCliOverride,
        hydrate,
        patch,
        restoreDefaults,
        saveFieldAsDefault,
        flush,
    };
});
