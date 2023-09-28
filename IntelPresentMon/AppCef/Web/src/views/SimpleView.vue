<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
<div class="page-wrap">

  <v-card class="page-card my-5 pt-3">
    <v-row>
      <v-col cols="3">
        Process
        <p class="text--secondary text-sm-caption mb-0">Application process to track, overlay and capture</p>
      </v-col>
      <v-col cols="9" class="d-flex align-center">
        <v-autocomplete
          :items="processes"
          v-model="pid"
          item-value="pid"
          :filter="selectFilter"
          label="Process"
          @click="refreshProcessList"
          append-icon=""
          :disabled="enableAutotargetting"
          hide-details
          outlined
          clearable
          dense
        >
          <template v-slot:selection="data">
            <template v-if="data.item.windowName">
              {{ makeSelectorName(data.item.windowName) }} 
              <span class="pid-node-inline">[{{ data.item.pid }}]</span>
            </template>            
            <template v-else>
              <div>
                {{ data.item.name }}
                <span class="pid-node">[{{ data.item.pid }}]</span>
              </div>
            </template>
          </template>
          <template v-slot:item="data">            
            <template v-if="data.item.windowName">
              <v-list-item-content>
                <v-list-item-title>{{ makeSelectorName(data.item.windowName) }}</v-list-item-title>
                <v-list-item-subtitle>
                  {{ data.item.name }}
                  <span class="pid-node">[{{ data.item.pid }}]</span>
                </v-list-item-subtitle>
              </v-list-item-content>
            </template>
            <template v-else>
              {{ data.item.name }}
              <span class="pid-node-inline">[{{ data.item.pid }}]</span>
            </template>
          </template>
        </v-autocomplete>
      </v-col>
    </v-row> 
    
    <v-row dense>       
      <v-col cols="3">
        Auto-target
        <p class="text--secondary text-sm-caption mb-0">Automatically target process with the highest GPU utilization</p>
      </v-col>

      <v-col cols="9" class="d-flex align-center">
        <v-switch v-model="enableAutotargetting" label="Enable" hide-details></v-switch>
      </v-col>
    </v-row>   
    
    <v-row dense>       
      <v-col cols="3">
        Overlay Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey to toggle overlay on/off</p>
      </v-col>

      <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="toggleOverlayAction"></hotkey-button>
      </v-col>
    </v-row>
  </v-card>
  
  <v-card class="page-card my-5 pt-3">
    <v-row>       
      <v-col cols="3">
        Preset
        <p class="text--secondary text-sm-caption mb-0">Select a preset configuration for overlay widget loadout etc.</p>
      </v-col>

      <v-col cols="9" class="d-flex justify-center align-center">        
        <v-btn-toggle v-model="selectedPreset" :mandatory="selectedPreset !== null">
          <v-btn class="px-5" large>
            Basic
          </v-btn>

          <v-btn class="px-5" large>
            GPU Focus
          </v-btn>

          <v-btn class="px-5" large>
            Power/Temp
          </v-btn>

          <v-btn class="px-5" large :value="1000">
            Custom
          </v-btn>        
        </v-btn-toggle>
        <v-btn
          :to="{name: 'loadout-config'}"
          color="primary" class="ml-5"
        >
          Edit
        </v-btn>
      </v-col>
    </v-row>

    <v-row dense>       
      <v-col cols="3">
        Preset Cycle Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey for cycling through presets</p>
      </v-col>

      <v-col cols="9" class="d-flex justify-center align-center">    
        <hotkey-button :action="cyclePresetAction"></hotkey-button>
      </v-col>  
    </v-row>
  </v-card>
  
  <v-card class="page-card my-5 pt-3">

    <v-row>
      <v-col cols="3">
        Capture Duration
        <p class="text--secondary text-sm-caption mb-0">Automatically stop capture after N seconds</p>
      </v-col>      
      <v-col cols="2">
          <v-switch v-model="enableCaptureDuration" label="Enable" hide-details></v-switch>
      </v-col>
      <v-col cols="3">
        <v-text-field
          label="Seconds"
          v-model="captureDuration"
          :disabled="!enableCaptureDuration"
          class="mt-4 ml-8"
          hide-details
          type="number"
          outlined
          dense
          hide-spin-buttons
        ></v-text-field>
      </v-col>
    </v-row>

    <v-row dense>       
      <v-col cols="3">
        Capture Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey for capture of per-frame performance data as CSV</p>
      </v-col>

      <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="toggleCaptureAction"></hotkey-button>
      </v-col>
    </v-row>

  </v-card>
  
  <v-card class="page-card my-5 pt-3">
    <v-row>       
      <v-col cols="3">
        Capture Storage
        <p class="text--secondary text-sm-caption mb-0">Open the folder containing all frame traces and stats summaries</p>
      </v-col>

      <v-col cols="9" class="d-flex justify-center align-center">
        <v-btn 
          large
          color="secondary"
          class="px-6"
          @click="handleExploreClick"
        >Open in Explorer</v-btn>
      </v-col>
    </v-row>
  </v-card>
  <v-row>
    <v-col cols="12" class="text-right">
      <router-link class="settings-link" :to="{name: 'overlay-config'}">
        Settings
        <v-icon large>mdi-cog</v-icon>
      </router-link>
    </v-col>
  </v-row>
</div>
</template>

<script lang="ts">
import Vue from 'vue'
import { Preferences } from '@/store/preferences'
import { Processes } from '@/store/processes'
import { Process } from '@/core/process'
import { Api } from '@/core/api'
import { Preset } from '@/core/preferences'
import { ModifierCode, KeyCode, Action } from '@/core/hotkey'
import { Hotkey } from '@/store/hotkey'
import HotkeyButton from '@/components/HotkeyButton.vue'
import { IsBlocked } from '@/core/block-list'
import { cancelTopPolling, launchAutotargetting } from '@/core/autotarget'


export default Vue.extend({
  name: 'SimpleView',

  components: {
    HotkeyButton,
  },

  data: () => ({
    processRefreshTimestamp: null as number|null,
    toggleCaptureAction: Action.ToggleCapture,
    toggleOverlayAction: Action.ToggleOverlay,
    cyclePresetAction: Action.CyclePreset,
  }),

  methods: {
    async refreshProcessList() {
      // @click gets triggered twice for some reason, suppress 2nd invocation
      const stamp = window.performance.now();
      const debounceThresholdMs = 150;
      if (this.processRefreshTimestamp === null || stamp - this.processRefreshTimestamp > debounceThresholdMs) {
        this.processRefreshTimestamp = stamp;
        await Processes.refresh();
      }
    },
    selectFilter(item: Process, query: string) {
      const winText = item.windowName?.toLowerCase();
      if (winText && winText.indexOf(query) > -1) {
        return true;
      }
      return item.name.toLowerCase().indexOf(query) > -1 ||
        item.pid.toString().indexOf(query) > -1;
    },
    handleCaptureClick() {
      Preferences.writeCapture(!this.capturing);
    },
    async handleExploreClick() {
      await Api.exploreCaptures();
    },
    makeSelectorName(winName: string): string {
      const maxLen = 73;
      const leading = 30;
      const trailing = 40;
      if (winName.length > maxLen) {
        return winName.substr(0, leading) + '...' + winName.substr(-trailing);
      }
      else {
        return winName;
      }
    },
    getModifierName(mod: ModifierCode): string {
      return Hotkey.modifierOptions.find(mo => mo.code === mod)?.text ?? '???';
    },
    getKeyName(key: KeyCode): string {
      return Hotkey.keyOptions.find(ko => ko.code === key)?.text ?? '???';
    },
    getCombinationText(action: Action): string {
      const combination = Hotkey.bindings[Action[action]]?.combination;
      if (combination) {
        let text = '';
        for (const m of combination.modifiers) {
          text += this.getModifierName(m) + ' + ';
        }
        return text + this.getKeyName(combination.key);
      }
      else {
        return '';
      }
    },
  },  
  computed: {
    processes(): Process[] {
      if (Preferences.preferences.enableTargetBlocklist) {
        return Processes.processes.filter(proc => !IsBlocked(proc.name));
      }
      else {
        return Processes.processes;
      }
    },
    hasActiveTarget(): boolean {
      return Preferences.pid !== null;
    },
    capturing(): boolean {
      return Preferences.capturing;
    },
    beginCaptureButtonColor(): string {
      return this.capturing ? 'warning' : 'primary';
    },
    beginCaptureButtonText(): string {
      return this.capturing ? 'END CAPTURE' : 'BEGIN CAPTURE';
    },

    // v-model enablers
    pid: {
      get(): number|null { return Preferences.pid; },
      set(val: number|null) {
        Preferences.setPid(val);
      },
    },
    selectedPreset: {
      get(): Preset|null { return Preferences.preferences.selectedPreset; },
      set(val: Preset|null) {
        Preferences.writeAttribute({attr: 'selectedPreset', val});
      }
    },
    captureDuration: {      
      get(): number { return Preferences.preferences.captureDuration; },
      set(val: number|null) {
        Preferences.writeAttribute({attr: 'captureDuration', val: Math.max(val ?? 1, 1)});
      }
    },
    enableCaptureDuration: {      
      get(): boolean { return Preferences.preferences.enableCaptureDuration; },
      set(val: boolean) {
        Preferences.writeAttribute({attr: 'enableCaptureDuration', val});
      }
    },
    enableAutotargetting: {      
      get(): boolean { return Preferences.preferences.enableAutotargetting; },
      set(val: boolean) {
        Preferences.writeAttribute({attr: 'enableAutotargetting', val});
      }
    },
  },
  watch: {
    async pid(newPid: number|null) {
      cancelTopPolling();
      if (newPid !== null) {
        if (this.processes.find(p => p.pid == newPid) == null) {
          await Processes.refresh();
        }
      }
      else {
        if (this.enableAutotargetting) {
          launchAutotargetting();
        }
      }
    },
    enableAutotargetting(enabled: boolean) {
      if (enabled && this.pid === null) {
        launchAutotargetting();
      }
      else {
        cancelTopPolling();
      }
    },
  }
});
</script>

<style lang="scss" scoped>
.pid-node {
  font-size: 10px;
  color: grey;
  padding-left: 2px;
}
.pid-node-inline {
  font-size: 12px;
  color: grey;
  padding-left: 8px;
}
.page-card {
  margin: 15px 0;
  padding: 0 15px 15px;
}
.page-wrap {
  max-width: 1024px;
}
.stepper-hilight.stepper-hilight.stepper-hilight.stepper-hilight {
  border:1px solid white;
  box-shadow:
    0 0 2px 1px hsl(125, 100%, 88%),  /* inner white */
    0 0 6px 4px hsl(84, 100%, 59%), /* middle green */
    0 0 9px 5px hsl(266, 100%, 59%); /* outer cyan */
}
.hilight-info {
  color: greenyellow;
}
.settings-link {
  font-size: 24px;
  color: #CCC;
  text-decoration: none;
  &:hover {
    color: #FFF
  }
  transition: color .3s;

  i.v-icon.v-icon {
    color: inherit;
  }
}
</style>

