<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script setup lang="ts">
import { usePreferencesStore } from '@/stores/preferences';
import { useIntrospectionStore } from '@/stores/introspection';

defineOptions({ name: 'DefaultAdapterSelect' });

interface Props {
  label?: string;
  density?: 'default' | 'comfortable' | 'compact';
  hideDetails?: boolean;
}

withDefaults(defineProps<Props>(), {
  label: undefined,
  density: 'default',
  hideDetails: false,
});

const prefs = usePreferencesStore();
const intro = useIntrospectionStore();
</script>

<template>
  <v-select
    v-model="prefs.preferences.adapterId"
    :items="intro.adapters"
    item-value="id"
    item-title="name"
    :disabled="intro.adapters.length === 0"
    :clearable="false"
    :label="label"
    :density="density"
    :hide-details="hideDetails"
  ></v-select>
</template>
