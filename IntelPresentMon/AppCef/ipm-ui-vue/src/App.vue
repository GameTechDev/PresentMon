<script setup lang="ts">
import { onMounted, ref, computed, watchEffect, watch } from 'vue';
import { useRoute } from 'vue-router';
import { usePreferencesStore } from './stores/preferences';
import { Preset } from './core/preferences';
import { Api } from './core/api';
import { useLoadoutStore } from './stores/loadout';
import { useHotkeyStore } from './stores/hotkey';
import { Action } from './core/hotkey';
import { useProcessesStore } from './stores/processes';
import { useNotificationsStore } from './stores/notifications';
import { dispatchDelayedTask } from './core/timing';

const route = useRoute()

// === State ===
interface ErrorMessage {
  title: string;
  text: string;
}
const dialogError = ref<ErrorMessage|null>(null);

// === Stores ===
const prefs = usePreferencesStore()
const loadout = useLoadoutStore()
const hotkeys = useHotkeyStore()
const procs = useProcessesStore()
const notes = useNotificationsStore()

// === Functions ===
function cyclePreset() {
  if (prefs.preferences.selectedPreset === null || prefs.preferences.selectedPreset >= 2) {
    prefs.preferences.selectedPreset = 0;
  } else {
    prefs.preferences.selectedPreset++;
  }
}

// === Lifecycle Hooks ===

// === Computed ===
const inSettings = computed(() => {
  const routeName = typeof route.name === 'symbol' ? route.name.toString() : route.name;
  return ['capture-config', 'overlay-config', 'data-config', 'other-config', 'flash-config']
    .includes(routeName ?? '')
});
const targetName = computed(() => {
  const target = prefs.pid;
  if (target === null) {
    return '';
  }
  return procs.processes.find((proc) => proc.pid === target)?.name ?? '';
});
const visibilityString = computed(() => {
  if (prefs.preferences.hideAlways) {
    return 'Hidden';
  } else if (prefs.preferences.hideDuringCapture && prefs.capturing) {
    return "(Auto)Hidden";
  } else if (prefs.preferences.hideDuringCapture && !prefs.capturing) {
    return "Autohide";
  } else {
    return '';
  }
});
const errorDialogActive = computed(() => dialogError.value !== null);
const notificationMoreText = computed(() => {
  if (notes.count > 1) {
    return `[${notes.count} more...]`;
  }
  return '';
});

// === Signal Handlers ===
Api.registerTargetLostHandler(() => {
  prefs.pid = null
})
Api.registerHotkeyHandler((action: number) => {
  switch (action as Action) {
    case Action.ToggleOverlay:
      prefs.preferences.hideAlways = !prefs.preferences.hideAlways
      break;
    case Action.CyclePreset:
      cyclePreset()
      break;
    case Action.ToggleCapture:
      prefs.toggleCapture()
      break;
    default:
      console.warn(`Unhandled hotkey action: ${action}`);
      break;
  }
})
Api.registerPresentmonInitFailedHandler(() => {
  dialogError.value = {
    title: 'PresentMon Initialization Error',
    text: 'Failed to initialize PresentMon API. Ensure that PresentMon Service is installed and running, and try again.',
  }
  console.error('received presentmon init failed signal')
})
Api.registerOverlayDiedHandler(() => {
  notes.notify({text: 'Error: overlay crashed unexpectedly'});
  prefs.pid = null
  console.error('received overlay died signal');
})
Api.registerStalePidHandler(() => {
  notes.notify({text: 'Selected process has already exited.'});
  prefs.pid = null
  console.warn('received stale pid signal');
})

// === Global Watchers ===
// react to change in selected preset and load the corresponding config file
watchEffect(async () => {
  const selectedPreset = prefs.preferences.selectedPreset
  if (selectedPreset === Preset.Custom) {
    const {payload} = await Api.loadConfig('custom-auto.json');
    const err = 'Failed to load autosave loadout file. ';
    await loadout.loadConfigFromPayload(payload, err);
  }
  else if (selectedPreset !== null) {
    const presetFileName = `preset-${selectedPreset}.json`;
    const {payload} = await Api.loadPreset(presetFileName);
    const err = `Failed to load preset file [${presetFileName}]. `;
    await loadout.loadConfigFromPayload(payload, err);
  }
})
// change in pid requires spec push but no serialize
watch(() => prefs.pid, async () => {
  await prefs.pushSpecification()
})
// change in preferences requires spec push and serialize
watch(() => prefs.preferences, async () => {
    prefs.serialize()
    await prefs.pushSpecification()
}, {deep: true})
// change in hotkeys requires only serialize
watch(() => hotkeys.bindings, async () => {
    prefs.serialize()
}, {deep: true})
// change in loadout requires push and additional loadout serialization if custom
watch(() => loadout.widgets, async () => {
    if (prefs.preferences.selectedPreset === Preset.Custom) {
      loadout.serializeCurrent()
    }
    await prefs.pushSpecification()
}, {deep: true})
</script>

<template>
  <v-app>
    <div class="app-layout">
      <div class="content-row">
        <v-navigation-drawer
          v-if="inSettings"
          permanent
          :width="180"
          color="#030308"
          class="custom-drawer pt-3"
        >
          <router-link :to="{ name: 'main' }" class="nav-back">
            <v-icon class="nav-back-arrow">mdi-arrow-left</v-icon> Top
          </router-link>
          <v-list nav>
            <v-list-item color="primary" :to="{ name: 'overlay-config' }">
              <v-list-item-title class="nav-item">Overlay</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'data-config' }">
              <v-list-item-title class="nav-item">Data</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'capture-config' }">
              <v-list-item-title class="nav-item">Capture</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'flash-config' }">
              <v-list-item-title class="nav-item">Flash</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'other-config' }">
              <v-list-item-title class="nav-item">Other</v-list-item-title>
            </v-list-item>
          </v-list>
        </v-navigation-drawer>

        <v-main class="main-view">
          <div class="d-flex justify-center">
            <router-view />
          </div>
        </v-main>
      </div>

      <div class="footer-wrap">
        <v-footer class="footer" color="blue-darken-3" height="22">
          <div class="sta-region">
            <div class="pl-2">{{ targetName }}</div>
            <v-icon v-show="prefs.capturing" small color="red-darken-1">mdi-camera-control</v-icon>
          </div>
          <div class="sta-region">
            <div>{{ visibilityString }}</div>
            <div>{{ prefs.preferences.metricPollRate }}Hz</div>
            <div>{{ prefs.preferences.overlayDrawRate }}fps</div>
          </div>
        </v-footer>
      </div>

      <!-- Fullscreen Modal for Serious Errors -->
      <v-dialog v-model="errorDialogActive" persistent max-width="500">
        <v-card>
          <v-card-title class="text-h5 text-error">
            {{ dialogError!.title }}
          </v-card-title>
          <v-card-text>
            {{ dialogError!.text }}
          </v-card-text>
        </v-card>
      </v-dialog>

      <!-- Snackbar for Notifications -->
      <v-snackbar v-model="notes.showing" :timeout="-1" location="bottom">        
        {{ notes.current!.text }} <span style="font-size: 11px; color: grey">{{ notificationMoreText }}</span>
        <template v-slot:actions>
          <v-btn icon @click="notes.dismiss">
            <v-icon>mdi-close</v-icon>
          </v-btn>
        </template>
      </v-snackbar>
    </div>
  </v-app>
</template>

<style scoped>
.app-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.content-row {
  display: flex;
  flex: 1 1 auto;
  min-height: 0;
  overflow: hidden;
}

.custom-drawer {
  height: calc(100vh - 22px);
  max-height: calc(100vh - 22px);
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.main-view {
  flex: 1;
  overflow-y: auto;
  height: 100%;
}

.footer-wrap {
  height: 22px;
  flex-shrink: 0;
}

.footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
  font-weight: 300;
  padding: 0;
  user-select: none;
  color: white;
}

.sta-region {
  display: flex;
}

.sta-region > div {
  display: flex;
  align-items: center;
  padding: 0 4px;
}

.nav-back {
  text-decoration: none;
  color: whitesmoke;
  margin-left: 10px;
  text-transform: uppercase;
  font-size: 18px;
}

.v-list-item {
  border-radius: 4px;
  transition: background-color 0.2s ease;
}

.v-list-item-title.v-list-item-title {
  font-size: 16px;
}

.v-list-item--active {
  font-weight: 400;
}

* {
  user-select: none; 
}
</style>
