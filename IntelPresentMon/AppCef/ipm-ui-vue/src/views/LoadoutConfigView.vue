<script setup lang="ts">
import Sortable from 'sortablejs';
import { useIntrospectionStore } from '@/stores/introspection';
import type { Widget } from '@/core/widget';
import LoadoutRow from '@/components/LoadoutRow.vue';
import { onMounted, onUnmounted, ref } from 'vue';
import { useLoadoutStore } from '@/stores/loadout';
import { useNotificationsStore } from '@/stores/notifications';
import { Api } from '@/core/api';

defineOptions({name: 'LoadoutConfigView'})

const intro = useIntrospectionStore();
const loadout = useLoadoutStore()
const notes = useNotificationsStore()

const activeAdapterId = ref<number|null>(null);
// Sortable instance used to manage drag-and-drop reordering for widget rows.
// This is created on mount and destroyed on unmount to avoid stale DOM references.
let sort: Sortable|null = null;

// Convert Sortable event indices into validated array indices.
// Sortable can report undefined (no drop), negative indices (drag outside),
// or out-of-range indices when the drop target is not inside the container.
// allowEnd controls whether we accept an index equal to length (append).
const getSortableIndex = (value: number | null | undefined, length: number, allowEnd: boolean): number | null => {
  if (typeof value !== 'number' || !Number.isInteger(value)) {
    return null;
  }
  if (value < 0) {
    return null;
  }
  if (allowEnd) {
    return value <= length ? value : null;
  }
  return value < length ? value : null;
};

// Restore DOM order to match the store order when a drag ends in an invalid state.
// This avoids the UI showing a different order than the canonical loadout data.
const resetSortOrder = () => {
  if (!sort) {
    return;
  }
  const order = loadout.widgets.map(w => String(w.key));
  if (order.length === 0) {
    return;
  }
  sort.sort(order);
};

onMounted(() => {
  // hook up the Sortable.js drag and drop machinery to our elements
  sort = new Sortable(document.querySelector('#sortable-row-container')!, {
    // Allow only the widget rows (not the add button or other elements) to be draggable.
    draggable: '.sortable-row',
    // Require dragging from the grip icon to avoid accidental drags while editing fields.
    handle: '.sortable-handle',
    // Use the fallback drag implementation so we control visuals consistently across browsers.
    forceFallback: true,
    // Identify each row by data-id so Sortable can reorder by stable widget keys.
    dataIdAttr: 'data-id',
    // Maintain a grabbing cursor on the dragged element for consistent feedback.
    onChoose: e => e.target.classList.add('sortable-grabbing'),
    onUnchoose: e => e.target.classList.remove('sortable-grabbing'),
    onStart: e => e.target.classList.add('sortable-grabbing'),
    onEnd: e => {
        // Always clear the grabbing cursor and attempt to apply the reorder.
        e.target.classList.remove('sortable-grabbing')
        dragReorder(e)
    },
  })
})

// Apply the reorder to the store if indices are valid, otherwise revert DOM order.
const dragReorder = (e: Sortable.SortableEvent) => {
  const length = loadout.widgets.length;
  // oldIndex must be a valid index into the current list (no "append" allowed).
  const from = getSortableIndex(e.oldIndex, length, false);
  // newIndex allows append, but still must be within bounds.
  const to = getSortableIndex(e.newIndex, length, true);
  if (from === null || to === null || from === to) {
    resetSortOrder();
    return;
  }
  // Delegate to the store to ensure any other logic stays centralized.
  loadout.moveWidget(from, to);
}

onUnmounted(() => {
  // Destroy Sortable on teardown to avoid handlers firing on a stale DOM.
  if (sort) {
    sort.destroy();
    sort = null;
  }
});

async function save() {
  try {
    await loadout.browseAndSerialize()
  }
  catch (e) {
    console.error('Error saving loadout:', e)
    notes.notify({ text: 'An error occurred while saving the loadout. Please try again.' })
  }
};

async function load() {
  try {
    let {payload} = await Api.browseReadSpec();
    // zero length result means user canceled out of dialog
    if (payload.length > 0) {
      const err = 'Failed to load preset file. ';
      await loadout.loadConfigFromPayload(payload, err);
    }
  }
  catch (e) {
    console.error('Error loading loadout:', e)
    notes.notify({ text: 'An error occurred while loading the loadout. Please try again.' })
  }
};

const addWidget = () => {
  loadout.addGraph()
};

const removeWidget = (widgetIdx:number) => {
  loadout.removeWidget(widgetIdx)
};
</script>

<template>
  <div class="flex-grow-1 flex-column mx-lg-12 mx-xl-16">
    <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
        <v-icon size="22" color="inherit">mdi-chevron-left</v-icon>
        Loadout Configuration
    </h2>

    <v-row class="mt-5 loadout-table" id="sortable-row-container">
    <loadout-row
        v-for="(w, i) in loadout.widgets" :key="w.key" :data-id="w.key" :stats="intro.stats"
        :widgetIdx="i" :widgets="loadout.widgets" :metrics="intro.metrics" 
        :metricOptions="intro.metricOptions" :adapterId="activeAdapterId" :locked="false" 
        @delete="removeWidget" 
    ></loadout-row>
    </v-row>
    <div class="add-btn-row">
        <v-btn @click="addWidget()" class="add-btn" variant="tonal" height="48" color="white">Add New Widget</v-btn>
    </div>

    <v-row>
    <v-col cols="6" style="text-align: center"><v-btn @click="save()" variant="tonal" color="white">Save</v-btn></v-col>
    <v-col cols="6" style="text-align: center"><v-btn @click="load()" variant="tonal" color="white">Load</v-btn></v-col>
    </v-row>
  </div>
</template>

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
    color: red !important;
  }
  .widget-row:hover .widget-btn.add-line-btn:hover {
    opacity: 1;
    color: rgb(47, 255, 64) !important;
  }
  .widget-row:hover .widget-btn.details-btn:hover {
    opacity: 1;
    color: rgb(92, 148, 253) !important;
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
    color: red !important;
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
  width: 110px;
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
  margin-top: 12px;
}
.sortable-grabbing * {
  cursor: grabbing !important;
}
.add-btn {
  margin: 8px
}
.link-head {
  display: inline-flex;
  align-items: center;
  color: white;
  cursor: pointer;
  user-select: none;
  transition: color .2s;
  gap: 4px;
}
.link-head:hover { 
  color: rgb(var(--v-theme-primary)); 
} 
</style>
