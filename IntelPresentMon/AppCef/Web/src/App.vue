<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <v-app>
    <v-dialog
      v-model="fatalErrorDialogActive"
      max-width="500"
      persistent
    >
      <v-card>
        <v-card-title class="text-h5 error--text">
          {{ fatalErrorTitle }}
        </v-card-title>
        <v-card-text>
          {{ fatalErrorText }}
        </v-card-text>
      </v-card>
    </v-dialog>

    <v-navigation-drawer
      app
      v-if="inSettings"
      permanent
      :width="180"
      color="#030308"
      class="pt-3"
    >
      <router-link :to="{name: 'simple'}" class="nav-back"><v-icon class="nav-back-arrow">mdi-arrow-left</v-icon> Back</router-link>

      <v-list nav>

        <v-list-item color="primary" :to="{name: 'overlay-config'}">
          <v-list-item-content>
            <v-list-item-title>Overlay</v-list-item-title>
          </v-list-item-content>
        </v-list-item>

        <v-list-item color="primary" :to="{name: 'metric-processing'}">
          <v-list-item-content>
            <v-list-item-title>Data Processing</v-list-item-title>
          </v-list-item-content>
        </v-list-item>

        <v-list-item color="primary" :to="{name: 'capture-config'}">
          <v-list-item-content>
            <v-list-item-title>Capture</v-list-item-title>
          </v-list-item-content>
        </v-list-item>

        <v-list-item color="primary" :to="{name: 'other-config'}">
          <v-list-item-content>
            <v-list-item-title>Other</v-list-item-title>
          </v-list-item-content>
        </v-list-item>

        <v-list-item v-if="isDevelopment" color="primary">
          <v-list-item-content>
            <v-btn @click="doPresetUpdate">Update Presets</v-btn>
          </v-list-item-content>
        </v-list-item>

      </v-list>
    </v-navigation-drawer>

    <v-main>
      <div class="d-flex justify-center"> 
        <router-view></router-view>
      </div>
    </v-main>

    <v-footer
      color="primary darken-3"
      height="22"
      class="status-bar"
      app
    >
      <div class="sta-region">
        <div><v-icon small :color="processCogColor">mdi-cog</v-icon>&nbsp;{{ targetName }}</div>
        <div><v-icon v-show="capturing" small color="red darken-1">mdi-camera-control</v-icon></div>
      </div>
      <div class="sta-region">
        <div>{{ visibilityString }}</div>
        <div>{{ pref.samplingPeriodMs }}ms</div>
        <div>{{ drawRateString }}fps</div>
      </div>
    </v-footer>

    <v-snackbar
      :value="showingNotificationSnack"
      :timeout="-1"
    >
      {{ currentNotificationText }} <span class="secondary--text text--lighten-3">{{ notificationCountMessage }}</span>
      <template v-slot:action="{ attrs }">
        <v-btn
          color="pink"
          icon
          v-bind="attrs"
          @click="dismissNotification"
        >
          <v-icon>mdi-close</v-icon>
        </v-btn>
      </template>
    </v-snackbar>
  </v-app>
</template>

<script lang="ts">
import Vue from 'vue'
import { Metrics } from '@/store/metrics'
import { Preferences as PrefStore } from '@/store/preferences'
import { Preferences } from '@/core/preferences'
import { Hotkey } from '@/store/hotkey'
import { Api, FileLocation } from '@/core/api'
import { Notifications } from '@/store/notifications'
import { Action } from '@/core/hotkey'
import { Processes } from '@/store/processes'
import { Adapters } from './store/adapters'
import { Preset } from './core/preferences'
import { Widget } from './core/widget'
import { Loadout } from './store/loadout'
import { launchAutotargetting } from './core/autotarget'
import { LoadBlocklists } from './core/block-list'

export default Vue.extend({
  name: 'AppRoot',

  data: () => ({
    fatalError: null as null|{title:string, text: string},
    autosaveDebounceId: null as number|null,
  }),

  async created() {
    Api.registerPresentmonInitFailedHandler(this.handlePresentmonInitFailed);
    Api.registerOverlayDiedHandler(this.handleOverlayDied);
    Api.registerStalePidHandler(this.handleStalePid);
    await Api.launchKernel();
    await Metrics.load();
    await Hotkey.refreshOptions();
    await Adapters.refresh();
    Api.registerTargetLostHandler(this.handleTargetLost);
    await Hotkey.initBindings();
    Api.registerHotkeyHandler(this.handleHotkeyFired);
    await this.initPreferences();
    PrefStore.writeAttribute({
      attr: 'samplesPerFrame',
      val: this.calculateSamplesPerFrame(this.desiredDrawRate, this.samplePeriod),
    });
    await LoadBlocklists();
    if (PrefStore.preferences.enableAutotargetting) {
      launchAutotargetting();
    }
  },

  methods: {
    async initPreferences() {
      try {
        const payload = await Api.loadPreferences();
        await PrefStore.parseAndReplaceRawPreferenceString(payload);
      }
      catch (e) {
        await Hotkey.bindDefaults();
        PrefStore.resetPreferences();
        PrefStore.setAttribute({attr:'selectedPreset', val:Preset.Slot1});
      }
    },
    async doPresetUpdate() {
      for (let i = 0; i < 3; i++) {        
        const presetFileName = `preset-${i}.json`;
        await Loadout.parseAndReplace(await Api.loadPreset(presetFileName));
        await Api.storeFile(Loadout.fileContents, FileLocation.Documents, `loadouts\\${presetFileName}`);
      }
    },
    async dismissNotification() {
      await Notifications.dismiss();
    },
    calculateSamplesPerFrame(drawRate: number, samplePeriod: number) {
      return Math.max(Math.round((1000 / drawRate) / samplePeriod), 1);
    },
    handlePresentmonInitFailed() {
      this.fatalError = {
        title: 'PresentMon Initialization Error',
        text: 'Failed to initialize PresentMon API. Ensure that PresentMon Service is installed and running, and try again.'
      };
      console.error('received presentmon init failed signal');
    },
    handleOverlayDied() {
      Notifications.notify({text: 'Error: overlay crashed unexpectedly'});
      PrefStore.setPid(null);
      console.warn('received overlay died signal');
    },
    handleTargetLost(pid: number) {
      PrefStore.setPid(null);
    },
    handleStalePid() {
      PrefStore.setPid(null);
      Notifications.notify({text: 'Selected process has already exited.'});
      console.warn('selected pid was stale');
    },
    async handleHotkeyFired(action: Action) {
      if (action === Action.ToggleCapture) {
        if (this.pid !== null) {
          await PrefStore.writeCapture(!this.capturing);
        }
      } else if (action === Action.ToggleOverlay) {
        PrefStore.writeAttribute({attr: 'hideAlways', val: !this.pref.hideAlways});
      } else if (action === Action.CyclePreset) {
        // cycle the selected preset index
        if (this.selectedPreset === null || this.selectedPreset >= 2) {
          this.selectedPreset = 0;
        } else {
          this.selectedPreset++;
        }
      }
    },
    async pushSpecification() {
        await Api.pushSpecification({
          pid: this.pid,
          preferences: this.pref,
          widgets: this.widgets,
        });
    }
  },

  computed: {
    isDevelopment(): boolean {
      return process?.env?.NODE_ENV === 'development';
    },
    widgets(): Widget[] {
      return Loadout.widgets;
    },
    inSettings(): boolean {
      return ['capture-config', 'overlay-config', 'metric-processing', 'other-config']
        .includes(this.$route.name ?? '');
    },
    fatalErrorTitle(): string {
      return this.fatalError?.title ?? '';
    },
    fatalErrorText(): string {
      return this.fatalError?.text ?? '';
    },
    fatalErrorDialogActive(): boolean {
      return this.fatalError !== null;
    },
    notificationCountMessage(): string {
      const count = Notifications.count;
      return count > 1 ? `(${count - 1} more)` : '';
    },
    currentNotificationText(): string {
      return Notifications.current?.text ?? '';
    },
    showingNotificationSnack(): boolean {
      return Notifications.showing;
    },
    pref(): Preferences {
      return PrefStore.preferences;
    },
    samplePeriod(): number {
      return this.pref.samplingPeriodMs;
    },
    desiredDrawRate(): number {
      return PrefStore.desiredOverlayDrawRate;
    },
    pid(): number|null {
      return PrefStore.pid;
    },
    capturing(): boolean {
      return PrefStore.capturing;
    },
    targetName(): string {
      if (this.pid === null) {
        return '';
      }
      return Processes.processes.find(p => p.pid === this.pid)?.name ?? '';
    },
    processCogColor(): string {
      return this.targetName.length === 0 ? 'secondary lighten-1' : 'white';
    },
    drawRateString(): string {
      return (1000 / (this.pref.samplingPeriodMs * this.pref.samplesPerFrame)).toFixed(1);
    },
    visibilityString(): string {
      if (this.pref.hideAlways) {
        return 'Hidden';
      } else if (this.pref.hideDuringCapture) {
        return "Autohide";
      } else {
        return '';
      }
    },
    selectedPreset: {
      get(): number|null { return PrefStore.preferences.selectedPreset; },
      set(preset: number|null) {
        PrefStore.writeAttribute({attr: 'selectedPreset', val: preset});
      }
    },
  },

  watch: {
    // watchers for pushing spec to backend
    pref: {
      async handler() {
        await this.pushSpecification();
      },
      deep: true
    },
    widgets: {
      async handler() {
        await this.pushSpecification();
      },
      deep: true
    },
    async pid() {
      await this.pushSpecification();
    },
    // watchers for calculating sample/frame ratio
    samplePeriod(newPeriod: number) {
      PrefStore.writeAttribute({
        attr: 'samplesPerFrame',
        val: this.calculateSamplesPerFrame(this.desiredDrawRate, newPeriod),
      });
    },
    desiredDrawRate(newRate: number) {
      PrefStore.writeAttribute({
        attr: 'samplesPerFrame',
        val: this.calculateSamplesPerFrame(newRate, this.samplePeriod),
      });
    },
    // capture watch
    async capturing(newCapturing: boolean) {
      await Api.setCapture(newCapturing);
    },
    // preset change watcher
    async selectedPreset(presetNew: Preset, presetOld: Preset|null) {
      if (presetNew === Preset.Custom) {
        try {
          await Loadout.parseAndReplace(await Api.loadConfig('custom-auto.json'));
        }
        catch (e) {
          await Notifications.notify({text:`Failed to load autosave loadout file.`});
          console.error([`Failed to load autosave loadout file.`, e]);
        }
      }
      else {
        const presetFileName = `preset-${presetNew}.json`;
        try {
          await Loadout.parseAndReplace(await Api.loadPreset(presetFileName));
        }
        catch (e) {
          await Notifications.notify({text:`Failed to load preset file [${presetFileName}].`});
          console.error([`Failed to load preset file [${presetFileName}].`, e]);
        }
      }      
    },
  }
});
</script>

<style lang="scss">
  @import '../node_modules/@fontsource/roboto/index.css';
  @import '../node_modules/@mdi/font/css/materialdesignicons.css';
</style>

<style scoped>
* {
  user-select: none; 
}
.status-bar {
  padding: 0;
  font-size: 12px;
  font-weight: 300;
  justify-content: space-between;
  user-select: none;
}
.sta-region {
  display: flex;
}
.sta-region > div {
  display: flex;
  align-items: center;
  padding: 0 4px;
}
.menu-item {
  border-radius: 0;
}
.nav-back {
  text-decoration: none;
  color: whitesmoke;
  margin-left: 10px;
  text-transform: uppercase;
  font-size: 18px;
}
</style>
