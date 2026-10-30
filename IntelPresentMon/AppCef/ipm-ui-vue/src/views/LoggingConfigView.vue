<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { computed, onBeforeUnmount, watch } from 'vue';
import { usePreferencesStore } from '@/stores/preferences';
import { useDiagnosticLoggingStore } from '@/stores/diagnostic-logging';
import { Api } from '@/core/api';
import { Action } from '@/core/hotkey';
import HotkeyButton from '@/components/HotkeyButton.vue';
import {
    LoggingConfigField,
    logLevelOptions,
    logLevelTitle,
    verboseModuleOptions,
} from '@/core/logging';

const prefs = usePreferencesStore()
const logging = useDiagnosticLoggingStore()
const diagnostic = computed(() => logging.draft)
let applyTimer: ReturnType<typeof setTimeout> | null = null

const selectedVerboseModules = computed<number[]>({
    get: () => verboseModuleOptions
        .filter(module => (diagnostic.value.verboseModules & module.value) !== 0)
        .map(module => module.value),
    set: values => {
        diagnostic.value.verboseModules = values.reduce((mask, value) => mask | value, 0)
    },
})

function fieldClass(field: LoggingConfigField) {
    return logging.isCliOverride(field) ? 'cli-override-field' : undefined
}

function overrideHint(field: LoggingConfigField): string {
    if (!logging.isCliOverride(field)) {
        return ''
    }
    const persisted = logging.persisted
    const cli = logging.snapshot.cli
    switch (field) {
        case LoggingConfigField.LogLevel:
            return `Command-line override active. Saved default: ${logLevelTitle(persisted.logLevel)}. CLI: ${logLevelTitle(cli.logLevel)}.`
        case LoggingConfigField.TraceLevel:
            return `Command-line override active. Saved default: ${logLevelTitle(persisted.traceLevel)}. CLI: ${logLevelTitle(cli.traceLevel)}.`
        case LoggingConfigField.VerboseModules:
            return `Command-line override active. Saved default mask: ${persisted.verboseModules}. CLI mask: ${cli.verboseModules}.`
        case LoggingConfigField.Folder: {
            const saved = persisted.folder || 'Default logs folder'
            const cliFolder = cli.folder || 'Default logs folder'
            return `Command-line override active. Saved default: ${saved}. CLI: ${cliFolder}.`
        }
        case LoggingConfigField.ExceptionTrace:
            return `Command-line override active. Saved default: ${persisted.exceptionTrace ? 'on' : 'off'}. CLI: ${cli.exceptionTrace ? 'on' : 'off'}.`
    }
    return ''
}

function canSaveAsDefault(field: LoggingConfigField) {
    if (!logging.isCliOverride(field)) {
        return false
    }
    const draft = diagnostic.value
    const cli = logging.snapshot.cli
    switch (field) {
        case LoggingConfigField.LogLevel:
            return draft.logLevel === cli.logLevel
        case LoggingConfigField.TraceLevel:
            return draft.traceLevel === cli.traceLevel
        case LoggingConfigField.VerboseModules:
            return draft.verboseModules === cli.verboseModules
        case LoggingConfigField.Folder:
            return draft.folder === cli.folder
        case LoggingConfigField.ExceptionTrace:
            return draft.exceptionTrace === cli.exceptionTrace
    }
}

async function applyDiagnosticLogging() {
    if (!logging.hydrated) {
        return
    }
    const draft = diagnostic.value
    const effective = logging.effective
    const patch: Parameters<typeof logging.patch>[0] = {}
    if (draft.logLevel !== effective.logLevel) {
        patch.logLevel = draft.logLevel
    }
    if (draft.traceLevel !== effective.traceLevel) {
        patch.traceLevel = draft.traceLevel
    }
    if (draft.verboseModules !== effective.verboseModules) {
        patch.verboseModules = draft.verboseModules
    }
    if (draft.folder !== effective.folder) {
        patch.folder = draft.folder
    }
    if (draft.exceptionTrace !== effective.exceptionTrace) {
        patch.exceptionTrace = draft.exceptionTrace
    }
    if (Object.keys(patch).length === 0) {
        return
    }
    try {
        await logging.patch(patch)
    }
    catch {
        // applyError is set by the store
    }
}

function scheduleDiagnosticLoggingApply() {
    if (!logging.hydrated) {
        return
    }
    if (applyTimer !== null) {
        clearTimeout(applyTimer)
        applyTimer = null
    }
    applyTimer = setTimeout(() => {
        applyTimer = null
        void applyDiagnosticLogging()
    }, 400)
}

async function resetDiagnosticLogging() {
    try {
        await logging.restoreDefaults()
    }
    catch {
        // applyError is set by the store
    }
}

async function saveFieldDefault(field: LoggingConfigField) {
    try {
        await logging.saveFieldAsDefault(field)
    }
    catch {
        // applyError is set by the store
    }
}

async function browseLogFolder() {
    try {
        const { path } = await Api.browseLogFolder(diagnostic.value.folder)
        if (path.length > 0) {
            diagnostic.value.folder = path
        }
    }
    catch (error) {
        logging.applyError = error instanceof Error ? error.message : String(error)
    }
}

async function exploreLogFolder() {
    if (logging.activeLogPaths === null) {
        return
    }
    try {
        await Api.exploreLogFolder(logging.activeLogPaths.clientLogPath)
    }
    catch (error) {
        logging.applyError = error instanceof Error ? error.message : String(error)
    }
}

async function flushDiagnosticLogs() {
    try {
        await logging.flush()
    }
    catch {
        // applyError is set by the store
    }
}

watch(diagnostic, scheduleDiagnosticLoggingApply, { deep: true })
onBeforeUnmount(() => {
    if (applyTimer !== null) {
        clearTimeout(applyTimer)
    }
})

const etlCaptureDisabled = true;
const etlCaptureDisabledMessage = 'ETL capture is currently disabled.';
async function handleEtlCapture() {
    prefs.notifyEtlLoggingDisabled()
}
async function handleEtlExplore() {
    await Api.exploreEtls()
}
function getEtlToggleButtonName() {
    return etlCaptureDisabled ? 'ETL Disabled' : prefs.etlLogging ? 'Finish ETL' : 'Start ETL';
}
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      Logging Configuration
    </h2>

    <v-card class="page-card">
        <v-card-title class="section-title">Diagnostic Logging</v-card-title>
        <v-card-subtitle>
            Configure PresentMon client and middleware diagnostic logs.
        </v-card-subtitle>

        <v-alert v-if="logging.applyError" type="error" variant="tonal" class="mt-4">
            Failed to apply logging configuration: {{ logging.applyError }}
        </v-alert>
        <v-alert v-else-if="logging.applying" type="info" variant="tonal" class="mt-4">
            Applying logging configuration...
        </v-alert>
        <v-alert
            v-else-if="logging.activeLogPaths"
            type="success"
            variant="tonal"
            class="mt-4 log-path-alert"
            role="button"
            tabindex="0"
            @click="exploreLogFolder"
            @keydown.enter.prevent="exploreLogFolder"
            @keydown.space.prevent="exploreLogFolder"
        >
            <div>Client: {{ logging.activeLogPaths.clientLogPath }}</div>
            <div v-if="logging.activeLogPaths.middlewareLogPath === logging.activeLogPaths.clientLogPath">
                Client and middleware are both logging to this file.
            </div>
            <div v-else>Middleware: {{ logging.activeLogPaths.middlewareLogPath }}</div>
            <div class="text-caption mt-1">Click to open this folder in Explorer.</div>
        </v-alert>

        <v-row class="mt-4">
            <v-col cols="4">
                Log Level
                <p class="text-medium-emphasis text-caption mb-0">
                    Include messages up to the selected verbosity.
                </p>
            </v-col>
            <v-col cols="8">
                <v-select
                    v-model="diagnostic.logLevel"
                    :items="logLevelOptions"
                    hide-details
                    :class="fieldClass(LoggingConfigField.LogLevel)"
                ></v-select>
                <div
                    v-if="overrideHint(LoggingConfigField.LogLevel)"
                    class="override-hint"
                    role="note"
                >
                    <v-chip size="x-small" color="amber-darken-2" class="mr-2" variant="flat">CLI</v-chip>
                    {{ overrideHint(LoggingConfigField.LogLevel) }}
                </div>
                <v-btn
                    v-if="canSaveAsDefault(LoggingConfigField.LogLevel)"
                    class="mt-1"
                    size="small"
                    variant="text"
                    @click="saveFieldDefault(LoggingConfigField.LogLevel)"
                >
                    Save current value as default
                </v-btn>
            </v-col>
        </v-row>

        <v-row class="mt-2">
            <v-col cols="4">
                Verbose Modules
                <p class="text-medium-emphasis text-caption mb-0">
                    Enable detailed output for selected subsystems when using a verbose log level.
                </p>
            </v-col>
            <v-col cols="8">
                <v-select
                    v-model="selectedVerboseModules"
                    :items="verboseModuleOptions"
                    multiple
                    chips
                    closable-chips
                    clearable
                    hide-details
                    placeholder="None"
                    :class="fieldClass(LoggingConfigField.VerboseModules)"
                ></v-select>
                <div
                    v-if="overrideHint(LoggingConfigField.VerboseModules)"
                    class="override-hint"
                    role="note"
                >
                    <v-chip size="x-small" color="amber-darken-2" class="mr-2" variant="flat">CLI</v-chip>
                    {{ overrideHint(LoggingConfigField.VerboseModules) }}
                </div>
                <v-btn
                    v-if="canSaveAsDefault(LoggingConfigField.VerboseModules)"
                    class="mt-1"
                    size="small"
                    variant="text"
                    @click="saveFieldDefault(LoggingConfigField.VerboseModules)"
                >
                    Save current value as default
                </v-btn>
            </v-col>
        </v-row>

        <v-row class="mt-2">
            <v-col cols="4">
                Log Folder
                <p class="text-medium-emphasis text-caption mb-0">
                    Leave blank to use the standard PresentMon logs folder. Files are generated
                    separately as pmlog-client-&lt;timestamp&gt;.txt and pmlog-middleware-&lt;timestamp&gt;.txt.
                </p>
            </v-col>
            <v-col cols="8">
                <div class="folder-picker">
                    <v-text-field
                        :model-value="diagnostic.folder || 'Default logs folder'"
                        hide-details
                        readonly
                        class="folder-field"
                        :class="fieldClass(LoggingConfigField.Folder)"
                        @click="browseLogFolder"
                    ></v-text-field>
                    <v-btn
                        v-if="diagnostic.folder"
                        variant="text"
                        @click="diagnostic.folder = ''"
                    >
                        Use Default
                    </v-btn>
                </div>
                <div
                    v-if="overrideHint(LoggingConfigField.Folder)"
                    class="override-hint"
                    role="note"
                >
                    <v-chip size="x-small" color="amber-darken-2" class="mr-2" variant="flat">CLI</v-chip>
                    {{ overrideHint(LoggingConfigField.Folder) }}
                </div>
                <v-btn
                    v-if="canSaveAsDefault(LoggingConfigField.Folder)"
                    class="mt-1"
                    size="small"
                    variant="text"
                    @click="saveFieldDefault(LoggingConfigField.Folder)"
                >
                    Save current value as default
                </v-btn>
            </v-col>
        </v-row>

        <v-row class="mt-2">
            <v-col cols="4">
                Stack Traces
                <p class="text-medium-emphasis text-caption mb-0">
                    Add traces to messages at the selected severity and optionally capture traces when exceptions are thrown.
                </p>
            </v-col>
            <v-col cols="8">
                <v-select
                    v-model="diagnostic.traceLevel"
                    :items="logLevelOptions"
                    label="Message trace level"
                    hide-details
                    :class="fieldClass(LoggingConfigField.TraceLevel)"
                ></v-select>
                <div
                    v-if="overrideHint(LoggingConfigField.TraceLevel)"
                    class="override-hint"
                    role="note"
                >
                    <v-chip size="x-small" color="amber-darken-2" class="mr-2" variant="flat">CLI</v-chip>
                    {{ overrideHint(LoggingConfigField.TraceLevel) }}
                </div>
                <v-btn
                    v-if="canSaveAsDefault(LoggingConfigField.TraceLevel)"
                    class="mt-1"
                    size="small"
                    variant="text"
                    @click="saveFieldDefault(LoggingConfigField.TraceLevel)"
                >
                    Save current value as default
                </v-btn>
                <v-switch
                    v-model="diagnostic.exceptionTrace"
                    label="Trace exceptions at throw site"
                    hide-details
                    class="mt-2"
                    :class="fieldClass(LoggingConfigField.ExceptionTrace)"
                ></v-switch>
                <div
                    v-if="overrideHint(LoggingConfigField.ExceptionTrace)"
                    class="override-hint"
                    role="note"
                >
                    <v-chip size="x-small" color="amber-darken-2" class="mr-2" variant="flat">CLI</v-chip>
                    {{ overrideHint(LoggingConfigField.ExceptionTrace) }}
                </div>
                <v-btn
                    v-if="canSaveAsDefault(LoggingConfigField.ExceptionTrace)"
                    class="mt-1"
                    size="small"
                    variant="text"
                    @click="saveFieldDefault(LoggingConfigField.ExceptionTrace)"
                >
                    Save current value as default
                </v-btn>
            </v-col>
        </v-row>

        <v-card-actions class="px-0 mt-3">
            <v-btn variant="text" @click="resetDiagnosticLogging">
                Restore Defaults
            </v-btn>
            <v-btn
                variant="outlined"
                :loading="logging.flushing"
                :disabled="logging.applying"
                @click="flushDiagnosticLogs"
            >
                Flush Logs
            </v-btn>
        </v-card-actions>
    </v-card>

    <v-card class="page-card">
        <v-card-title class="section-title">ETL Trace</v-card-title>
        <v-card-subtitle>
            Capture raw Event Tracing for Windows data for diagnostics.
        </v-card-subtitle>

        <v-alert type="info" variant="tonal" class="mt-5">
          {{ etlCaptureDisabledMessage }}
        </v-alert>

        <v-row class="mt-5">
            <v-col cols="3">
                ETL Capture Hotkey
                <p class="text-medium-emphasis text-caption mb-0">
                    Hotkey for starting/finishing an ETL trace
                </p>
            </v-col>
            <v-col cols="9">
                <v-row>
                    <v-col cols="6">
                        <hotkey-button :action="Action.ToggleEtlLogging" :disabled="etlCaptureDisabled"></hotkey-button>
                    </v-col>
                </v-row>
            </v-col>
        </v-row>

        <v-row class="mt-5">
            <v-col cols="3">
                Capture ETL
                <p class="text-medium-emphasis text-caption mb-0">
                    Raw ETL capture is currently disabled.
                </p>
            </v-col>
            <v-col cols="9">
                <v-row>
                    <v-col cols="6">
                        <v-btn :disabled="etlCaptureDisabled" @click="handleEtlCapture">{{getEtlToggleButtonName()}}</v-btn>
                    </v-col>
                </v-row>
            </v-col>
        </v-row>

        <v-row class="mt-5">
            <v-col cols="3">
                ETL Folder
                <p class="text-medium-emphasis text-caption mb-0">
                    Navigate to the folder that receives the captured .etl trace files
                </p>
            </v-col>
            <v-col cols="9">
                <v-row>
                    <v-col cols="6">
                        <v-btn @click="handleEtlExplore">Open in Explorer</v-btn>
                    </v-col>
                </v-row>
            </v-col>
        </v-row>

    </v-card>
  </div>
</template>

<style scoped>
.top-label {
  margin: 0;
  padding: 0;
  height: auto;
}
.header-top {
  color: white;
  user-select: none;
}
.page-card {
  margin: 15px 0;
  padding: 10px 15px 15px;
}
.page-wrap {
  max-width: 900px;
  flex-grow: 1;
}
.section-title {
  padding-left: 0;
}
.folder-picker {
  display: flex;
  align-items: center;
  gap: 8px;
}
.folder-field {
  flex: 1 1 auto;
  cursor: pointer;
}
.folder-field :deep(.v-field),
.folder-field :deep(input) {
  cursor: pointer;
}
.log-path-alert {
  cursor: pointer;
}
.cli-override-field :deep(.v-field),
.cli-override-field :deep(.v-switch__track) {
  outline: 2px solid #ffb300;
  outline-offset: 1px;
}
.override-hint {
  margin-top: 6px;
  color: rgba(255, 255, 255, 0.7);
  font-size: 0.75rem;
  line-height: 1.35;
}
</style>
