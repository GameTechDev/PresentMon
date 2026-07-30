# Phase 1 - CMake Infrastructure

## Status

Complete for the infrastructure-only scope. No product binary, test, or packaging target was converted in this phase.

## Scope

Phase 1 established:

- A root CMake project for Windows, Visual Studio 2022, and MSVC.
- Shared x64 developer and production presets.
- Debug and Release build presets.
- Component options with all maintained components enabled by default.
- Version parsing from `Version.props`.
- Shared MSVC runtime, language, warning, and output policy targets.
- Pinned vcpkg manifest integration.
- Optional UCI discovery.
- Production signing backend selection.
- Artifact registration and verification support.

## Added Build Files

- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/PresentMonArtifacts.cmake`
- `cmake/PresentMonCompiler.cmake`
- `cmake/PresentMonOptions.cmake`
- `cmake/PresentMonOutput.cmake`
- `cmake/PresentMonSigning.cmake`
- `cmake/PresentMonUci.cmake`
- `cmake/PresentMonVcpkg.cmake`
- `cmake/PresentMonVersion.cmake`
- `cmake/VerifyArtifacts.cmake`

`CMakeUserPresets.json` is ignored so machine-specific paths and overrides remain local.

## Presets

Configure presets:

- `windows-x64-dependencies`: explicitly restores pinned vcpkg dependencies.
- `windows-x64-developer`: configures a developer deployment.
- `windows-x64-production`: configures a production deployment.

Build presets:

- `windows-x64-developer-debug`
- `windows-x64-developer-release`
- `windows-x64-production-release`

Normal developer and production configuration does not permit vcpkg network restoration. Dependency presets perform explicit restores into per-triplet directories under `build/ThirdParty/vcpkg_installed`, with source downloads under `build/ThirdParty/vcpkg-downloads`.

## Component Options

These Boolean options default to `ON`:

- `PMON_BUILD_CONSOLE`
- `PMON_BUILD_SERVICE`
- `PMON_BUILD_SDK`
- `PMON_BUILD_UI`
- `PMON_BUILD_PROVIDER`
- `PMON_BUILD_TOOLS`
- `PMON_BUILD_TESTS`
- `PMON_BUILD_INSTALLER`
- `PMON_BUILD_MERGE_MODULE`
- `PMON_BUILD_REFLECTOR`

Policy options:

- `PMON_DEPLOYMENT_PROFILE=DEVELOPER|PRODUCTION`
- `PMON_ENABLE_UCI=AUTO|ON|OFF`
- `PMON_SIGNING_BACKEND=AUTO|EDSS|SIGNTOOL`
- `PMON_USE_VCPKG=ON|OFF`
- `PMON_AUTO_RESTORE_VCPKG=ON|OFF`

Invalid enum values fail configuration.

## Output Contract

The default artifact root is `build`.

- Runtime, shared library, import library, and PDB outputs use `build/<Configuration>/`.
- Compiler PDB outputs use `build/obj/<Configuration>/`.
- Generator build trees use `build/cmake/<preset>/`.
- Restored C++ dependencies use `build/ThirdParty/`.

Target-specific exceptions such as `build/lib/CommonUtilities-x64-<Configuration>` will be assigned when those targets are converted.

## Compiler Contract

`pmon::build_options` provides:

- Parallel compilation.
- Conforming preprocessor and language behavior through `/permissive-`.
- `/std:c++latest`.
- Suppressed warnings for external headers.
- Existing shared cereal and Boost definitions.

`pmon::warnings_default` supplies `/W3`. `pmon::warnings_strict` supplies `/W4 /WX` and linker warnings-as-errors. Converted targets will select the policy matching their MSBuild baseline.

The static MSVC runtime is the default for both Debug and Release.

## UCI Behavior

Discovery order:

1. `PMON_UCI_ROOT`
2. Environment variable `PMON_UCI_SDK_DIR`
3. `IntelPresentMon/ControlLib/uci/external`

`AUTO` disables UCI when the SDK is absent. `ON` fails configuration when required headers or the runtime DLL are absent. `OFF` disables discovery. The `pmon::uci_sdk` target carries the SDK include path when UCI is enabled.

Runtime collector staging and packaging remain part of the target and packaging phases.

## Signing Behavior

Developer configurations select no signing backend.

Production `AUTO` selection uses:

1. An explicitly configured EDSS script, the `PMON_EDSS_SIGN_SCRIPT` environment variable, or `C:/PresentMonBuilder/full-sign-build.ps1`.
2. The newest Windows SDK SignTool.
3. Configuration failure when neither exists.

Explicit `EDSS` and `SIGNTOOL` selections fail configuration when the requested tool is unavailable. Artifact signing, certificate validation, signature verification, and packaging order remain Phase 4 work.

## Artifact Verification

Converted targets will call `pmon_register_artifact()` for externally consumed outputs. The `pmon_verify_artifacts` target checks the generated per-configuration manifest and fails when an expected artifact is missing.

Phase 1 registers no product artifacts because no product targets exist yet.

## Verification

Verified on Windows with Visual Studio 2022, MSVC 19.44, and Windows SDK 10.0.26100.0:

```powershell
cmake --preset windows-x64-dependencies
cmake --preset windows-x64-developer
cmake --build --preset windows-x64-developer-debug
cmake --build --preset windows-x64-developer-release
cmake --build --preset windows-x64-developer-debug --target pmon_verify_artifacts
cmake --preset windows-x64-production
cmake --build --preset windows-x64-production-release
```

Results:

- Pinned vcpkg dependencies restored successfully.
- Developer Debug and Release infrastructure builds succeeded.
- Production Release infrastructure build succeeded and selected EDSS first.
- Removing the EDSS script from consideration selected SignTool.
- `PMON_ENABLE_UCI=AUTO` configured without the unavailable SDK.
- `PMON_ENABLE_UCI=ON` failed as required when the SDK was unavailable.
- Artifact verification succeeded with the expected Phase 1 count of zero.

## Deferred

- Product and code-generation targets.
- Win32 presets for the limited standalone target set.
- CEF, web UI, and test-data restore targets.
- UCI runtime payload staging.
- Signing commands and signature verification.
- MSI and MSM integration.
