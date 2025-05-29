
Some metrics are enabled or disabled depending on the command line options:

| Command line option  | Enabled metrics | Disabled metrics |
| -------------------- | --------------- | ---------------- |
| `--track_frame_type` | *FrameType*     | |
| `--track_gpu_video`  | *VideoBusy*     | |
| `--no_track_gpu`     |                 | *GPULatency<br>GPUBusy<br>GPUWait<br>VideoBusy*  |
| `--no_track_input`   |                 | *ClickToPhotonLatency* |
| `--no_track_display`<br>(requires `--no_track_gpu` and `--no_track_input` as well) | | *AllowsTearing<br>PresentMode<br>DisplayLatency<br>DisplayedTime* |
| `--v1_metrics`       |                 | Most of the above metrics.  See a [1.x README.md](https://github.com/GameTechDev/PresentMon/blob/v1.9.2/README.md#csv-columns) for details on Presentmon 1.x metrics. |
| `--v2_metrics`       |                 | Most of the above metrics.  See a [2.x README.md](https://github.com/GameTechDev/PresentMon/blob/v2.3.0/README-ConsoleApplication.md#csv-columns) for details on Presentmon 2.x metrics. |

The following values are used in the PresentMode column:

| PresentMode                           | Description |
| ------------------------------------- | ----------- |
| Hardware: Legacy Flip                 | Indicates the app took ownership of the screen, and is swapping the displayed surface every frame. |
| Hardware: Legacy Copy to front buffer | Indicates the app took ownership of the screen, and is copying new contents to an already-on-screen surface every frame. |
| Hardware: Independent Flip            | Indicates the app does not have ownership of the screen, but is still swapping the displayed surface every frame. |
| Composed: Flip                        | Indicates the app is windowed, is using ["flip model" swapchains](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model), and is sharing its surfaces with DWM to be composed. |
| Hardware Composed: Independent Flip   | Indicates the app is using ["flip model" swapchains](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model), and has been granted a hardware overlay plane. |
| Composed: Copy with GPU GDI           | Indicates the app is windowed, and is copying contents into a surface that's shared with GDI. |
| Composed: Copy with CPU GDI           | Indicates the app is windowed, and is copying contents into a dedicated DirectX window surface. GDI contents are stored separately, and are composed together with DX contents by the DWM. |

For more information on the performance implications of these, see:

- https://www.youtube.com/watch?v=E3wTajGZOsA
- https://software.intel.com/content/www/us/en/develop/articles/sample-application-for-direct3d-12-flip-model-swap-chains.html
