import { createMemoryHistory, createRouter } from 'vue-router'
import MainView from '@/views/MainView.vue'
import LoadoutConfigView from '@/views/LoadoutConfigView.vue'
import GraphConfigView from '@/views/GraphConfigView.vue'
import ReadoutConfigView from '@/views/ReadoutConfigView.vue'
import DataConfigView from '@/views/DataConfigView.vue'
import OverlayConfigView from '@/views/OverlayConfigView.vue'
import CaptureConfigView from '@/views/CaptureConfigView.vue'
import FlashConfigView from '@/views/FlashConfigView.vue'
import OtherConfigView from '@/views/OtherConfigView.vue'

const router = createRouter({
  history: createMemoryHistory(),
  routes: [
    {
      path: '/',
      redirect: { name: 'main' },
    },
    {
      path: '/main',
      name: 'main',
      component: MainView,
    },
    {
      path: '/graphs/:index',
      name: 'graph-config',
      component: GraphConfigView,
      props: route => ({ index: Number(route.params.index) }),
    },
    {
      path: '/readouts/:index',
      name: 'readout-config',
      component: ReadoutConfigView,
      props: route => ({ index: Number(route.params.index) }),
    },
    {
      path: '/overlay',
      name: 'overlay-config',
      component: OverlayConfigView,
    },
    {
      path: '/data',
      name: 'data-config',
      component: DataConfigView,
    },
    {
      path: '/loadout',
      name: 'loadout-config',
      component: LoadoutConfigView,
    },
    {
      path: '/capture',
      name: 'capture-config',
      component: CaptureConfigView,
    },
    {
      path: '/other',
      name: 'other-config',
      component: OtherConfigView,
    },
    {
      path: '/flash',
      name: 'flash-config',
      component: FlashConfigView,
    },
  ],
})

export default router
