<script lang="ts" setup>
import { computed, ref, watchEffect } from 'vue'
import { type ListItem } from 'vuetify/lib/composables/list-items.mjs';
import { type Process } from '@/core/process';
import { Action } from '@/core/hotkey';
import { Preset } from '@/core/preferences';
import HotkeyButton from '@/components/HotkeyButton.vue';
import { usePreferencesStore } from '@/stores/preferences';
import { useProcessesStore } from '@/stores/processes';
import { isBlocked } from '@/core/block-list';
import { cancelTopPolling, launchAutotargetting } from '@/core/autotarget';
import { Api } from '@/core/api';

defineOptions({name: 'MainView'})

// === Stores ===
const prefs = usePreferencesStore()
const procs = useProcessesStore()

// === State ===
const loadingProcs = ref(false)

// match autocomplete typed text if substring of window name or process name or pid
function selectFilter(item: Process, query: string) {
    const winText = item.windowName?.toLowerCase();
    if (winText && winText.indexOf(query) > -1) {
        return true;
    }
    return item.name.toLowerCase().indexOf(query) > -1 ||
        item.pid.toString().indexOf(query) > -1;
}
// truncate long process/window names by eliding the middle part
function makeSelectorName(winName: string): string {
    const maxLen = 73;
    const leading = 30;
    const trailing = 40;
    if (winName.length > maxLen) {
     return winName.substr(0, leading) + '...' + winName.substr(-trailing);
    }
    else {
        return winName;
    }
}
// handle click on the capture explore button
async function handleCaptureExplore() {
    await Api.exploreCaptures()
}
// load the process list
async function loadProcesses() {
    loadingProcs.value = true
    await procs.refresh()
    loadingProcs.value = false
    
}

// === Computed ===
const processes = computed(() => {    
    if (prefs.preferences.enableTargetBlocklist) {
        return procs.processes.filter(proc => !isBlocked(proc.name));
    }
    else {
        return procs.processes;
    }
})

// === Watchers ===
// watching selected pid and autotargetting enabled state
watchEffect(async () => {
    const pid = prefs.pid
    if (pid !== null) {
        cancelTopPolling()
        // if the top reported process is not in current list, refresh the list
        if (procs.processes.find(p => p.pid == pid) == null) {
            await procs.refresh()
        }
    }
    else {
        if (prefs.preferences.enableAutotargetting) {
            launchAutotargetting((pid: number) => {
                prefs.pid = pid
            })
        }
        else {
            cancelTopPolling()
        }
    }
})
</script>


<template>
<div class="page-wrap">

    <v-card class="page-card my-5 pt-3">
    <v-row>
        <v-col cols="3">
        Process
        <p class="text-medium-emphasis text-caption mb-0">Application process to track, overlay and capture</p>
        </v-col>
        <v-col cols="9" class="d-flex align-center">
        <v-autocomplete
            :items="processes"
            v-model="prefs.pid"
            item-value="pid"
            :filter="selectFilter"
            label="Process"
            :loading="loadingProcs"
            @click="loadProcesses"
            append-icon=""
            :disabled="prefs.preferences.enableAutotargetting"
            clearable
        >
            <template v-slot:selection="{item, index}: {item:ListItem<Process>, index:number}">
                <template v-if="item.raw.windowName">
                    {{ makeSelectorName(item.raw.windowName) }} 
                    <span class="pid-node-inline">[{{ item.raw.pid }}]</span>
                </template>            
                <template v-else>
                    <div>
                    {{ item.raw.name }}
                    <span class="pid-node-inline">[{{ item.raw.pid }}]</span>
                    </div>
                </template>
            </template>
            <template v-slot:item="{item, props, index}: {item:ListItem<Process>, props:any, index:number}">
                <v-list-item v-if="item.raw.windowName" v-bind="props" :title="makeSelectorName(item.raw.windowName)">
                    <v-list-item-subtitle>
                        {{ item.raw.name }}
                        <span class="pid-node">[{{ item.raw.pid }}]</span>
                    </v-list-item-subtitle>
                </v-list-item>
                <v-list-item v-else v-bind="props" :title="undefined">
                    <v-list-item-title>
                        {{ makeSelectorName(item.raw.name) }}
                        <span class="pid-node-inline">[{{ item.raw.pid }}]</span>
                    </v-list-item-title>
                </v-list-item>
            </template>
        </v-autocomplete>
        </v-col>
    </v-row> 
    
    <v-row dense>       
        <v-col cols="3">
        Auto-target
        <p class="text-medium-emphasis text-caption mb-0">Automatically target process with the highest GPU utilization</p>
        </v-col>

        <v-col cols="9" class="d-flex align-center">
        <v-switch label="Enable" v-model="prefs.preferences.enableAutotargetting"></v-switch>
        </v-col>
    </v-row>   
    
    <v-row dense>       
        <v-col cols="3">
        Overlay Hotkey
        <p class="text-medium-emphasis text-caption mb-0">Set hotkey to toggle overlay on/off</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="Action.ToggleOverlay"></hotkey-button>
        </v-col>
    </v-row>
    </v-card>
    
    <v-card class="page-card my-5 pt-3">
    <v-row>       
        <v-col cols="3">
        Preset
        <p class="text-medium-emphasis text-caption mb-0">Select a preset configuration for overlay widget loadout etc.</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">        
        <v-btn-toggle v-model="prefs.preferences.selectedPreset" :mandatory="prefs.preferences.selectedPreset !== null" variant="outlined" divided>
            <v-btn class="px-5" large>
            Basic
            </v-btn>

            <v-btn class="px-5" large>
            GPU Focus
            </v-btn>

            <v-btn class="px-5" large>
            Power/Temp
            </v-btn>

            <v-btn class="px-5" large :value="Preset.Custom">
            Custom
            </v-btn>        
        </v-btn-toggle>
        <v-btn
            :to="{name: 'loadout-config'}"
            :disabled="prefs.preferences.selectedPreset !== Preset.Custom"
            color="primary" class="ml-5"
        >
            Edit
        </v-btn>
        </v-col>
    </v-row>

    <!-- minimal hotkey component front -->
    <v-row dense>       
        <v-col cols="3">
        Preset Cycle Hotkey
        <p class="text-medium-emphasis text-caption mb-0">Set hotkey for cycling through presets</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">    
        <hotkey-button :action="Action.CyclePreset"></hotkey-button>
        </v-col>  
    </v-row>
    </v-card>
    
    <v-card class="page-card my-5 pt-3">

    <v-row>
        <v-col cols="3">
        Capture Duration
        <p class="text-medium-emphasis text-caption mb-0">Automatically stop capture after N seconds</p>
        </v-col>      
        <v-col cols="2">
            <v-switch v-model="prefs.preferences.enableCaptureDuration" color="primary" label="Enable" hide-details></v-switch>
        </v-col>
        <v-col cols="3">
        <v-text-field
            label="Seconds"
            v-model="prefs.preferences.captureDuration"
            :disabled="!prefs.preferences.enableCaptureDuration"
            class="mt-4 ml-8"
            hide-details
            type="number"
            hide-spin-buttons
        ></v-text-field>
        </v-col>
    </v-row>

    <!-- minimal hotkey component front -->
    <v-row dense>       
        <v-col cols="3">
        Capture Hotkey
        <p class="text-medium-emphasis text-caption mb-0">Set hotkey for capture of per-frame performance data as CSV</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="Action.ToggleCapture"></hotkey-button>
        </v-col>
    </v-row>

    </v-card>
    
    <v-card class="page-card my-5 pt-3">
    <v-row>       
        <v-col cols="3">
        Capture Storage
        <p class="text-medium-emphasis text-caption mb-0">Open the folder containing all frame traces and stats summaries</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <v-btn 
            large
            color="secondary"
            class="px-6"
            @click="handleCaptureExplore"
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

    