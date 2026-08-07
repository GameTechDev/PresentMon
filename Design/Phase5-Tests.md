# Phase 5 - Tests

## Status

In progress (CMake targets and CTest registration landed on `feature/cm`). MSBuild test projects remain the parity reference until CMake runs are fully verified in CI.

## Purpose

Phase 5 adds CMake build targets for the three native test suites, preserves
existing test frameworks (Google Test and the Visual Studio C++ Unit Test
Framework), preserves auxiliary-data and generated API2 test-case inputs, and
registers runnable tests with CTest so developers and CI can run
`ctest` against CMake-built artifacts.

Phase 5 does **not** migrate test source to Catch2 or another framework, rewrite
test logic, or fix baseline environment failures documented in
[Phase 0 - Parity Baseline](Phase0-ParityBaseline.md).

## Scope

### In scope

- CMake targets that reproduce the three MSBuild test projects.
- `PMON_BUILD_TESTS` component option (default `ON` when other components allow).
- Explicit auxiliary test-data restore target (no download during normal configure).
- `GenerateEtlCsvTestCases.ps1` integration for API2 tests.
- CTest registration with labels separating deterministic and environment-heavy
  tests.
- Documentation updates in this file, `BUILDING.md`, and
  [CMake Migration Design](CMakeMigration.md) when the phase completes.

### Out of scope

- C# **Reflector** (not a C++ test project; optional `dotnet test` is Phase 7
  documentation unless explicitly pulled forward).
- Rewriting `Tools/run_tests.cmd` to require CMake (may add a thin CMake-aware
  wrapper or document `ctest` as the CMake path; MSBuild script can remain for
  legacy workflows until cutover).
- Fixing `run_tests.cmd` version lookup (`Version.props` vs `PresentMon.props`);
  CMake/CTest must use `PMON_VERSION` from CMake, not copy the broken cmd
  parsing.
- MSI/MSM and packaging (Phase 6).

## Current MSBuild Baseline

| Project | Path | Output | Framework | Platforms |
| --- | --- | --- | --- | --- |
| Console regression | `Tests/PresentMonTests.vcxproj` | `build/<Config>/PresentMonTests-<version>-<arch>.exe` | GTest (amalgamated `gtest-all.cc` via vcpkg) | x64, Win32 (CMake: same as console) |
| Intel unit tests | `IntelPresentMon/UnitTests/UnitTests.vcxproj` | `build/<Config>/PresentMonUnitTests.dll` | VS C++ Unit Test (`NativeUnitTestProject`) | x64 only |
| API2 integration | `IntelPresentMon/PresentMonAPI2Tests/PresentMonAPI2Tests.vcxproj` | `build/<Config>/PresentMonAPI2Tests.dll` | VS C++ Unit Test | x64 only |

**PresentMonTests** does not link the console at build time. At runtime it
requires `--presentmon=` pointing at `PresentMon-<version>-<arch>.exe`. Gold
ETL/CSV cases live under `Tests/Gold/`. Optional extended CSV suite:
`Tests/Full/` with `--gtest_filter=GoldEtlCsvTests.*`.

**PresentMonUnitTests** links foundation and Intel stacks:
`pmon_common_utilities`, `pmon_present_data`, `pmon_metrics`, `pmon_control`,
`pmon_core` (via MSBuild project references).

**PresentMonAPI2Tests** links most of the product graph (service, API2, UI
executables as build-order/deployment dependencies). Tests spawn or attach to
`PresentMonService`, `PresentMon.exe` (KernelProcess), `PresentMonAPI2.dll`,
and related binaries from `build/<Config>/`. `Tools/PresentBench.exe` is
required for many realtime scenarios (same as `run_tests.cmd`).

**Auxiliary data:** pinned in `Tests/aux-data.lock.json`; restored by
`bootstrap.ps1` / `Tests/pull-aux.ps1` into `Tests/AuxData` (gitignored).
API2 ETL playback tests depend on this layout.

**Generated API2 input:** `IntelPresentMon/PresentMonAPI2Tests/test_cases.csv`
drives `GenerateEtlCsvTestCases.ps1`, which writes
`EtlCsvTestCases.g.h` under the build tree (`$(IntDir)Generated/` in MSBuild).

## Design Decisions

### Keep existing test frameworks

Use **vcpkg `gtest`** for `PresentMonTests` (replace amalgamated include with
`GTest::gtest` / `GTest::gtest_main`, or keep single-TU amalgamation if linking
parity requires it; prefer vcpkg targets unless amalgamation is required for
warning policy).

Use **Microsoft.VisualStudio.CppUnitTestFramework** for both DLL projects.
Provide a small CMake module (for example `cmake/PresentMonCppUnitTest.cmake`)
that locates headers and `cppunittest.lib` from the active VS installation
(`CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE` / `VCToolsInstallDir`). Do not
vendor the framework.

### Component option

```cmake
option(PMON_BUILD_TESTS "Build test projects and register CTest entries" ON)
```

When `OFF`, skip test subdirectories and do not call test registration functions.
When `ON` but required product targets are `OFF`, either skip dependent suites
with a configure `STATUS` message or use `cmake_dependent_option` so API2 tests
require service, SDK, and UI targets.

Suggested dependency rule:

- `PresentMonTests` requires `PMON_BUILD_CONSOLE`.
- `PresentMonUnitTests` requires `PMON_BUILD_SDK` or a narrower check (Core +
  ControlLib + foundation); match MSBuild link set.
- `PresentMonAPI2Tests` requires `PMON_BUILD_SERVICE`, `PMON_BUILD_SDK`, and
  `PMON_BUILD_UI` (KernelProcess + staged payloads for UI launch tests).

### Architecture

- **PresentMonTests:** x64 and Win32 presets (same as `PresentMonConsole`).
- **PresentMonUnitTests** and **PresentMonAPI2Tests:** x64 only (match MSBuild).

ARM/ARM64 test configurations are excluded (same as Phase 0 CMake scope).

### Output contract

Preserve public names:

```text
build/<Configuration>/PresentMonTests-<PMON_VERSION>-x64.exe
build/<Configuration>/PresentMonTests-<PMON_VERSION>-x86.exe   # Win32 preset
build/<Configuration>/PresentMonUnitTests.dll
build/<Configuration>/PresentMonAPI2Tests.dll
```

Use the same version suffix logic as `PresentMonConsole` (`PresentMon.props`
/`pmon_version` helpers).

### Auxiliary data restore

Add an explicit target (not part of default build):

```text
pmon_restore_aux_testdata
```

Behavior mirrors `pmon_restore_cef`:

- Invokes `Tests/pull-aux.ps1` (or equivalent PowerShell with pinned lock).
- No network during `cmake` configure.
- Document in `BUILDING.md` and preset `-dependencies` if a dedicated preset is
  useful later.

### Running VS C++ Unit Test DLLs from CTest

Build each test project as a **shared library** with `/SUBSYSTEM:WINDOWS` and
link `cppunittest.lib`, matching MSBuild.

Register CTest entries that invoke **`vstest.console.exe`** from the VS
installation (discover via `vswhere` or CMake's `CMAKE_VS_MSBUILD_COMMAND` /
`VCINSTALLDIR`):

```text
vstest.console.exe /InIsolation build/<Config>/PresentMonUnitTests.dll
```

Use `/Settings:` or `/TestCaseFilter:` only when needed to match Test Explorer
defaults. Set `WORKING_DIRECTORY` to `build/<Configuration>/` so dependent DLLs
and EXEs resolve. Add explicit `DEPENDS` on product targets (and UI payload
staging targets for API2) so a test build copies or builds prerequisites.

Alternative acceptable for Phase 5 closure: one CTest per DLL plus a documented
manual Test Explorer path, as long as headless `vstest.console.exe` runs in CI.

### CTest layout and labels

Call `enable_testing()` from the root `CMakeLists.txt` when `PMON_BUILD_TESTS`
is on.

Use **labels** (CTest `LABELS` property) at minimum:

| Label | Meaning |
| --- | --- |
| `console` | PresentMonTests executable |
| `console.gold` | Gold ETL/CSV deterministic suite |
| `console.realtime` | Tests needing PresentBench / ETW / PLU |
| `unit` | PresentMonUnitTests |
| `api2` | PresentMonAPI2Tests |
| `api2.realtime` | API2 tests that start ETW or realtime sessions |
| `deterministic` | Safe default CI subset |
| `environment` | Requires elevation, PLU, mock GPU, or UI desktop |

**Default CI recommendation:** run only tests labeled `deterministic` (gold
ETL/CSV, most unit tests, non-realtime API2). Skip or mark `environment` tests
`SKIP_RETURN_CODE` or document `ctest -LE environment`.

For **PresentMonTests**, prefer one or more `add_test` commands with fixed
arguments rather than `gtest_discover_tests` initially, because gold cases are
registered dynamically at startup from disk:

```text
PresentMonTests-<ver>-x64.exe
  --presentmon=<PMON_OUTPUT_ROOT>/<Config>/PresentMon-<ver>-x64.exe
  --golddir=<repo>/Tests/Gold
  --norealtime   # if such a flag exists; otherwise use gtest_filter to exclude
```

If no filter exists for realtime command-line tests, add a documented
`--gtest_filter` negative list matching Phase 0 baseline (`CommandLineTests.*`
realtime cases vs gold-only CI job).

### PresentMonTests compile contract

Mirror `PresentMonTests.vcxproj`:

- Unicode, static CRT, `/W4` + `/WX` if console uses strict warnings via props
  (match `PresentMon.props` / `PresentMonTests` compile settings from vcxproj).
- Sources: `CommandLineTests.cpp`, `GoldEtlCsvTests.cpp`, `PresentMonTests.cpp`,
  `PresentMon.cpp`.
- Include generated `version.h` from `pmon_version_header`.
- Link `GTest::gtest` (and main if not using custom `wmain` in
  `PresentMonTests.cpp`).
- `/EHsc` or project-default exception handling per vcxproj.
- **Dependency:** `add_dependencies(PresentMonTests PresentMonConsole)` for
  build order only.

Do not modify test source except includes if switching from amalgamation to
vcpkg gtest targets.

### PresentMonUnitTests compile contract

Mirror `UnitTests.vcxproj` source list (including
`CommonUtilities/test/MachineExpectations.cpp`).

Link CMake targets:

- `pmon::common_utilities`, `pmon::present_data`, `pmon::metrics`,
  `pmon::control`, `pmon::core`

Explicit Windows libs: `tdh` (as in vcxproj).

### PresentMonAPI2Tests compile contract

Mirror `PresentMonAPI2Tests.vcxproj` sources (exclude `EtlLoggerTests.cpp` if
still excluded in all configs).

**Code generation:** `add_custom_command` running
`GenerateEtlCsvTestCases.ps1` with outputs under
`build/obj/<Config>/PresentMonAPI2Tests/Generated/EtlCsvTestCases.g.h` (or
equivalent per-config path). Wire `pch.h` / include path to generated directory.

**Link / dependency graph (CMake targets, not MSBuild app references):**

- Foundation through service and SDK: interprocess, middleware, API2, loader,
  wrappers, versioning, service executable target.
- **KernelProcess** and **PresentMonUI** as dependencies for build order and
  output layout (tests load binaries from output dir; do not link UI EXEs into
  the test DLL).
- **CEF/UI payload staging:** API2 UI launch tests need the same staged files as
  a normal UI build; depend on `PresentMonUI` / payload staging targets when
  `PMON_BUILD_UI` is on.

Copy or ensure presence of: `PresentMonService.exe`, `PresentMonAPI2.dll`,
`PresentMon.exe`, `SampleClient.exe`, provider DLL if tests require it, and
aux-data paths documented in test helpers (`Folders.h`, etc.).

### Optional aggregate targets

```text
pmon_run_deterministic_tests   # ctest -L deterministic
pmon_run_console_gold_tests    # single gold ETL invocation
```

These are convenience `add_custom_target` wrappers for documentation and CI.

## Proposed CMake Layout

```text
cmake/PresentMonCppUnitTest.cmake   # find VS cppunittest headers/libs
cmake/PresentMonAuxTestData.cmake   # pmon_restore_aux_testdata
cmake/PresentMonCTest.cmake         # registration helpers, labels, vstest discovery

Tests/CMakeLists.txt                # PresentMonTests executable
IntelPresentMon/UnitTests/CMakeLists.txt
IntelPresentMon/PresentMonAPI2Tests/CMakeLists.txt
```

Root `CMakeLists.txt` changes:

- `include(PresentMonAuxTestData)` / `PresentMonCTest` as needed.
- `if(PMON_BUILD_TESTS)` blocks adding test subdirectories.
- `enable_testing()` when tests enabled.

## Execution Checklist

Work in order. Stop for review after each numbered **work unit** unless noted.

### Work unit 1 - Options and scaffolding

- [x] Add `PMON_BUILD_TESTS` to `cmake/PresentMonOptions.cmake` (default `ON`).
- [x] Add `cmake_dependent_option` or configure-time checks for console/unit/api2
  test prerequisites.
- [x] Add `cmake/PresentMonCppUnitTest.cmake` with verified find logic on VS
  2022 + v143.
- [x] Add `cmake/PresentMonAuxTestData.cmake` and target
  `pmon_restore_aux_testdata`.
- [x] Add `cmake/PresentMonCTest.cmake` with functions:
  `pmon_register_vstest_dll(...)`, `pmon_register_presentmon_gold_test(...)`.
- [x] Extend root `CMakeLists.txt` to include modules and `enable_testing()`.
- [x] Print `PMON_BUILD_TESTS` in the configuration summary block.
- [x] Verify configure succeeds with `PMON_BUILD_TESTS=OFF` (no test targets).

### Work unit 2 - PresentMonTests (GTest console regression)

- [x] Add `Tests/CMakeLists.txt` building target `PresentMonTests` with output
  name `PresentMonTests-${PMON_VERSION}-<arch>` matching MSBuild.
- [x] Wire includes, warnings, CRT, and `GTest` from vcpkg (`GTest::gtest`; amalgamated TU removed).
- [x] Depend on `pmon_version_header` and `PresentMonConsole`.
- [x] Guard with `PMON_BUILD_CONSOLE` and `PMON_BUILD_TESTS`.
- [ ] Build x64 Debug/Release via `windows-x64-developer` preset; build Win32
  when console Win32 preset is used (Debug x64 verified; Release and Win32 open).
- [x] Record exact link/compile parity notes if amalgamation vs `GTest::gtest`
  differs from MSBuild (see Verification).

### Work unit 3 - PresentMonUnitTests (VS unit test DLL)

- [x] Add `IntelPresentMon/UnitTests/CMakeLists.txt` for shared library
  `PresentMonUnitTests`.
- [x] Link `pmon::` targets matching MSBuild references.
- [x] Apply `/EHa`, static CRT, and `/W3` policy consistent with vcxproj.
- [x] x64 only; output to `build/<Configuration>/PresentMonUnitTests.dll`.
- [ ] Build Debug and Release without MSBuild (Debug verified; Release open).

### Work unit 4 - PresentMonAPI2Tests (VS unit test DLL + generator)

- [x] Add `IntelPresentMon/PresentMonAPI2Tests/CMakeLists.txt`.
- [x] Implement `EtlCsvTestCases.g.h` generation with explicit inputs/outputs.
- [x] Link all required product targets; set include paths for generated header.
- [x] Add build dependencies on service, API2, KernelProcess, PresentMonUI, and
  payload staging sufficient for `UiLaunchTests` and service scenarios.
- [x] Document which tests require `pmon_restore_aux_testdata` and
  `Tests/AuxData` (Environment-qualified tests section).
- [ ] Build Debug and Release x64 (Debug verified; Release open).

### Work unit 5 - CTest registration

- [x] Register gold ETL/CSV test: `add_test` running `PresentMonTests` with
  `--presentmon`, `--golddir=${CMAKE_SOURCE_DIR}/Tests/Gold`, working directory
  under output root; label `console.gold;deterministic`.
- [x] Register optional `Tests/Full` suite behind a CMake option
  `PMON_CTEST_FULL_CSV_TESTS` (default `OFF`) to mirror `run_tests.cmd fullcsvs`.
- [x] Register `PresentMonUnitTests.dll` via `vstest.console.exe`; label
  `unit;deterministic` (exclude known flaky `QpcTimerSpinChrono` via filter if
  documented in Phase 0).
- [x] Register `PresentMonAPI2Tests.dll` via `vstest.console.exe`; split labels
  `api2;deterministic` vs `api2;environment` using `/TestCaseFilter` where
  feasible, or document manual subset until filters are mapped.
- [x] Register console/API2 **realtime** / PresentBench tests only with label
  `environment` (API2 environment filter; console `CommandLineTests.*` not registered).
- [x] Add custom target `pmon_run_deterministic_tests` running
  `ctest -L deterministic -C $<CONFIG>`.
- [x] Document `ctest --test-dir build/cmake/windows-x64-developer -C Debug`
  in this file's Verification section.

### Work unit 6 - Documentation and migration master doc

- [x] Update `BUILDING.md` with CMake test build, aux restore, and `ctest` usage.
- [x] Update [CMake Migration Design](CMakeMigration.md) Phase 5 status when
  complete (in progress until Release/Win32/ctest gate passes).
- [x] Add this document to [Design README](README.md) index.
- [x] Record verification results below (partial; full `ctest` run open).

## Verification

### Build verification

```powershell
cmake --preset windows-x64-dependencies
cmake --preset windows-x64-developer -DPMON_BUILD_TESTS=ON
cmake --build --preset windows-x64-developer --config Debug --target PresentMonTests PresentMonUnitTests PresentMonAPI2Tests
cmake --build --preset windows-x64-developer --config Release --target PresentMonTests PresentMonUnitTests PresentMonAPI2Tests
```

Win32 PresentMonTests only:

```powershell
cmake --preset windows-win32-developer -DPMON_BUILD_TESTS=ON -DPMON_BUILD_SERVICE=OFF -DPMON_BUILD_SDK=OFF -DPMON_BUILD_UI=OFF
cmake --build --preset windows-win32-developer --config Debug --target PresentMonTests
```

Expected: all three x64 test artifacts (or Win32 regression exe) exist under
`build/<Configuration>/` with baseline names.

### CTest verification (deterministic)

Prerequisites: `Tests/Gold` present; CMake-built `PresentMon-<version>-x64.exe`;
for API2 non-realtime cases, product DLLs/EXEs in output directory.

```powershell
cmake --build --preset windows-x64-developer --config Debug
ctest --test-dir build/cmake/windows-x64-developer -C Debug -L deterministic --output-on-failure
```

**2026-08-06 (local, `feature/cm`):** Configure with `PMON_BUILD_TESTS=ON` succeeds on `windows-x64-developer`. Debug build produced `build/Debug/PresentMonTests-2.6.0-x64.exe`, `PresentMonUnitTests.dll`, and `PresentMonAPI2Tests.dll`. CTest lists four entries (five when `PMON_CTEST_FULL_CSV_TESTS=ON`). API2 vstest filters use validated `Name~` / `Name!~` patterns (113 deterministic + 15 environment of 128 total). Full `ctest -L deterministic` not executed in this pass.

Expected:

- Gold ETL/CSV: 18/18 pass (Phase 0 baseline under same conditions).
- Unit tests: pass except documented flaky timing test if not filtered.
- API2 deterministic subset: passes to the extent documented when realtime/PLU
  tests are excluded.

Record actual counts, filters, and environment gaps in this section when executed.

### Environment-qualified tests

Do not treat failure as a CMake regression when Phase 0 already failed for the
same reason:

- Realtime ETW / Performance Log Users / elevation.
- PresentBench-dependent console command-line tests.
- API2 multi-client, realtime presenter, and frame-store cases.
- Tests requiring full aux-data ETL paths without `pmon_restore_aux_testdata`.

Mark these as `environment` label or `SKIP` with message in CTest registration.

## Completion Criteria

Phase 5 is complete when:

- All three test projects build from CMake on x64 Debug and Release without
  MSBuild.
- `PresentMonTests` builds on Win32 Debug and Release when the Win32 developer
  preset is used.
- `pmon_restore_aux_testdata` exists and matches lock-driven restore behavior.
- API2 generated header is produced by CMake with correct dependencies.
- CTest registers at least the deterministic gold console suite and headless
  unit/API2 DLL runs via `vstest.console.exe`.
- Verification commands and results are recorded in this document.
- `BUILDING.md` and `CMakeMigration.md` reflect the new workflow.

## Deferred

- Reflector and any managed test harness.
- Replacing `Tools/run_tests.cmd` entirely.
- `gtest_discover_tests` per-case registration (optional follow-up).
- CTest fixtures automating PresentBench start/stop (optional follow-up).
- Phase 7 MSBuild retirement and artifact diff against MSBuild-built test DLLs.
