<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { usePreferencesStore } from '@/stores/preferences';
import { Api } from '@/core/api';
import { Action } from '@/core/hotkey';
import HotkeyButton from '@/components/HotkeyButton.vue';

const prefs = usePreferencesStore();
async function handleEtlCapture() {
    prefs.toggleEtlLogging()
}
async function handleEtlExplore() {
    await Api.exploreEtls()
}
function getEtlToggleButtonName() {
    return prefs.etlLogging ? 'Finish ETL' : 'Start ETL';
}
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      Logging Configuration
    </h2>

    <v-card class="page-card">

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
              <hotkey-button :action="Action.ToggleEtlLogging"></hotkey-button>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-5">
        <v-col cols="3">
          Capture ETL
          <p class="text-medium-emphasis text-caption mb-0">
            Capture ETW events to a raw event stream trace for offline analysis and reporting
          </p>
        </v-col>
        <v-col cols="9">
          <v-row>
            <v-col cols="6">
              <v-btn @click="handleEtlCapture">{{ getEtlToggleButtonName() }}</v-btn>
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
  padding: 0 15px 15px;
}
.page-wrap {
  max-width: 750px;
  flex-grow: 1;
}
</style>
