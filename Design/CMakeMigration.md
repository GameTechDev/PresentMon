# CMake Migration Design

## Status

Draft. Build implementation is complete through Phase 3. Phase 4 is in
progress, and its fixed CEF dependency boundary passed preflight acceptance.
Phase 2 still requires a deterministic functional backfill under the
phase-local behavioral gate; Phase 3 satisfies that gate.

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

The developer profile preserves the current externally consumed layout under
the `build/` output root. The production profile uses the isolated
`build/production/` root so manifest and signature state cannot overwrite a
developer payload. Within the selected profile root, the layout is:

- Runtime artifacts: `<output-root>/<Configuration>/`
- Intermediate and generated files: `<output-root>/obj/`
- Common utility libraries: `<output-root>/lib/`
- Localized installer outputs: `<output-root>/<Configuration>/en-us/`

Production staging, signing, verification, and packaging consume the production
root exclusively. The fixed CMake CEF dependency stage remains shared at
`build/ThirdParty/cef`; it is an input dependency, not a signed product output.

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
PMON_BUILD_TESTS
PMON_BUILD_INSTALLER
PMON_BUILD_MERGE_MODULE
PMON_BUILD_REFLECTOR
```

Deployment policy is independent of Debug and Release optimization:

```cmake
PMON_DEPLOYMENT_PROFILE=DEVELOPER|PRODUCTION
```

The developer profile uses `uiAccess=false`, does not require signing, and produces locally runnable builds. The production profile uses `uiAccess=true` and requires signed payloads before packaging.

Signing is implemented as an explicit CMake build stage. Automatic backend selection uses this order:

1. EDSS PowerShell backend when available.
2. Direct SignTool backend using the named certificate in `PrivateCertStore`.
3. Fail when neither backend is available.

If the EDSS backend is detected but signing fails, the build fails instead of falling back to the test certificate. CMake verifies payload signatures before building production packages and verifies the MSI after signing.

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
  pmon_cef_support
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
- Add artifact manifest and parity-check support.

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

Status: In progress. The CEF Restoration Plan below is accepted. See
[Phase 4 - UI and Capture](Phase4-UiAndCapture.md) for the implementation and
verification record.

- Integrate locked CEF restoration and wrapper build.
- Convert shader compilation.
- Convert UI and kernel process targets.
- Stage CEF, web, preset, blocklist, and CLI payloads.
- Apply the existing developer and production deployment profiles to the UI,
  kernel process, and their staged payloads.
- Apply the existing EDSS and direct SignTool signing backends to Phase 4
  payloads.
- Verify production signatures before packaging.

#### Phase 4 Preflight Rules

The CefNano and KernelProcess registration-only translation units must be linked
directly into their executable or supplied through an object-library mechanism
that guarantees inclusion. They must not be left as unreferenced members of a
static implementation library. This applies to `CefActionRegistration.cpp` and
both `KernelActionRegistration.cpp` files and preserves the static action
registrars by the same rule established for the Phase 3 service split.

Developer and production outputs must not share a runtime root. Developer
presets retain the parity layout under `build/<Configuration>/`; the production
preset uses `build/production/<Configuration>/`. Production staging, signing,
signature verification, and packaging must consume only the isolated production
root. This separation must be verified before production payload work begins.

Before Phase 4 is marked complete, the phase document must define and record a
deterministic behavioral workflow for PresentMonUI and KernelProcess. The check
must exercise the CEF/web UI and a representative capture or control workflow,
including deterministic readiness and shutdown. Launch, `--help`, and artifact
checks alone do not satisfy this requirement, and Phase 5 does not own it.

#### CEF Restoration Plan

Status: Accepted on August 3, 2026. Steps 1 through 5 and the fixed-stage
verification contract passed. Results are recorded in
[Phase 4 - UI and Capture](Phase4-UiAndCapture.md).

Complete and review steps 1 through 3 before converting the UI target.

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
3. Add a non-downloading `pmon_validate_cef` target. It validates the lock
   metadata and runtime payload plus the headers, import libraries, and Debug
   and Release wrapper libraries required by consumers. Once the UI target is
   added, its build depends on validation. Missing or invalid CEF fails with the
   exact `pmon_restore_cef` command to run.
4. Add an interface target for the CEF headers and imported targets for
   `libcef` and the Debug and Release wrapper libraries. Consumers use these
   targets instead of paths under `IntelPresentMon/AppCef/Cef`.
5. Add incremental CMake staging with declared outputs and dependencies. Copy
   the locked CEF runtime files into the selected build output, remove only
   stale files previously owned by that staging command, and validate every
   locked file after copying. Add a default-excluded `pmon_verify_cef` target
   that exercises stage and output validation plus owned stale-file removal
   without network access.

Verification must prove that the legacy stage still works, CMake restores and
validates its fixed separate stage, normal configure performs no download, a
corrupt or structurally incomplete stage is rejected, a repeated restore
succeeds, Debug and Release select the correct wrapper libraries, and CMake
never changes the legacy stage. It must also prove that both CMake CLI default
builds and generated Visual Studio Build Solution omit the four CEF utility
targets until a real consumer depends on them.

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

Convert shader compilation and the UI and kernel process targets under the
Phase 4 preflight rules. Then wire the accepted CEF boundary into the UI,
complete runtime payload staging, and apply the existing deployment and signing
infrastructure.
