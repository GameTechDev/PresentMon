[![](https://img.shields.io/github/license/GameTechDev/PresentMon)]()
[![](https://img.shields.io/github/v/release/GameTechDev/PresentMon)](https://github.com/GameTechDev/PresentMon/releases/latest)
[![](https://img.shields.io/github/commits-since/GameTechDev/PresentMon/latest/main)]()
[![](https://img.shields.io/github/issues/GameTechDev/PresentMon)]()
[![](https://img.shields.io/github/last-commit/GameTechDev/PresentMon)]()

# PresentMon

PresentMon is a set of tools to capture and analyze the high-level performance characteristics of graphics applications on Windows.  PresentMon traces key performance metrics such as the CPU, GPU, and Display frame durations and latencies; and works across different graphics API such as DirectX, OpenGL, and Vulkan, different hardware configurations, and for both desktop and UWP applications.

This repository contains several components:

- The PresentData/ directory contains the **PresentMon Collection and Analysis library**: a library that performs the lowest-level collection and analysis of [ETW](https://msdn.microsoft.com/en-us/library/windows/desktop/bb968803%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396) events.  See [PresentData/PresentMonTraceConsumer.hpp](PresentData/PresentMonTraceConsumer.hpp) for more information.

- The PresentMon/ directory contains the **PresentMon Console Application**: a standalone console application that can be used to collect CSV data from target applications.  See [README-ConsoleAppliation.md](README-ConsoleApplication.md) for more information.

- The IntelPresentMon/ directory contains the **PresentMon Service** and **PresentMon Capture Application**: an easy-to-use GUI that can be used to collect and analyze target applications.  See [README-Service.md](README-Service.md) and [README-CaptureAppliation.md](README-CaptureApplication.md) for more information.

There are also several other programs that build on this functionality and/or help visualize the resulting data.  For example, see:

- [AMD OCAT](https://gpuopen.com/ocat/)
- [CapFrameX](https://capframex.com/)
- [Guru3D RTSS RivaTuner Statistics Server](https://www.guru3d.com/files-details/rtss-rivatuner-statistics-server-download.html)
- [Microsoft PIX on Windows System Monitor](https://devblogs.microsoft.com/pix/system-monitor/)
- [NVIDIA FrameView](https://www.nvidia.com/en-us/geforce/technologies/frameview/)

Binaries for the main releases of PresentMon are provided on [intel.com](https://game.intel.com/story/intel-presentmon/) or [github.com](https://github.com/GameTechDev/PresentMon/releases/latest) ([list of all releases](https://github.com/GameTechDev/PresentMon/releases)).

See [CONTRIBUTING.md](CONTRIBUTING.md) for information on how to request features, report issues, or contribute code changes.

See [BUILDING.md](BUILDING.md) for information on how to build PresentMon components from source.

## License

Copyright (C) 2017-2023 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Troubleshooting

See [GitHub Issues](https://github.com/GameTechDev/PresentMon/issues) for a current list of reported issues.

### User access denied

PresentMon needs to be run by a user who is a member of the "Performance Log Users" user group, or to be run with administrator privilege. If neither of these are true, you will get an error "failed to start trace session (access denied)".

To add a user to the "Performance Log Users" user group:

1. Run `compmgmt.msc` as administrator.
2. In the "Computer Management" window, expand "System Tools", expand "Local Users and Groups", and then click "Groups".
3. Double-click "Performance Log Users", and then click "Add".
4. In the "Enter the object names to select" text box, type the name of the user account or group account that you want to add, and then click "OK".
5. Sign out and log back in for the changes to take effect.

If PresentMon is not run with administrator privilege, it will not have complete process information for processes running on different user accounts.  Such processes will be listed in the console and CSV as "\<error>", and they cannot be targeted by name (`-process_name`).

### Analyzing OpenGL and Vulkan applications

Applications that do not use D3D9 or DXGI APIs for presenting frames (e.g., as is typical with OpenGL or Vulkan applications) will report the following:

- *Runtime* = Other
- *SwapChainAddress* = 0
- *msInPresentAPI* = 0

In this case, *TimeInSeconds* will represent the first time the present is observed in the kernel, as opposed to the runtime, and therefore will be sometime after the application presented the frame (typically ~0.5ms).  Since *msUntilRenderComplete* and *msUntilDisplayed* are deltas from *TimeInSeconds*, they will be correspondingly smaller then they would have been if measured from application present.  *msBetweenDisplayChange* will still be correct, and *msBetweenPresents* should be correct on average.

### Tracking GPU work with Hardware-Accelerated GPU Scheduling enabled

When using `-track_gpu` on a system that uses Hardware-Accelerated GPU
Scheduling (HWS), the GPU execution metrics are less accurate than when HWS is
disabled resulting in *msUntilRenderStart*, *msUntilRenderComplete*,
*msGPUActive*, and *msGPUVideoActive* measurements that are later/larger than
they should be.  For example, in a GPU-bound scenario the frame's *msGPUActive*
may be reported ~0.5ms larger than the true GPU work duration, though the
specific amount of the inaccuracy will be workload- and GPU-dependent.

An improved solution is WIP.

### Shutting down PresentMon on Windows 7

Some users have observed system stability issues when forcibly shutting down PresentMon on Windows 7.  If you are having similar issues, they can be avoided by using Ctrl+C in the PresentMon window to shut it down.
