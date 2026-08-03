# Phase 3 - Service and SDK

## Status

Complete for Debug and Release on x64.

The service, SDK, and UI support stacks have no maintained Win32 configuration in
MSBuild, so they are configured for x64 only. The Phase 2 Win32 standalone subset
was rebuilt to confirm this phase introduced no regression there.

## Scope

Phase 3 converted:

- Versioning.
- Interprocess.
- ControlLib.
- PresentMonAPIWrapperCommon and PresentMonAPIWrapper.
- PresentMonService, split into an implementation library and a thin executable.
- PresentMonMiddleware.
- PresentMonAPI2 and PresentMonAPI2Loader.
- SampleClient.
- Core.

No source files were moved or modified.

## Service Split

`PresentMonMiddleware` referenced the `PresentMonService` application project in
MSBuild. Because MSBuild does not link the outputs of an `Application` project
reference, that reference contributed headers and build ordering only:
`ActionClient.h` includes the generated `AllActions.h`, and `Middleware.cpp`
includes `GlobalIdentifiers.h`.

CMake represents this explicitly with three targets:

```text
pmon_service_headers (INTERFACE)  ->  pmon_service_core (STATIC)  ->  PresentMonService (EXE)
                 ^
                 |
          pmon_middleware
```

`pmon_service_headers` carries include directories and an ordering edge to the
`AllActions.h` generator, but no objects. Middleware links that target rather
than `pmon_service_core`, so no service implementation code can be drawn into
`PresentMonAPI2.dll`. This preserves the baseline DLL contents.

`pmon_service_core` owns every service compilation unit except
`ActionRegistration.cpp` and `ServiceMain.cpp`. The `PresentMonService`
executable owns those two files plus the version resource.
`ActionRegistration.cpp` must be linked directly into the executable because
its only effects are static action registrars; placing it in a static library
allows the linker to discard the unreferenced archive member. No other
cross-translation-unit symbol spans the split: `serviceName` is defined and
used only in `ServiceMain.cpp`, and `Service.cpp` uses the identifier solely as
a constructor parameter name.

`PresentMonServiceCore.lib` is an internal implementation artifact. It is written
to `build/obj/<Configuration>/` rather than the public output directory, matching
the treatment the Metrics library received in Phase 2, and it is not part of the
parity manifest.

## Targets

Foundation and service:

- `pmon_versioning`, alias `pmon::versioning`
- `pmon_interprocess`, alias `pmon::interprocess`
- `pmon_control`, alias `pmon::control`
- `pmon_service_headers`, alias `pmon::service_headers`
- `pmon_service_core`, alias `pmon::service_core`
- `PresentMonService`

SDK:

- `pmon_api_wrapper_common`, alias `pmon::api_wrapper_common`
- `pmon_api_wrapper`, alias `pmon::api_wrapper`
- `pmon_middleware`, alias `pmon::middleware`
- `PresentMonAPI2`
- `PresentMonAPI2Loader`
- `SampleClient`

UI support:

- `pmon_core`, alias `pmon::core`

Visual Studio folders group these under Foundation, Service and SDK, and UI and
Capture.

## Dependency Contract

The converted graph is:

```text
common_utilities -> present_data -> metrics -> interprocess -> control -> service_core -> PresentMonService
                                                  |                            ^
                                                  |                            | headers only
                                                  +--> middleware -------------+
                                                            |
                                                            v
                                                      PresentMonAPI2

common_utilities -> versioning -> PresentMonAPI2Loader -> core
api_wrapper_common -> api_wrapper -> SampleClient
```

Dependencies used by public headers are PUBLIC. `Boost::interprocess` is PUBLIC
on Interprocess because `SharedMemoryTypes.h` exposes it, and
`Boost::circular_buffer` and `Boost::container` are PUBLIC on middleware because
`FrameMetricsSource.h` and `DynamicMetric.h` expose them. `nlohmann_json` is
PRIVATE on ControlLib because only `UciTelemetryProvider.cpp` uses it.

Windows libraries are declared explicitly rather than relying on
`#pragma comment(lib)`: `pdh`, `shlwapi`, and `tdh` on the service; `wbemuuid` on
ControlLib; `d2d1`, `d3d11`, `d3dcompiler`, `dcomp`, `dwrite`, `dxgi`, `pdh`,
`user32`, and `winmm` on Core; `shlwapi` on PresentMonAPI2; `winmm` on
SampleClient.

Static CRT selection, `/W3`, `/EHa`, `/we4062`, `/Zc:preprocessor`, `/fp:fast`,
`/bigobj`, control flow guard, and the `PRESENTMONAPI2_EXPORTS`, `_LIB`,
`_WINDOWS`, `_USRDLL`, `_CRTDBG_MAP_ALLOC`, and `IS_DEBUG` definitions preserve
the corresponding MSBuild settings.

`/MANIFESTUAC:NO` is applied to both DLLs to match `EnableUAC=false`.

No Phase 3 project uses delay loading or a module definition file. Exports are
produced entirely by `__declspec(dllexport)` behind `PRESENTMONAPI2_EXPORTS`, so
CMake generates the import libraries without additional configuration.

## Character Set Handling

SampleClient is the one MultiByte target in this phase; CefNano will be the other
in Phase 4. Rather than duplicating the shared option set, `pmon::build_options`
now selects the character set from a target property:

```cmake
set_target_properties(SampleClient PROPERTIES PMON_MULTIBYTE ON)
```

Targets that do not set the property continue to receive `UNICODE` and
`_UNICODE`, so every previously converted target is unaffected.

## Generated Files

Three generators were converted.

`build_id.h` remains a source-tree output at
`IntelPresentMon/Versioning/generated/`. `BuildId.cpp` includes it as
`"generated/build_id.h"` relative to its own directory, so relocating it into the
build tree would require modifying source. The directory is gitignored, so the
output stays untracked. The `pmon_build_id` target runs the existing
`scripts/pre-build.ps1` on every build; the script's own signature comparison
decides whether to rewrite the header.

`AllActions.h` remains a source-tree output because it is a tracked, maintained
artifact. The `pmon_service_actions` target runs the existing
`build-scripts/pre-build.bat`, which rewrites the file only when the aggregated
`acts/*.h` set changes. Both `pmon_service_core` and `pmon_middleware` take an
explicit ordering edge to that target, since an INTERFACE library carries no
build rule of its own.

`EnumMetric.h` moved to a build-tree output at
`build/obj/generated/metadata/EnumMetric.h`, generated by
`cmake/GenerateEnumMetric.cmake`. That script invokes `Tools/awk.exe` with the
existing `metrics.awk` program and captures stdout through `OUTPUT_FILE`.
`Tools/generate/EnumMetric/generate.cmd` is not used because it hard-codes the
output path relative to itself; driving awk directly lets the output follow
`PMON_OUTPUT_ROOT` and gives the custom command explicit inputs. The generated
content is unchanged.

## Output Contract

x64 outputs added by this phase:

```text
build/<Configuration>/ControlLib.lib
build/<Configuration>/Core.lib
build/<Configuration>/Interprocess.lib
build/<Configuration>/IPMVersioning.lib
build/<Configuration>/PresentMonAPI2.dll
build/<Configuration>/PresentMonAPI2.lib
build/<Configuration>/PresentMonAPI2Loader.dll
build/<Configuration>/PresentMonAPI2Loader.lib
build/<Configuration>/PresentMonAPIWrapper.lib
build/<Configuration>/PresentMonAPIWrapperCommon.lib
build/<Configuration>/PresentMonMiddleware.lib
build/<Configuration>/PresentMonService.exe
build/<Configuration>/SampleClient.exe
build/obj/<Configuration>/PresentMonServiceCore.lib
```

`ddETWExternal.xml` is staged next to `PresentMonService.exe`, preserving the
`CopyNvidiaManifest` post-build step. UCI runtime staging is implemented and
gated on `PMON_UCI_ENABLED`; it was not exercised because no UCI SDK is available
in this environment.

## Shared Version Resource

`PM_VER_FILE_NUM`, `PM_VER_PRODUCT_NUM`, `PM_VER_FILE_STR`, `PM_VER_PRODUCT_STR`,
and `PM_VER_COPYRIGHT` are now supplied by a single `pmon::version_resource`
INTERFACE target created in `cmake/PresentMonVersion.cmake`. Provider was
switched from its inline copy to that target; its output is unchanged.

## Artifact Verification

x64 verification now checks twenty artifacts: the seven registered in Phase 2 and
thirteen added here. The service implementation library and the Metrics library
are internal artifacts and are excluded.

## Verification

Configuration and build:

```powershell
cmake --preset windows-x64-dependencies
cmake --preset windows-x64-developer
cmake --build --preset windows-x64-developer-debug
cmake --build --preset windows-x64-developer-debug --target pmon_verify_artifacts
cmake --build --preset windows-x64-developer-debug --target pmon_verify_service_sdk
cmake --build --preset windows-x64-developer-release
cmake --build --preset windows-x64-developer-release --target pmon_verify_artifacts
cmake --build --preset windows-x64-developer-release --target pmon_verify_service_sdk
```

The `pmon_service_sdk_smoke` CTest test is the Phase 3 behavioral check.
`pmon_verify_service_sdk` first builds its four product dependencies and then
runs that test for the selected configuration. The test starts the service in
console test-control mode with unique control-pipe, shared-memory, and ETW names;
waits for both the test-control acknowledgement and action-server pipe; runs
`SampleClient --mode Introspection` against the CMake-built middleware DLL; and
requires a successful API session, metric introspection data, and zero process
exit codes. It sends the service a test-control quit command and uses PID-scoped
termination only as failure or timeout cleanup. Both processes are also assigned
to a kill-on-close Windows Job Object so attached child processes are terminated
if the harness is cancelled. Per-run stdout and stderr logs are retained under
the CMake build tree.

Win32 regression check, because this phase changed the shared compiler options
target and Provider:

```powershell
cmake --preset windows-win32-dependencies
cmake --preset windows-win32-developer
cmake --build --preset windows-win32-developer-debug
cmake --build --preset windows-win32-developer-release
cmake --build --preset windows-win32-developer-release --target pmon_verify_artifacts
```

Results:

- Debug x64 and Release x64 built with no errors.
- Artifact verification reported twenty artifacts in both configurations.
- Win32 Debug and Release built with no errors and verified the same six
  artifacts as Phase 2.
- The Win32 build produced only x86 outputs. The x64 binaries in
  `build/Release` were left untouched, confirming that no Phase 3 target is
  configured for Win32. `pm_convert_csv.exe` is rewritten by both
  architectures, which is the shared legacy public path documented in Phase 2.
- No source file was modified by the build. `git status` showed changes only to
  build files, and the tracked `AllActions.h` was left untouched.
- `build_id.h` was regenerated with the current commit hash and dirty flag.

Runtime and binary checks:

- `PresentMonAPI2.dll` exports 32 `pm*` entry points.
- `PresentMonAPI2Loader.dll` exports 33 `pm*` entry points.
- `PresentMonAPI2.dll`, `PresentMonAPI2Loader.dll`, and `PresentMonService.exe`
  report file and product version `2.6.0.0` with correct original filenames.
- `SampleClient.exe` carries no version resource, matching MSBuild, where the
  project has no `.rc`.
- `PresentMonService.exe --help` printed its CLI11 usage and exited 0. Running
  from a console falls through `StartServiceCtrlDispatcher` into application
  mode, so no Windows service was installed or started.
- A console-mode service and `SampleClient --mode Introspection` completed an
  API session successfully in both Debug and Release. The client exited 0 and
  returned metric introspection data. The automated `pmon_verify_service_sdk`
  target repeated this workflow successfully in both configurations and shut
  down each service instance through its test-control channel.
- `SampleClient.exe --help` printed its usage and mode list and exited 0 in both
  configurations.

## Observations

Linking any binary that instantiates `cereal::detail::StaticObject` produces a
`.lib` and `.exp` alongside the executable, because cereal annotates that class
with `__declspec(dllexport)` unconditionally on MSVC. This affects `ETLTrimmer`
and the console from Phase 2 as well as `PresentMonService` and `SampleClient`
here. It is source-driven rather than a build-system choice, so MSBuild produces
the same files; the Phase 0 artifact list simply did not enumerate them. The
Phase 7 artifact comparison should confirm this rather than treat the extra files
as a CMake difference.

Core sets `RunCodeAnalysis` in MSBuild. CMake does not run static analysis as
part of the build. This changes build-time diagnostics only, not the produced
artifact, and is left for a later decision.

## Deferred

- CEF restoration, shader compilation, PresentMonUI, and KernelProcess.
- Payload staging for CEF, web, presets, and blocklists.
- Exercising UCI runtime staging against a real SDK.
- Production payload signing and signature verification.
- Full automated test-suite conversion and remaining CTest registration.
- MSI and MSM integration.
- Narrowing the repository-root and `IntelPresentMon` public include paths, which
  current header include styles still require.
