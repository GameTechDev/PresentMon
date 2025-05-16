import { ref } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import type { Process } from '@/core/process';

export const useProcessesStore = defineStore('processes', () => {
  // === State ===
  const processes = ref<Process[]>([]);

  // === Actions ===
  async function refresh() {
    processes.value = await Api.enumerateProcesses();
  }

  // === Exports ===
  return {
    processes,
    refresh,
  };
});