# Phase 0 - Parity Baseline

## Status

Complete for the primary x64 migration baseline.

Win32 builds, signed Release packaging, and UCI-inclusive packaging require prerequisites or validation not available in the current environment. ARM and ARM64 were reviewed and excluded from the CMake scope.

## Purpose

This document records the current MSBuild behavior that the CMake build must preserve. It is a compatibility contract, including current failures and environment dependencies.

## Baseline Revision

- Branch: `feature/cm`
- Commit: `e78256372b028a5ab0c0ce76c159777dfc23cde6`
- Date: 2026-07-30
- Source changes during baseline: none
- Design files were untracked while the baseline was captured.

## Environment

- OS: Windows 10.0.26200
- Visual Studio: 2022 Professional 17.14
- MSBuild: 17.14.40
- MSVC: 19.44.35228
- Windows SDK selected by CEF: 10.0.26100.0
- CMake: 4.2.0-rc2
- .NET SDK: 9.0.315
- Node.js: 24.11.0
- npm: 11.6.1
- WiX: 3.14.1.8722
- WiX environment: `WIX=C:\Program Files (x86)\WiX Toolset v3.14\`

Environment gaps:

- `PMON_UCI_SDK_DIR` is not set.
- `IntelPresentMon/ControlLib/uci/external` is absent.
- The Release signing certificate is not installed in `PrivateCertStore`.
- `msbuild.exe` is not on `PATH`; the Visual Studio installation path must be resolved.

## Restored Inputs

The repository bootstrap completed successfully:

```powershell
.\bootstrap.ps1
```

It performed:

- Locked CEF download, hash validation, extraction, and wrapper builds.
- Auxiliary test-data restoration.
- Web UI dependency installation and build.

CEF baseline:

- Version: `149.0.4+g2f1bfd8+chromium-149.0.7827.156`
- Architecture: x64
- CEF wrapper configurations: Debug and Release
- Runtime and resource staging is validated against `IntelPresentMon/AppCef/cef-lock.json`.

## Existing Build Scope

Tracked project types:

- 23 C++ projects.
- 2 C# projects.
- 3 WiX projects.
- 1 shared shader project.
- 4 Visual Studio solutions.

Projects omitted from the main solution:

- `Tools/etw_list/etw_list.vcxproj`
- `Tools/pm_convert_csv/pm_convert_csv.vcxproj`

`Reflector/Reflector.csproj` is present in the main solution but is not selected for solution builds.

Conditional references to the absent `IntelPresentMon/PresentMonUtils/PresentMonUtils.vcxproj` exist in ControlLib and PresentMonService.

## Configuration and Platform Matrix

### Main Solution

`PresentMon.sln` advertises:

- `Debug|x64`
- `Debug|x86`
- `Release|x64`
- `Release|x86`
- `Release-EDSS|x64`
- `Release-EDSS|x86`
- `Release-EDSS-MSI|x64`
- `Release-EDSS-MSI|x86`

These are solution mappings, not a uniform project matrix:

- Most Intel PresentMon projects are x64-only.
- Several x86 solution mappings select x64 project configurations without building them.
- WiX projects use an x86 project platform while producing x64 packages.
- `Release-EDSS-MSI` primarily selects packaging projects.

### Standalone Console and Regression Tests

The console, PresentData, and regression test projects advertise:

- Win32
- x64
- ARM
- ARM64

Observed limitations:

- CommonUtilities does not provide native ARM or ARM64 project configurations.
- ARM and ARM64 solution mappings can select x64 CommonUtilities.
- PresentMon ETW generation commands are only defined for x64.

These incomplete ARM and ARM64 configurations are excluded from the CMake implementation.

### Verified Matrix

Verified in this phase:

- Main solution Debug x64 clean rebuild: success.
- Main solution Release x64 clean rebuild: native outputs and MSM produced; overall build failed at signing.
- `etw_list` Debug and Release x64 clean rebuilds: success.
- `pm_convert_csv` Debug and Release x64 clean rebuilds: success.
- Reflector Debug and Release x64 builds: success.

Not executed:

- Win32 clean rebuilds.
- ARM clean rebuilds.
- ARM64 clean rebuilds.
- Release-EDSS clean rebuild.
- Signed Release build.

## Shared Compilation Contract

Common behavior that CMake must model:

- Visual Studio 2022 `v143` toolset.
- Windows SDK 10.
- C++ latest language mode for Intel PresentMon targets.
- Static MSVC runtime by default:
  - Debug: `/MTd`
  - Release: `/MT`
- `CEREAL_THREAD_SAFE=1`.
- `BOOST_ALLOW_DEPRECATED_HEADERS`.
- Multiprocess compilation.
- Unicode for most targets; CefNano uses MultiByte.
- Release function-level linking, COMDAT folding, and reference optimization where configured.
- Control Flow Guard where configured.
- `/EHa` on Intel targets that currently enable asynchronous exception handling.
- `/we4062` on Interprocess and PresentMonMiddleware.

Warning policy:

- PresentMon, PresentData, Provider, and standalone tools generally use `/W4`.
- `PresentMon.props` enables warnings as errors for the console build.
- Intel PresentMon targets generally use `/W3`.
- Third-party headers are treated as external includes with warnings disabled where configured.

Existing broad include behavior:

- `IntelPresentMon/Common.props` injects `..` and `../..`.
- CMake must replace these with target-scoped includes, but temporary compatibility includes may be required while targets are converted.

## Windows Link Contract

The project files and source pragmas require Windows libraries including:

- `advapi32`
- `d2d1`
- `d3d11`
- `d3dcompiler`
- `dcomp`
- `dwrite`
- `dwmapi`
- `dxgi`
- `ntdll`
- `ole32`
- `pdh`
- `rpcrt4`
- `shell32`
- `shcore`
- `shlwapi`
- `tdh`
- `user32`
- `wbemuuid`
- `winmm`

CMake targets must declare these explicitly rather than relying on transitive project behavior or `#pragma comment(lib)` alone.

## Artifact Contract

### Debug x64 Main Solution

The clean rebuild produced:

```text
build/Debug/ControlLib.lib
build/Debug/Core.lib
build/Debug/ETLTrimmer.exe
build/Debug/Intel-PresentMon.dll
build/Debug/Interprocess.lib
build/Debug/IPMVersioning.lib
build/Debug/PresentData.lib
build/Debug/PresentMon-2.6.0-x64.exe
build/Debug/PresentMon.exe
build/Debug/PresentMonAPI2.dll
build/Debug/PresentMonAPI2.lib
build/Debug/PresentMonAPI2Loader.dll
build/Debug/PresentMonAPI2Loader.lib
build/Debug/PresentMonAPI2Tests.dll
build/Debug/PresentMonAPIWrapper.lib
build/Debug/PresentMonAPIWrapperCommon.lib
build/Debug/PresentMonMiddleware.lib
build/Debug/PresentMonService.exe
build/Debug/PresentMonTests-2.6.0-x64.exe
build/Debug/PresentMonUI.exe
build/Debug/PresentMonUnitTests.dll
build/Debug/SampleClient.exe
build/lib/CommonUtilities-x64-Debug/IPMCommonUtilities.lib
```

The UI post-build also stages CEF binaries, CEF resources, shaders, presets, blocklists, CLI wrappers, and web assets under `build/Debug`.

### Release x64 Main Solution

Before the signing failure, the clean rebuild produced the Release equivalents plus:

```text
build/obj/PMInstallerExtension-Release/PMInstallerExtension.dll
build/Release/PMInstallerLib.wixlib
build/Release/en-us/PresentMonSharedService.msm
```

`build/Release/PresentMon.exe` was linked successfully before its post-build signing command failed.

`PresentMon.msi` was not produced because the MSI project depends on the failed KernelProcess project.

### Projects Outside the Default Build

Verified outputs:

```text
build/Debug/etw_list-2.6.0-x64.exe
build/Release/etw_list-2.6.0-x64.exe
build/Debug/pm_convert_csv.exe
build/Release/pm_convert_csv.exe
Reflector/bin/Debug/net8.0-windows10.0.17763.0/win-x64/Reflector.dll
Reflector/bin/Release/net8.0-windows10.0.17763.0/win-x64/Reflector.dll
```

The Reflector project is an executable with an apphost even though the build summary reports its managed DLL.

## Generated and Staged Outputs

CMake must preserve these generation relationships:

- `Tools/generate_version_header.cmd`
  - Input: version properties.
  - Output: `build/obj/generated/version.h`.
- `Tools/generate/EnumMetric/generate.cmd`
  - Inputs: `IntelPresentMon/metrics.csv`, script, and awk program.
  - Output: `build/obj/generated/metadata/EnumMetric.h`.
- `IntelPresentMon/Versioning/scripts/pre-build.ps1`
  - Inputs: Git revision and worktree state.
  - Outputs: build ID and signature files.
- `IntelPresentMon/PresentMonService/build-scripts/pre-build.bat`
  - Output: `IntelPresentMon/PresentMonService/AllActions.h`.
- `IntelPresentMon/PresentMonAPI2Tests/GenerateEtlCsvTestCases.ps1`
  - Output: test intermediate generated header.
- `mc.exe` processing:
  - `PresentMon/ddETWExternal.xml`
  - `Provider/Intel-PresentMon.man`
- HLSL compilation:
  - `IntelPresentMon/Shaders/Line_PS.hlsl`
  - `IntelPresentMon/Shaders/Line_VS.hlsl`
- CEF post-build:
  - Runtime binaries and resources.
  - Web UI.
  - Presets and blocklist.
- `IntelPresentMon/PMInstallerLib/wix-uci-dist.ps1`
  - Output: `IntelPresentMon/PMInstallerLib/Generated/UciDist.wxs`.

## Packaging Contract

Required package outputs:

```text
build/Release/PMInstallerLib.wixlib
build/Release/en-us/PresentMon.msi
build/Release/en-us/PresentMonSharedService.msm
```

Required ordering:

1. Native payloads and import libraries.
2. CEF, shader, web, preset, and blocklist staging.
3. PMInstallerExtension build.
4. UCI fragment generation.
5. PMInstallerLib build.
6. MSI and MSM linking.
7. Signing where enabled.

Observed packaging behavior:

- Missing UCI payload is a warning, not a build error.
- PMInstallerLib and the MSM build without UCI files.
- Release KernelProcess signing is mandatory in the MSBuild project.
- Missing signing certificate causes the MSI dependency graph to fail.
- Building PMInstaller directly with `BuildProjectReferences=false` does not preserve WiX project-reference target variables. It incorrectly resolves service payloads under `Release/` instead of `build/Release/`.

The CMake packaging implementation must pass payload paths explicitly and must not rely on MSBuild solution variables being synthesized accidentally.

## Baseline Build Commands and Results

### Debug x64

```powershell
MSBuild.exe PresentMon.sln /t:Rebuild /m /p:Configuration=Debug /p:Platform=x64
```

Result: success.

### Release x64

```powershell
MSBuild.exe PresentMon.sln /t:Rebuild /m /p:Configuration=Release /p:Platform=x64
```

Result: failed after native linking because SignTool could not find the required certificate. The MSM and shared wixlib were produced; the MSI was not.

### Omitted Tools

Debug and Release x64 clean rebuilds of both standalone tool solutions succeeded.

### Reflector

```powershell
dotnet build Reflector/Reflector.csproj --configuration Debug
dotnet build Reflector/Reflector.csproj --configuration Release
```

Result: success.

## Baseline Test Results

### Existing Test Driver

Command:

```powershell
Tools/run_tests.cmd x64 debug nobuild norealtime
```

Result: failed before test execution.

The script reads `PresentMonVersion` from `PresentMon.props`, but the version now resides in `Version.props`. The empty version then causes a cmd parsing failure.

This is a current baseline defect. CMake/CTest integration must not copy this version lookup.

### PresentMon Regression Tests

The test executable was run directly with the Debug x64 console and Gold directory.

Results:

- Total: 25
- Passed: 19
- Failed: 6
- Gold ETL/CSV tests: 18 of 18 passed.
- Command-line realtime/session tests: 1 of 7 passed.

The six failures returned PresentMon exit code 6 or failed to establish an ETW session. The environment does not have the documented Performance Log Users setup or elevation required for realtime tests.

### PresentMon Unit Tests

Results:

- Total: 289
- Passed on initial run: 288
- Failed on initial run: 1
- Failed test: `QpcTimerSpinChrono`
- Isolated rerun: passed.

The failure was a timing tolerance miss and is considered a flaky baseline test.

### PresentMon API2 Tests

Results:

- Total: 128
- Passed: 107
- Failed: 21

Failures were concentrated in realtime presenter tracking, frame-store, multi-client, and realtime metric cases. Many failed while starting tracking or communicating with PresentBench. Non-realtime playback, introspection, telemetry, and UI launch tests substantially passed.

The full API2 suite requires a machine environment suitable for realtime ETW capture and remains an environment-qualified gate.

## Current Baseline Defects and Risks

These behaviors predate CMake and must not be misclassified as migration regressions:

- Release build cannot complete without a specifically named local certificate.
- Signing uses SHA-1 and an obsolete timestamp endpoint.
- Missing UCI silently produces reduced packages.
- The standard test driver cannot parse the current version location.
- Realtime tests fail without ETW permissions or elevation.
- One timing test is observably flaky.
- Direct WiX project builds depend on solution-generated target path variables.
- ARM and ARM64 mappings do not represent a complete native dependency graph.
- Main solution default build omits two tools and Reflector.
- Source-tree and shared build-tree generators rely on implicit working directories.

## CMake Parity Gates Derived from Phase 0

The CMake migration must:

- Produce all verified Debug and Release x64 native artifacts.
- Include `etw_list`, `pm_convert_csv`, and Reflector in the default build.
- Preserve exact public binary names and staging locations.
- Preserve static CRT selection.
- Preserve resource, ETW manifest, HLSL, CEF, and web generation.
- Provide separate developer and production deployment profiles.
- Require signed payloads for production packaging.
- Support optional UCI in developer and production builds.
- Report clearly whether UCI is included in each build and package.
- Build wixlib, MSI, and MSM from explicit CMake artifact paths.
- Pass all 18 Gold ETL/CSV tests.
- Pass the unit suite, allowing investigation of known timing flakiness.
- Distinguish realtime tests from deterministic tests.
- Preserve supported Win32 components.
- Exclude ARM and ARM64.

## Deferred Baseline Work

The following work requires environment or product-owner input:

- Install or provide the expected signing certificate and capture a signed Release/MSI baseline.
- Supply a UCI SDK payload and capture complete MSI/MSM contents.
- Run realtime tests with the documented permissions.
- Validate Win32.

## Decisions from Phase 0 Review

### Platform Scope

The CMake migration supports x64 and the existing Win32 components. ARM and ARM64 are excluded.

ARM support was historically intentional for PresentMon, PresentData, and PresentMonTests, but it does not extend across Intel PresentMon. CommonUtilities maps ARM configurations to x64, and ETW generation is incomplete for ARM. The current project graph is not a valid native ARM build.

### Deployment Profiles

Optimization configuration and deployment policy are separate concepts:

- `DEVELOPER`
  - Supports Debug or Release optimization.
  - Uses `uiAccess=false`.
  - Does not require signing.
  - Produces binaries that run locally for normal same-integrity applications.
- `PRODUCTION`
  - Uses Release optimization.
  - Uses `uiAccess=true`.
  - Requires signed payloads before packaging.
  - Preserves overlay support across Windows integrity boundaries.

The existing `Release-EDSS` and `Release-EDSS-MSI` configurations will become CMake presets and build stages rather than custom compiler configurations.

### Signing Workflow

The existing production script at `C:\PresentMonBuilder\full-sign-build.ps1` establishes the intended workflow:

1. Build `Release-EDSS` x64 and x86 payloads.
2. Sign payload binaries with the EDSS backend.
3. Build `Release-EDSS-MSI` with project-reference rebuilding disabled.
4. Sign the MSI.

CMake will preserve this ordering with explicit build, sign, package, and package-sign stages.

Automatic signing backend selection:

1. Use the EDSS PowerShell backend when available.
2. Otherwise invoke SignTool directly with the named `PrivateCertStore` certificate.
3. Fail if neither backend is available.

If EDSS is detected but signing fails, signing fails immediately. It does not silently fall back to the test certificate. CMake will verify signatures before production packaging.

The normal MSBuild `Release|x64` failure recorded above uses the legacy local test-certificate post-build command. It is not representative of the EDSS production workflow.

### Optional UCI

UCI remains optional for developer and production builds.

CMake will provide:

```text
PMON_ENABLE_UCI=AUTO|ON|OFF
```

- `AUTO`: default; enable UCI when a valid SDK is found.
- `ON`: require UCI and fail configuration when it is absent.
- `OFF`: build the existing non-UCI fallback.

A valid UCI SDK root is supplied through a CMake path setting or the existing `PMON_UCI_SDK_DIR` environment variable. It contains:

```text
include/uci/uci.h
include/uci/uci-data-records.h
unified-collector-interface.dll
collectors/...
```

UCI provides CPU package power, CPU temperature, and per-core temperature telemetry through the elevated service. The service dynamically loads `unified-collector-interface.dll`.

CMake must explicitly report whether UCI is compiled and packaged. It will stage only required runtime redistributables instead of copying the entire SDK root.

## Phase 0 Outcome

The primary x64 artifact and behavior contract is established. Phase 1 can begin without changing the current source layout or assuming that existing baseline failures must be fixed as part of the CMake conversion.
