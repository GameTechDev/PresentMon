// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { ref } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import type { Adapter } from '@/core/adapter';

export const useAdaptersStore = defineStore('adapters', () => {
  // === State ===
  const adapters = ref<Adapter[]>([]);

  // === Actions ===
  async function refresh() {
    adapters.value = await Api.enumerateAdapters();
  }

  // === Exports ===
  return {
    adapters,
    refresh,
  };
});