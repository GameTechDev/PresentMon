import { ref } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import type { Metric } from '@/core/metric';
import type { Stat } from '@/core/stat';
import type { Unit } from '@/core/unit';
import type { Adapter } from '@/core/adapter';
import type { MetricAvailabilityReason } from '@/core/metric-availability-constants';

export const useIntrospectionStore = defineStore('introspection', () => {
  // === State ===
  const metrics = ref<Metric[]>([]);
  const stats = ref<Stat[]>([]);
  const units = ref<Unit[]>([]);
  const adapters = ref<Adapter[]>([]);
  const systemDeviceId = ref<number>(0);
  const metricAvailabilityReasons = ref<MetricAvailabilityReason[]>([]);
  const introspectionLoaded = ref(false);

  // === Actions ===
  async function load() {
    if (introspectionLoaded.value) {
      return;
    }
    const intro = await Api.introspect();
    metrics.value = intro.metrics;
    stats.value = intro.stats;
    units.value = intro.units;
    adapters.value = intro.adapters;
    systemDeviceId.value = intro.systemDeviceId;
    metricAvailabilityReasons.value = Array.isArray(intro.metricAvailabilityReasons)
      ? intro.metricAvailabilityReasons
      : [];
    introspectionLoaded.value = true;
  }

  // === Exports ===
  return {
    metrics,
    stats,
    units,
    adapters,
    systemDeviceId,
    metricAvailabilityReasons,
    load,
  };
});
