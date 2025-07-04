# PresentMon Console Application

The PresentMon/ directory contains source for a standalone console application that uses the
*PresentMon SDK* to capture and analyze graphics applications, outputting data to the console and/or
CSV file(s).

A binary of the console application is provided in the release, e.g.:
[PresentMon-2.3.1-x64.exe](https://github.com/GameTechDev/PresentMon/releases/download/v2.3.1/PresentMon-2.3.1-x64.exe).

## Command line options

| Capture Target Options         |     |
| ------------------------------ | --- |
| `--process_name name`          | Only record processes with the specified exe name.  This argument can be repeated to capture multiple processes. |
| `--exclude name`               | Do not record processes with the specified exe name.  This argument can be repeated to exclude multiple processes. |
| `--process_id id`              | Only record the process with the specified process ID. |
| `--etl_file path`              | Analyze an ETW trace log file instead of the actively running processes. |

| Output Options                 |     |
| ------------------------------ | --- |
| `--output_file path`           | Write CSV output to the specified path. |
| `--output_stdout`              | Write CSV output to STDOUT. |
| `--multi_csv`                  | Create a separate CSV file for each captured process. |
| `--no_csv`                     | Do not create any output CSV file. |
| `--no_console_stats`           | Do not display active swap chains and frame statistics in the console. |
| `--qpc_time`                   | Output the CPU start time as a performance counter value. |
| `--qpc_time_ms`                | Output the CPU start time as a performance counter value converted to milliseconds. |
| `--date_time`                  | Output the CPU start time as a date and time with nanosecond precision. |
| `--exclude_dropped`            | Exclude frames that were not displayed to the screen from the CSV output. |
| `--v1_metrics`                 | Output a CSV using PresentMon 1.x metrics. |
| `--v2_metrics`                 | Output a CSV using PresentMon 2.x metrics. |

| Recording Options              |     |
| ------------------------------ | --- |
| `--hotkey key`                 | Use the specified key press to start and stop recording.  'key' is of the form MODIFIER+KEY, e.g., "ALT+SHIFT+F11". |
| `--delay seconds`              | Wait for specified amount of time before starting to record.  If using --hotkey, the delay occurs each time the recording key is pressed. |
| `--timed seconds`              | Stop recording after the specified amount of time. |
| `--scroll_indicator`           | Enable scroll lock while recording. |
| `--track_gpu_video`            | Track the video encode/decode portion of GPU work separately from other engines. |
| `--no_track_display`           | Do not track frames all the way to display. |
| `--no_track_input`             | Do not track keyboard/mouse clicks impacting each frame. |
| `--no_track_gpu`               | Do not track the duration of GPU work in each frame. |

| Execution Options              |     |
| ------------------------------ | --- |
| `--session_name name`          | Use the specified session name instead of the default "PresentMon".  This can be used to start multiple captures at the same time, as long as each is using a distinct, case-insensitive name. |
| `--stop_existing_session`      | If a trace session with the same name is already running, stop the existing session before starting a new session. |
| `--terminate_existing_session` | If a trace session with the same name is already running, stop the existing session then exit. |
| `--restart_as_admin`           | If not running with elevated privilege, restart and request to be run as administrator. |
| `--terminate_on_proc_exit`     | Terminate PresentMon when all the target processes have exited. |
| `--terminate_after_timed`      | When using --timed, terminate PresentMon after the timed capture completes. |

| Beta Options                   |     |
| ------------------------------ | --- |
| `--track_frame_type`           | Track the type of each displayed frame; requires application and/or driver instrumentation using Intel-PresentMon provider. |
| `--track_hw_measurements`      | Tracks HW-measured latency and/or power data coming from a LMT and/or PCAT device. |
| `--track_app_timing`           | Track app timines for each displayed frame; requires application and/or driver instrumentation using Intel-PresentMon provider. |
| `--track_hybrid_present`       | Tracks if the present is a hybrid present and is performing a cross adapter copy. |
| `--track_pc_latency`           | Track app timines for each displayed frame; requires application instrumentation using PC Latency events. |

## Comma-separated value (CSV) file output

### CSV file names

By default, PresentMon creates a CSV file named "PresentMon-\<Time>.csv", where "\<Time>" is the
creation time in ISO 8601 format.  To specify your own output location, use the `--output_file PATH`
command line argument.

If `--multi_csv` is used, then one CSV is created for each process captured and
"-\<ProcessName>-\<ProcessId>" is appended to the file name.

If `--hotkey` is used, then one CSV is created for each time recording is started and "-\<Index>" is
appended to the file name.

### CSV columns

Each row of the CSV represents a frame that an application rendered and presented to the system for
display, and each column contains a particular metric value associated with that frame.  All time values
are in milliseconds.

Default metrics include:

| Column Header          | Description |
| ---------------------- | ----------- |
| *Application*          | The name of the process that generated the frame. |
| *ProcessID*            | The process ID of the process that generated the frame. |
| *SwapChainAddress*     | The address of the swap chain used to present the frame. |
| *PresentRuntime*       | The API used to present the frame. |
| *SyncInterval*         | The sync interval provided by the application when presenting the frame. Note: this value may be modified later by the driver, e.g., based on control panel overrides. |
| *PresentFlags*         | The present flags provided by the application when presenting the frame. |
| *AllowsTearing*        | 1 if partial frames might be displayed on the screen, or 0 if only full frames are displayed. |
| *PresentMode*          | The presentation mode used by the system for this frame.  See the table below for more details. |
| *FrameType*            | Whether the frame was rendered by the application or generated by a driver/SDK. |
| *CPUStartTime<br>(CPUStartQPC)<br>(CPUStartQPCTime)<br>(CPUStartDateTime)* | The time the CPU started working on this frame.  By default, this is the time since recording started unless:<br>&bull; When `--qpc_time` is used, the value is a [performance counter value](https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter) and the column is named *CPUStartQPC*.<br>&bull; When `--qpc_time_ms` is used, the value is the query performance counter value converted to milliseconds and the column is named *CPUStartQPCTime*.<br>&bull; When `--date_time` is used, the value is a date and the column is named *CPUStartDateTime*. |
| *MsCPUBusy*            | How long the CPU spent working on this frame before presenting it. |
| *MsCPUWait*            | How long the CPU spent waiting before starting the next frame. |
| *MsGPULatency*         | How long it took from the start of this frame until the GPU started working on it. |
| *MsGPUTime*            | The total amount of time that GPU was working on this frame. |
| *MsGPUBusy*            | How long the GPU was actively working on this frame (i.e., the time during which at least one GPU engine is executing work from the target process). |
| *MsGPUWait*            | How long the GPU was idle while working on this frame. |
| *VideoBusy*            | How long the GPU video encode/decode engines were actively working on this frame. |
| *DisplayLatency*       | How long it took from the start of this frame until the frame was displayed on the screen. |
| *DisplayedTime*        | How long the frame was displayed on the screen, or 'NA' if the frame was not displayed. |
| *MsAnimationError*     | The difference between the previous frame's CPU delta and display delta. |
| *AnimationTime*        | The time the CPU started animation work on this frame. |
| *MsClickToPhotonLatency* | How long it took from the earliest mouse click that contributed to this frame until this frame was displayed. |
| *MsAllInputToPhotonLatency* | How long it took from the earliest keyboard or mouse interaction that contributed to this frame until this frame was displayed. |
| *InstrumentedLatency*  | Instrumented Frame Start To Display Latency |
| *MsBetweenPresents*    | The time between this Present() call and the previous one, in milliseconds. |
| *MsInPresentAPI*       | The time spent inside the Present() call, in milliseconds. |
| *MsBetweenDisplayChange* | How long the previous frame was displayed before this Present() was displayed, in milliseconds. |
| *MsUntilDisplayed*     | The time between the Present() call and when the frame was displayed, in milliseconds. |
| *MsRenderPresentLatency* | The time between the Present() call and when GPU work for this frame completed, in milliseconds. |
| *MsBetweenSimulationStart* | The time between the start of simulation processing of the previous frame and this one, in milliseconds. |
| *MsPCLatency*          | Time between PC receiving input and frame being sent to the display, in milliseconds. |
| *MsBetweenAppStart*    | How long it took from the start of this frame until the CPU started working on the next frame, in milliseconds. |

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
