<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->
  
<script setup lang="ts">
import { usePreferencesStore } from '@/stores/preferences';
import OverlayPositioner from '@/components/OverlayPositioner.vue';
import ColorPicker from '@/components/ColorPicker.vue';
const prefs = usePreferencesStore();
</script>

<template>
<div class="page-wrap">
    
    <h2 class="mt-5 ml-5 header-top">
        Overlay Configuration
    </h2>

    <v-card class="page-card">

        <v-row class="mt-5">
        <v-col cols="3">
            Windowed Mode
            <p class="text-medium-emphasis text-caption mb-0">Display widgets on a standalone window instead of an overlay tracking the target</p>
        </v-col>
        <v-col cols="9">
            <v-row>
                <v-col cols="6">
                    <v-switch v-model="prefs.preferences.independentWindow" label="Enable"></v-switch>
                </v-col>
            </v-row>
        </v-col>
        </v-row>

        <v-row class="mt-8">
        <v-col cols="3">
            Automatic Hide
            <p class="text-medium-emphasis text-caption mb-0">Automatically disable the overlay during capture</p>
        </v-col>
        <v-col cols="9">
            <v-row>
                <v-col cols="6">
                    <v-switch v-model="prefs.preferences.hideDuringCapture" label="Enable"></v-switch>
                </v-col>
            </v-row>
        </v-col>
        </v-row>

        <v-row class="mt-8">       
        <v-col cols="3">
            Position
            <p class="text-medium-emphasis text-caption mb-0">Where the overlay appears on the target window</p>
        </v-col>
        <v-col cols="9">
            <overlay-positioner v-model="prefs.preferences.overlayPosition"></overlay-positioner>
        </v-col>
        </v-row>

        <v-row class="mt-8">
        <v-col cols="3">
            Width
            <p class="text-medium-emphasis text-caption mb-0">Width of the overlay window (height determined by content)</p>
        </v-col>
        <v-col cols="9">
            <v-row>
                <v-col cols="12">
                    <v-slider
                        v-model="prefs.preferences.overlayWidth"
                        :max="1920"
                        :min="200"
                        thumb-label="always"
                    ></v-slider>
                </v-col>
            </v-row>
        </v-col>
        </v-row>

        <v-row class="mt-8">
        <v-col cols="3">
            Time Scale
            <p class="text-medium-emphasis text-caption mb-0">Range of time (s) displayed on graphs' x-axes. Controls the scrolling speed.</p>
        </v-col>
        <v-col cols="9">
            <v-slider
            v-model="prefs.preferences.timeRange"
            :max="10"
            :min="0.1"
            :step="0.1"
            thumb-label="always"
            ></v-slider>
        </v-col>
        </v-row>

        <v-row class="mt-8">
        <v-col cols="3">
            Graphics Scaling
            <p class="text-medium-emphasis text-caption mb-0">Upscale overlay graphics to make text more readable on high DPI displays</p>
        </v-col>
        <v-col cols="9">
            <v-row>
                <v-col cols="4">
                    <v-switch v-model="prefs.preferences.upscale" label="Enable"></v-switch>
                </v-col>
                <v-col cols="8">
                    <v-slider
                        class="mt-3"
                        label="Factor"
                        v-model="prefs.preferences.upscaleFactor"
                        :max="5"
                        :min="1"
                        :step="0.1"
                        :disabled="!prefs.preferences.upscale"
                        thumb-label="always"
                    ></v-slider>
                </v-col>
            </v-row>
        </v-col>
        </v-row>

        <v-row class="mt-8">       
        <v-col cols="3">
            Draw Rate
            <p class="text-medium-emphasis text-caption mb-0">Rate at which to draw the overlay</p>
        </v-col>
        <v-col cols="9">
            <v-slider
                v-model="prefs.preferences.overlayDrawRate"
                :max="120"
                :min="1"
                thumb-label="always"
            ></v-slider>
        </v-col>
        </v-row>

        <v-row class="mt-8">       
        <v-col cols="3">
            Background Color
            <p class="text-medium-emphasis text-caption mb-0">Control background color of entire overlay</p>
        </v-col>
        <v-col cols="3">
            <color-picker v-model="prefs.preferences.overlayBackgroundColor" class="color-picker" label="Background"></color-picker>
        </v-col>
        </v-row>

    </v-card>

</div>
</template>

<style scoped>
.top-label {
    margin: 0;
    padding: 0;
    height: auto;
}
.header-top {
color: white;
user-select: none;
}
.page-card {
margin: 15px 0;
padding: 0 15px 15px;
}
.page-wrap {
max-width: 750px;
flex-grow: 1;
}
</style>
