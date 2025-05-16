<!-- Copyright (C) 2023 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->
<script lang="ts" setup>
import { computed } from 'vue';
import { ref, nextTick } from 'vue';
import type { Combination, ModifierCode, KeyCode } from '@/core/hotkey';
import { Action } from '@/core/hotkey';
import HotkeyDialog from './HotkeyDialog.vue';
import { useHotkeyStore } from '@/stores/hotkey';

// options
defineOptions({ name: 'HotkeyButton'})
interface Props {
  action: number
}
const props = defineProps<Props>()
const emit = defineEmits<{ (e: 'update:modelValue', value: Combination | null): void }>();
// element refs
const dialogRef = ref<InstanceType<typeof HotkeyDialog>|null>(null)
// stores
const hotkeys = useHotkeyStore()

// call exposed function with necessary delay
async function openHotkeyDialog() {
  // we need to delay showing of the dialog until the computed properties have caught up
  // otherwise the wrong combination will be captured internally by the dialog state
  await nextTick();
  dialogRef.value?.show();
}
// reformat the action enum name for display (add spaces)
function getHotkeyActionName(action: Action): string {
  const key = Action[action];
  return key.match(/([A-Z]?[^A-Z]*)/g)?.slice(0, -1).join(" ") ?? "";
}
// lookup modifier name via code
function getHotkeyModifierName(mod: ModifierCode): string {
  return hotkeys.modifierOptions.find(mo => mo.code === mod)?.text ?? '???';
}
// lookup hotkey name via code
function getHotkeyKeyName(key: KeyCode): string {
  return hotkeys.keyOptions.find(ko => ko.code === key)?.text ?? '???';
}

// computed
const actionName = computed(() => getHotkeyActionName(props.action))
const hotkeyCombination = computed({
  get() {
    return hotkeys.bindings[Action[props.action]].combination;
  },
  set(value: Combination | null) {
    if (value !== null) {
      hotkeys.bindHotkey({
        action: props.action,
        combination: value,
      });
    }
    else {
      hotkeys.clearHotkey(props.action)
    }
  }
});
</script>

<template>
  <!-- TODO: this could probably be rolled up with HotkeyDialog as one component under Vuetify 3 -->
<div class="hot-border" @click="openHotkeyDialog">
    <div v-if="hotkeyCombination != null" class="hot-combo">
        <div v-for="m in hotkeyCombination.modifiers" :key="m" class="hot-mod">
            <div class="hot-key">{{ getHotkeyModifierName(m) }}</div>
            <v-icon color="secondary" small>mdi-plus</v-icon>
        </div>
        <div class="hot-key">
            {{ getHotkeyKeyName(hotkeyCombination.key) }}
        </div>
    </div>
    <div v-else class="text-grey">
        Select hotkey chord
    </div>

    <hotkey-dialog
        ref="dialogRef"
        :name="actionName"
        v-model="hotkeyCombination"
    ></hotkey-dialog>
</div>
</template>

<style lang="scss" scoped>
.hot-border {
  display: flex;
  justify-content: center;
  width: 300px;
  height: 50px;
  padding: 10px 10px;
  border-radius: 5px;
  border: solid rgb(var(--v-theme-secondary)) 2px;
  margin: 0;
  user-select: none;
  cursor: pointer;
  transition: border-color .4s;
  &:hover {
    background-color: hsl(240, 1%, 18%);
    border-color: color-mix(in srgb, var(--v-theme-secondary) 80%, white);
  }
}
.hot-combo {
  display: flex;
  color: rgb(var(--v-theme-primary));
  font-size: 12px
}
.hot-mod {
  display: flex;
  align-items: center;
}
.hot-key {
  border: solid rgb(var(--v-theme-primary)) 1.5px;
  padding: 2px 7px;
  border-radius: 5px;
  min-width: 30px;
  text-align: center;
}
</style>
