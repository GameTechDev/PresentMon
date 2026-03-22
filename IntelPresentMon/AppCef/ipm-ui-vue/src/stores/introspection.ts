import { ref, computed } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import type { Metric } from '@/core/metric';
import type { Stat } from '@/core/stat';
import type { Unit } from '@/core/unit';
import type { MetricOption } from '@/core/metric-option';
import type { Adapter } from '@/core/adapter';

export const useIntrospectionStore = defineStore('introspection', () => {
  // === State ===
  const metrics = ref<Metric[]>([]);
  const stats = ref<Stat[]>([]);
  const units = ref<Unit[]>([]);
  const adapters = ref<Adapter[]>([]);
  const systemDeviceId = ref<number>(0);
  const defaultAdapterId = ref<number>(0);

  // === Computed ===
  const metricOptions = computed<MetricOption[]>(() => {
    return metrics.value.flatMap(m => 
      Array.from({ length: m.arraySize }, (_, i) => ({
        metricId: m.id,
        name: m.arraySize > 1 ? `${m.name} [${i}]` : m.name,
        arrayIndex: i,
      }))
    );
  });

  // === Actions ===
  async function load() {
    if (metrics.value.length === 0 || systemDeviceId.value === 0 || defaultAdapterId.value === 0) {
      const intro = await Api.introspect();
      metrics.value = intro.metrics;
      stats.value = intro.stats;
      units.value = intro.units;
      adapters.value = intro.adapters;
      systemDeviceId.value = intro.systemDeviceId;
      defaultAdapterId.value = intro.defaultAdapterId;
    }
  }

  // === Exports ===
  return {
    metrics,
    stats,
    units,
    adapters,
    systemDeviceId,
    defaultAdapterId,
    metricOptions,
    load
  };
});
