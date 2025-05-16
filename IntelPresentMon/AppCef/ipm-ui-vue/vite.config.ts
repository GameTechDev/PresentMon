import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import vuetify from 'vite-plugin-vuetify'
import type { Plugin } from 'rollup'
import path from 'node:path'

// Rollup plugin to keep only .woff2 fonts by dropping any font assets
// whose file name ends in .woff, .ttf or .eot
function keepOnlyWoff2(): Plugin {
  return {
    name: 'keep-only-woff2',
    generateBundle(_, bundle) {
      for (const fileName of Object.keys(bundle)) {
        // if it's one of the font extensions and NOT .woff2, delete it
        if (/\.(woff|ttf|eot)$/.test(fileName)) {
          delete bundle[fileName]
        }
      }
    }
  }
}


// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
    vuetify({autoImport: true}),
    keepOnlyWoff2(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  build: {
    rollupOptions: {
      output: {
        entryFileNames: 'js/app.js',
        chunkFileNames: 'js/chunk.js',
        // route assets by extension
        assetFileNames: assetInfo => {
          const ext = path.extname(assetInfo.name ?? '').toLowerCase()
          if (ext === '.css') {
            return 'css/[name][extname]'
          }
          if (ext === '.woff2') {
            return 'fonts/[name][extname]'
          }
          // fallback for anything else
          return 'assets/[name][extname]'
        }
      }
    }
  }
})
