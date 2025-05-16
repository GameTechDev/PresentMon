<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue';
import ColorPicker from '@/components/ColorPicker.vue';
import { asGraph, WidgetType as WidgetTypeEnum } from '@/core/widget';
import { useLoadoutStore } from '@/stores/loadout';
import { usePreferencesStore } from '@/stores/preferences';

defineOptions({ name: 'GraphConfigView' });

interface Props {
  index: number;
}
const props = defineProps<Props>();

// === Stores ===
const loadoutStore = useLoadoutStore();
const prefs = usePreferencesStore();

// === Functions ===
function parseNumber(val:string|number): number {
  if (typeof val === 'number') return val;
  const parsed = parseFloat(val);
  return isNaN(parsed) ? 0 : parsed;
}

// === Computed ===
const widget = computed(() => loadoutStore.widgets[props.index]);
const graph = computed(() => asGraph(widget.value));
const graphType = computed(() => graph.value.graphType);
const typeName = computed(() => graph.value.graphType.name);
const totalCount = computed(() => prefs.preferences.timeRange * prefs.preferences.metricPollRate);
// left range text helpers
const rangeLeft = computed(() => graph.value.graphType.range);
const typeRangeTextMin = computed({
  get: () => graph.value.graphType.range[0],
  set: (rangeLeftMin:string|number) => { graphType.value.range[0] = parseNumber(rangeLeftMin) }
});
const typeRangeTextMax = computed({
  get: () => graph.value.graphType.range[1],
  set: (rangeLeftMax:string|number) => { graphType.value.range[1] = parseNumber(rangeLeftMax); }
});
// right range text helpers
const rangeRight = computed(() => graph.value.graphType.rangeRight);
const typeRangeRightTextMin = computed({
  get: () => graph.value.graphType.rangeRight[0],
  set: (rangeRightMin:string|number) => { graphType.value.rangeRight[0] = parseNumber(rangeRightMin); }
});
const typeRangeRightTextMax = computed({
  get: () => graph.value.graphType.rangeRight[1],
  set: (rangeRightMax:string|number) => { graphType.value.rangeRight[1] = parseNumber(rangeRightMax); }
});
// count range text helpers
const typeCountRange = computed(() => graph.value.graphType.countRange);
const typeCountRangeTextMin = computed({
  get: () => graph.value.graphType.countRange[0],
  set: (countRangeMin:string|number) => { graphType.value.countRange[0] = parseNumber(countRangeMin); }
});
const typeCountRangeTextMax = computed({
  get: () => graph.value.graphType.countRange[1],
  set: (countRangeMax:string|number) => { graphType.value.countRange[1] = parseNumber(countRangeMax); }
});
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
      <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
      Detailed Graph Configuration
    </h2>

    <v-card class="page-card my-7">
      <v-card-title class="mt-0 text-medium-emphasis">Graph Settings</v-card-title>
      <v-divider class="ma-0"></v-divider>

      <v-row class="mt-8">
        <v-col v-if="typeName !== 'Line'" cols="3">
          Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values for the bins this histogram</p>
        </v-col>
        <v-col v-else cols="3">
          Left Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values to be displayed by the left axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="graphType.range"
              :min="0"
              :max="5000"
              :disabled="graphType.autoLeft"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRangeTextMin"
                :disabled="graphType.autoLeft"
                type="number"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRangeTextMax"
                :disabled="graphType.autoLeft"
                type="number"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-3">
        <v-col v-if="typeName !== 'Line'" cols="3">
          Autoscale Range
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of values for the bins this histogram</p>
        </v-col>
        <v-col v-else cols="3">
          Autoscale Left
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of the left y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="graphType.autoLeft" hide-details></v-switch>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-8">
        <v-col cols="3">
          Right Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values to be displayed by the right axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="graphType.rangeRight"
              :min="0"
              :max="5000"
              :disabled="graphType.autoRight"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRangeRightTextMin"
                :disabled="graphType.autoRight"
                type="number"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRangeRightTextMax"
                :disabled="graphType.autoRight"
                type="number"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-3">
        <v-col cols="3">
          Autoscale Right
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of the right y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="graphType.autoRight" hide-details></v-switch>
        </v-col>
      </v-row>

      <div v-show="typeName === 'Histogram'">
        <v-row class="mt-8">
          <v-col cols="3">
            Number of Bins
            <p class="text-medium-emphasis text-caption mb-0">Number of bins (bars) in histogram</p>
          </v-col>
          <v-col cols="9">
            <v-slider
              v-model="graphType.binCount"
              :min="5"
              :max="200"
              thumb-label="always"
                hide-details
            ></v-slider>
          </v-col>
        </v-row>

        <v-row class="mt-8">
          <v-col cols="3">
            Count Range
            <p class="text-medium-emphasis text-caption mb-0">Range of bin counts displayed by this histogram</p>
          </v-col>
          <v-col cols="9">
            <v-row dense>
              <v-range-slider
                v-model="typeCountRange"
                :min="0"
                :max="5000"
                :disabled="graphType.autoCount"
                hide-details
              ></v-range-slider>
            </v-row>
            <v-row no-gutters class="mt-2">
              <v-col cols="2">
                <v-text-field
                  v-model="typeCountRangeTextMin"
                  :disabled="graphType.autoCount"
                  type="number"
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
              <v-col cols="2" offset="8">
                <v-text-field
                  v-model="typeCountRangeTextMax"
                  :disabled="graphType.autoCount"
                  type="number"
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
            </v-row>
          </v-col>
        </v-row>

        <v-row v-if="typeName !== 'Line'" class="mt-3">
          <v-col cols="3">
            Autoscale Count
            <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of bin counts</p>
          </v-col>
          <v-col cols="9">
            <v-switch v-model="graphType.autoCount" hide-details></v-switch>
          </v-col>
        </v-row>

        <v-row v-if="typeName !== 'Line'" class="mt-3">
          <v-col cols="3">
            Total Count
            <p class="text-medium-emphasis text-caption mb-0">Total expected data points being counted into bins</p>
          </v-col>
          <v-col cols="9">
            <p class="text-medium-emphasis text-caption">
              The total count of data points displayed is controlled by <span style="color: orange;">Time Scale</span>
              (<router-link class="app-link" :to="{name: 'overlay-config'}">Settings>Overlay</router-link>) multiplied by <span style="color: orange;">Metric Poll Rate</span>
              (<router-link class="app-link" :to="{name: 'data-config'}">Settings>Data Processing</router-link>). <br> Currently it is
              <span style="color: green;">{{ prefs.preferences.timeRange }}s</span> * <span style="color: green;">
                {{ prefs.preferences.metricPollRate }}Hz</span> = <span style="color: violet;">{{ totalCount }}</span> data points.
            </p>
          </v-col>
        </v-row>
      </div>
    </v-card>

    <v-card class="page-card my-7">
      <v-card-title class="mt-0 text-medium-emphasis">Style Settings</v-card-title>
      <v-divider class="ma-0"></v-divider>

      <v-row class="mt-5">
        <v-col cols="3">
          Graph Height
          <p class="text-medium-emphasis text-caption mb-0">Vertical size of the graph</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="graph.height"
            :min="20"
            :max="450"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Grid Vertical
          <p class="text-medium-emphasis text-caption mb-0">Number of vertical divisions in the grid</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="graph.vDivs"
            :min="1"
            :max="40"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Grid Horizontal
          <p class="text-medium-emphasis text-caption mb-0">Number of horizontal divisions in the grid</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="graph.hDivs"
            :min="1"
            :max="100"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Font Size
          <p class="text-medium-emphasis text-caption mb-0">Size of text in this readout widget</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="graph.textSize"
            :min="5"
            :max="80"
            :step="0.5"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Colors
          <p class="text-medium-emphasis text-caption mb-0">Colors of various elements of the graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-col cols="4">
              <color-picker v-model="graph.gridColor" class="color-picker" label="Grid"></color-picker>
            </v-col>
            <v-col cols="4">
              <color-picker v-model="graph.backgroundColor" class="color-picker" label="Background"></color-picker>
            </v-col>
            <v-col cols="4">
              <color-picker v-model="graph.textColor" class="color-picker" label="Text"></color-picker>
            </v-col>
          </v-row>
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
  max-width: 750px;
}
a.app-link {
  color: rgb(var(--v-theme-primary));
}
</style>