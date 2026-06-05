<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { Api } from '@/core/api';
import { signature as preferencesSignature } from '@/core/preferences';
import { signature as loadoutSignature } from '@/core/loadout';
import { type AppInfo } from '@/core/app-info';

interface InfoRow {
  label: string;
  value: string;
}

defineOptions({ name: 'AboutConfigView' });

const appInfo = ref<AppInfo|null>(null);
const errorMessage = ref('');

function boolText(value: boolean): string {
  return value ? 'Yes' : 'No';
}

const applicationRows = computed<InfoRow[]>(() => {
  if (appInfo.value === null) {
    return [];
  }
  return [
    { label: 'Product', value: appInfo.value.productName },
    { label: 'Product Version', value: appInfo.value.productVersion },
    { label: 'API Version', value: appInfo.value.apiVersion },
    { label: 'Middleware API Version', value: appInfo.value.middlewareApiVersion },
    { label: 'Preferences Format', value: preferencesSignature.version },
    { label: 'Loadout Format', value: loadoutSignature.version },
    { label: 'UI Dev Mode', value: boolText(appInfo.value.devModeEnabled) },
    { label: 'Chromium Debugging', value: boolText(appInfo.value.chromiumDebugEnabled) },
    { label: 'Debug Blocklist', value: boolText(appInfo.value.debugBlocklistEnabled) },
    { label: 'Log Level', value: appInfo.value.logLevel },
    { label: 'Verbose Modules', value: appInfo.value.verboseModules },
  ];
});

const buildRows = computed<InfoRow[]>(() => {
  if (appInfo.value === null) {
    return [];
  }
  return [
    { label: 'Git Hash', value: appInfo.value.buildHash },
    { label: 'Short Hash', value: appInfo.value.buildHashShort },
    { label: 'Build Date/Time', value: appInfo.value.buildDateTime },
    { label: 'Build Config', value: appInfo.value.buildConfig },
    { label: 'Dirty Build', value: boolText(appInfo.value.buildDirty) },
  ];
});

const serviceRows = computed<InfoRow[]>(() => {
  if (appInfo.value === null) {
    return [];
  }
  return [
    { label: 'Service Build ID', value: appInfo.value.serviceBuildId },
    { label: 'Service Build Time', value: appInfo.value.serviceBuildTime },
    { label: 'Service Version', value: appInfo.value.serviceVersion },
  ];
});

const runtimeRows = computed<InfoRow[]>(() => {
  if (appInfo.value === null) {
    return [];
  }
  return [
    { label: 'CEF Version', value: appInfo.value.cefVersion },
    { label: 'MSVC Version', value: appInfo.value.msvcVersion },
    { label: 'Windows SDK', value: appInfo.value.winSdkVersion },
    { label: 'CRT Version', value: appInfo.value.crtVersion },
    { label: 'CRT Runtime', value: appInfo.value.crtRuntime },
  ];
});

onMounted(async () => {
  try {
    appInfo.value = await Api.getAppInfo();
  }
  catch (e) {
    errorMessage.value = e instanceof Error ? e.message : String(e);
  }
});
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      About
    </h2>

    <v-card class="page-card">
      <v-progress-linear v-if="appInfo === null && errorMessage === ''" indeterminate color="primary" class="mt-4"></v-progress-linear>

      <v-alert v-if="errorMessage !== ''" type="error" variant="tonal" class="mt-5">
        {{ errorMessage }}
      </v-alert>

      <template v-if="appInfo !== null">
        <v-card-title class="section-title">Application</v-card-title>
        <v-table density="compact" class="info-table">
          <tbody>
            <tr v-for="row in applicationRows" :key="row.label">
              <td class="info-label">{{ row.label }}</td>
              <td class="info-value">{{ row.value }}</td>
            </tr>
          </tbody>
        </v-table>

        <v-card-title class="section-title">Build</v-card-title>
        <v-table density="compact" class="info-table">
          <tbody>
            <tr v-for="row in buildRows" :key="row.label">
              <td class="info-label">{{ row.label }}</td>
              <td class="info-value">{{ row.value }}</td>
            </tr>
          </tbody>
        </v-table>

        <v-card-title class="section-title">Service</v-card-title>
        <v-table density="compact" class="info-table">
          <tbody>
            <tr v-for="row in serviceRows" :key="row.label">
              <td class="info-label">{{ row.label }}</td>
              <td class="info-value">{{ row.value }}</td>
            </tr>
          </tbody>
        </v-table>

        <v-card-title class="section-title">Runtime</v-card-title>
        <v-table density="compact" class="info-table">
          <tbody>
            <tr v-for="row in runtimeRows" :key="row.label">
              <td class="info-label">{{ row.label }}</td>
              <td class="info-value">{{ row.value }}</td>
            </tr>
          </tbody>
        </v-table>
      </template>
    </v-card>
  </div>
</template>

<style scoped>
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
.section-title {
  color: rgba(255, 255, 255, 0.7);
  font-size: 16px;
  padding: 18px 0 6px;
}
.info-table {
  background: transparent;
}
.info-label {
  width: 210px;
  color: rgba(255, 255, 255, 0.7);
  white-space: nowrap;
}
.info-value {
  overflow-wrap: anywhere;
  user-select: text;
}
</style>
