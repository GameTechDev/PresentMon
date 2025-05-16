<script setup lang="ts">
import { ref, computed } from 'vue';
import { type Widget, WidgetType } from '@/core/widget';
import type { Metric } from '@/core/metric';
import type { MetricOption } from '@/core/metric-option';
import type { QualifiedMetric } from '@/core/qualified-metric';
import type { Stat } from '@/core/stat';
import { AxisAffinity } from '@/core/widget-metric';
import ColorPicker from './ColorPicker.vue';
import { asGraph } from '@/core/widget';
import { useLoadoutStore } from '@/stores/loadout';
import type { ListItem } from 'vuetify/lib/composables/list-items.mjs';

defineOptions({name: 'LoadoutLine'})
interface Props {
  widgetIdx: number,
  lineIdx: number,
  widgets: Widget[],
  metrics: Metric[],
  stats: Stat[],
  metricOptions: MetricOption[],
  locked?: boolean,
  adapterId: number|null,
}
const props = withDefaults(defineProps<Props>(), {
  locked: false,
})
const emit = defineEmits<{
  (e: 'add'): void,
  (e: 'delete', val: number): void,
  (e: 'clearMulti', val: number): void,
}>()

const loadoutStore = useLoadoutStore();

const widget = computed(() => props.widgets[props.widgetIdx]);
const widgetMetric = computed(() => widget.value.metrics[props.lineIdx]);

// TODO: use fewer get/set constructions here and bind directly where possible
const widgetType = computed({
  get: () => widget.value.widgetType,
  set: (type: WidgetType) => {
    if (type !== widget.value.widgetType) {
      loadoutStore.resetWidgetAs(props.widgetIdx, type);
    }
  },
});
const widgetSubtype = computed({
  get: () => (widgetType.value === WidgetType.Graph ? asGraph(widget.value).graphType.name : ''),
  set: (val: string) => {
    if (widgetType.value === WidgetType.Graph) {
      asGraph(widget.value).graphType.name = val
    }
  },
});

const axisAffinityRight = computed({
  get: () => widgetMetric.value.axisAffinity === AxisAffinity.Right,
  set: (affinityRight: boolean) => {
    const axisAffinity = affinityRight ? AxisAffinity.Right : AxisAffinity.Left;
    const metric = { ...widgetMetric.value, axisAffinity };
    loadoutStore.setWidgetMetric(props.widgetIdx, props.lineIdx, metric);
  },
});

const metricOption = computed({
  get: () => {
    const option = props.metricOptions.find(
      (o) =>
        widgetMetric.value.metric.metricId === o.metricId &&
        widgetMetric.value.metric.arrayIndex === o.arrayIndex
    );
    if (!option) throw new Error('Option not found matching metricId:arrayIndex');
    return option;
  },
  set: (opt: MetricOption) => {
    const currentStatId = widgetMetric.value.metric.statId;
    const newAvailableStats = statsForMetric(opt.metricId);
    let statId = currentStatId;
    if (!newAvailableStats.some((s) => s.id === currentStatId)) {
      statId = newAvailableStats[0].id;
    }
    const newMetric = findMetricById(opt.metricId);
    if (!newMetric.availableDeviceIds.length) {
      throw new Error('Metric in selected metric option is not available for any device');
    }
    if (!newMetric.numeric) {
      widgetType.value = WidgetType.Readout;
    }
    const qualifiedMetric: QualifiedMetric = {
      metricId: opt.metricId,
      arrayIndex: opt.arrayIndex,
      deviceId: 0,
      statId,
      desiredUnitId: newMetric.preferredUnitId,
    };
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    loadoutStore.setWidgetMetric(props.widgetIdx, props.lineIdx, metric);
  },
});

const stat = computed({
  get: () => {
    const stat = props.stats.find((s) => s.id === widgetMetric.value.metric.statId);
    if (!stat) throw new Error(`Stat ID ${widgetMetric.value.metric.statId} not found`);
    return stat;
  },
  set: (stat: Stat) => {    
    const qualifiedMetric = { ...widgetMetric.value.metric, statId: stat.id };
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    loadoutStore.setWidgetMetric(props.widgetIdx, props.lineIdx, metric);
  },
});

const statsForMetric = (metricId: number) => {
  return props.stats.filter((s) => findMetricById(metricId).availableStatIds.includes(s.id));
};

const findMetricById = (metricId: number) => {
  const metric = props.metrics.find((m) => m.id === metricId);
  if (!metric) throw new Error(`Metric ID ${metricId} not found`);
  return metric;
};

const widgetTypeToString = (t: WidgetType) => WidgetType[t];

const metricOptionsFiltered = computed(() => {
  return props.lineIdx === 0
    ? props.metricOptions
    : props.metricOptions.filter((o) => findMetricById(o.metricId).numeric);
});

const widgetTypeOptions = computed(() => {
  const opts = [WidgetType.Readout];
  if (findMetricById(widgetMetric.value.metric.metricId).numeric) {
    opts.push(WidgetType.Graph);
  }
  return opts;
});

const widgetSubtypeOptions = computed(() => {
  return widgetType.value === WidgetType.Graph ? ['Line', 'Histogram'] : [];
});

const statOptions = computed(() => statsForMetric(widgetMetric.value.metric.metricId));

const isMaster = computed(() => props.lineIdx === 0);
const isGraphWidget = computed(() => widgetType.value === WidgetType.Graph);
const isLineGraphWidget = computed(() => isGraphWidget.value && asGraph(widget.value).graphType.name === 'Line');
const isReadoutWidget = computed(() => widgetType.value === WidgetType.Readout);
</script>

<template>
  <div class="widget-line">
    <div class="widget-cell col-metric">
      <v-autocomplete
        v-model="metricOption"
        item-title="name"
        :items="metricOptionsFiltered"
        :disabled="locked"
        return-object
        :density="isMaster ? 'default' : 'compact'"
      >
        <template v-slot:item="{item, props: itemProps}: {item:ListItem<MetricOption>, props:any}">
          <v-tooltip :text="findMetricById(item.raw.metricId).description">
            <template v-slot:activator="{props: tooltipProps}">
              <v-list-item v-bind="{...itemProps, ...tooltipProps}" :title="item.raw.name"/>
            </template>
          </v-tooltip>
        </template>
    </v-autocomplete>
    </div>
    <div class="widget-cell col-stat"> 
      <v-select
        v-model="stat"
        item-title="shortName"
        :items="statOptions"
        :disabled="locked || statOptions.length < 2"
        return-object
        :density="isMaster ? 'default' : 'compact'"
      >
        <template v-slot:item="{item, props: itemProps}: {item:ListItem<Stat>, props:any}">
          <v-tooltip :text="`${item.raw.name}: ${item.raw.description}`">
            <template v-slot:activator="{props: tooltipProps}">
              <v-list-item v-bind="{...itemProps, ...tooltipProps}" :title="item.raw.shortName"/>
            </template>
          </v-tooltip>
        </template>
      </v-select>
    </div>
    <div class="widget-cell col-type">
      <v-select
        v-if="isMaster"
        v-model="widgetType"
        :items="widgetTypeOptions"
        :disabled="locked || widgetTypeOptions.length < 2"
        :density="isMaster ? 'default' : 'compact'"
      >
        <template v-slot:selection="{item, index}: {item:ListItem<WidgetType>, index:number}">
          {{ widgetTypeToString(item.raw) }}
        </template>
        <template v-slot:item="{item, props, index}: {item:ListItem<WidgetType>, props:any, index:number}">
          <v-list-item v-bind="props" :title="widgetTypeToString(item.raw)">
          </v-list-item>
        </template>
      </v-select>
    </div>
    <div class="widget-cell col-subtype">
      <v-select
        v-if="isMaster"
        v-model="widgetSubtype"
        :items="widgetSubtypeOptions"
        :disabled="locked || widgetSubtypeOptions.length < 2"
        :density="isMaster ? 'default' : 'compact'"
      ></v-select>
      <v-switch v-else v-model="axisAffinityRight" label="Right Axis" hide-details density="compact" class="mt-0" color="primary"></v-switch>
    </div>
    <div class="widget-cell col-line-color">
      <color-picker
        v-if="isGraphWidget"
        v-model="widgetMetric.lineColor"
        :minimal="true"
        class="mb-1"
      ></color-picker>
      <color-picker
        v-if="isGraphWidget"
        v-model="widgetMetric.fillColor"
        :minimal="true"
      ></color-picker>
    </div>
    <div class="widget-cell col-controls text-right">
      <v-btn icon v-if="isMaster && !locked && isGraphWidget"
        :to="{name: 'graph-config', params: {index: widgetIdx}}"
        class="widget-btn details-btn"
        variant="text"
        size="large"
        color="white">
        <v-icon size="x-large">mdi-cog</v-icon>
      </v-btn>
      <v-btn icon v-if="!locked && isReadoutWidget"
        :to="{name: 'readout-config', params: {index: widgetIdx}}"
        class="widget-btn details-btn"
        variant="text"
        color="white">
        <v-icon size="x-large">mdi-cog</v-icon>
      </v-btn>
      <v-btn icon v-if="isMaster && !locked && isLineGraphWidget"
        @click="emit('add')"
        class="widget-btn add-line-btn"
        variant="text"
        color="white">
        <v-icon size="x-large">mdi-plus</v-icon>
      </v-btn>
      <v-btn icon v-if="isMaster && !locked"
        @click="emit('delete', lineIdx)"
        class="widget-btn remove-btn"
        variant="text"
        color="white">
        <v-icon size="x-large">mdi-close</v-icon>
      </v-btn>
      <v-btn icon v-if="!isMaster && !locked"
        @click="emit('delete', lineIdx)"
        class="widget-line-btn line-btn mr-1"
        variant="text"
        size="small"
        color="white">
        <v-icon>mdi-close</v-icon>
      </v-btn>
    </div>
  </div>
</template>

<style scoped lang="scss">
</style>