// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { Notification } from '@/core/notification';
import { delayFor } from '@/core/timing';

export const useNotificationsStore = defineStore('notifications', () => {
  // === State ===
  const notifications = ref<Notification[]>([]);
  const showing = ref(false);

  // === Computed ===
  const current = computed(() => notifications.value.length > 0 ? notifications.value[0] : null);
  const count = computed(() => notifications.value.length);

  // === Functions ===
  function push(notification: Notification) {
    notifications.value.push(notification);
  }
  function pop() {
    if (notifications.value.length > 0) {
      notifications.value.shift();
    }
  }

  // === Actions ===
  async function notify(note: Notification) {
    push(note);
    showing.value = true;
  }
  async function dismiss() {
    showing.value = false;
    await delayFor(250);
    pop();
    if (notifications.value.length > 0) {
      showing.value = true;
    }
  }

  return {
    notifications,
    showing,
    current,
    count,
    notify,
    dismiss,
  };
});