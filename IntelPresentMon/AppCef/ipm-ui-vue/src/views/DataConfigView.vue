<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { computed } from 'vue';
import { usePreferencesStore } from '@/stores/preferences';
import { isDevelopment } from '@/core/env-vars';
import { useAdaptersStore } from '@/stores/adapters';

const prefs = usePreferencesStore();
const adaptersStore = useAdaptersStore();

const metricPollMessages = computed(() => {
  if (prefs.preferences.metricPollRate % prefs.preferences.overlayDrawRate !== 0) {
    return [`Recommend setting poll rate to be a whole multiple of the overlay draw rate (currently ${prefs.preferences.overlayDrawRate}fps).`];
  }
  return [];
});

</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      Data Processing Configuration
    </h2>

    <v-card class="page-card">
      <v-row class="mt-5" v-if="isDevelopment()">
        <v-col cols="3">
          ETW Manual Flush
          <p class="text-medium-emphasis text-caption mb-0">
            Control whether manual ETW flushing is performed or the default 1000ms timer is used (may require service restart).
          </p>
        </v-col>
        <v-col cols="9">
          <v-row>
            <v-col cols="6">
              <v-switch v-model="prefs.preferences.manualEtwFlush" label="Enable" color="primary"></v-switch>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-5" v-if="isDevelopment()">
        <v-col cols="3">
          ETW Manual Flush Period
          <p class="text-medium-emphasis text-caption mb-0">
            Rate of manual flushing of the ETW event buffers. Offset should roughly match this.
          </p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.etwFlushPeriod"
            :max="1000"
            :min="1"
            :disabled="!prefs.preferences.manualEtwFlush"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-5">
        <v-col cols="3">
          Polling Rate
          <p class="text-medium-emphasis text-caption mb-0">
            Rate at which to poll API for metric data (Hz). Controls temporal resolution of graphs and readouts.
          </p>
        </v-col>
        <v-col cols="9">
          <v-slider
            class="metric-poll-rate"
            v-model="prefs.preferences.metricPollRate"
            :max="240"
            :min="1"
            :messages="metricPollMessages"
            thumb-label="always"
            :hide-details="false"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-5" v-if="isDevelopment()">
        <v-col cols="3">
          Metric Window offset
          <p class="text-medium-emphasis text-caption mb-0">
            Time in ms to offset the sliding window by to ensure it doesn't slide into the time region of frames not yet received.
          </p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.metricsOffset"
            :max="1500"
            :min="0"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-5">
        <v-col cols="3">
          Telemetry Period
          <p class="text-medium-emphasis text-caption mb-0">
            Time between service-side power telemetry polling calls (ms). Indirectly affects temporal resolution of a subset of metrics, such as GPU power and temperature.
          </p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.telemetrySamplingPeriodMs"
            :max="500"
            :min="1"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Window Size
          <p class="text-medium-emphasis text-caption mb-0">
            Size of sample window used for calculating statistics such as average or 99% (ms)
          </p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="prefs.preferences.metricsWindow"
            :max="5000"
            :min="10"
            :step="10"
            thumb-label="always"
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Adapter
          <p class="text-medium-emphasis text-caption mb-0">
            Adapter used to source GPU telemetry data such as power
          </p>
        </v-col>
        <v-col cols="9">
          <v-select
            v-model="prefs.preferences.adapterId"
            :items="adaptersStore.adapters"
            item-value="id"
            item-title="name"
            placeholder="Default adapter"
          ></v-select>
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
