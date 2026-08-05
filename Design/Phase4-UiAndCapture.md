# Phase 4 - UI and Capture

## Status

Phase 4 is in progress. The CEF dependency integration, shader compilation,
native `PresentMonUI` and `KernelProcess` CMake targets, and the remaining
runtime payload staging are implemented. Deployment-profile manifests, signing,
and behavioral verification are still open before Phase 4 is complete.

| Area | State | Current boundary |
| --- | --- | --- |
| CEF dependency | Implemented | Fixed CMake stage, explicit restore, incremental runtime staging, and explicit CI integrity verification. |
| Shaders | Implemented | The two shipping HLSL shaders compile through CMake with the original MSBuild settings. |
| PresentMonUI | Implemented | The x64 `WIN32` executable reproduces the original `CefNano.vcxproj` native target contract. |
| KernelProcess | Implemented | The x64 `WIN32` executable reproduces the original native target contract; deployment-profile manifest selection remains open. |
| Runtime payload | Implemented | Web output, presets, blocklist, and CLI wrappers stage incrementally through `cmake/PresentMonUiPayload.cmake`. |
| Deployment and signing | Remaining | CMake does not yet sign and verify the complete production native payload built through Phase 4; MSI and MSM packaging belongs to Phase 6. |
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

The legacy AppCef post-build batch file is not invoked from CMake. Its CEF
responsibilities are replaced by explicit CMake dependencies. Shader compilation
came from `Shaders.vcxitems` and is replaced separately. Web, preset, and
blocklist staging is implemented in `cmake/PresentMonUiPayload.cmake`; CLI
wrappers stage through the same module on `KernelProcess`.

## Implemented KernelProcess Contract

`IntelPresentMon/KernelProcess/CMakeLists.txt` builds the x64 `WIN32` target
`KernelProcess` as `PresentMon.exe`. The registration-only
`kact/KernelActionRegistration.cpp` remains a direct executable source.
KernelProcess has build-order dependencies on `PresentMonUI` and
`PresentMonService`, matching the two legacy application project references
without linking either executable.

The native target applies deployment manifest policy through CMake:

- Debug uses `uiAccess=false` in every deployment profile.
- Release uses `uiAccess=true` in both DEVELOPER and PRODUCTION profiles
  (matching legacy `KernelProcess.vcxproj` Release / Release-EDSS linking).

Developer Release builds run the legacy test-certificate SignTool post-build on
`PresentMon.exe` when the certificate is installed; production Release builds
rely on the EDSS signing stage and do not attach the test-certificate
post-build to KernelProcess.

## Current Output Boundary

An enabled default UI build currently produces or stages:

- `PresentMonUI.exe` and its normal debug outputs.
- `PresentMon.exe` (the KernelProcess executable) and its normal debug outputs.
- `Shaders/Line_PS.cso` and `Shaders/Line_VS.cso`.
- Every CEF runtime and resource file named by the lock.
- The locked Vite web output under `ipm-ui-vue/`, four preset JSON files under
  `Presets/`, and the blocklist under `BlockLists/`.
- `presentmon-cli.cmd` beside `PresentMon.exe` and `presentmon-cli.bat` under
  `pathed/`.

Building `PresentMonUI` and `KernelProcess` now stages the legacy runtime
payload, but deployment-profile manifests, signing, and the deterministic
behavioral gate are still required before Phase 4 is complete.

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

Production configuration uses EDSS only (`PMON_EDSS_SIGN_SCRIPT` must point at an
external script before configure).
Developer configuration uses SignTool for the KernelProcess Release post-build.
Phase 4 owns signing and
signature verification for the complete native payload built through this
phase, including the products converted in Phases 2 and 3. MSI and MSM
packaging remains Phase 6 work.

The remaining deployment work must define the complete payload, apply developer
and production manifest policy, sign the required production files, and verify
signatures after signing and before packaging.
PresentMonUI must retain its current non-elevated manifest; KernelProcess must
receive its own legacy-compatible deployment policy rather than inheriting the
UI manifest.

### Production signing closure (build order)

There is no `windows-win32-production` configure preset. Win32 uses the
DEVELOPER profile only (`windows-win32-developer` +
`windows-win32-developer-release`).

Production EDSS signing runs after the full Release payload exists under
`${PMON_OUTPUT_ROOT}/Release`:

1. Configure and build x64 **production** Release (`windows-x64-production` /
   `windows-x64-production-release`) so the main native payload, CEF staging, and
   `Intel-PresentMon.dll` (x64 provider) are in `build/Release`.
2. Configure and build Win32 **developer** Release
   (`windows-win32-developer-release`) into the same `build/Release` tree so
   maintained x86 artifacts (for example `PresentMon-<version>-x86.exe` and
   `Intel-PresentMon32.dll`) land beside the x64 outputs without a second
   output root.
3. Invoke `pmon_sign_production_payload` (external script from
   `PMON_EDSS_SIGN_SCRIPT`, template
   `C:\PresentMonBuilder\sign-production-payload.ps1`) with `-OutputRoot`
   `"${PMON_OUTPUT_ROOT}/Release"`, `-Verify`, and (by default)
   `-IntuneSigning` when `PMON_EDSS_INTUNE_SIGNING=ON`. Set
   `-DPMON_EDSS_INTUNE_SIGNING=OFF` to use Credential Manager and the signing
   account logon probe instead. CMake always passes `-Verify` for production
   payload signing. The script runs `signtool verify /pa` on each signed
   artifact before Phase 6 packaging.

**Installer provider DLL:** PMInstaller packages the optional ETW provider from
`$(var.Provider.TargetPath)` (`IntelPresentMon/PMInstaller/PresentMon.wxs`,
`provider_group`). Under `Release-EDSS-MSI|x64` the solution maps Provider to
`Release|x64` (`PresentMon.sln`), so the MSI ships **Intel-PresentMon.dll**,
not **Intel-PresentMon32.dll**. The Win32 provider DLL is built for the legacy
x86 payload step and manual `Provider/install_provider.cmd` on 32-bit hosts; it
is not a WiX/MSM file entry today.

## Remaining Work Plan

### 1. Apply Deployment Manifest Policy

- Keep `PresentMonUI` non-elevated in every profile.
- KernelProcess manifest policy: Debug uses `uiAccess=false`; Release uses
  `uiAccess=true` for both DEVELOPER and PRODUCTION profiles.
- Developer Release: test-certificate SignTool sign and verify on KernelProcess
  (legacy vcxproj post-build parity).
- Production Release: EDSS payload signing via `pmon_sign_production_payload`
  after x64 production Release and Win32 developer Release populate shared
  `build/Release` (see **Production signing closure** above); invokes the script
  from `PMON_EDSS_SIGN_SCRIPT` with `-OutputRoot`
  `"${PMON_OUTPUT_ROOT}/Release"`, `-Verify`, and `-IntuneSigning` when
  `PMON_EDSS_INTUNE_SIGNING=ON` (default); no Win32 production configure; no
  test-certificate post-build on KernelProcess.

### 2. Add Deterministic Behavioral Verification

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

### 3. Apply Production Signing

- Attach EDSS to the complete production native payload built through Phase 4,
  including the products
  converted in Phases 2 and 3.
- Sign the complete production native payload under shared `build/Release` using
  the build order in **Production signing closure** (x64 production, then Win32
  developer Release). The EDSS sign list matches MSI-relevant binaries (x64
  `Intel-PresentMon.dll`); `Intel-PresentMon32.dll` is not packaged by
  PMInstaller and is omitted from the template sign script unless packaging
  policy changes.
- Verify signatures after all payload mutation and before Phase 6 packaging.
- Keep signing credentials and tool-specific configuration outside the source
  tree (reference template: `C:\PresentMonBuilder\sign-production-payload.ps1`).

### 4. Complete the Build Matrix

- Build clean x64 Debug and Release developer configurations.
- Confirm immediate repeat builds do not restage unchanged CEF files,
  recompile unchanged shaders or C++, or relink unchanged products.
- Confirm `PMON_BUILD_UI=OFF` does not create UI or shader targets.
- Confirm Win32 remains free of UI, shader, and CEF product dependencies.
- Build Win32 developer Release into shared `build/Release` (no Win32 production
  preset).
- Run the explicit CEF integrity target in CI.
- Build the production payload, sign it, and verify it.
- Run the deterministic UI and capture workflow against the staged payload.
- Leave MSI and MSM packaging to Phase 6.

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
