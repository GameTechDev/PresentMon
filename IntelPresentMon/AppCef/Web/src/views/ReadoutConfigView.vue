<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="page-wrap">
    
  <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
      <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
      Detailed Readout Configuration
  </h2>

  <v-card class="page-card my-7">
    <v-subheader class="mt-0">Style Settings</v-subheader>
    <v-divider class="ma-0"></v-divider>
    
    <v-row class="mt-8">       
      <v-col cols="3">
        Font Size
        <p class="text--secondary text-sm-caption mb-0">Size of text in this readout widget</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="fontSize"
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
        <p class="text--secondary text-sm-caption mb-0">Colors of various elements of the widget</p>
      </v-col>
      <v-col cols="9">
        <v-row dense>
          <v-col cols="6">
            <color-picker v-model="fontColor" class="color-picker" label="Text"></color-picker>
          </v-col>
          <v-col cols="6">
            <color-picker v-model="backgroundColor" class="color-picker" label="Background"></color-picker>
          </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-3">       
      <v-col cols="3">
        Show Label
        <p class="text--secondary text-sm-caption mb-0">Show label for this readout widget</p>
      </v-col>
      <v-col cols="9">
        <v-switch v-model="showLabel" hide-details label="Show"></v-switch>
      </v-col>
    </v-row>
  </v-card>

  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import ColorPicker from '@/components/ColorPicker.vue'
import { RgbaColor } from '@/core/color'
import { Readout } from '@/core/readout'
import { Widget, AsReadout } from '@/core/widget'
import { Loadout } from '@/store/loadout'

export default Vue.extend({
  name: 'WidgetConfig',
  components: {
    ColorPicker,
  },

  props: {
    index: {required: true, type: Number},
  },
  beforeMount() {
  },
  data: () => ({
  }),
  methods: {
  },  
  computed: {
    widget(): Widget {
      return Loadout.widgets[this.index];
    },
    readout(): Readout {
      return AsReadout(this.widget);
    },

    // v-model enablers
    showLabel: {
      get(): boolean { return this.readout.showLabel; },
      set(val: boolean) {
        Loadout.setReadoutAttribute({ index: this.index, attr: 'showLabel', val });
      },
    },
    fontSize: {
      get(): number { return this.readout.fontSize; },
      set(fontSize: number) {
        Loadout.setReadoutAttribute({ index: this.index, attr: 'fontSize', val: fontSize });
      },
    },
    fontColor: {
      get(): RgbaColor { return this.readout.fontColor; },
      set(fontColor: RgbaColor) {
        Loadout.setReadoutAttribute({ index: this.index, attr: 'fontColor', val: fontColor });
      },
    },
    backgroundColor: {
      get(): RgbaColor { return this.readout.backgroundColor; },
      set(backgroundColor: RgbaColor) {
        Loadout.setReadoutAttribute({ index: this.index, attr: 'backgroundColor', val: backgroundColor });
      },
    },
  },
});
</script>

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
  color: #2196f3;
}
.page-card {
  margin: 15px 0;
  padding: 0 15px 15px;
}
.page-wrap {
  width: 750px;
}
</style>

