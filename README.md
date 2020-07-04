[![](https://img.shields.io/github/license/GameTechDev/PresentMon)]()
[![](https://img.shields.io/github/v/release/GameTechDev/PresentMon)](https://github.com/GameTechDev/PresentMon/releases/latest)
[![](https://img.shields.io/github/commits-since/GameTechDev/PresentMon/latest/main)]()
[![](https://img.shields.io/github/issues/GameTechDev/PresentMon)]()
[![](https://img.shields.io/github/last-commit/GameTechDev/PresentMon)]()

# PresentMon

PresentMon is a tool to capture and analyze [ETW](https://msdn.microsoft.com/en-us/library/windows/desktop/bb968803%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396) events related to swap chain presentation on Windows.  It can be used to trace key performance metrics for graphics applications (e.g., CPU and Display frame durations and latencies) and works across different graphics APIs, different hardware configurations, and for both desktop and UWP applications.

While PresentMon itself is focused on lightweight collection and analysis, there are several other programs that build on its functionality and/or helps visualize the resulting data.  For example, see
- [CapFrameX](https://github.com/DevTechProfile/CapFrameX)
- [FrameView](https://www.nvidia.com/en-us/geforce/technologies/frameview/)
- [OCAT](https://github.com/GPUOpen-Tools/OCAT)
- [PIX](https://devblogs.microsoft.com/pix/download/) (used as part of its [system monitor UI](https://devblogs.microsoft.com/pix/system-monitor/))
- [presentmon-graph](https://github.com/PaulPiatek/presentmon-graph)



## License

Copyright 2017-2020 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



## Releases

Binaries for main release versions of PresentMon are provided on GitHub:
- [Latest release](https://github.com/GameTechDev/PresentMon/releases/latest)
- [List of all releases](https://github.com/GameTechDev/PresentMon/releases)

See [CONTRIBUTING](https://github.com/GameTechDev/PresentMon/blob/main/CONTRIBUTING.md) for information on how to request features, report issues, or contribute code changes.



## Command line options

```
Capture target options:
  -captureall              Record all processes (default).
  -process_name name       Record only processes with the provided exe name.
                           This argument can be repeated to capture multiple
                           processes.
  -exclude name            Don't record processes with the provided exe name.
                           This argument can be repeated to exclude multiple
                           processes.
  -process_id id           Record only the process specified by ID.
  -etl_file path           Consume events from an ETW log file instead of
                           running processes.

Output options (see README for file naming defaults):
  -output_file path        Write CSV output to the provided path.
  -output_stdout           Write CSV output to STDOUT.
  -multi_csv               Create a separate CSV file for each captured process.
  -no_csv                  Do not create any output file.
  -no_top                  Don't display active swap chains in the console
                           window.
  -qpc_time                Output present time as a performance counter value.

Recording options:
  -hotkey key              Use provided key to start and stop recording, writing
                           to a unique CSV file each time. 'key' is of the form
                           MODIFIER+KEY, e.g., alt+shift+f11. (See README for
                           subsequent file naming).
  -delay seconds           Wait for provided time before starting to record. If
                           using -hotkey, the delay occurs each time recording
                           is started.
  -timed seconds           Stop recording after the provided amount of time.
  -exclude_dropped         Exclude dropped presents from the csv output.
  -scroll_indicator        Enable scroll lock while recording.
  -simple                  Disable GPU/display tracking.
  -verbose                 Adds additional data to output not relevant to normal
                           usage.

Execution options:
  -session_name name       Use the provided name to start a new realtime ETW
                           session, instead of the default "PresentMon". This
                           can be used to start multiple realtime capture
                           process at the same time (using distinct names). A
                           realtime PresentMon capture cannot start if there are
                           any existing sessions with the same name.  name is
                           not sensitive to case.
  -stop_existing_session   If a trace session with the same name is already
                           running, stop the existing session (to allow this one
                           to proceed).
  -dont_restart_as_admin   Don't try to elevate privilege.  Elevated privilege
                           isn't required to trace a process you started, but
                           PresentMon requires elevated privilege in order to
                           query processes started on another account. Without
                           it, these processes cannot be targetted by name and
                           will be listed as '<error>'.
  -terminate_on_proc_exit  Terminate PresentMon when all the target processes
                           have exited.
  -terminate_after_timed   When using -timed, terminate PresentMon after the
                           timed capture completes.

Beta options:
  -qpc_time_s              Output present time as a performance counter value
                           converted to seconds.
  -terminate_existing      Terminate any existing PresentMon realtime trace
                           sessions, then exit. Use with -session_name to target
                           particular sessions.
  -include_mixed_reality   Capture Windows Mixed Reality data to a CSV file with
                           "_WMR" suffix.
```



## Comma-separated value (CSV) file output

### CSV file names

By default, PresentMon creates a CSV file named `PresentMon-TIME.csv`, where `TIME` is the creation time in ISO 8601 format.  To specify your own output location, use the `-output_file PATH` command line argument.

If `-multi_csv` is used, then one CSV is created for each process captured and `-PROCESSNAME` appended to the file name.

If `-hotkey` is used, then one CSV is created for each time recording is started and `-INDEX` appended to the file name.

### CSV columns

| Column Header | Data Description | Required argument |
|---|---|---|
| Application            | The name of the process that called Present(). ||
| ProcessID              | The process ID of the process that called Present(). ||
| SwapChainAddress       | The address of the swap chain that was presented into. ||
| Runtime                | The runtime used to present (e.g., D3D9 or DXGI). ||
| SyncInterval           | The sync interval used in the Present() call. ||
| PresentFlags           | Flags used in the Present() call. ||
| PresentMode            | The presentation mode used by the system for this Present().  See the table below for more details. | not `-simple` |
| AllowsTearing          | Whether tearing is possible (1) or not (0). | not `-simple` |
| TimeInSeconds          | The time of the Present() call, in seconds, relative to when the PresentMon started recording. | |
| QPCTime                | The time of the Present() call, as a [performance counter value](https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter).  If `-qpc_time_s` is used, the value is converted to seconds by dividing by the counter frequency. | `-qpc_time` or `-qpc_time_s` |
| MsInPresentAPI         | The time spent inside the Present() call, in milliseconds. ||
| MsUntilRenderComplete  | The time between the Present() call and when the GPU work completed, in milliseconds. | not `-simple` |
| MsUntilDisplayed       | The time between the Present() call and when the frame was displayed, in milliseconds. | not `-simple` |
| Dropped                | Whether the frame was dropped (1) or displayed (0).  Note, if dropped, MsUntilDisplayed will be 0. ||
| MsBetweenPresents      | The time between this Present() call and the previous one, in milliseconds. ||
| MsBetweenDisplayChange | How long the previous frame was displayed before this Present() was displayed, in milliseconds. | not `-simple` |
| WasBatched             | Whether the frame was submitted by the driver on a different thread than the app (1) or not (0). | `-verbose` |
| DwmNotified            | Whether the desktop compositor was notified about the frame (1) or not (0). | `-verbose` |


The following values are used in the PresentMode column:

| PresentMode | Description |
|---|---|
| Hardware: Legacy Flip | Indicates the app took ownership of the screen, and is swapping the displayed surface every frame. |
| Hardware: Legacy Copy to front buffer | Indicates the app took ownership of the screen, and is copying new contents to an already-on-screen surface every frame. |
| Hardware: Independent Flip | Indicates the app does not have ownership of the screen, but is still swapping the displayed surface every frame. |
| Composed: Flip | Indicates the app is windowed, is using ["flip model" swapchains](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model), and is sharing its surfaces with DWM to be composed. |
| Hardware Composed: Independent Flip | Indicates the app is using ["flip model" swapchains](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model), and has been granted a hardware overlay plane. |
| Composed: Copy with GPU GDI | Indicates the app is windowed, and is copying contents into a surface that's shared with GDI. |
| Composed: Copy with CPU GDI | Indicates the app is windowed, and is copying contents into a dedicated DirectX window surface. GDI contents are stored separately, and are composed together with DX contents by the DWM. |
| Composed: Composition Atlas | Indicates use of DirectComposition. |

For more information on the performance implications of these, see:

- https://www.youtube.com/watch?v=E3wTajGZOsA
- https://software.intel.com/content/www/us/en/develop/articles/sample-application-for-direct3d-12-flip-model-swap-chains.html

### Windows Mixed Reality

*Note: Windows Mixed Reality support is in beta, with limited OS support and maintenance.*

If `-include_mixed_reality` is used, a second CSV file will be generated with `_WMR` appended to the filename with the following columns:

| Column Header | Data Description | Required argument |
|---|---|---|
| Application                                  | Process name (if known) | `-include_mixed_reality` |
| ProcessID                                    | Process ID | `-include_mixed_reality` |
| DwmProcessID                                 | Compositor Process ID | `-include_mixed_reality` |
| TimeInSeconds                                | Time since PresentMon recording started | `-include_mixed_reality` |
| MsBetweenLsrs                                | Time between this Lsr CPU start and the previous one | `-include_mixed_reality` |
| AppMissed                                    | Whether Lsr is reprojecting a new (0) or old (1) App frame (App GPU work must complete before Lsr CPU start) | `-include_mixed_reality` |
| LsrMissed                                    | Whether Lsr displayed a new frame (0) or not (1+) at the intended V-Sync (Count V-Syncs with no display change) | `-include_mixed_reality` |
| MsAppPoseLatency                             | Time between App's pose sample and the intended mid-photon frame display | `-include_mixed_reality` |
| MsLsrPoseLatency                             | Time between Lsr's pose sample and the intended mid-photon frame display | `-include_mixed_reality` |
| MsActualLsrPoseLatency                       | Time between Lsr's pose sample and mid-photon frame display | `-include_mixed_reality` |
| MsTimeUntilVsync                             | Time between Lsr CPU start and the intended V-Sync | `-include_mixed_reality` |
| MsLsrThreadWakeupToGpuEnd                    | Time between Lsr CPU start and GPU work completion | `-include_mixed_reality` |
| MsLsrThreadWakeupError                       | Time between intended Lsr CPU start and Lsr CPU start | `-include_mixed_reality` |
| MsLsrPreemption                              | Time spent preempting the GPU with Lsr GPU work | `-include_mixed_reality` |
| MsLsrExecution                               | Time spent executing the Lsr GPU work | `-include_mixed_reality` |
| MsCopyPreemption                             | Time spent preempting the GPU with Lsr GPU cross-adapter copy work (if required) | `-include_mixed_reality` |
| MsCopyExecution                              | Time spent executing the Lsr GPU cross-adapter copy work (if required) | `-include_mixed_reality` |
| MsGpuEndToVsync                              | Time between Lsr GPU work completion and V-Sync | `-include_mixed_reality` |
| MsBetweenAppPresents                         | Time between App's present and the previous one | `-include_mixed_reality` and not `-simple` |
| MsAppPresentToLsr                            | Time between App's present and Lsr CPU start | `-include_mixed_reality` and not `-simple` |
| HolographicFrameID                           | App's Holographic Frame ID | `-include_mixed_reality` `-verbose` |
| MsSourceReleaseFromRenderingToLsrAcquire     | Time between composition end and Lsr acquire | `-include_mixed_reality` `-verbose` |
| MsAppCpuRenderFrame                          | Time between App's CreateNextFrame() API call and PresentWithCurrentPrediction() API call | `-include_mixed_reality` `-verbose` |
| MsAppMisprediction                           | Time between App's intended pose time and the intended mid-photon frame display | `-include_mixed_reality` `-verbose` |
| MsLsrCpuRenderFrame                          | Time between Lsr CPU render start and GPU work submit | `-include_mixed_reality` `-verbose` |
| MsLsrThreadWakeupToCpuRenderFrameStart       | Time between Lsr CPU start and CPU render start | `-include_mixed_reality` `-verbose` |
| MsCpuRenderFrameStartToHeadPoseCallbackStart | Time between Lsr CPU render start and pose sample | `-include_mixed_reality` `-verbose` |
| MsGetHeadPose                                | Time between Lsr pose sample start and pose sample end | `-include_mixed_reality` `-verbose` |
| MsHeadPoseCallbackStopToInputLatch           | Time between Lsr pose sample end and input latch | `-include_mixed_reality` `-verbose` |
| MsInputLatchToGpuSubmission                  | Time between Lsr input latch and GPU work submit | `-include_mixed_reality` `-verbose` |



## Known issues

See [GitHub Issues](https://github.com/GameTechDev/PresentMon/issues) for a current list of reported issues.

### Analyzing OpenGL and Vulkan applications

Applications that do not use D3D9 or DXGI APIs for presenting frames (e.g., as is typical with OpenGL or Vulkan applications) will report the following:
- Runtime = Other.
- SwapChainAddress = 0
- SyncInterval = -1
- PresentFlags = 0
- MsInPresentAPI = 0

In this case, TimeInSeconds will represent the first time the present is observed in the kernel, as opposed to the runtime, and therefore will be sometime after the application presented the frame (typically ~0.5ms).  Since MsUntilRenderComplete and MsUntilDisplayed are deltas from TimeInSeconds, they will be correspondingly smaller then they would have been if measured from application present.  MsBetweenDisplayChange will still be correct, and MsBetweenPresents should be correct on average.

### Measuring application latency

PresentMon doesn't directly measure the latency from a user's input to the display of that frame because it doesn't have insight into when the application collects and applies user input.  A potential approximation is to assume that the application collects user input immediately after presenting the previous frame.  To compute this, search for the previous row that uses the same swap chain and then:

```LatencyMs =~ MsBetweenPresents + MsUntilDisplayed - previous(MsInPresentAPI)```

### Shutting down PresentMon on Windows 7

Some users have observed system stability issues when forcibly shutting down PresentMon on Windows 7.  If you are having similar issues, they can be avoided by using Ctrl+C in the PresentMon window to shut it down.
