# PresentMon

PresentMon is a tool to extract ETW information related to swap chain
presentation.  It can be used to trace and analyze frame durations of D3D10+
applications, including UWP applications.

## License

Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Command line options

```html
Capture target options:
    -captureall                Record all processes (default).
    -process_name [exe name]   Record specific process specified by name.
    -process_id [integer]      Record specific process specified by ID.

Output options:
    -no_csv                    Do not create any output file.
    -output_file [path]        Write CSV output to specified path. Otherwise, the default is
                               PresentMon-PROCESSNAME-TIME.csv.

Control and filtering options:
    -etl_file [path]           Consume events from an ETL file instead of a running process.
    -scroll_toggle             Only record events while scroll lock is enabled.
    -scroll_indicator          Set scroll lock while recording events.
    -hotkey [key]              Use specified key to start and stop recording, writing to a
                               unique file each time (default is F11).
    -delay [seconds]           Wait for specified time before starting to record. When using
                               -hotkey, delay occurs each time recording is started.
    -timed [seconds]           Stop recording after the specified amount of time.  PresentMon will exit
                               timer expires.
    -exclude_dropped           Exclude dropped presents from the csv output.
    -terminate_on_proc_exit    Terminate PresentMon when all instances of the specified process exit.
    -terminate_after_timed     Terminate PresentMon after the timed trace, specified using -timed, completes.
    -simple                    Disable advanced tracking (try this if you encounter crashes).
    -verbose                   Adds additional data to output not relevant to normal usage.
    -dont_restart_as_admin     Don't try to elevate privilege.
    -no_top                    Don't display active swap chains in the console window.
```

## Comma-separated value (CSV) file output

### Simple Columns (-simple command line argument)

| CSV Column Header | CSV Data Description |
|---|---|
| Application            | Process name (if known) |
| ProcessID              | Process ID |
| SwapChainAddress       | Swap chain address |
| Runtime                | Swap chain runtime (e.g., D3D9 or DXGI) |
| SyncInterval           | Sync interval used |
| PresentFlags           | Present flags used |
| Dropped                | Whether the present was dropped (1) or displayed (0) |
| TimeInSeconds          | Time since PresentMon recording started |
| MsBetweenPresents      | Time between this Present() API call and the previous one |
| MsInPresentAPI         | Time spent inside the Present() API call |

### Default Columns

All of the above columns, plus:

| CSV Column Header | CSV Data Description |
|---|---|
| AllowsTearing          | Whether tearing possible (1) or not (0) |
| PresentMode            | Present mode |
| MsBetweenDisplayChange | Time between when this frame was displayed, and previous was displayed |
| MsUntilRenderComplete  | Time between present start and GPU work completion |
| MsUntilDisplayed       | Time between present start and frame display |

### Verbose Columns (-verbose command line argument)

All of the above columns above, plus:

| CSV Column Header | CSV Data Description |
|---|---|
| WasBatched  | The frame was submitted by the driver on a different thread than the app |
| DwmNotified | The desktop compositor was notified about the frame. |

