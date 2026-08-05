# CMake Migration Design

## Status

Draft. Build implementation is complete through Phase 3. Phase 4 has implemented
the fixed CEF dependency boundary, shader compilation, and the native
`PresentMonUI` and `KernelProcess` targets. The remaining Phase 4
payload, deployment, signing, behavioral verification, and final build-matrix
work are still open. Phase 2 still requires a deterministic functional backfill
under the phase-local behavioral gate; Phase 3 satisfies that gate.

This document defines a new CMake migration plan based only on the current source tree. Existing experimental branches and designs are intentionally excluded.

## Goals

- Make CMake the authoritative build entry point.
- Build every maintained project from the default configuration.
- Preserve all existing EXE, DLL, LIB, MSI, MSM, shader, and generated outputs.
- Preserve current output names and runtime staging layout during migration.
- Keep components independently selectable with CMake options.
- Express internal and external dependencies through CMake targets.
- Improve PUBLIC and PRIVATE dependency boundaries incrementally.
- Keep the Visual Studio build operational until CMake reaches verified parity.

## Initial Non-Goals

- Moving or renaming source directories.
- Rewriting WiX authoring.
- Replacing all package managers with FetchContent.
- Cross-platform support.
- Removing Visual Studio project files before parity is established.
- Broad source cleanup unrelated to the build conversion.

## Current Build Scope

The repository currently contains:

- 23 C++ projects.
- 2 C# projects.
- 3 WiX projects.
- 1 shared shader project.
- 4 Visual Studio solutions.

The main solution omits:

- `Tools/etw_list/etw_list.vcxproj`
- `Tools/pm_convert_csv/pm_convert_csv.vcxproj`

The CMake default build will include these projects as well as the projects represented by the main solution.

## Core Design Decisions

### Migration Style

CMake will be added alongside the existing MSBuild files. Targets will be converted in dependency order and compared against MSBuild outputs after each phase.

Source files will remain in their current directories initially. CMake target organization will establish the desired logical structure before any physical directory reorganization.

### Platform and Toolchain

The first implementation will support:

- Windows 10 SDK.
- Visual Studio 2022 and MSVC v143.
- Static MSVC runtime by default.
- Debug and Release configurations.
- x64 as the primary platform.
- Win32 for the existing standalone components that support it.

Existing solution-only configurations such as `Release-EDSS` and `Release-EDSS-MSI` will be represented by presets and options instead of new compiler configurations.

ARM and ARM64 are excluded. Their existing project configurations cover only part of the standalone console stack and do not form a valid native dependency graph.

### Output Contract

All profiles preserve the current externally consumed layout under `build/`:

- Runtime artifacts: `build/<Configuration>/`
- Intermediate and generated files: `build/obj/`
- Common utility libraries: `build/lib/`
- Localized installer outputs: `build/<Configuration>/en-us/`

The fixed CMake CEF dependency stage is `build/ThirdParty/cef`; it is an input
dependency, not a signed product output.

Important names to preserve include:

- `PresentMon-<version>-<platform>.exe`
- `PresentMon.exe`
- `PresentMonUI.exe`
- `PresentMonService.exe`
- `PresentMonAPI2.dll`
- `PresentMonAPI2Loader.dll`
- `Intel-PresentMon.dll`
- `Intel-PresentMon32.dll`
- `PresentMon.msi`
- `PresentMonSharedService.msm`

### Default Build and Options

All supported components will be enabled by default:

```cmake
PMON_BUILD_CONSOLE
PMON_BUILD_SERVICE
PMON_BUILD_SDK
PMON_BUILD_UI
PMON_BUILD_PROVIDER
PMON_BUILD_TOOLS
```

Deployment policy is independent of Debug and Release optimization:

```cmake
PMON_DEPLOYMENT_PROFILE=DEVELOPER|PRODUCTION
```

The developer profile uses `uiAccess=false`, does not require signing, and
produces locally runnable builds. In the production profile, KernelProcess uses
`uiAccess=true` and requires signing; `PresentMonUI` remains
`uiAccess=false` in every profile. Production payloads must be signed before
packaging.

Signing will be an explicit CMake build stage. Automatic backend selection uses
this order:

1. EDSS PowerShell backend when available.
2. Direct SignTool backend using the named certificate in `PrivateCertStore`.
3. Fail when neither backend is available.

If the EDSS backend is detected but signing fails, the signing stage must fail
instead of falling back to the test certificate. Phase 4 verifies payload
signatures before Phase 6 packaging. Phase 6 verifies the MSI after signing.

Options will control component inclusion. Individual internal libraries will follow automatically from target dependencies rather than having separate user-facing switches.

### Target Dependency Boundaries

The migration will not reproduce the repository-wide `..` and `../..` include paths. Each target will declare its own source, include, compile, and link requirements.

Dependencies used by public headers will be PUBLIC or INTERFACE. Implementation-only dependencies will be PRIVATE.

Boundary cleanup will be incremental. Build parity takes priority over large header refactors during the first pass.

### Known Internal Cycle

`PresentData` depends on `CommonUtilities`, while the `CommonUtilities/mc` sources depend on `PresentData`.

The first CMake design will separate the `CommonUtilities/mc` sources into a metrics target without moving the files:

```text
CommonUtilities base -> PresentData -> Metrics
```

Consumers that require metric calculations will link the metrics target explicitly.

Application projects currently used as reusable dependencies, especially `PresentMonService` and `CefNano`, will be split logically into implementation libraries and thin executable targets where required.

### Third-Party Dependencies

The first pass will retain vcpkg for the existing C++ dependency set. Replacing Boost and the Windows-specific packages while converting the build would introduce unnecessary version and runtime-linkage risk.

Third-party policy:

- Keep vcpkg dependencies pinned.
- Use dedicated INTERFACE targets for vendored SDK headers.
- Keep npm for the web UI.
- Keep NuGet for .NET projects.
- Restore CEF through its existing lock and hash validation.
- Avoid network access during normal CMake configure.
- Place restored or CMake-managed external content under `build/ThirdParty/`.
- Evaluate FetchContent individually after build parity.

CEF will be represented as an explicit restore/build dependency, not downloaded implicitly during configure.

UCI remains optional for developer and production builds:

```cmake
PMON_ENABLE_UCI=AUTO|ON|OFF
```

- `AUTO` is the default and enables UCI when a valid SDK is available.
- `ON` requires a valid SDK and fails configuration when it is absent.
- `OFF` builds the existing non-UCI fallback.

CMake will report whether UCI is included during configuration and packaging. A valid UCI root supplies `include/uci/uci.h`, `include/uci/uci-data-records.h`, `unified-collector-interface.dll`, and its collector runtime payload. Only required redistributable files will be staged and packaged; development headers will not be installed.

### Code Generation

Existing generation steps will become `add_custom_command()` outputs with explicit inputs and dependencies:

- Version header generation.
- Metric enumeration generation.
- Build identity generation.
- Service action aggregation.
- ETL test-case generation.
- ETW message compiler outputs.
- HLSL shader compilation.
- UCI WiX fragment generation.
- CEF and web payload staging.

Generated files should be placed in the build tree where practical. Source-tree generation will remain only when changing it would alter maintained generated artifacts.

### Installer and Merge Module

The existing WiX 3 authoring and GUID behavior will be preserved.

CMake will provide targets for:

```text
PMInstallerExtension.dll
PMInstallerLib.wixlib
PresentMon.msi
PresentMonSharedService.msm
```

The initial packaging implementation may invoke the existing WiX/MSBuild projects from CMake after staging CMake-built payloads. Project-reference rebuilding must be disabled so packaging consumes CMake outputs.

A later step may replace the wrapper with direct WiX `heat`, `candle`, `lit`, and `light` commands once parity is established.

Packaging dependencies must include:

- Native binaries.
- CEF runtime and resources.
- Web UI.
- Shaders, presets, and blocklists.
- UCI payload and generated fragment.
- Provider manifest.
- SDK headers and import libraries.
- License and third-party notices.

## Proposed Target Groups

```text
Build support
  pmon_build_options
  pmon_version_header
  pmon_generated_metadata

Foundation
  pmon_common_utilities
  pmon_present_data
  pmon_metrics
  pmon_versioning

Service and SDK
  pmon_interprocess
  pmon_api_wrapper_common
  pmon_api_wrapper
  pmon_control
  pmon_service_core
  pmon_middleware
  PresentMonAPI2
  PresentMonAPI2Loader
  PresentMonService

UI and capture
  pmon_core
  pmon_cef
  pmon_compile_shaders
  PresentMonUI
  KernelProcess

Standalone components
  PresentMonConsole
  PresentMonProvider
  ETLTrimmer
  etw_list
  pm_convert_csv
  SampleClient
  Reflector

Tests
  PresentMonTests
  PresentMonUnitTests
  PresentMonAPI2Tests

Packaging
  PMInstallerExtension
  PMInstallerLib
  PresentMonInstaller
  PresentMonMergeModule
```

Final CMake target names may retain existing product names while using aliases and folder properties for organization.

## Migration Phases

### Phase 0: Establish the Parity Baseline

- Record supported configurations and platforms.
- Record artifact names and output locations.
- Record compile definitions, runtime selection, warning levels, and link libraries.
- Produce a clean MSBuild reference build.
- Record test and package validation commands.

### Phase 1: CMake Infrastructure

Status: Complete. See [Phase 1 - CMake Infrastructure](Phase1-CMakeInfrastructure.md).

- Add root CMake project and presets.
- Add common MSVC, output, version, and generation helpers.
- Add pinned dependency integration.
- Add component options.
- Define the common output contract.

### Phase 2: Standalone and Foundation Targets

Status: Build implementation complete; behavioral verification pending. See
[Phase 2 - Foundation and Standalone Targets](Phase2-FoundationAndStandalone.md).

- Convert CommonUtilities base, PresentData, and metrics.
- Convert the console application.
- Convert Provider, ETLTrimmer, `etw_list`, and `pm_convert_csv`.
- Verify Win32 and x64 outputs.

### Phase 3: Service and SDK

Status: Complete. See [Phase 3 - Service and SDK](Phase3-ServiceAndSdk.md).

- Convert Versioning, Interprocess, wrapper libraries, ControlLib, and Core.
- Create the service implementation target and executable.
- Convert middleware, API DLLs, loader, and SampleClient.
- Preserve DLL exports, import libraries, delay loading, and runtime staging.

### Phase 4: UI and Capture

Status: In progress. CEF restore and runtime staging, shader compilation, and
the PresentMonUI and KernelProcess targets are implemented. See
[Phase 4 - UI and Capture](Phase4-UiAndCapture.md) for the current contract and
remaining plan.

- Integrate locked CEF restoration, wrapper build, and incremental runtime
  staging. Complete.
- Convert shader compilation. Complete as a standalone `pmon_compile_shaders`
  target, now wired into the `PresentMonUI` build graph through
  `pmon_target_uses_shaders()`.
- Convert UI and kernel process targets. `PresentMonUI` and `KernelProcess`
  are complete.
- Stage the remaining web, preset, blocklist, and CLI payloads. Complete.
- Apply the developer and production deployment policies to the UI, kernel
  process, and their staged payloads.
- Run the deterministic UI and representative capture/control workflow.
- Sign the complete production native payload built through Phase 4 and verify
  its signatures before Phase 6 packaging.

#### Remaining Phase 4 Work Units

Complete these units separately and stop for review after each one:

1. Convert only the remaining runtime payload. Complete.
2. Apply deployment-profile manifest policy. Both Debug and Release developer
   builds use `uiAccess=false`; the production Release build uses
   `uiAccess=true`. Replace the current configuration-only KernelProcess choice
   with `PMON_DEPLOYMENT_PROFILE`; `PresentMonUI` remains non-elevated.
3. Run and record the deterministic developer-profile UI and representative
   capture/control workflow against the complete staged payload.
4. Attach the selected EDSS or direct SignTool backend to the complete
   production native payload built through Phase 4, including the products
   converted in Phases 2 and 3. Add the Win32 production Release build required
   for the maintained x86 payload, including `Intel-PresentMon32.dll`, and
   verify every required signature after payload mutation.
5. Complete the Phase 4 build matrix and regression checks. Stop at a signed,
   signature-verified native payload; MSI and MSM packaging remain Phase 6.

#### Phase 4 Rules

The CefNano and KernelProcess registration-only translation units must be linked
directly into their executable or supplied through an object-library mechanism
that guarantees inclusion. They must not be left as unreferenced members of a
static implementation library. This applies to `CefActionRegistration.cpp` and
both `KernelActionRegistration.cpp` files and preserves the static action
registrars by the same rule established for the Phase 3 service split.

Before Phase 4 is marked complete, the phase document must define and record a
deterministic behavioral workflow for PresentMonUI and KernelProcess. The check
must exercise the CEF/web UI and a representative capture or control workflow,
including deterministic readiness and shutdown. Launch, `--help`, and artifact
checks alone do not satisfy this requirement, and Phase 5 does not own it.

#### PresentMonUI Target Integration Plan

Status: Implemented. `IntelPresentMon/AppCef/CMakeLists.txt` builds
`PresentMonUI` as a `WIN32` executable with CEF runtime staging and shader
compilation in its normal build graph. The exact target contract and current
verification results are in
[Phase 4 - UI and Capture](Phase4-UiAndCapture.md).

1. Correct the two shared prerequisites instead of working around them in the
   UI target. Make `pmon_core`'s `IS_DEBUG` definition private because only Core
   implementation files consume it and AppCef defines the same macro locally.
   Make the shared CMake version-resource copyright match `Version.props`; do
   not create a PresentMonUI-only resource definition.
2. Add `IntelPresentMon/AppCef/CMakeLists.txt` and include it after Core inside
   the existing x64 and `PMON_BUILD_UI` guard. Create a `WIN32` executable named
   `PresentMonUI` from all 19 `ClCompile` items in `CefNano.vcxproj`,
   `AppCef.rc`, and `compatibility.manifest`. Keep
   `CefActionRegistration.cpp` and `KernelActionRegistration.cpp` directly in
   the executable so their static registration cannot be discarded. Normalize
   the four malformed AppCef sibling-relative includes to the existing
   target-root include form instead of adding an AppCef include-directory shim.
3. Reproduce the legacy target contract with target-scoped CMake: MultiByte and
   static CRT, the version/build macros, `/EHa`, `/fp:fast`, matching debug
   information format (`/ZI`+JMC for Debug, `/Zi` for Release) and Release
   function-level linking, intrinsics, and Control Flow Guard, and whole-program
   behavior through `INTERPROCEDURAL_OPTIMIZATION_RELEASE`, the Windows
   subsystem, the embedded compatibility manifest, and the existing output name
   and directory. Link the existing CommonUtilities, Core, Interprocess,
   Versioning, CEF, and build policy targets plus the explicit DWrite, Dwmapi,
   and Shcore dependencies; do not link Boost.Process, which legacy `CefNano`
   does not link either. Do not add legacy CEF paths or call `post-build.bat`.
4. Put CEF runtime staging and shader compilation in the product graph.
   Building PresentMonUI incrementally copies the fixed CMake CEF runtime and
   compiles both shaders. Normal builds do not download or validate CEF. Restore
   validates before publishing, and integrity verification remains an explicit
   CI operation.
5. Verify a fresh x64 Debug and Release default build, generated Visual Studio
   Build Solution, failure and recovery for a missing or invalid CEF stage,
   incremental rebuild behavior, `PMON_BUILD_UI=OFF`, Win32 exclusion,
   x64/Windows-subsystem imports, version/icon/manifest resources,
   and an unchanged legacy CEF-stage fingerprint. Record the commands and
   results in the Phase 4 document, then stop for review. This gate establishes
   a correctly built and staged native executable; it does not claim UI runtime
   parity before the separately scoped web and behavioral work is complete.

#### CEF Restoration Plan

Status: Implemented. Results are recorded in
[Phase 4 - UI and Capture](Phase4-UiAndCapture.md).

1. Update `cef-lock.psm1`, `pull-cef.ps1`, and `validate-cef.ps1` to select
   exactly one of two named stages: `Legacy` at `IntelPresentMon/AppCef/Cef` or
   `CMake` at `build/ThirdParty/cef`. `Legacy` remains the default so existing
   MSBuild callers are unchanged. Do not expose an arbitrary published stage
   destination.
2. Add an explicit `pmon_restore_cef` target for the fixed CMake stage. It is
   excluded from CMake CLI and generated Visual Studio default builds, and
   configure must not download CEF. A CMake-stage restore accepts only the
   locked URI or a matching archive, builds in temporary storage, validates the
   result, and publishes a complete stage. An extracted-directory fallback is
   available only to the legacy pull workflow; lock upgrades require a URI or
   archive so every lock records an archive SHA-256.
3. Keep integrity checks out of normal product builds. Restore validates the
   lock metadata, runtime payload, headers, import libraries, and Debug and
   Release wrapper libraries before publishing the CMake stage. The explicit
   `pmon_verify_cef` CI target validates the published stage and copied payload
   without downloading anything.
4. Add an interface target for the CEF headers and imported targets for
   `libcef` and the Debug and Release wrapper libraries. Consumers use these
   targets instead of paths under `IntelPresentMon/AppCef/Cef`.
5. Add incremental CMake staging with declared outputs and dependencies. Copy
   only the locked CEF runtime files into the selected build output using
   copy-if-different behavior. Do not perform hash validation during normal
   product builds.

Verification must prove that the legacy stage still works, CMake restores and
validates its fixed separate stage, normal configure performs no download, a
corrupt or structurally incomplete stage is rejected, a repeated restore
succeeds, Debug and Release select the correct wrapper libraries, and CMake
never changes the legacy stage. Normal builds include runtime staging only when
a real CEF consumer is enabled; restore and CI verification remain explicit.

### Phase 5: Tests

- Convert the GTest regression executable.
- Integrate Visual Studio C++ Unit Test projects.
- Preserve auxiliary data restoration and generated test cases.
- Register the remaining applicable tests with CTest.

Phase 5 owns conversion of the complete maintained test suites and registration
of the remaining applicable tests with CTest. It does not defer behavioral
verification of products or subsystems converted in earlier phases.

### Phase 6: Packaging

- Build the WiX preprocessor extension.
- Generate the UCI WiX fragment.
- Build the shared WiX library.
- Build and validate MSI and MSM outputs.
- Verify package contents and installation behavior.

### Phase 7: Default Build and Cutover

- Ensure a default clean build includes every enabled target.
- Add CI presets and clean-build validation.
- Compare CMake and MSBuild artifacts.
- Update build documentation.
- When MSBuild is retired, remove the `Legacy` CEF stage kind, its callers, and
  `IntelPresentMon/AppCef/Cef`. The fixed `build/ThirdParty/cef` path then
  becomes the only published CEF stage.
- Retire MSBuild files only after explicit approval and sustained parity.

### Phase 8: Structural Cleanup

- Reorganize physical directories if still beneficial.
- Tighten installed/public header boundaries.
- Remove obsolete conditional project references.
- Evaluate dependency migration from vcpkg to FetchContent.
- Replace packaging wrappers with direct WiX commands if useful.

## Verification Gates

Each phase must pass its applicable gates before it is marked complete. Every
runnable product or subsystem added or materially changed in a phase must have
a representative behavioral workflow that is run in that phase. Process
startup, `--help`, artifact existence, and binary inspection are supporting
checks, but do not by themselves satisfy the behavioral gate.

Phase-local checks may be focused smoke or integration procedures and do not
require CTest registration before Phase 5. The phase document must record the
command or procedure, expected result, actual result, and any environment
limitation that prevents an applicable check from running. An
environment-qualified omission is recorded as such; it is not a passing result.

- CMake configure succeeds from a clean checkout.
- Clean build succeeds with no reliance on stale MSBuild outputs.
- Expected artifacts exist with exact names and architectures.
- DLL exports and required imports match the baseline.
- Generated resources and manifests are present.
- Applicable automated tests and phase-local behavioral checks pass after the
  build succeeds.
- Runtime payloads launch or load from the staged output and complete their
  representative workflows.
- MSI and MSM contain the expected files and metadata.
- Installer upgrade, service, provider, PATH, and cleanup behavior remains intact.

## Immediate Next Step

PresentMonUI, KernelProcess, CEF runtime staging, shader compilation, and the
remaining runtime payload are implemented. The next implementation unit is
deployment-profile manifest policy for KernelProcess. Stop for review before
signing or behavioral verification.
