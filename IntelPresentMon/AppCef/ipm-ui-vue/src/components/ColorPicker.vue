<script setup lang="ts">
import { ref, computed, watchEffect } from 'vue';
import type { RgbaColor } from '@/core/color';
import { makeCssString } from '@/core/color';

defineOptions({name: 'ColorPicker'})
interface Props {
  label?: string,
  modelValue: RgbaColor,
  minimal?: boolean,
}
const props = withDefaults(defineProps<Props>(), {
  label: '', minimal: false
})
const emit = defineEmits<{
  (e: 'update:modelValue', val: RgbaColor): void
}>()

const menuActive = ref(false);
// TODO: try computed from prop instead of local ref data + watcher
const color = ref<RgbaColor>(props.modelValue)
watchEffect(() => {
  color.value = props.modelValue
})
const swatchStyle = computed(() => ({
    backgroundColor: makeCssString(props.modelValue),
}));
</script>

<template>
  <v-card color="#242426">
    <v-card-text class="d-flex flex-column align-center" :class="{'pa-0': minimal}">
      <label v-if="label.length > 0" class="v-label pb-3">{{ label }}</label>
      <v-menu v-model="menuActive" top :close-on-content-click="false">
        <template v-slot:activator="{ props }">
          <div class="swatch" :style="swatchStyle" v-bind="props"></div>
        </template>
        <v-card>
          <v-card-text class="pa-0">
            <v-color-picker @update:model-value="c => emit('update:modelValue', c)" :modelValue="color" flat></v-color-picker>
          </v-card-text>
        </v-card>
      </v-menu>
    </v-card-text>
  </v-card>
</template>

<style lang="scss" scoped>
.swatch {
  transition: border-color .1s;
  border: 1px solid #333;
  cursor: pointer;
  min-width: 30px;
  min-height: 18px;
  align-self: stretch;
  border-radius: 4px;
}
.swatch:hover {
  border: 1.5px solid #fff;
}
</style>