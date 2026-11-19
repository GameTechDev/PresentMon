# PresentBench2

In-tree presenting workload for PresentMon validation. The default scene is an
untextured, depth-tested cube rotating on three axes with ambient plus two
restrained point lights (DX9/11/12/VK). Also covers provider markers, agent
control, and optional XeSS-FG.

## Build

PresentBench2 builds with `PMON_BUILD_TOOLS` (enabled for `PMON_BUILD_SCOPE=FULL` on x64).

```powershell
cmake --build build --config Debug --target PresentBench2
```

Vulkan headers/loader come from vcpkg (`vulkan-headers`, `vulkan-loader`) and are
required. The VK lit-cube path loads checked-in SPIR-V from `CubeShadersSpirv.h`
(generated from the GLSL in `CubeShaders.h`). After editing the GLSL sources,
regenerate SPIR-V with `py -3 Scripts/generate_presentbench2_spirv.py`.

Optional XeSS-FG SDK (default OFF). Public pin is baked into
`cmake/PresentBench2Sdks.cmake` (XeSS SDK 3.0.2); override URL/SHA256 cache vars
for internal mirrors:

```powershell
cmake -B build -DPMON_PRESENTBENCH2_FETCH_XESS_FG=ON
```

Redistributable DLLs (`libxess_fg.dll`, `libxell.dll`) are copied beside
`PresentBench2.exe` on build. Without the fetch flag the build stays green and
reports structured skip reasons from `--describe-capabilities`.

## CLI

Flags use CommonUtilities `CliFramework`. API and window mode are selected with
mutually exclusive flags (default DX12, windowed). Per-API options are rejected
by declarative `MutualExclusion` / `Dependency` rules. `--enable-debug-layer`
enables the D3D11/D3D12 debug layer or Vulkan Khronos validation layer.

| Area | Examples |
|------|----------|
| API | `--dx9`, `--dx11`, `--dx12` (default), `--vk` |
| Window | `--borderless`, `--fullscreen` (DX only; excluded with `--vk`) |
| Swap / present | `--width`, `--height`, `--buffer-count`, `--swap-effect` (DXGI, not DX9/VK), `--vk-mode` (needs `--vk`), `--sync-interval`, `--flags` |
| Timing | `--frame-sleep` (target interval), `--coarse-frame-wait`, `--frame-count`, `--frames-in-flight` (1-4, default 2) |
| Workload | `--clears-per-frame`, `--cpu-busy-ms`, `--pso-compiles` (DX12), `--inject-mouse`, `--inject-keyboard` |
| Instrumentation | provider markers on by default; `--no-emit-provider-markers`, `--emit-pcl-markers`, `--use-xess-fg`, `--fg-interpolated-frames` |
| Control / output | `--enable-stdio-control`, `--result-json`, `--describe-capabilities` |

## Control protocol

When `--enable-stdio-control` is set, commands use the same framing as PresentMon
service tests:

- request: `%ping`
- response: `%%{ping-ok}%%`

Commands: `ping`, `capabilities`, `status`, `wait-for-frame`, `quit`.
Capability probing is lazy and reports support for the selected `--adapter`.

With control enabled the process stays alive after the frame run finishes until
`quit` (or the window closes), so agents can read the final status.
Vulkan validation (`--vk --enable-debug-layer`) cannot be combined with stdio
control because validation layers may write directly to stdout and corrupt the
framed response stream.

## Metric coverage notes

| Path | Mechanism |
|------|-----------|
| Present/CPU/GPU/display | DXGI/D3D/Vulkan presents |
| Simulation / instrumented latency | Intel-PresentMon provider markers via `ProviderSink` |
| Frame generation | `--describe-capabilities` reports `xessFg.{compiled,runtime,reason}`. `--dx12 --use-xess-fg` uses the XeFG proxy swapchain with depth and NDC motion-vector inputs; the XeFG present component emits XeLL markers |
| PCL latency | `--emit-pcl-markers` emits PCLStats TraceLogging events |
| Input-to-photon | `--inject-mouse` / `--inject-keyboard` + provider `InputSample` |
| PSO compile | `--pso-compiles` on DX12 |
| Low-latency serialization | `--frames-in-flight 1` |

## Future work

- Scriptable frametime schedules and cyclic workload waveforms
- Mid-run swap-chain recreation
- Multi-window and multi-swapchain scenarios
- Richer ground-truth logs and PresentMon CSV diff hooks

## Tests

- `PresentBench2Tests`: cube scene math, frame-loop ordering (fake backend), control framing, CLI reject cases
- CTest GPU smoke coverage: DX9, DX11, DX12, and Vulkan
- When `glslc` is available, CTest verifies that checked-in SPIR-V matches the GLSL
- GPU smoke / PresentMon API2 integration: launch `PresentBench2.exe` with
  `--enable-stdio-control` via `ConnectedTestProcess` patterns
- Vendor paths: skip cleanly unless capabilities report SDK+runtime readiness
