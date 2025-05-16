<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import type { Widget } from '@/core/widget'
import type { Metric } from '@/core/metric'
import type { MetricOption } from '@/core/metric-option'
import type { Stat } from '@/core/stat'
import LoadoutLine from './LoadoutLine.vue'
import { useLoadoutStore } from '@/stores/loadout'

defineOptions({ name: 'LoadoutRow' })

interface Props {
  widgetIdx: number
  widgets: Widget[]
  metrics: Metric[]
  stats: Stat[]
  metricOptions: MetricOption[]
  locked?: boolean
  adapterId: number | null
}

const props = withDefaults(defineProps<Props>(), {
  locked: false
})

const emit = defineEmits<{
  (e: 'delete', widgetIdx: number): void
}>()

const lineKeys = ref<number[]>([])
const nextKey = ref(0)

const loadoutStore = useLoadoutStore()

onMounted(() => {
  lineKeys.value = Array(props.metrics.length).fill(0).map((n, i) => i)
  nextKey.value = props.metrics.length
})

const deleteLine = (lineIdx: number) => {
  loadoutStore.removeWidgetMetric(props.widgetIdx, lineIdx)
  lineKeys.value.splice(lineIdx, 1)
}

const handleDelete = (lineIdx: number) => {
  if (lineIdx === 0) {
    emit('delete', props.widgetIdx)
  } else {
    deleteLine(lineIdx)
  }
}

const handleAdd = () => {
  try {
    loadoutStore.addWidgetMetric(props.widgetIdx, null)
    lineKeys.value.push(nextKey.value++)
  } catch (e) {}
}

const handleClearMulti = () => {
  while (lineKeys.value.length > 1) {
    deleteLine(1)
  }
}

const widget = computed(() => props.widgets[props.widgetIdx])
const widgetMetrics = computed(() => widget.value.metrics)
</script>

<template>
<div class="widget-row sortable-row">
  <div class="grip-wrap">
    <v-icon v-show="!props.locked" class="sortable-handle">mdi-drag-horizontal-variant</v-icon>
  </div>
  <div class="line-wrap">
    <loadout-line
      v-for="(oneBased, i) in widgetMetrics.length" :key="lineKeys[i]"
      :widgetIdx="props.widgetIdx" :lineIdx="i" :widgets="props.widgets" :stats="props.stats"
      :metrics="props.metrics" :metricOptions="props.metricOptions" :locked="props.locked" :adapterId="props.adapterId"
      @delete="handleDelete" @add="handleAdd" @clearMulti="handleClearMulti"
    ></loadout-line>
  </div>
</div>
</template>

<style scoped lang="scss">
</style>