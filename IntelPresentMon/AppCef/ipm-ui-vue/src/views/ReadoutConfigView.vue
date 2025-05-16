<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->
  
<script setup lang="ts">
import { computed } from 'vue';
import ColorPicker from '@/components/ColorPicker.vue';
import { asReadout, type Widget } from '@/core/widget';
import { useLoadoutStore } from '@/stores/loadout';

defineOptions({ name: 'WidgetConfig' });

interface Props {
  index: number;
}
const props = defineProps<Props>();

const loadoutStore = useLoadoutStore();

const widget = computed(() => loadoutStore.widgets[props.index]);
const readout = computed(() => asReadout(widget.value));
</script>

<template>
    <div class="page-wrap">
      <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
          <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
          Detailed Readout Configuration
      </h2>
    
      <v-card class="page-card my-7">
        <v-card-title class="mt-0 text-medium-emphasis">Style Settings</v-card-title>
        <v-divider class="ma-0"></v-divider>
        
        <v-row class="mt-8">       
          <v-col cols="3">
            Font Size
            <p class="text-medium-emphasis text-caption mb-0">Size of text in this readout widget</p>
          </v-col>
          <v-col cols="9">
            <v-slider
              v-model="readout.fontSize"
              :min="5"
              :max="80"
              :step="0.5"
              thumb-label="always"
            ></v-slider>
          </v-col>
        </v-row>
      
        <v-row class="mt-8">       
          <v-col cols="3">
            Colors
            <p class="text-medium-emphasis text-caption mb-0">Colors of various elements of the widget</p>
          </v-col>
          <v-col cols="9">
            <v-row dense>
              <v-col cols="6">
                <color-picker v-model="readout.fontColor" class="color-picker" label="Text"></color-picker>
              </v-col>
              <v-col cols="6">
                <color-picker v-model="readout.backgroundColor" class="color-picker" label="Background"></color-picker>
              </v-col>
            </v-row>
          </v-col>
        </v-row>
    
        <v-row class="mt-3">       
          <v-col cols="3">
            Show Label
            <p class="text-medium-emphasis text-caption mb-0">Show label for this readout widget</p>
          </v-col>
          <v-col cols="9">
            <v-switch v-model="readout.showLabel" hide-details label="Show"></v-switch>
          </v-col>
        </v-row>
      </v-card>
    
      </div>
    </template>
  
  <style scoped>
  .color-picker {
    max-width: 150px;
  }
  i.v-icon.v-icon {
    color: inherit;
  }
  .link-head {
    color: white;
    cursor: pointer;
    user-select: none;
    transition: color .2s;
  }
  .link-head:hover {
    color: rgb(var(--v-theme-primary));
  }
  .page-card {
    margin: 15px 0;
    padding: 0 15px 15px;
  }
  .page-wrap {
    width: 750px;
  }
  </style>

