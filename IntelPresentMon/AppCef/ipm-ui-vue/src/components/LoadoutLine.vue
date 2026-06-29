<script setup lang="ts">
import { computed } from 'vue';
import { type Widget, WidgetType } from '@/core/widget';
import type { Metric } from '@/core/metric';
import type { QualifiedMetric } from '@/core/qualified-metric';
import type { Stat } from '@/core/stat';
import type { Adapter } from '@/core/adapter';
import { AxisAffinity } from '@/core/widget-metric';
import ColorPicker from './ColorPicker.vue';
import { asGraph } from '@/core/widget';
import { useLoadoutStore } from '@/stores/loadout';
import { useIntrospectionStore } from '@/stores/introspection';
import { usePreferencesStore } from '@/stores/preferences';
import {
  arraySizeForDevice,
  clampArrayIndex,
  effectiveGpuDeviceId,
  effectiveGpuDeviceIdForQualifiedMetric,
  isGpuMetric,
  applyRuntimeQualifiedMetricFields,
  type MetricQueryContext,
} from '@/core/metric-device';
import {
  isMetricAvailable,
  isMetricAvailableForGpuDeviceId,
  isMetricUnavailableInAutocomplete,
  metricAvailabilityReasonForGpuDeviceId,
  metricTooltipAvailabilityReason,
  qualifiedMetricForAvailabilityProbe,
} from '@/core/metric-availability';
import type { ListItem } from 'vuetify/lib/composables/list-items.mjs';

defineOptions({name: 'LoadoutLine'})
interface Props {
  widgetIdx: number,
  lineIdx: number,
  widgets: Widget[],
  metrics: Metric[],
  stats: Stat[],
  adapters: Adapter[],
  locked?: boolean,
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
const intro = useIntrospectionStore();
const prefs = usePreferencesStore();

const widget = computed(() => props.widgets[props.widgetIdx]);
const widgetMetric = computed(() => widget.value.metrics[props.lineIdx]);

const queryCtx = computed((): MetricQueryContext => ({
  systemDeviceId: intro.systemDeviceId,
  preferenceDefaultAdapterId: prefs.preferences.adapterId,
  enablePerMetricDeviceSelection: prefs.preferences.enablePerMetricDeviceSelection,
}));

const storedDeviceId = computed(() => widgetMetric.value.metric.deviceId);

const effectiveDeviceId = computed(() => {
  const m = currentMetric.value;
  if (!isGpuMetric(m)) {
    return storedDeviceId.value ?? 0;
  }
  return effectiveGpuDeviceIdForQualifiedMetric(m, widgetMetric.value.metric, queryCtx.value);
});

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

const currentMetric = computed(() => findMetricById(widgetMetric.value.metric.metricId));

const showDeviceSelector = computed(() =>
  prefs.preferences.enablePerMetricDeviceSelection &&
  isGpuMetric(currentMetric.value) &&
  props.adapters.length > 0
);

const showArrayIndexSelector = computed(() => {
  const size = arraySizeForDevice(currentMetric.value, effectiveDeviceId.value);
  return size > 1;
});

const metricAvailabilityReasons = computed(() => intro.metricAvailabilityReasons);

type DeviceSelectItem = {
  id: number | null,
  title: string,
  unavailable: boolean,
  availabilityReason: string | null,
};

function buildDeviceSelectItem(id: number | null, title: string): DeviceSelectItem {
  const m = currentMetric.value;
  const qm = widgetMetric.value.metric;
  const unavailable = !isMetricAvailableForGpuDeviceId(m, qm, queryCtx.value, id);
  const reason = metricAvailabilityReasonForGpuDeviceId(
    m,
    qm,
    queryCtx.value,
    id,
    metricAvailabilityReasons.value,
  );
  return {
    id,
    title,
    unavailable,
    availabilityReason: reason,
  };
}

const deviceSelectItems = computed((): DeviceSelectItem[] => [
  buildDeviceSelectItem(null, 'Default'),
  ...props.adapters.map((a) =>
    buildDeviceSelectItem(a.id, adapterSelectTitle(a)),
  ),
]);

function deviceSelectItemForId(deviceId: number | null): DeviceSelectItem | undefined {
  return deviceSelectItems.value.find((item) => item.id === deviceId);
}

function adapterSelectTitle(adapter: Adapter): string {
  return `[${adapter.id}] ${adapter.name}`;
}

function deviceSelectionTitle(deviceId: number | null): string {
  if (deviceId === null || deviceId === 0) {
    return 'Default';
  }
  const adapter = props.adapters.find((a) => a.id === deviceId);
  if (adapter !== undefined) {
    return adapterSelectTitle(adapter);
  }
  return `[${deviceId}] Unknown GPU`;
}

const arrayIndexOptions = computed(() => {
  const size = arraySizeForDevice(currentMetric.value, effectiveDeviceId.value);
  return Array.from({ length: size }, (_, i) => i);
});

const selectedMetric = computed({
  get: () => currentMetric.value,
  set: (newMetric: Metric) => {
    const currentStatId = widgetMetric.value.metric.statId;
    const newAvailableStats = statsForMetric(newMetric.id);
    let statId = currentStatId;
    if (!newAvailableStats.some((s) => s.id === currentStatId)) {
      statId = newAvailableStats[0]?.id ?? currentStatId;
    }
    if (!newMetric.numeric) {
      widgetType.value = WidgetType.Readout;
    }
    const qualifiedMetric: QualifiedMetric = {
      metricId: newMetric.id,
      arrayIndex: widgetMetric.value.metric.arrayIndex,
      deviceId: widgetMetric.value.metric.deviceId,
      statId,
    };
    applyRuntimeQualifiedMetricFields(newMetric, qualifiedMetric, queryCtx.value);
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    loadoutStore.setWidgetMetric(props.widgetIdx, props.lineIdx, metric);
  },
});

const selectedDeviceId = computed({
  get: () => storedDeviceId.value,
  set: (deviceId: number | null) => {
    const m = currentMetric.value;
    const effectiveId = isGpuMetric(m)
      ? effectiveGpuDeviceId(deviceId, queryCtx.value.preferenceDefaultAdapterId)
      : (deviceId ?? 0);
    const qualifiedMetric = {
      ...widgetMetric.value.metric,
      deviceId,
      arrayIndex: clampArrayIndex(m, effectiveId, widgetMetric.value.metric.arrayIndex),
    };
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    loadoutStore.setWidgetMetric(props.widgetIdx, props.lineIdx, metric);
  },
});

const arrayIndex = computed({
  get: () => widgetMetric.value.metric.arrayIndex,
  set: (index: number) => {
    const m = currentMetric.value;
    const qualifiedMetric = {
      ...widgetMetric.value.metric,
      arrayIndex: clampArrayIndex(m, effectiveDeviceId.value, index),
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
const metricFromItem = (item: ListItem<unknown>) => item.raw as Metric;
const deviceSelectItemFromListItem = (item: ListItem<unknown>) => item.raw as DeviceSelectItem;
const statFromItem = (item: ListItem<unknown>) => item.raw as Stat;
const widgetTypeFromItem = (item: ListItem<unknown>) => item.raw as WidgetType;

const metricsFiltered = computed(() => {
  return props.lineIdx === 0
    ? props.metrics
    : props.metrics.filter((m) => m.numeric);
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

const showMetricAvailabilityOnMetricField = computed(
  () => !showDeviceSelector.value,
);

const currentMetricUnavailable = computed(() =>
  !isMetricAvailable(currentMetric.value, widgetMetric.value.metric, queryCtx.value),
);

const metricFieldGreyedOut = computed(() =>
  isMetricUnavailableInAutocomplete(
    currentMetric.value,
    widgetMetric.value.metric,
    queryCtx.value,
    props.adapters,
  ),
);

const currentDeviceSelectionUnavailable = computed(() => {
  const item = deviceSelectItemForId(storedDeviceId.value ?? null);
  return item?.unavailable ?? false;
});

const selectedDeviceSelectItem = computed(() =>
  deviceSelectItemForId(storedDeviceId.value ?? null),
);

const isMetricUnavailableInList = (metric: Metric) => {
  const probe = qualifiedMetricForAvailabilityProbe(
    metric,
    widgetMetric.value.metric,
    queryCtx.value,
  );
  return isMetricUnavailableInAutocomplete(metric, probe, queryCtx.value, props.adapters);
};

function metricAvailabilityReasonForMetric(metric: Metric): string | null {
  return metricTooltipAvailabilityReason(
    metric,
    qualifiedMetricForAvailabilityProbe(metric, widgetMetric.value.metric, queryCtx.value),
    queryCtx.value,
    metricAvailabilityReasons.value,
    props.adapters,
  );
}

const currentMetricAvailabilityReason = computed(() =>
  metricAvailabilityReasonForMetric(currentMetric.value),
);

const isMaster = computed(() => props.lineIdx === 0);
const isGraphWidget = computed(() => widgetType.value === WidgetType.Graph);
const isLineGraphWidget = computed(() => isGraphWidget.value && asGraph(widget.value).graphType.name === 'Line');
const isReadoutWidget = computed(() => widgetType.value === WidgetType.Readout);
</script>

<template>
  <div class="widget-line">
    <div class="widget-cell col-metric">
      <v-tooltip
        :disabled="!showMetricAvailabilityOnMetricField"
        location="top"
      >
        <template v-slot:activator="{ props: tooltipProps }">
          <div v-bind="tooltipProps" class="metric-autocomplete-wrap">
            <v-autocomplete
              v-model="selectedMetric"
              item-title="name"
              item-value="id"
              :items="metricsFiltered"
              :disabled="locked"
              return-object
              :density="isMaster ? 'default' : 'compact'"
              :class="{ 'metric-unavailable': metricFieldGreyedOut }"
            >
              <template
                v-if="showMetricAvailabilityOnMetricField && currentMetricUnavailable"
                v-slot:prepend-inner
              >
                <v-icon size="small">mdi-information-outline</v-icon>
              </template>
              <template v-slot:item="{ item, props: itemProps }">
                <v-tooltip location="top">
                  <template v-slot:activator="{props: tooltipProps}">
                    <v-list-item
                      v-bind="{...itemProps, ...tooltipProps}"
                      :title="metricFromItem(item).name"
                      :class="{ 'metric-option-unavailable': isMetricUnavailableInList(metricFromItem(item)) }"
                    >
                      <template v-slot:prepend v-if="isMetricUnavailableInList(metricFromItem(item))">
                        <v-icon size="small" class="mr-1">mdi-information-outline</v-icon>
                      </template>
                    </v-list-item>
                  </template>
                  <div class="device-availability-tooltip">
                    <div>{{ metricFromItem(item).description }}</div>
                    <div
                      v-if="metricAvailabilityReasonForMetric(metricFromItem(item))"
                      class="device-availability-tooltip-reason"
                    >
                      {{ metricAvailabilityReasonForMetric(metricFromItem(item)) }}
                    </div>
                  </div>
                </v-tooltip>
              </template>
            </v-autocomplete>
          </div>
        </template>
        <div class="device-availability-tooltip">
          <div>{{ currentMetric.description }}</div>
          <div
            v-if="currentMetricAvailabilityReason"
            class="device-availability-tooltip-reason"
          >
            {{ currentMetricAvailabilityReason }}
          </div>
        </div>
      </v-tooltip>
    </div>
    <div v-if="showDeviceSelector" class="widget-cell col-device">
      <v-select
        v-model="selectedDeviceId"
        class="loadout-field-compact"
        :class="{ 'metric-unavailable': currentDeviceSelectionUnavailable }"
        :items="deviceSelectItems"
        item-value="id"
        item-title="title"
        label="Device"
        :disabled="locked"
        :density="isMaster ? 'default' : 'compact'"
      >
        <template v-slot:selection>
          <v-tooltip location="top">
            <template v-slot:activator="{ props: tooltipProps }">
              <span v-bind="tooltipProps" class="d-flex align-center device-selection-text">
                <v-icon
                  v-if="currentDeviceSelectionUnavailable"
                  size="small"
                  class="device-select-info-icon flex-shrink-0"
                >mdi-information-outline</v-icon>
                <span class="text-truncate">{{ deviceSelectionTitle(storedDeviceId) }}</span>
              </span>
            </template>
            <div v-if="selectedDeviceSelectItem" class="device-availability-tooltip">
              <div>{{ selectedDeviceSelectItem.title }}</div>
              <div
                v-if="selectedDeviceSelectItem.availabilityReason"
                class="device-availability-tooltip-reason"
              >
                {{ selectedDeviceSelectItem.availabilityReason }}
              </div>
            </div>
            <div v-else>{{ deviceSelectionTitle(storedDeviceId) }}</div>
          </v-tooltip>
        </template>
        <template v-slot:item="{ item, props: itemProps }">
          <v-tooltip location="top">
            <template v-slot:activator="{ props: tooltipProps }">
              <v-list-item
                v-bind="{ ...itemProps, ...tooltipProps }"
                :class="{
                  'metric-option-unavailable': deviceSelectItemFromListItem(item).unavailable,
                }"
              >
                <template v-slot:title>
                  <span class="device-select-item-title">
                    <v-icon
                      v-if="deviceSelectItemFromListItem(item).unavailable"
                      size="small"
                      class="device-select-info-icon"
                    >mdi-information-outline</v-icon>
                    {{ deviceSelectItemFromListItem(item).title }}
                  </span>
                </template>
              </v-list-item>
            </template>
            <div class="device-availability-tooltip">
              <div>{{ deviceSelectItemFromListItem(item).title }}</div>
              <div
                v-if="deviceSelectItemFromListItem(item).availabilityReason"
                class="device-availability-tooltip-reason"
              >
                {{ deviceSelectItemFromListItem(item).availabilityReason }}
              </div>
            </div>
          </v-tooltip>
        </template>
      </v-select>
    </div>
    <div v-if="showArrayIndexSelector" class="widget-cell col-array-index">
      <v-select
        v-model="arrayIndex"
        class="loadout-field-idx"
        :items="arrayIndexOptions"
        label="Idx"
        hide-details
        :disabled="locked"
        :density="isMaster ? 'default' : 'compact'"
      ></v-select>
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
        <template v-slot:item="{ item, props: itemProps }">
          <v-tooltip :text="`${statFromItem(item).name}: ${statFromItem(item).description}`">
            <template v-slot:activator="{props: tooltipProps}">
              <v-list-item v-bind="{...itemProps, ...tooltipProps}" :title="statFromItem(item).shortName"/>
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
        <template v-slot:selection="{ item }">
          {{ widgetTypeToString(widgetTypeFromItem(item)) }}
        </template>
        <template v-slot:item="{ item, props }">
          <v-list-item v-bind="props" :title="widgetTypeToString(widgetTypeFromItem(item))">
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
.metric-autocomplete-wrap {
  width: 100%;
}
.metric-unavailable :deep(.v-field) {
  opacity: 0.65;
}
.metric-option-unavailable {
  opacity: 0.65;
}
.device-select-info-icon {
  margin-inline-end: 4px;
  vertical-align: middle;
}
.device-select-item-title {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  min-width: 0;
}
.device-availability-tooltip {
  display: block;
  white-space: normal;
}
.device-availability-tooltip-reason {
  margin-top: 4px;
}
</style>
