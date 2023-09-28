<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="page-wrap">
    
  <h2 class="mt-5 ml-5 link-head">
      Overlay Configuration
  </h2>

  <v-card class="page-card">

    <v-row class="mt-5">
      <v-col cols="3">
        Windowed Mode
        <p class="text--secondary text-sm-caption mb-0">Display widgets on a standalone window instead of an overlay tracking the target</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="6">
                <v-switch v-model="independent" label="Enable"></v-switch>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-8">
      <v-col cols="3">
        Automatic Hide
        <p class="text--secondary text-sm-caption mb-0">Automatically disable the overlay during capture</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="6">
                <v-switch v-model="hideDuringCapture" label="Enable" :disabled="!visible"></v-switch>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-8">       
      <v-col cols="3">
        Position
        <p class="text--secondary text-sm-caption mb-0">Where the overlay appears on the target window</p>
      </v-col>
      <v-col cols="9">
        <overlay-positioner v-model="position">           
        </overlay-positioner>
      </v-col>
    </v-row>
  
    <v-row class="mt-8">
      <v-col cols="3">
        Width
        <p class="text--secondary text-sm-caption mb-0">Width of the overlay window (height determined by content)</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="12">
                <v-slider
                    v-model="width"
                    :max="1920"
                    :min="120"
                    thumb-label="always"
                    hide-details
                ></v-slider>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-8">
      <v-col cols="3">
        Time Scale
        <p class="text--secondary text-sm-caption mb-0">Range of time (s) displayed on graphs' x-axes. Controls the scrolling speed.</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="timeRange"
          :max="10"
          :min="0.1"
          :step="0.1"
          thumb-label="always"
          hide-details
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">
      <v-col cols="3">
        Graphics Scaling
        <p class="text--secondary text-sm-caption mb-0">Upscale overlay graphics to make text more readable on high DPI displays</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="4">
                <v-switch v-model="upscale" label="Enable"></v-switch>
            </v-col>
            <v-col cols="8">
                <v-slider
                    class="mt-4"
                    label="Factor"
                    v-model="upscaleFactor"
                    :max="5"
                    :min="1"
                    :step="0.1"
                    thumb-label="always"
                ></v-slider>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-8">       
      <v-col cols="3">
        Draw Rate
        <p class="text--secondary text-sm-caption mb-0">Closest valid rate will be targeted (depends on metric sample period)</p>
      </v-col>
      <v-col cols="9">
        <v-slider
            class="overlay-draw-rate"
            v-model="desiredDrawRate"
            :max="120"
            :min="1"
            :messages="[drawRateMessage]"
            thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">       
      <v-col cols="3">
        Background Color
        <p class="text--secondary text-sm-caption mb-0">Control background color of entire overlay</p>
      </v-col>
      <v-col cols="3">
          <color-picker v-model="backgroundColor" class="color-picker" label="Background"></color-picker>
      </v-col>
    </v-row>

  </v-card>

  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import { Preferences } from '@/store/preferences'
import { Processes } from '@/store/processes'
import { Process } from '@/core/process'
import { RgbaColor } from '@/core/color'
import OverlayPositioner from '@/components/OverlayPositioner.vue'
import ColorPicker from '@/components/ColorPicker.vue'

export default Vue.extend({
  name: 'OverlayConfig',

  components: {
    OverlayPositioner,
    ColorPicker,
  },
  data: () => ({
    processRefreshTimestamp: null as number|null,
  }),
  methods: {
    async refresh() {
      // @click gets triggered twice for some reason, suppress 2nd invocation
      const stamp = window.performance.now();
      const debounceThresholdMs = 150;
      if (this.processRefreshTimestamp === null || stamp - this.processRefreshTimestamp > debounceThresholdMs) {
        this.processRefreshTimestamp = stamp;
        await Processes.refresh();
      }
    }
  },  
  computed: {
    processes(): Process[] {
      return Processes.processes;
    },
    position: {
      get(): number { return Preferences.preferences.overlayPosition; },
      set(position: number) {
        Preferences.writeAttribute({ attr: 'overlayPosition', val: position });
      },
    },
    drawRateMessage(): string {
      const actual = 1000 / (Preferences.preferences.samplingPeriodMs * Preferences.preferences.samplesPerFrame);
      return `Actual target overlay FPS: ${actual.toFixed(1)}`;
    },

    // v-model enablers
    width: {
      get(): number { return Preferences.preferences.overlayWidth; },
      set(width: number) {
        Preferences.writeAttribute({ attr: 'overlayWidth', val: width });
      },
    },
    upscale: {
      get(): boolean { return Preferences.preferences.upscale; },
      set(upscale: boolean) {
        Preferences.writeAttribute({ attr: 'upscale', val: upscale });
      },
    },
    upscaleFactor: {
      get(): number { return Preferences.preferences.upscaleFactor; },
      set(upscaleFactor: number) {
        Preferences.writeAttribute({ attr: 'upscaleFactor', val: upscaleFactor });
      },
    },
    hideDuringCapture: {
      get(): boolean { return Preferences.preferences.hideDuringCapture; },
      set(hide: boolean) {
        Preferences.writeAttribute({ attr: 'hideDuringCapture', val: hide });
      },
    },
    visible: {
      get(): boolean { return !Preferences.preferences.hideAlways; },
      set(visible: boolean) {
        Preferences.writeAttribute({ attr: 'hideAlways', val: !visible });
      },
    },
    timeRange: {
      get(): number { return Preferences.preferences.timeRange; },
      set(timeRange: number) {
        Preferences.writeAttribute({ attr: 'timeRange', val: timeRange });
      },
    },
    independent: {
      get(): boolean { return Preferences.preferences.independentWindow; },
      set(independent: boolean) {
        Preferences.writeAttribute({ attr: 'independentWindow', val: independent });
      },
    },
    // this is used to calculate spec.samplesPerFrame in App.vue
    desiredDrawRate: {
      get(): number { return Preferences.desiredOverlayDrawRate; },
      set(drawRate: number) {
        Preferences.setDesiredOverlayDrawRate(drawRate);
      },
    },
    backgroundColor: {
      get(): RgbaColor { return Preferences.preferences.overlayBackgroundColor; },
      set(backgroundColor: RgbaColor) {
        Preferences.writeAttribute({attr: 'overlayBackgroundColor', val: backgroundColor});
      }
    }
  },
  watch: {
  }
});
</script>

<style scoped>
.top-label {
    margin: 0;
    padding: 0;
    height: auto;
}
.overlay-draw-rate >>> .v-messages__message {
  color: blueviolet;
  padding-left: 10px;
}
.link-head {
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
