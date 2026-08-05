# Phase 4 - UI and Capture

## Status

Phase 4 is in progress. The CEF dependency integration, shader compilation,
and native `PresentMonUI` and `KernelProcess` CMake targets are implemented.
They establish the native UI and kernel process build boundary, but they do
not yet produce the complete runnable UI and capture deployment.

| Area | State | Current boundary |
| --- | --- | --- |
| CEF dependency | Implemented | Fixed CMake stage, explicit restore, incremental runtime staging, and explicit CI integrity verification. |
| Shaders | Implemented | The two shipping HLSL shaders compile through CMake with the original MSBuild settings. |
| PresentMonUI | Implemented | The x64 `WIN32` executable reproduces the original `CefNano.vcxproj` native target contract. |
| KernelProcess | Implemented | The x64 `WIN32` executable reproduces the original `KernelProcess.vcxproj` native target contract. |
| Runtime payload | Remaining | Web content, presets, blocklists, and CLI-related payload are not staged by CMake. |
| Deployment and signing | Remaining | Phase 4 products are not signed or packaged yet. |
| Behavioral verification | Remaining | A deterministic UI and representative capture/control workflow is still required. |

`PresentMonUI` and shader compilation are available only for x64 when
`PMON_BUILD_UI` is enabled. CEF restore and CI verification remain available in
any x64 configure; runtime staging enters normal builds only through a CEF
consumer. Win32 has no maintained UI stack. The generated configurations remain
Debug and Release.

Developer and production profiles preserve the original
`build/<Configuration>/` output layout.

## Implemented CEF Contract

### Source of Truth and Fixed Stage

`IntelPresentMon/AppCef/cef-lock.json` is the source of truth for the CEF
archive identity and runtime payload. CMake uses exactly one published CEF
stage:

```text
build/ThirdParty/cef
```

The legacy MSBuild stage under `IntelPresentMon/AppCef/Cef` remains separate.
CMake targets do not read from it, write to it, or accept an arbitrary CEF root.

CMake configure only creates targets and reads the lock. Configure never
downloads, extracts, builds, restores, or publishes CEF. A fresh configure is
therefore allowed to succeed when the fixed stage does not exist.

### Explicit Restore

CEF restore is an opt-in operation:

```powershell
cmake --build --preset windows-x64-developer-debug --target pmon_restore_cef
```

`PMON_CEF_SOURCE` may name a local archive or URI. When it is empty, restore
uses the URI recorded by the lock. The archive must match the locked SHA-256;
an extracted directory is not accepted for the CMake stage.

Restore extracts and builds the wrapper in temporary storage, validates the
completed temporary stage, and publishes it only after validation succeeds.
Stage replacement and inspection are serialized across processes. A failed
restore must not publish a partial stage over a previously valid one.

Restore validates the version metadata, locked runtime and resource files,
required headers, Debug and Release import libraries, and Debug and Release
wrapper libraries before publication.

### Normal UI Builds

`pmon_target_uses_cef(PresentMonUI)` links the CEF targets and makes
`pmon_stage_cef_runtime` a product dependency. Normal UI builds perform only
incremental CEF runtime staging. They do not perform a full CEF validation and
they never invoke restore.

Runtime staging copies the lock-derived payload from `build/ThirdParty/cef`
to the selected configuration output. The files are declared build outputs,
and unchanged files are not recopied. Staging copies the current locked payload;
it does not run integrity checks or manage unrelated output files.

If the fixed stage or a declared stage input is absent, the build fails. The
developer must run the explicit restore target and build again.

### Explicit CI Integrity Check

`pmon_verify_cef` is the explicit, offline CEF integrity target for CI and
troubleshooting:

```powershell
cmake --build --preset windows-x64-developer-debug --target pmon_verify_cef
```

It validates the fixed stage and staged output. It is intentionally excluded
from normal builds. CI should invoke it explicitly when CEF integrity is part
of the job's scope.

### CEF Consumer Targets

| Target | Contract |
| --- | --- |
| `pmon::cef_headers` | Headers from `build/ThirdParty/cef/Include`. |
| `pmon::cef_libcef` | `libcef.dll` plus the matching Debug or Release import library. |
| `pmon::cef_wrapper` | The matching Debug or Release `libcef_dll_wrapper` static library. |
| `pmon::cef` | Aggregate interface used by product targets. |

## Implemented Shader Contract

`cmake/PresentMonShaders.cmake` is configured only for an x64 UI build. A build
with `PMON_BUILD_UI=OFF` does not create the shader target and does not require
`fxc.exe`.

`pmon_compile_shaders` produces:

| Source | Profile | Entry | Output |
| --- | --- | --- | --- |
| `IntelPresentMon/Shaders/Line_PS.hlsl` | `ps_4_0` | `main` | `<output>/<Configuration>/Shaders/Line_PS.cso` |
| `IntelPresentMon/Shaders/Line_VS.hlsl` | `vs_4_0` | `main` | `<output>/<Configuration>/Shaders/Line_VS.cso` |

This reproduces the original `Shaders.vcxitems` output contract. Debug uses
`/Od /Zi`; Release uses neither option. Those Debug settings are inherited by
the original MSBuild project through `UseDebugLibraries`, so CMake expresses
the resulting flags directly by configuration.

With no override, CMake selects the x64 `fxc.exe` from the same Windows SDK
version selected for the native build. `PMON_FXC_EXECUTABLE` may override that
path and must resolve to an existing file, not a directory. Both each HLSL
source and the resolved compiler are tracked as build inputs.

`pmon_target_uses_shaders(PresentMonUI)` attaches shader compilation to the UI
target. Repeated builds are incremental, and removing one output rebuilds only
that shader.

Debug shader output contains nondeterministic compiler debug data. Debug
parity should therefore be checked by size and disassembly, not by requiring a
stable file hash. Release output can be compared directly for the selected SDK
and inputs.

## Implemented PresentMonUI Contract

`IntelPresentMon/AppCef/CMakeLists.txt` defines `PresentMonUI` as one x64
`WIN32` executable. Its native source list contains the same 19 compilation
units as `IntelPresentMon/AppCef/CefNano.vcxproj`, plus `AppCef.rc` and
`compatibility.manifest`.

The two static-registration translation units are direct executable sources:

```text
source/util/cact/CefActionRegistration.cpp
source/util/KernelActionRegistration.cpp
```

They must remain direct sources, or move only to an object-library mechanism
that guarantees inclusion. Ordinary unreferenced members of a static library
could be discarded and silently remove their registrations.

### Original MSBuild Target Contract

The CMake target preserves the observable contract of `CefNano.vcxproj`:

| Contract | CMake result |
| --- | --- |
| Product identity | `PresentMonUI.exe`, Windows subsystem, in the common configuration output root. |
| Architecture | x64 only; no Win32 target. |
| Runtime and character set | Static MSVC runtime and MultiByte (`_MBCS`). |
| Common compile policy | `/W3`, `/sdl`, `/permissive-`, C++ latest, multiprocess compilation, and external warnings disabled. |
| Target compile policy | `_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING`, `/EHa`, and `/fp:fast`. |
| Release compile policy | Whole-program optimization, `/Gy`, `/Oi`, and Control Flow Guard. |
| Debug information | Debug Edit and Continue with Just My Code; Release Program Database information and linker `/DEBUG`. |
| Release link policy | Reference optimization and COMDAT folding. |
| PresentMon libraries | `CommonUtilities`, `Core`, `Interprocess`, and `Versioning` through their CMake targets. |
| Platform libraries | `dwrite`, `dwmapi`, and `shcore` are explicit link dependencies. |
| CEF | Normal load-time `libcef.dll` import through `pmon::cef`; no delay-load contract. |
| Resources | Existing icon and VERSIONINFO fields plus the compatibility manifest. |
| Manifest | `asInvoker` with `uiAccess=false`. |

The build strings remain observable as `PM_BUILD_WINSDK_VERSION_STR="10.0"`
and `PM_BUILD_CRT_RUNTIME_STR="MultiThreadedDebug"` or `"MultiThreaded"`.
Version and copyright strings come from the shared version-resource target and
match `Version.props`.

The resource's tracked `CefNano.exe` InternalName and OriginalFilename values
remain unchanged for parity even though the produced target is
`PresentMonUI.exe`.

Native include paths come from linked target interfaces. The UI target does
not add a raw legacy CEF include or library path. `Core` keeps its `IS_DEBUG`
definition private, so it does not leak into the UI compilation. The unused
Boost.Process include and dependency are not part of the target contract.

The legacy AppCef post-build batch file is not invoked from CMake. Its CEF and
shader responsibilities are replaced by explicit CMake dependencies. Its
remaining payload and deployment responsibilities have not yet been converted.

## Current Output Boundary

An enabled default UI build currently produces or stages:

- `PresentMonUI.exe` and its normal debug outputs.
- `PresentMon.exe` (the KernelProcess executable) and its normal debug outputs.
- `Shaders/Line_PS.cso` and `Shaders/Line_VS.cso`.
- Every CEF runtime and resource file named by the lock.

It does not yet stage the web application, presets, blocklists, or CLI
wrappers. Building `PresentMonUI` is therefore not yet proof of a complete
launchable UI deployment.

## Current Verification

The cleaned target graph was verified on August 4, 2026:

- x64 Debug and Release `PresentMonUI` builds succeeded.
- x64 Debug and Release `KernelProcess` builds succeeded, producing
  `build/<Config>/PresentMon.exe` with the reproduced per-configuration manifest.
- An immediate repeat changed none of the 18 staged CEF files, ran no CEF copy
  commands, and ran no CEF validation.
- Removing one staged CEF file caused only that file to be copied again.
- The explicit `pmon_verify_cef` target validated both the fixed stage and the
  Debug and Release runtime payloads.
- A Win32 Release build succeeded without UI or CEF product targets.
- An x64 configure with `PMON_BUILD_UI=OFF` generated neither `PresentMonUI` nor
  `pmon_compile_shaders`.

## Deployment and Signing Status

Production configuration can select the existing EDSS or SignTool backend. No
Phase 4 product signing pipeline has been attached yet. In particular,
`PresentMonUI`, `KernelProcess`, and their staged payload are not currently
signed, signature-verified, or packaged by the CMake Phase 4 flow.

The remaining deployment work must define the complete payload, apply developer
and production manifest policy, sign the required production files, and verify
signatures after signing and before packaging.
PresentMonUI must retain its current non-elevated manifest; KernelProcess must
receive its own legacy-compatible deployment policy rather than inheriting the
UI manifest.

## Remaining Work Plan

### 1. Convert the Remaining Payload

- Build and stage the web application consumed by the CEF scheme handlers.
- Stage presets, blocklists, and CLI-related files from explicit source lists.
- Replace the remaining behavior of the legacy AppCef post-build batch file.

### 2. Apply Deployment Policy and Signing

- Define the complete developer and production payload manifests.
- Attach the selected signing backend to the required production binaries.
- Verify signatures after all payload mutation and before packaging.
- Keep signing credentials and tool-specific configuration outside the source
  tree.

### 3. Add Deterministic Behavioral Verification

The completion check must exercise the staged web UI and a representative
KernelProcess capture or control path. It must define:

- A deterministic readiness signal.
- An observable successful UI operation.
- An observable capture or control result.
- Bounded timeouts for startup, operation, and shutdown.
- Controlled shutdown and cleanup after both success and failure.

A process launch or `--help` result alone is insufficient. If deterministic
readiness or shutdown needs a small product seam, add that seam as part of this
phase rather than relying on timing guesses.

### 4. Complete the Build Matrix

- Build clean x64 Debug and Release developer configurations.
- Confirm immediate repeat builds do not restage unchanged CEF files,
  recompile unchanged shaders or C++, or relink unchanged products.
- Confirm `PMON_BUILD_UI=OFF` does not create UI or shader targets.
- Confirm Win32 remains free of UI, shader, and CEF product dependencies.
- Run the explicit CEF integrity target in CI.
- Build the production payload, sign it, verify it, and package it.
- Run the deterministic UI and capture workflow against the staged payload.

## Phase Completion Criteria

Phase 4 is complete when all of the following are true:

- CMake builds both `PresentMonUI` and KernelProcess with their original native
  target contracts.
- The output contains the complete owned CEF, shader, web, preset, blocklist,
  CLI, and native executable payload.
- Ordinary builds remain incremental and never download CEF.
- Restore validates CEF before publication, and CI runs the explicit offline
  CEF integrity check.
- The production payload is signed and signature verification passes.
- The deterministic UI and representative capture/control workflow passes with
  bounded startup and shutdown.
- Existing non-UI configurations continue to build with their intended target
  set.
