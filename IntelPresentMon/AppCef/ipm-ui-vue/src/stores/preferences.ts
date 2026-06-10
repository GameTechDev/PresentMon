// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { ref } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import { lowestAdapterId } from '@/core/adapter';
import { type Preferences as PreferencesType, type PreferenceFile, makeDefaultPreferences, Preset } from '@/core/preferences';
import { combinationsAreSame } from '@/core/hotkey';
import { signature } from '@/core/preferences';
import { useHotkeyStore } from './hotkey';
import { debounce, type DelayedTask, dispatchDelayedTask } from '@/core/timing';
import { migratePreferences } from '@/core/preferences-migration';
import { useLoadoutStore } from './loadout';
import { useIntrospectionStore } from './introspection';
import { deepToRaw } from '@/core/vue-utils';
import { asGraph, asReadout, WidgetType } from '@/core/widget';
import {
    isGpuMetric,
    makeMetricQueryContext,
    prepareWidgetMetricForPush,
} from '@/core/metric-device';
import { useNotificationsStore } from './notifications';

export const usePreferencesStore = defineStore('preferences', () => {
  // === Dependent Stores ===
  const loadout = useLoadoutStore()
  const intro = useIntrospectionStore()
  const hotkeys = useHotkeyStore()
  const notes = useNotificationsStore()

  // === State ===
  const preferences = ref<PreferencesType>(makeDefaultPreferences())
  const capturing = ref(false)
  const etlLogging = ref(false)
  const pid = ref<number | null>(null)

  // === Nonreactive State ===
  let serializeDebounceTask: DelayedTask<void> | null = null
  let captureAutostopTask: DelayedTask<void> | null = null

  // === Functions ===
  function setAllPreferences(prefs: PreferencesType) {
    preferences.value = prefs;
    capturing.value = false;
    pid.value = null;
    serializeDebounceTask = null;
    captureAutostopTask = null;
  }

  function resetPreferences() {
    setAllPreferences(makeDefaultPreferences());
    preferences.value.selectedPreset = Preset.Slot1;
    preferences.value.adapterId = lowestAdapterId(intro.adapters);
  }

  async function parseAndReplaceRawPreferenceString(payload: { payload: string }) {
    const config = JSON.parse(payload.payload) as PreferenceFile;
    if (config.signature.code !== signature.code) throw new Error('Bad file format');
    let migrated = false;
    if (config.signature.version !== signature.version) {
      migratePreferences(config, { adapters: intro.adapters });
      migrated = true;
    }

    Object.assign(preferences.value, config.preferences);

    for (const key in config.hotkeyBindings) {
      const newBinding = config.hotkeyBindings[key];
      const oldBinding = hotkeys.bindings[key];
      if (newBinding.combination === null) {
        if (oldBinding.combination !== null) {
          await hotkeys.clearHotkey(newBinding.action);
        }
      } else if (oldBinding.combination === null || !combinationsAreSame(newBinding.combination, oldBinding.combination)) {
        await hotkeys.bindHotkey(newBinding);
      }
    }
    if (migrated) {
      serialize();
    }
  }

  function metricQueryContext() {
    return makeMetricQueryContext(
      intro.systemDeviceId,
      preferences.value.adapterId,
      preferences.value.enablePerMetricDeviceSelection,
    );
  }

  // === Actions ===
  function serialize() {
    debounce(() => {
      const file: PreferenceFile = {
        signature,
        preferences: preferences.value,
        hotkeyBindings: hotkeys.bindings,
      };
      Api.storePreferences(JSON.stringify(file, null, 3));
    }, 400, serializeDebounceTask);
  }

  function writeCapture(reqActive: boolean) {
    if (reqActive) {
      if (preferences.value.enableCaptureDuration) {
        captureAutostopTask = dispatchDelayedTask(
          () => writeCapture(false),
          preferences.value.captureDuration * 1000
        )       
      }
      Api.setCapture(true);
      capturing.value = true;
    } else {
      if (captureAutostopTask) {
        captureAutostopTask.token.cancel();
        captureAutostopTask = null;
      }
      Api.setCapture(false);
      capturing.value = false;
    }
  }

  function toggleCapture() {
    if (capturing.value) {
      writeCapture(false);
    } else {
      writeCapture(true);
    }
  }
  function notifyEtlLoggingDisabled() {
    etlLogging.value = false;
    notes.notify({ text: 'ETL capture is currently disabled.' });
  }

  function toggleEtlLogging() {
    notifyEtlLoggingDisabled
  }
  
  async function pushSpecification() {
    await intro.load();
    // we need to get a non-proxy object for the  call
    const widgets = deepToRaw(loadout.widgets);
    const queryCtx = metricQueryContext();
    const enablePerMetricDeviceSelection = preferences.value.enablePerMetricDeviceSelection;
    for (const widget of widgets) {
      widget.metrics = widget.metrics.filter(widgetMetric => {
        const metric = intro.metrics.find(m => m.id === widgetMetric.metric.metricId);
        if (metric === undefined) {
          return false;
        }
        const pushMetric = { ...widgetMetric.metric };
        if (!enablePerMetricDeviceSelection && isGpuMetric(metric)) {
          pushMetric.deviceId = null;
        }
        if (!prepareWidgetMetricForPush(metric, pushMetric, queryCtx)) {
          return false;
        }
        widgetMetric.metric = pushMetric;
        return true;
      });
      if (!enablePerMetricDeviceSelection) {
        if (widget.widgetType === WidgetType.Graph) {
          const graph = asGraph(widget);
          graph.labelIncludeDeviceId = false;
          graph.labelIncludeDeviceName = false;
        } else if (widget.widgetType === WidgetType.Readout) {
          const readout = asReadout(widget);
          readout.labelIncludeDeviceId = false;
          readout.labelIncludeDeviceName = false;
        }
      }
    }
    await Api.pushSpecification({
      pid: pid.value,
      preferences: preferences.value, // NOTE: if arrays are embedded in prefs in future, will need deepToRaw
      widgets: widgets.filter(w => w.metrics.length > 0),
    });
  }
  
  async function initPreferences() {
    try {
      const payload = await Api.loadPreferences();
      await parseAndReplaceRawPreferenceString(payload);
    }
    catch (e) {
      await hotkeys.bindDefaults();
      resetPreferences();
      preferences.value.selectedPreset = Preset.Slot1;
      serialize();
      console.info('Preferences reset due to load failure: ' + e);
    }
  }

  // === Exports ===
  return {
    preferences,
    capturing,
    etlLogging,
    pid,
    serialize,
    writeCapture,
    toggleCapture,
    toggleEtlLogging,
    notifyEtlLoggingDisabled,
    pushSpecification,
    initPreferences,
    resetPreferences
  };
});
