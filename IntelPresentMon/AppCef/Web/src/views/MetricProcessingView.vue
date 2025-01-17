<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="page-wrap">
    
  <h2 class="mt-5 ml-5 link-head">
      Data Processing Configuration
  </h2>

  <v-card class="page-card">

    <v-row class="mt-5" v-if="isDevelopment">
      <v-col cols="3">
        ETW Manual Flush
        <p class="text--secondary text-sm-caption mb-0">Control whether manual ETW flushing is performed or the default 1000ms timer is used (may require service restart).</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="6">
                <v-switch v-model="manualEtwFlush" label="Enable"></v-switch>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-5" v-if="isDevelopment">
      <v-col cols="3">
        ETW Manual Flush Period
        <p class="text--secondary text-sm-caption mb-0">Rate of manual flushing of the ETW event buffers. Offset should roughly match this.</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="etwFlushPeriod"
          :max="1000"
          :min="1"
          :disabled="!manualEtwFlush"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-5">
      <v-col cols="3">
        Polling Rate
        <p class="text--secondary text-sm-caption mb-0">Rate at which to poll API for metric data (Hz). Controls temporal resolution of graphs and readouts.</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          class="metric-poll-rate"
          v-model="metricPollRate"
          :max="240"
          :min="1"
          :messages="metricPollMessages"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-5" v-if="isDevelopment">
      <v-col cols="3">
        Metric Window offset
        <p class="text--secondary text-sm-caption mb-0">Time in ms to offset the sliding window by to ensure it doesn't slide into the time region of frames not yet received.</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="offset"
          :max="1500"
          :min="0"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-5">
      <v-col cols="3">
        Telemetry Period
        <p class="text--secondary text-sm-caption mb-0">Time between service-side power telemetry polling calls (ms). Indirectly affects temporal resolution of a subset of metrics, such as GPU power and temperature.</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="telemetrySamplingPeriod"
          :max="500"
          :min="1"
          thumb-label="always"
          hide-details
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">
      <v-col cols="3">
        Window Size
        <p class="text--secondary text-sm-caption mb-0">Size of sample window used for calculating statistics such as average or 99% (ms)</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="window"
          :max="5000"
          :min="10"
          :step="10"
          thumb-label="always"
          hide-details
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">
      <v-col cols="3">
        Adapter
        <p class="text--secondary text-sm-caption mb-0">Adapter used to source GPU telemetry data such as power</p>
      </v-col>
      <v-col cols="9">
        <v-select
          v-model="adapterId"
          :items="adapters"
          item-value="id"
          item-text="name"
          placeholder="Default adapter"
          outlined
          dense
          hide-details
        ></v-select>
      </v-col>
    </v-row>
  
  </v-card>

  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import { Preferences } from '@/store/preferences'
import { Adapter } from '@/core/adapter'
import { Adapters } from '@/store/adapters'
import { Api } from '@/core/api'
import { IsDevelopment } from '@/core/env-vars'


export default Vue.extend({
  name: 'MetricProcessing',

  data: () => ({
  }),
  methods: {
  },  
  computed: {
    isDevelopment(): boolean {
      return IsDevelopment();
    },
    // v-model enablers
    metricPollRate: {
      get(): number { return Preferences.preferences.metricPollRate; },
      set(rate: number) {
        Preferences.writeAttribute({ attr: 'metricPollRate', val: rate });
      },
    },
    overlayDrawRate(): number {
      return Preferences.preferences.overlayDrawRate;
    },
    metricPollMessages(): string[] {
      if (this.metricPollRate % this.overlayDrawRate !== 0) {
        return [`Recommend setting poll rate to be a whole multiple of the overlay draw rate (currently ${this.overlayDrawRate}fps).`];
      }
      return [];
    },
    offset: {
      get(): number { return Preferences.preferences.metricsOffset; },
      set(metricsOffset: number) {
        Preferences.writeAttribute({ attr: 'metricsOffset', val: metricsOffset });
      },
    },
    window: {
      get(): number { return Preferences.preferences.metricsWindow; },
      set(metricsWindow: number) {
        Preferences.writeAttribute({ attr: 'metricsWindow', val: metricsWindow });
      },
    },
    etwFlushPeriod: {
      get(): number { return Preferences.preferences.etwFlushPeriod; },
      set(etwFlushPeriod: number) {
        Preferences.writeAttribute({ attr: 'etwFlushPeriod', val: etwFlushPeriod });
      },
    },
    manualEtwFlush: {
      get(): boolean { return Preferences.preferences.manualEtwFlush; },
      set(manualEtwFlush: boolean) {
        Preferences.writeAttribute({ attr: 'manualEtwFlush', val: manualEtwFlush });
      },
    },
    adapters(): Adapter[] {
      return Adapters.adapters;
    },
    telemetrySamplingPeriod: {
      get(): number { return Preferences.preferences.telemetrySamplingPeriodMs; },
      set(period: number) {
        Preferences.writeAttribute({ attr: 'telemetrySamplingPeriodMs', val: period });
      },
    },
    adapterId: {
      get(): number|null { return Preferences.preferences.adapterId; },
      set(id: number) {
        Preferences.writeAdapterId(id);
      },
    },
  }
});
</script>

<style scoped>
.top-label {
    margin: 0;
    padding: 0;
    height: auto;
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
.metric-poll-rate >>> .v-messages__message {
  color: blueviolet;
  padding-left: 10px;
}
</style>
