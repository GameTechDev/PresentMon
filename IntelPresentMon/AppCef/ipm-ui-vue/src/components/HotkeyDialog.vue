<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->
<script lang="ts" setup>
import { ref, computed, watch } from 'vue';
import type { Combination, KeyOption, ModifierOption, ModifierCode, KeyCode } from '@/core/hotkey';
import { useHotkeyStore } from '@/stores/hotkey';

// options
defineOptions({ name: 'HotkeyDialog'})
interface Props {
  modelValue: Combination|null,
  name: string,
}
const props = defineProps<Props>()
const emit = defineEmits<{
  (e: 'update:modelValue', val: Combination | null): void
}>()
// stores
const hotkeys = useHotkeyStore()
// state
const active = ref(false)
const key_ = ref<KeyCode|null>(null)
const modifiers_ = ref<ModifierCode[]>([])

// === Computed State ===
const keyOptions = computed<KeyOption[]>(() => hotkeys.keyOptions)
const modifierOptions = computed<ModifierOption[]>(() => hotkeys.modifierOptions)

// === Computed validation ===
const combinationValid = computed(() => key_.value !== null)
const combinationSet = computed(() => props.modelValue !== null)

// === Watchers ===
// Sync local state when dialog opens
watch(active, (isOpen) => {
  if (isOpen) {
    if (props.modelValue) {
      key_.value = props.modelValue.key
      modifiers_.value = [...props.modelValue.modifiers]
    } else {
      key_.value = null
      modifiers_.value = []
    }
  }
})

// === Methods ===
function show() {
  active.value = true
}

function cancel() {
  active.value = false
}

function submit() {
  const combination: Combination = {
    key: key_.value as KeyCode,
    modifiers: modifiers_.value,
  }
  emit('update:modelValue', combination)
  active.value = false
}

function clear() {
  emit('update:modelValue', null)
  active.value = false
}

// === Expose show() to HotkeyButton ===
defineExpose({ show })
</script>

<template>
  <v-dialog
    v-model="active"
    max-width="680"
    overlay-color="black"
    persistent
  >
    <v-card>
      <v-card-title>
        <span class="text-h6 text-grey">Hotkey for <span class="text-blue-lighten-3">{{ name }}</span></span>
      </v-card-title>
      <v-card-text>
        <v-row>
          <v-col :cols="6">
            <v-select
              :items="modifierOptions"
              v-model="modifiers_"
              label="Modifiers"
              item-title="text"
              item-value="code"
              variant="outlined"
              multiple
              hide-details
              color="primary"
            >
            </v-select>
          </v-col>
          <v-col :cols="6">
            <v-select
              :items="keyOptions"
              v-model="key_"
              label="Key"
              item-title="text"
              item-value="code"
              variant="outlined"
              clearable
              hide-details
              color="primary"
            >
            </v-select>
          </v-col>
        </v-row>
      </v-card-text>      
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn
            color="red-darken-2"
            variant="text"
            @click="clear"
            :disabled="!combinationSet"
          >
            Clear
          </v-btn>
          <v-btn
            color="white-darken-3"
            variant="text"
            @click="cancel"
          >
            Cancel
          </v-btn>
          <v-btn
            color="primary"
            variant="text"
            @click="submit"
            :disabled="!combinationValid"
          >
            OK
          </v-btn>
        </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<style lang="scss" scoped>
</style>

