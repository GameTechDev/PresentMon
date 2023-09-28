<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="flex-grow-1 flex-column mx-lg-12 mx-xl-16">
    
    <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
        <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
        Loadout Configuration
    </h2>

    <v-row class="mt-5 loadout-table" id="sortable-row-container">
      <loadout-row
        v-for="(w, i) in widgets" :key="w.key" 
        :widgetIdx="i" :widgets="widgets" :metrics="metrics" 
        :metricOptions="metricOptions" :locked="false" 
        @delete="removeWidget" 
      ></loadout-row>
      <div class="add-btn-row">
        <v-btn @click="addWidget()" class="add-btn">Add New Widget</v-btn>
      </div>
    </v-row>

    <v-row>
      <v-col cols="6" style="text-align: center"><v-btn @click="save()">Save</v-btn></v-col>
      <v-col cols="6" style="text-align: center"><v-btn @click="load()">Load</v-btn></v-col>
    </v-row>
  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import Sortable from 'sortablejs'
import { Preferences } from '@/store/preferences'
import { Preferences as PrefType } from '@/core/preferences'
import { Loadout } from '@/store/loadout'
import { Metrics } from '@/store/metrics'
import { Metric, MetricOption } from '@/core/metric'
import { Widget } from '@/core/widget'
import { Api } from '@/core/api'
import { Notifications } from '@/store/notifications'
import LoadoutRow from '@/components/LoadoutRow.vue'

export default Vue.extend({
  name: 'LoadoutConfig',
  components: {
    LoadoutRow,
  },

  data: () => ({
  }),

  mounted() {
    new Sortable(document.querySelector('#sortable-row-container')!, {
      draggable: '.sortable-row',
      handle: '.sortable-handle',
      forceFallback: true,
      onChoose: e => e.target.classList.add('sortable-grabbing'),
      onUnchoose: e => e.target.classList.remove('sortable-grabbing'),
      onStart: e => e.target.classList.add('sortable-grabbing'),
      onEnd: e => {
          e.target.classList.remove('sortable-grabbing');
          this.dragReorder(e);
      },
    });
  },
  methods: {
    async save() {
      try {
        await Loadout.browseAndSerialize();
      } catch (e) {
        await Notifications.notify({text: 'Failed to save loadout.'});
      }
    },
    async load() {
      try {
        const browseResult = await Api.browseReadSpec();
        // zero length result means user canceled out of dialog
        if (browseResult.payload.length > 0) {
          await Loadout.parseAndReplace(browseResult);
          await Loadout.serializeCustom();
        }
      } catch (e) {
        await Notifications.notify({text:'Failed to load config file.'});
        console.error(['Failed to load config file.', e]);
      }
    },
    addWidget() {
      Loadout.addGraph();
    },
    removeWidget(widgetIdx: number) {
      Loadout.removeWidget(widgetIdx);
    },
    dragReorder(e: Sortable.SortableEvent) {
      if (e.oldIndex !== undefined && e.newIndex !== undefined) {
        Loadout.moveWidget({from: e.oldIndex, to: e.newIndex});
      }
    }
  },  
  computed: {
    pref(): PrefType {
      return Preferences.preferences;
    },
    metrics(): Metric[] {
      return Metrics.metrics;
    },
    widgets(): Widget[] {
      return Loadout.widgets;
    },
    metricOptions(): MetricOption[] {
      // maps a cat+name string to the option, used for filtering and merging stats
      const visited = new Map<string, MetricOption>();
      // map metrics to options and make unique (filter out stat variations, add them to array)
      return this.metrics
        .map((m, i) => ({name: `${m.name}`, metricIds: [i], className: m.className} as MetricOption))
        .filter(o => {
          const existing = visited.get(o.name);
          if (existing) {
            existing.metricIds.push(o.metricIds[0]);
            return false;
          } else {
            visited.set(o.name, o);
            return true;
          }
        });
    },
    selectedPreset: {
      set(preset: number|null) {
        Preferences.writeAttribute({attr: 'selectedPreset', val: preset});
      },
      get(): number|null {
        return Preferences.preferences.selectedPreset;
      }
    },
  },
});
</script>

<style lang="scss">
.loadout-table { 
  user-select: none; 
  .sortable-handle {
    cursor: grab;
  }
  .sortable-handle:active {
    cursor: grabbing !important;
  }
  .sortable-ghost {
      background-color: #444 !important;
  }
  .widget-btn, .line-btn {
    visibility: hidden;
    opacity: 0.4;
  }
  .widget-row:hover .widget-btn {
    visibility: visible;
  }
  .widget-row:hover .widget-btn.remove-btn:hover {
    opacity: 1;
    color: red;
  }
  .widget-row:hover .widget-btn.add-line-btn:hover {
    opacity: 1;
    color: greenyellow;
  }
  .widget-row:hover .widget-btn.details-btn:hover {
    opacity: 1;
    color: lightskyblue;
  }
}
.widget-row {
  width: 100%;
  display: flex;
  margin: 3px;
  border: 1px solid rgb(32, 32, 32);
  border-radius: 3px;
  background-color: rgb(26, 26, 26);
}
.grip-wrap {
  flex-grow: 0;
  align-self: center;
  padding-left: 2px;
}
.line-wrap {
  flex-grow: 1;
}
.widget-line {
  display: flex;
  width: 100%;
  &:hover .line-btn {    
    visibility: visible;
  }
  &:hover .line-btn:hover {
    opacity: 1;
    color: red;
  }
}
.widget-row .widget-cell {
  margin: 2px;
  padding: 2px;
}
.widget-row .widget-cell.col-metric {
  flex: 3;
}
.widget-row .widget-cell.col-stat {
  width: 90px;
}
.widget-row .widget-cell.col-type {
  width: 120px;
}
.widget-row .widget-cell.col-subtype {
  width: 140px;
}
.widget-row .widget-cell.col-line-color {
  max-width: 50px;
  flex: 0.7;
  display: flex;
  flex-direction: column;
}
.widget-row .widget-cell.col-controls {
  width: 160px;
  vertical-align: middle;
}
.add-btn-row {
  width: 100%;
  display: flex;
  justify-content: center;
}
.sortable-grabbing * {
  cursor: grabbing !important;
}
button.v-btn.add-btn.add-btn {
  padding: 24px 96px;
  margin: 8px
}
.link-head { 
  color: white; 
  cursor: pointer; 
  user-select: none; 
  transition: color .2s; 
} 
.link-head:hover { 
  color: #2196f3; 
} 
</style>

