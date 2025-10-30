<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { watch, nextTick, ref } from 'vue';
import { usePreferencesStore } from '@/stores/preferences';
import ColorPicker from '@/components/ColorPicker.vue';
import { VTextField } from 'vuetify/components';
const prefs = usePreferencesStore();

// ref on field
const overrideTextField = ref<InstanceType<typeof VTextField>|null>(null);
// Vuetify rule: only validate (and flag red) when enabled
const overrideRules = [
  (v: string) => !prefs.preferences.flashInjectionEnableTargetOverride || !!v || 'Required'
];
// whenever we enable override, force validation
watch(
  () => prefs.preferences.flashInjectionEnableTargetOverride,
  (enabled) => {
    if (enabled) {
      nextTick(() => overrideTextField.value?.validate());
    }
  }
);
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      Flash Injection <span style="color:red">*Experimental*</span>
    </h2>

    <v-card class="page-card">
      <v-row class="mt-8">
        <v-col cols="3">
          Enable Flash Injection
          <p class="text-medium-emphasis text-caption mb-0">Enable the injector child process.</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="prefs.preferences.enableFlashInjection" label="Enable"></v-switch>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Enable Target Override
          <p class="text-medium-emphasis text-caption mb-0">Use a custom name instead of last-targeted process name.</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="prefs.preferences.flashInjectionEnableTargetOverride" label="Enable"></v-switch>
        </v-col>
      </v-row>

      <v-row class="mt-8" v-if="prefs.preferences.flashInjectionEnableTargetOverride">
        <v-col cols="3">
          Target Name Override
          <p class="text-medium-emphasis text-caption mb-0">Process module name to use for injection.</p>
        </v-col>
        <v-col cols="9">
          <v-text-field
            ref="overrideTextField"
            v-model="prefs.preferences.flashInjectionTargetOverride"
            label="Module Name"
            :rules="overrideRules"
            required
          />
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Enable Background
          <p class="text-medium-emphasis text-caption mb-0">Renders a static rectangle every frame. When a click occurs the "flash" rectangle is rendered on top of the background.</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="prefs.preferences.flashInjectionBackgroundEnable" label="Enable"></v-switch>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Colors
          <p class="text-medium-emphasis text-caption mb-0">Colors of various elements of the graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-col cols="6">
              <color-picker v-model="prefs.preferences.flashInjectionColor" class="color-picker" label="Flash"></color-picker>
            </v-col>
            <v-col cols="6">
              <color-picker v-model="prefs.preferences.flashInjectionBackgroundColor" class="color-picker" label="Background"></color-picker>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Rainbow Strip
          <p class="text-medium-emphasis text-caption mb-0">Draw a strip on the left-hand side of the screen that cycles through colors every frame.</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="prefs.preferences.flashInjectionUseRainbow" label="Enable"></v-switch>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Flash Width
          <p class="text-medium-emphasis text-caption mb-0">Width of the flash rectangle</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.flashInjectionSize"
            :min="0.01"
            :max="1"
            :step="0.01"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Background Width
          <p class="text-medium-emphasis text-caption mb-0">Width of the background rectangle (if active)</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.flashInjectionBackgroundSize"
            :min="0.01"
            :max="1"
            :step="0.01"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Flash Offset
          <p class="text-medium-emphasis text-caption mb-0">How far to offset the flash rectangle from the left of the screen</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.flashInjectionRightShift"
            :min="0"
            :max="1"
            :step="0.01"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Flash Duration
          <p class="text-medium-emphasis text-caption mb-0">Duration of the flash in seconds.</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.flashInjectionFlashDuration"
            :min="0"
            :max="1"
            :step="0.001"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-4">
        <v-col>
          <h3>Notes</h3>
          <p class="text-medium-emphasis text-caption mb-1">The flash injector injects code into a target process, causing it to draw a rectangle on top of its normal output whenever it detects a mouse click.</p>
          <p class="text-medium-emphasis text-caption mb-1">It can be used, in conjunction with specialized hardware instruments, to measure latency between a click input and a corresponding visual response of the target app.</p>
          <p class="text-medium-emphasis text-caption mb-1">To perform injection, target an app with the overlay and then restart the app. Any changes to the above settings will similarly only take effect after the target app has been restarted.</p>
          <p class="text-orange text-caption mb-1">WARNING: using this feature with certain titles (in particular, competitive online games protected by anticheat services) may result in a permanent ban.</p>
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
.color-picker {
  max-width: 150px;
}
.header-top {
  color: white;
  user-select: none;
}
.page-card {
  margin: 15px 0;
  padding: 0 15px 15px;
}
.page-wrap {
  max-width: 750px;
  flex-grow: 1;
}
</style>
