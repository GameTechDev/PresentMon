<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="page-wrap">
    
  <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
      <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
      Detailed Graph Configuration
  </h2>    

  <v-card class="page-card my-7" v-if="widgetTypeName === 'Graph'">
    <v-subheader class="mt-0">Graph Settings</v-subheader>
    <v-divider class="ma-0"></v-divider>

      <v-row class="mt-8">       
        <v-col v-if="typeName !== 'Line'" cols="3">
          Value Range
          <p class="text--secondary text-sm-caption mb-0">Range of metric values for the bins this histogram</p>
        </v-col>
        <v-col v-else cols="3">
          Left Value Range
          <p class="text--secondary text-sm-caption mb-0">Range of metric values to be displayed by the left axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="typeRange"
              :min="0"
              :max="5000"
              :step="0.1"
              :disabled="autoLeft"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRange[0]"
                :disabled="autoLeft"
                type="number"
                hide-details
                outlined
                dense
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRange[1]"
                :disabled="autoLeft"
                type="number"
                hide-details
                outlined
                dense
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-3">       
        <v-col v-if="typeName !== 'Line'" cols="3">
          Autoscale Range
          <p class="text--secondary text-sm-caption mb-0">Automatically adjust the range of values for the bins this histogram</p>
        </v-col>       
        <v-col v-else cols="3">
          Autoscale Left
          <p class="text--secondary text-sm-caption mb-0">Automatically adjust the range of the left y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="autoLeft" hide-details></v-switch>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-8">
        <v-col cols="3">
          Right Value Range
          <p class="text--secondary text-sm-caption mb-0">Range of metric values to be displayed by the right axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="typeRangeRight"
              :min="0"
              :max="5000"
              :step="0.1"
              :disabled="autoRight"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRangeRight[0]"
                :disabled="autoRight"
                type="number"
                hide-details
                outlined
                dense
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRangeRight[1]"
                :disabled="autoRight"
                type="number"
                hide-details
                outlined
                dense
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-3">       
        <v-col cols="3">
          Autoscale Right
          <p class="text--secondary text-sm-caption mb-0">Automatically adjust the range of the right y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="autoRight" hide-details></v-switch>
        </v-col>
      </v-row>

      <v-expand-transition><div v-show="typeName === 'Histogram'">

        <v-row class="mt-8">       
          <v-col cols="3">
            Number of Bins
            <p class="text--secondary text-sm-caption mb-0">Number of bins (bars) in histogram</p>
          </v-col>
          <v-col cols="9">
            <v-slider
              v-model="typeBinCount"
              :min="5"
              :max="200"
              thumb-label="always"
            ></v-slider>
          </v-col>
        </v-row>

        <v-row class="mt-8">       
          <v-col cols="3">
            Count Range
            <p class="text--secondary text-sm-caption mb-0">Range of bin counts displayed by this histogram</p>
          </v-col>
          <v-col cols="9">
            <v-row dense>
              <v-range-slider
                v-model="typeCountRange"
                :min="0"
                :max="5000"
                :step="0.1"
                :disabled="autoCount"
                hide-details
              ></v-range-slider>
            </v-row>
            <v-row no-gutters class="mt-2">
              <v-col cols="2">
                <v-text-field
                  v-model="typeCountRange[0]"
                  :disabled="autoCount"
                  type="number"
                  hide-details
                  outlined
                  dense
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
              <v-col cols="2" offset="8">
                <v-text-field
                  v-model="typeCountRange[1]"
                  :disabled="autoCount"
                  type="number"
                  hide-details
                  outlined
                  dense
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
            </v-row>
          </v-col>
        </v-row>

      <v-row v-if="typeName !== 'Line'" class="mt-3">       
        <v-col cols="3">
          Autoscale Count
          <p class="text--secondary text-sm-caption mb-0">Automatically adjust the range of bin counts</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="autoCount" hide-details></v-switch>
        </v-col>
      </v-row>
        
      </div></v-expand-transition>

  </v-card>

  <v-card class="page-card my-7" v-if="widgetTypeName === 'Graph'">
    <v-subheader class="mt-0">Style Settings</v-subheader>
    <v-divider class="ma-0"></v-divider>
    
    <v-row class="mt-5">       
      <v-col cols="3">
        Graph Height
        <p class="text--secondary text-sm-caption mb-0">Vertical size of the graph</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="height"
          :min="20"
          :max="450"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">       
      <v-col cols="3">
        Grid Vertical
        <p class="text--secondary text-sm-caption mb-0">Number of vertical divisions in the grid</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="vDivs"
          :min="1"
          :max="40"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>

    <v-row class="mt-8">       
      <v-col cols="3">
        Grid Horizontal
        <p class="text--secondary text-sm-caption mb-0">Number of horizontal divisions in the grid</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="hDivs"
          :min="1"
          :max="100"
          thumb-label="always"
        ></v-slider>
      </v-col>
    </v-row>
    
    <v-row class="mt-8">       
      <v-col cols="3">
        Font Size
        <p class="text--secondary text-sm-caption mb-0">Size of text in this readout widget</p>
      </v-col>
      <v-col cols="9">
        <v-slider
          v-model="textSize"
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
        <p class="text--secondary text-sm-caption mb-0">Colors of various elements of the graph</p>
      </v-col>
      <v-col cols="9">
        <v-row dense>
          <v-col cols="4">
            <color-picker v-model="gridColor" class="color-picker" label="Grid"></color-picker>
          </v-col>
          <v-col cols="4">
            <color-picker v-model="backgroundColor" class="color-picker" label="Background"></color-picker>
          </v-col>
          <v-col cols="4">
            <color-picker v-model="textColor" class="color-picker" label="Text"></color-picker>
          </v-col>
        </v-row>
      </v-col>
    </v-row>
  </v-card>

  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import ColorPicker from '@/components/ColorPicker.vue'
import { RgbaColor } from '@/core/color'
import { Graph } from '@/core/graph'
import { Widget, AsGraph, WidgetType } from '@/core/widget'
import { makeDefaultWidgetMetric } from '@/core/widget-metric'
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
    this.widgetTypeName = WidgetType[this.widget.widgetType];
  },
  data: () => ({
    widgetTypeName: '',
  }),
  methods: {
  },  
  computed: {
    widget(): Widget {
      return Loadout.widgets[this.index];
    },
    graph(): Graph {
      return AsGraph(this.widget);
    },

    // v-model enablers
    metricId: {
      get(): number { return this.widget.metrics[0].metricId; },
      set(metricId: number) {
        Loadout.setWidgetMetrics({index: this.index, metrics: [makeDefaultWidgetMetric(metricId)]});
      },
    },
    height: {
      get(): number { return this.graph.height; },
      set(height: number) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'height', val: height });
      },
    },
    vDivs: {
      get(): number { return this.graph.vDivs; },
      set(vDivs: number) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'vDivs', val: vDivs });
      },
    },
    hDivs: {
      get(): number { return this.graph.hDivs; },
      set(hDivs: number) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'hDivs', val: hDivs });
      },
    },
    showBottomAxis: {
      get(): boolean { return this.graph.showBottomAxis; },
      set(showBottomAxis: boolean) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'showBottomAxis', val: showBottomAxis });
      },
    },
    typeName: {
      get(): string { return this.graph.graphType.name; },
      set(name: string) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'name', val: name });
      },
    },
    typeRange: {
      get(): [number, number] { return this.graph.graphType.range; },
      set(range: [number, number]) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'range', val: range });
      },
    },
    typeRangeRight: {
      get(): [number, number] { return this.graph.graphType.rangeRight; },
      set(rangeRight: [number, number]) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'rangeRight', val: rangeRight });
      },
    },
    autoLeft: {
      get(): boolean { return this.graph.graphType.autoLeft; },
      set(val: boolean) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'autoLeft', val });
      },
    },
    autoRight: {
      get(): boolean { return this.graph.graphType.autoRight; },
      set(val: boolean) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'autoRight', val });
      },
    },
    autoCount: {
      get(): boolean { return this.graph.graphType.autoCount; },
      set(val: boolean) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'autoCount', val });
      },
    },
    typeBinCount: {
      get(): number { return this.graph.graphType.binCount; },
      set(binCount: number) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'binCount', val: binCount });
      },
    },
    typeCountRange: {
      get(): [number, number] { return this.graph.graphType.countRange; },
      set(countRange: [number, number]) {
        Loadout.setGraphTypeAttribute({ index: this.index, attr: 'countRange', val: countRange });
      },
    },
    gridColor: {
      get(): RgbaColor { return this.graph.gridColor; },
      set(gridColor: RgbaColor) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'gridColor', val: gridColor });
      },
    },
    dividerColor: {
      get(): RgbaColor { return this.graph.dividerColor; },
      set(dividerColor: RgbaColor) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'dividerColor', val: dividerColor });
      },
    },
    backgroundColor: {
      get(): RgbaColor { return this.graph.backgroundColor; },
      set(backgroundColor: RgbaColor) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'backgroundColor', val: backgroundColor });
      },
    },
    borderColor: {
      get(): RgbaColor { return this.graph.borderColor; },
      set(borderColor: RgbaColor) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'borderColor', val: borderColor });
      },
    },
    textColor: {
      get(): RgbaColor { return this.graph.textColor; },
      set(textColor: RgbaColor) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'textColor', val: textColor });
      },
    },
    textSize: {
      get(): number { return this.graph.textSize; },
      set(textSize: number) {
        Loadout.setGraphAttribute({ index: this.index, attr: 'textSize', val: textSize });
      },
    },
  },
  watch: {
    widgetTypeName(type: string, oldType: string) {
      if (oldType !== '') {
        Loadout.resetWidgetAs({index: this.index, type: WidgetType[type as keyof typeof WidgetType]});
      }
    }
  }
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
  max-width: 750px;
}
</style>

