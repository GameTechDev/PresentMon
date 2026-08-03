# Phase 4 - UI and Capture

## Status

In progress. The fixed two-stage CEF dependency preflight passed acceptance on
August 3, 2026. Shader compilation, the PresentMonUI and KernelProcess targets,
web, preset, blocklist, and CLI staging, and application of the deployment and
signing policies remain open.

Phase 2 deterministic behavioral verification is still pending. That debt does
not prevent Phase 4 implementation from proceeding, but it must be resolved
before full migration parity and cutover are declared.

## Scope Implemented for Review

- Root CMake integration for CEF support.
- An explicit CMake restore target and a non-downloading validation target.
- CEF header, `libcef`, and wrapper targets for future consumers.
- Lock-driven CEF runtime staging infrastructure.
- Separate developer and production output roots.

No C or C++ product source file has been moved or modified for this work. Build
changes include the root `CMakeLists.txt`, files under `cmake/`, CMake presets
and output policy, and the AppCef batch scripts. Documentation changes are
recorded in this file, the migration design, the design index, and the CEF lock
guide.

## Phase 4 Preflight Decisions

### Registration-Only Translation Units

The Phase 3 service conversion established that a translation unit whose only
effect is static registration can be discarded when it is an unreferenced member
of a static library. Phase 4 has the same risk in:

```text
IntelPresentMon/AppCef/source/util/cact/CefActionRegistration.cpp
IntelPresentMon/AppCef/source/util/KernelActionRegistration.cpp
IntelPresentMon/KernelProcess/kact/KernelActionRegistration.cpp
```

These files must be compiled directly into their executable or supplied through
an object-library mechanism that guarantees inclusion. A Phase 4 implementation
must not leave them as ordinary unreferenced members of a static implementation
library.

### Deployment Output Isolation

Developer and production builds must not write different manifest or signature
states to the same runtime directory. Developer presets retain the parity path:

```text
build/<Configuration>/
```

The production preset uses:

```text
build/production/<Configuration>/
```

Production staging, signing, signature verification, and packaging must consume
only the production root. The output helper rejects a production configuration
that explicitly selects the developer root. This isolation was verified before
production payload work begins.

### Behavioral Verification

Phase 4 cannot be completed by build, artifact, launch, or `--help` checks. Its
completion record must include a deterministic workflow that exercises the
staged CEF/web UI and a representative KernelProcess capture or control path.
The procedure must define readiness, observable success, timeout behavior, and
controlled shutdown. It may be a focused phase-local integration test and does
not need to wait for Phase 5 CTest conversion.

This workflow is a Phase 4 completion requirement, not a blocker to beginning
shader compilation or target conversion. It must be defined before the UI target
architecture is considered final because deterministic readiness or shutdown
may require a small test seam.

## Fixed CEF Destinations

Only two published CEF destinations are supported:

```text
Legacy  IntelPresentMon/AppCef/Cef
CMake   build/ThirdParty/cef
```

`pull-cef.ps1` and `validate-cef.ps1` select them with
`-StageKind Legacy|CMake`; `Legacy` is the default for compatibility with
existing MSBuild callers. Published-stage selection is closed; a third
destination cannot be configured.

This two-stage boundary is transitional. At the Phase 7 MSBuild cutover,
remove the `Legacy` stage kind, its callers, and
`IntelPresentMon/AppCef/Cef`. `build/ThirdParty/cef` then becomes the sole
published stage; the transition must not preserve both indefinitely.

Temporary extraction, wrapper-build, staging, and retired directories are
internal implementation details. They do not create another supported
published stage.

Both stages are restored from `IntelPresentMon/AppCef/cef-lock.json`. The lock
covers the 13 runtime files under `Bin` and the five resource files under
`Resources`. Stage validation also checks the CEF version metadata and the
header and library artifacts required by consumers even though those build-time
files are not part of the shipping payload lock.

## Restore and Validation Contract

The CMake-stage restore accepts the locked URI or a local archive whose SHA-256
matches the lock. It does not accept an extracted directory. Only the legacy
pull workflow retains the extracted-directory fallback. Lock upgrades require
a URI or archive so every new lock records an archive SHA-256.

Restore builds the wrapper in temporary storage, validates the completed stage,
and then publishes it. Restore and stage validation use a named cross-process
mutex so a validator cannot inspect a stage while it is being replaced. The CEF
wrapper helper uses CMake configure and build commands. `pull-cef.ps1` accepts
`-Generator`, `-Platform`, and `-Toolset` and forwards them to the wrapper
helper; the defaults are Visual Studio 17 2022, x64, and v143. An empty platform
or toolset omits the corresponding CMake generator option. The wrapper build
does not depend on `vswhere.exe` being present on `PATH`.

Stage validation covers:

- CEF version metadata from `Include/include/cef_version.h`.
- Every locked runtime and resource file, including rejection of unexpected
  files under the locked `Bin` and `Resources` trees.
- The CEF headers required by current consumers and the Debug and Release
  `libcef` import libraries.
- The Debug and Release `libcef_dll_wrapper` libraries.

Output validation checks every locked runtime file after staging. Incremental
runtime staging declares its outputs and records only the files it owns, so a
later lock change can remove stale CEF files without deleting unrelated output.
The ownership manifest is stored under the selected output root rather than a
configure tree, so deleting or replacing a CMake build tree does not lose the
cleanup record for the shared runtime output.

## CMake Integration

`cmake/PresentMonCef.cmake` provides `pmon_configure_cef()`, called from the root
`CMakeLists.txt`, and `pmon_target_uses_cef()` for future consumers.

The only CEF restore cache input is:

```cmake
PMON_CEF_SOURCE   # optional matching archive or the locked URI
```

Configure never downloads or restores CEF. The CMake stage is always the fixed
`build/ThirdParty/cef` path.

Utility targets:

- `pmon_restore_cef` explicitly restores the fixed CMake stage.
- `pmon_validate_cef` validates that stage locally and prints the exact restore
  command when validation fails.
- `pmon_stage_cef_runtime` incrementally stages the locked runtime payload into
  the selected configuration output and validates it there.
- `pmon_verify_cef` depends on runtime staging and repeatably verifies the stage,
  output payload, and owned stale-file removal without network access.

All four utility targets must be excluded from CMake CLI and generated Visual
Studio default builds until a real product consumer depends on them. The future
UI target will use `pmon_target_uses_cef()` and depend on validation; runtime
staging will become part of that product dependency graph. No product target is
wired to CEF yet.

Consumer targets:

```text
pmon_cef_headers  (INTERFACE)        build/ThirdParty/cef/Include
pmon_cef_libcef   (IMPORTED SHARED)  Bin/libcef.dll, Lib/<Config>/libcef.lib
pmon_cef_wrapper  (IMPORTED STATIC)  Lib/<Config>/libcef_dll_wrapper.lib
pmon_cef          (INTERFACE)        aggregate, alias pmon::cef
```

Debug and Release use their matching import and wrapper library directories.
`MAP_IMPORTED_CONFIG_RELWITHDEBINFO` and
`MAP_IMPORTED_CONFIG_MINSIZEREL` map only those two conventional configurations
to Release. PresentMon itself supports and generates only Debug and Release.

## Acceptance Verification

Accepted on August 3, 2026, using Visual Studio 2022, MSVC v143, and CMake
4.4.0-rc2. These are results from the fixed-destination implementation; results
from the earlier arbitrary-destination prototype were not reused.

Use a matching local archive for a network-independent restore test:

```powershell
cmake --preset windows-x64-developer `
    -DPMON_CEF_SOURCE:STRING=C:/path/to/locked-cef-archive.tar.bz2
cmake --build --preset windows-x64-developer-debug --target pmon_validate_cef
cmake --build --preset windows-x64-developer-debug --target pmon_restore_cef
cmake --build --preset windows-x64-developer-debug --target pmon_validate_cef
cmake --build --preset windows-x64-developer-release --target pmon_validate_cef
cmake --build --preset windows-x64-developer-debug --target pmon_stage_cef_runtime
cmake --build --preset windows-x64-developer-debug --target pmon_verify_cef
cmake --build --preset windows-x64-developer-debug
cmake --build --preset windows-x64-developer-release

IntelPresentMon/AppCef/Batch/pull-cef.ps1 `
    C:/path/to/locked-cef-archive.tar.bz2 -StageKind Legacy
IntelPresentMon/AppCef/Batch/validate-cef.ps1 -Mode Stage -StageKind Legacy
```

| Check | Recorded result |
| --- | --- |
| Fixed stages and source policy | Both stages validated with 313 files. Invalid stage names, extracted CMake sources, alternate URIs, wrong archive hashes, and extracted-directory lock upgrades were rejected. |
| Configure behavior | A fresh x64 configure with both stages absent succeeded without downloading or creating either stage. Win32 configure created no CEF targets. |
| Restore behavior | CMake archive restore passed in 45.3 seconds and repeated in 41.8 seconds; legacy archive restore passed in 42.2 seconds. Each restore left the other stage unchanged and left no staging or retired directories. |
| Failure diagnostics | Missing, hash-corrupt, and structurally incomplete CMake stages were rejected, and the CMake target printed the exact `pmon_restore_cef` command. |
| Concurrency | A second process waited 2,999 milliseconds on the named stage mutex and then completed, proving cross-process serialization. |
| Consumer targets | A throwaway `pmon::cef` consumer compiled, linked, and ran in Debug and Release. Its generated project selected the matching import and wrapper libraries, and validation ran before compilation. |
| Runtime staging | Debug and Release staged and validated all 18 locked files. Repeated staging was incremental; the verification target removed its owned stale sentinel, preserved an unrelated sentinel, and reused ownership state from a different configure tree. |
| Default builds | CMake CLI Debug and Release default builds performed no CEF restore, validation, or staging. All four CEF utilities had zero Visual Studio `Build.0` entries. |
| Output isolation | Developer configure retained `build`; production default and preset configure selected `build/production`. Production rejected both exact and case-variant forms of the developer root. |
| Regression | Clean x64 Debug and Release builds passed, with the service/SDK smoke test passing in both configurations. Clean Win32 Debug and Release builds passed; that configuration defines no tests. |

The two stages had identical 313-path file sets. All non-wrapper file hashes
matched; the four locally built Debug and Release wrapper library and PDB hashes
differed.

Production configure selected the SignTool backend. A signed production build
was not run because the required certificate and EDSS script are unavailable in
this environment; production signature verification remains Phase 4 product
work.

## Deferred

- Shader compilation and the PresentMonUI and KernelProcess targets.
- Wiring CEF validation and runtime staging into PresentMonUI.
- Web, preset, blocklist, and CLI payload staging.
- Application of the existing developer/production manifest policy to the UI
  and kernel process.
- Application of the existing EDSS and direct SignTool backends to Phase 4
  payloads and production signature verification.
- The deterministic Phase 4 UI and capture behavioral workflow.
