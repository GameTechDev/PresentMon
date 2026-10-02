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
const dialogError = ref<ErrorMessage|null>(
  Api.presentmonInitFailed ? {
    title: 'PresentMon Initialization Error',
    text: 'Failed to initialize PresentMon API. Ensure that PresentMon Service is installed and running, and try again.',
  } : null
);

// === Stores ===
const prefs = usePreferencesStore()
const loadout = useLoadoutStore()
const hotkeys = useHotkeyStore()
const procs = useProcessesStore()
const notes = useNotificationsStore()

// === Functions ===
function cyclePreset() {
  if (prefs.preferences.selectedPreset === null || prefs.preferences.selectedPreset >= 3) {
    prefs.preferences.selectedPreset = 0;
  } else {
    prefs.preferences.selectedPreset++;
  }
}

// === Lifecycle Hooks ===

// === Computed ===
const inSettings = computed(() => {
  const routeName = typeof route.name === 'symbol' ? route.name.toString() : route.name;
  return ['capture-config', 'overlay-config', 'data-config', 'other-config', 'logging-config', 'about-config']
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
    case Action.ToggleEtlLogging:
      prefs.notifyEtlLoggingDisabled();
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
            <v-list-item color="primary" :to="{ name: 'logging-config' }">
              <v-list-item-title class="nav-item">Logging</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'other-config' }">
              <v-list-item-title class="nav-item">Other</v-list-item-title>
            </v-list-item>
            <v-list-item color="primary" :to="{ name: 'about-config' }">
              <v-list-item-title class="nav-item">About</v-list-item-title>
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
            <div v-show="prefs.etlLogging">ETL</div>
            <div>{{ visibilityString }}</div>
            <div>{{ prefs.preferences.metricPollRate }}Hz</div>
            <div>{{ prefs.preferences.overlayDrawRate }}fps</div>
          </div>
        </v-footer>
      </div>

      <!-- Fullscreen Modal for Serious Errors -->
      <v-dialog
        v-model="errorDialogActive"
        persistent
        max-width="560"
        scrim="#080a0d"
        aria-labelledby="initialization-error-title"
        aria-describedby="initialization-error-description"
      >
        <v-card class="error-dialog-card" rounded="lg" elevation="24">
          <div class="error-dialog-accent"></div>
          <v-card-text class="pa-0">
            <div class="error-dialog-header">
              <div class="error-dialog-icon">
                <v-icon size="34">mdi-alert-outline</v-icon>
              </div>
              <div>
                <div class="error-dialog-kicker">Service unavailable</div>
                <div id="initialization-error-title" class="error-dialog-title">
                  {{ dialogError!.title }}
                </div>
              </div>
            </div>
            <div class="error-dialog-body">
              <p id="initialization-error-description">
                {{ dialogError!.text }}
              </p>
              <div class="error-dialog-guidance">
                <v-icon size="19">mdi-information-outline</v-icon>
                <span>Start or restart the PresentMon Service, then relaunch Intel PresentMon.</span>
              </div>
            </div>
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

.error-dialog-card {
  overflow: hidden;
  color: #f5f7fa;
  background: linear-gradient(145deg, #24272d 0%, #1b1d22 100%);
  border: 1px solid rgba(255, 82, 82, 0.28);
  box-shadow:
    0 24px 64px rgba(0, 0, 0, 0.55),
    0 0 0 1px rgba(255, 255, 255, 0.03);
}

.error-dialog-accent {
  height: 4px;
  background: linear-gradient(90deg, #ff5252 0%, #ff7b6b 55%, rgba(255, 123, 107, 0.15) 100%);
}

.error-dialog-header {
  display: flex;
  flex-direction: column;
  gap: 14px;
  align-items: center;
  padding: 28px 28px 18px;
  text-align: center;
}

.error-dialog-icon {
  display: flex;
  flex: 0 0 58px;
  align-items: center;
  justify-content: center;
  width: 58px;
  height: 58px;
  color: #ff6b6b;
  background: rgba(255, 82, 82, 0.12);
  border: 1px solid rgba(255, 82, 82, 0.22);
  border-radius: 50%;
}

.error-dialog-kicker {
  margin-bottom: 4px;
  color: #ff8a80;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.error-dialog-title {
  color: #ff6b6b;
  font-size: 22px;
  font-weight: 500;
  line-height: 1.25;
}

.error-dialog-body {
  padding: 0 32px 28px;
  text-align: center;
  color: rgba(245, 247, 250, 0.86);
  font-size: 15px;
  line-height: 1.55;
}

.error-dialog-body p {
  margin: 0;
}

.error-dialog-guidance {
  display: flex;
  gap: 10px;
  align-items: flex-start;
  margin-top: 20px;
  padding: 14px 16px;
  text-align: left;
  color: rgba(245, 247, 250, 0.72);
  font-size: 13px;
  line-height: 1.45;
  background: rgba(255, 255, 255, 0.045);
  border-left: 2px solid rgba(255, 138, 128, 0.65);
  border-radius: 4px;
}

.error-dialog-guidance .v-icon {
  flex: 0 0 auto;
  margin-top: 1px;
  color: #ff8a80;
}

* {
  user-select: none; 
}
</style>
