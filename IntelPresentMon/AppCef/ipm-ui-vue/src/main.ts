import '@/assets/global.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'

// Vuetify
import { createVuetify } from 'vuetify'
import 'vuetify/styles'
// css
import '@mdi/font/css/materialdesignicons.css'
import '@fontsource/roboto/index.css'

import App from '@/App.vue'
import router from '@/router'
import { isDevBuild } from './core/env-vars'
import { md2 } from 'vuetify/blueprints'

import { useHotkeyStore } from '@/stores/hotkey';
import { useIntrospectionStore } from '@/stores/introspection';
import { useAdaptersStore } from './stores/adapters'
import { loadBlocklists } from './core/block-list'
import { usePreferencesStore } from './stores/preferences'

if (isDevBuild()) {
    const script = document.createElement('script');
    script.src = 'http://localhost:8098';
    document.head.appendChild(script);
}

const vuetify = createVuetify({
    blueprint: md2,
    theme: {
        defaultTheme: 'customDark',
        themes: {
            customDark: {
                dark: true,
                colors: {
                    primary: '#1976D2',   // Vuetify 2 blue
                    secondary: '#424242', // Vuetify 2 grey
                    accent: '#82B1FF',
                    error: '#FF5252',
                    info: '#2196F3',
                    success: '#4CAF50',
                    warning: '#FFC107',
                    background: '#121212',
                    surface: '#1E1E1E',
                },
            },
        },
    },
    defaults: {
        global: {
            hideDetails: true,
        },
        VSlider: {
            color: "primary",
            thumbSize: 16,
            density: "compact",
            step: 1,
        },
        VRangeSlider: {
            color: "primary",
            thumbSize: 16,
            density: "compact",
            step: 1,
        },
        VTextField: {
            color: "primary",
            variant: "outlined",
            density: "compact",
        },
        VSwitch: {
            color: "primary",
        },
        VSelect: {
            color: "primary",
            variant: "outlined",
            density: "compact",
        },
        VAutocomplete: {
            color: "primary",
            variant: "outlined",
            density: "compact",
        },
        VNumberInput: {
            color: "primary",
            variant: "outlined",
            density: "compact",
            controlVariant: "hidden",
        },
        VTooltip: {
            openDelay: 750,
            location: "bottom",
            openOnHover: true,
        }
    }
})

async function initStores() {
    await Promise.all([
        useHotkeyStore().refreshOptions(),
        useIntrospectionStore().load(),
        useAdaptersStore().refresh(),
        loadBlocklists(),
    ])
    await usePreferencesStore().initPreferences()
}

var app:any;
async function boot() {
    app = createApp(App)
    app.use(createPinia())
    await initStores()
    app.use(router)
    app.use(vuetify)
    await router.isReady()
    app.mount('#app')
}

boot()