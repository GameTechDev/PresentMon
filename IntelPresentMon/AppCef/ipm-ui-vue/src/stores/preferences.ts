// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { ref, computed } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import { type Preferences as PreferencesType, type PreferenceFile, makeDefaultPreferences, Preset } from '@/core/preferences';
import { combinationsAreSame } from '@/core/hotkey';
import { signature } from '@/core/preferences';
import { useHotkeyStore } from './hotkey';
import { debounce, type DelayedTask, type DelayToken, dispatchDelayedTask } from '@/core/timing';
import { migratePreferences } from '@/core/preferences-migration';
import type { Widget } from '@/core/widget';
import { useLoadoutStore } from './loadout';
import { useIntrospectionStore } from './introspection';
import { deepToRaw } from '@/core/vue-utils';
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
  }

  async function parseAndReplaceRawPreferenceString(payload: { payload: string }) {
    const config = JSON.parse(payload.payload) as PreferenceFile;
    if (config.signature.code !== signature.code) throw new Error('Bad file format');
    if (config.signature.version !== signature.version) {
      migratePreferences(config);
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
  
  async function pushSpecification() {
    // we need to get a non-proxy object for the  call
    const widgets = deepToRaw(loadout.widgets);
    for (const widget of widgets) {
      // Filter out the widgetMetrics that do not meet the condition, modify those that do
      widget.metrics = widget.metrics.filter(widgetMetric => {
        const metric = intro.metrics.find(m => m.id === widgetMetric.metric.metricId);
        if (metric === undefined || metric.availableDeviceIds.length === 0) {
          // If the metric is undefined, this widgetMetric will be dropped
          return false;
        }
        // If the metric is found, set up the deviceId and desiredUnitId as needed
        widgetMetric.metric.deviceId = 0; // establish universal device id
        // Check whether metric is a gpu metric, then we need non-universal device id
        if (!metric.availableDeviceIds.includes(0)) {
          // if no specific adapter id set, assume adapter id = 1 is active
          const adapterId = preferences.value.adapterId !== null ? preferences.value.adapterId : 1;
          // Set adapter id for this query element to the active one if available
          if (metric.availableDeviceIds.includes(adapterId)) {
            widgetMetric.metric.deviceId = adapterId;
          } else { // if active adapter id is not available drop this widgetMetric
            return false;
          }
        }
        // Fill out the unit
        widgetMetric.metric.desiredUnitId = metric.preferredUnitId;
        // Since the metric is defined, keep this widgetMetric by returning true
        return true;
      });
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
      console.warn('Preferences reset due to load failure: ' + e)
      notes.notify({ text: `Preferences reset due to load failure: ${e}` })
    }
  }

  // === Exports ===
  return {
    preferences,
    capturing,
    pid,
    serialize,
    writeCapture,
    toggleCapture,
    pushSpecification,
    initPreferences,
    resetPreferences
  };
});