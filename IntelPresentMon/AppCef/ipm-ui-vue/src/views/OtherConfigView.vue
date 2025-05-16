<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { ref } from 'vue';
import { usePreferencesStore } from '@/stores/preferences';
import { useHotkeyStore } from '@/stores/hotkey';

const dialog = ref(false);
const preferencesStore = usePreferencesStore();
const hotkeyStore = useHotkeyStore();

async function reset() {
  preferencesStore.resetPreferences();
  await hotkeyStore.bindDefaults();
  dialog.value = false;
};
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 header-top">
      Other Configuration
    </h2>

    <v-card class="page-card">
      <v-row class="mt-2">
        <v-col cols="3">
          Reset Preferences
          <p class="text-medium-emphasis text-caption mb-0">Reset all preferences to their defaults</p>
        </v-col>
        <v-col cols="9">
          <v-row>
            <v-col cols="6">
              <v-btn class="mt-2" color="primary" @click="dialog = true">Reset</v-btn>

              <v-dialog v-model="dialog" max-width="500px">
                <v-card>
                  <v-card-title class="headline">Reset Preferences</v-card-title>

                  <v-card-text>
                    Preferences will be set to their default values. All current settings will be lost. Continue?
                  </v-card-text>

                  <v-card-actions>
                    <v-spacer></v-spacer>

                    <v-btn color="primary" @click="reset">Reset</v-btn>

                    <v-btn color="grey darken-1" text @click="dialog = false">Cancel</v-btn>
                  </v-card-actions>
                </v-card>
              </v-dialog>
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
