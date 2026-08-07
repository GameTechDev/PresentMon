# CMake Build User Guide

This guide is for developers using the **CMake** entry point described in
[CMake Migration Design](CMakeMigration.md). The legacy root `PresentMon.sln`
and MSBuild workflow remain supported until Phase 7 cutover. Phase 6 (WiX MSI/MSM
from CMake) is not complete; production steps below end at signed native payload
plus the interim MSBuild installer path.

## Prerequisites

Install once on the machine:

- Visual Studio 2022 with the **Desktop development with C++** workload (MSVC
  v143, Windows 10/11 SDK).
- [CMake](https://cmake.org) 3.25 or newer (Visual Studio ships a compatible
  copy; the `cmake` on PATH must meet the preset minimum).
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` pointing at the
  checkout, **or** rely on the VS-bundled toolchain under
  `%VCPKG_ROOT%` / `%ProgramFiles(x86)%\Microsoft Visual Studio\2022\...\VC\vcpkg`.
- [Node.js / npm](https://nodejs.org) for the AppCef web UI (CMake invokes
  `build-web.ps1` when building UI targets).
- WiX 3 only if you still build MSI via the legacy MSBuild installer projects
  (Phase 6 CMake packaging is pending).

Optional but common:

- Membership in **Performance Log Users** (sign out/in after adding) for ETW
  realtime tests and UI process tracking.
- A trusted **test certificate** in `PrivateCertStore` for local **Release**
  `PresentMon.exe` (KernelProcess) when using the developer profile; see
  [BUILDING.md](../BUILDING.md).

Machine-specific CMake overrides belong in `CMakeUserPresets.json` (gitignored).
Do not commit local paths there.

## Directory layout (CMake)

| Location | Purpose |
| --- | --- |
| `build/cmake/<preset>/` | CMake generator tree; open `PresentMon.sln` here for Visual Studio |
| `build/Debug`, `build/Release` | Runnable binaries, DLLs, staged CEF/web payloads (`PMON_OUTPUT_ROOT`) |
| `build/obj/` | Intermediate and generated files |
| `build/lib/` | Static libraries (for example Metrics) |
| `build/ThirdParty/` | vcpkg installs, vcpkg downloads, CMake CEF stage (`cef/`) |

Configure presets are defined in [CMakePresets.json](../CMakePresets.json).

---

## 1. Clean checkout (first time on a machine)

Assume an empty clone at `PresentMonRepoDir` and a PowerShell shell at the repo
root.

### 1.1 Restore pinned C++ dependencies (vcpkg)

Normal **developer** configure does not download vcpkg packages. Run the
dependency preset once (or after manifest/ baseline changes):

```powershell
cd PresentMonRepoDir
cmake --preset windows-x64-dependencies
```

Packages land under `build/ThirdParty/vcpkg_installed/`; downloads under
`build/ThirdParty/vcpkg-downloads/`.

For Win32 standalone targets (console, provider, tools), also run:

```powershell
cmake --preset windows-win32-dependencies
```

### 1.2 Configure the x64 developer tree

```powershell
cmake --preset windows-x64-developer
```

Review the configure summary at the end (version, deployment profile, UCI,
signing backend, CEF stage path). Fix `VCPKG_ROOT` or missing SDK errors before
continuing.

Optional: disable UCI when you have no SDK:

```powershell
cmake --preset windows-x64-developer -DPMON_ENABLE_UCI=OFF
```

### 1.3 Restore CEF for CMake (UI / capture)

CMake does **not** download CEF during configure. Restore the fixed stage at
`build/ThirdParty/cef` explicitly:

```powershell
cmake --build --preset windows-x64-developer --config Debug --target pmon_restore_cef
```

Use the same command with `--config Release` when you need Release UI builds.
Details: [ceflock.md](../IntelPresentMon/AppCef/ceflock.md).

Legacy `bootstrap.ps1` still restores CEF under `IntelPresentMon/AppCef/Cef` for
the root MSBuild solution; **CMake UI builds use the CMake stage**, not that
path alone.

### 1.4 Build Debug (runnable app + tests)

After CEF restore, one build produces the capture stack, console test binary,
and unit/API2 test DLLs:

```powershell
cmake --build --preset windows-x64-developer --config Debug
```

The **windows-x64-developer** build preset builds target **`pmon_developer_debug`**
(KernelProcess and its dependencies, plus test projects). The web UI is built
via the UI payload graph (`build-web.ps1`).

Outputs:

- Binaries: `build\Debug\`
- Visual Studio solution: `build\cmake\windows-x64-developer\PresentMon.sln`
  (startup project **KernelProcess**, configuration **Debug**, then F5)

To build every CMake target (console tools, provider, trimmer, etc.), use
**Build Solution** in Visual Studio or:

```powershell
cmake --build build/cmake/windows-x64-developer --config Debug
```

(without `--preset`, which selects `pmon_developer_debug` only).

### 1.5 Optional: tests and auxiliary data

Tests are on by default (`PMON_BUILD_TESTS=ON`).

Restore pinned API2/aux ETL data (not part of configure):

```powershell
cmake --build --preset windows-x64-developer --target pmon_restore_aux_testdata
```

Build test binaries:

```powershell
cmake --build --preset windows-x64-developer --config Debug --target PresentMonTests PresentMonUnitTests PresentMonAPI2Tests
```

Run all registered tests (gold console, command-line console, unit, API2):

```powershell
ctest --test-dir build/cmake/windows-x64-developer -C Debug --output-on-failure
```

Same via build target: `cmake --build --preset windows-x64-developer --config Debug --target pmon_run_all_tests`.

For CI or machines without PLU/live ETW, use the **deterministic** label only:
`ctest ... -L deterministic`. Skip environment-heavy cases explicitly with
`-LE environment`. Gold CSV under `Tests/Gold/` requires no extra restore.
Optional local aux cases: create
`Tests/PresentMonTests.local.runsettings` and re-run `cmake --preset
windows-x64-developer` so CMake generates
`PresentMonTests.generated.runsettings` for Test Explorer.

### 1.6 Optional: Release test certificate (developer profile)

For local **Release** KernelProcess (`uiAccess=true`), create and trust the
internal test certificate once (administrator command prompt):

```bat
makecert -r -pe -n "CN=Test Certificate - For Internal Use Only" -ss PrivateCertStore testcert.cer
certutil -addstore root testcert.cer
```

Release `PresentMon.exe` is normally run from a secure directory (Program Files
or System32), not F5 from the build tree; see [BUILDING.md](../BUILDING.md).

---

## 2. Day-to-day developer workflow

### 2.1 Open the project

Prefer the generated solution:

```text
build\cmake\windows-x64-developer\PresentMon.sln
```

The repo-root `PresentMon.sln` is the legacy MSBuild graph; both can coexist.

### 2.2 Incremental builds

From the repo root (capture + tests):

```powershell
cmake --build --preset windows-x64-developer --config Debug
```

Build a single target when needed:

```powershell
cmake --build --preset windows-x64-developer --config Debug --target PresentMonService
```

Switch to Release when needed:

```powershell
cmake --build --preset windows-x64-developer --config Release --target KernelProcess
```

### 2.3 When to reconfigure

Re-run configure when you change:

- `CMakeLists.txt`, files under `cmake/`, or `CMakePresets.json`
- Cache options (`-DPMON_BUILD_UI=OFF`, `-DPMON_ENABLE_UCI=...`, test flags)
- `Tests/PresentMonTests.local.runsettings` (regenerates CTest/runsettings)

```powershell
cmake --preset windows-x64-developer
```

You do **not** need to reconfigure for ordinary C++ source edits.

### 2.4 Component toggles

All product components default **ON**. Example: SDK/service only, no UI:

```powershell
cmake --preset windows-x64-developer -DPMON_BUILD_UI=OFF
```

Win32 work uses a separate configure tree:

```powershell
cmake --preset windows-win32-developer
cmake --build --preset windows-win32-developer --config Debug --target PresentMonConsole
```

Service, SDK, UI, and x64 unit/API2 tests exist only under x64 presets.

### 2.5 Tests during development

Build what you need, then run the full CTest suite:

```powershell
ctest --test-dir build/cmake/windows-x64-developer -C Debug --output-on-failure
```

`ctest -N` lists five top-level entries (`presentmon.console.gold`,
`presentmon.console.commandline`, `presentmon.unit`, and two API2 vstest runs).
Command-line and API2 environment tests need Performance Log Users, staged
`build\Debug` product binaries, and related machine setup or they fail.

In Visual Studio with the CMake solution: **Test** > **Configure Run Settings**
> select `PresentMonTests.generated.runsettings`, then **Refresh Tests**.

### 2.6 CEF and web UI maintenance

- After lock changes or a corrupt stage: `pmon_restore_cef` again (Debug or
  Release config matching your build).
- CI integrity check without download: target `pmon_verify_cef` (see Phase 4 doc).
- Web package changes: rebuild `PresentMonUI` or the `pmon_ui_payload` dependency
  chain so `build-web.ps1` runs.

### 2.7 Debugging tips

- Missing `PresentMonAPI2.dll` or pipe errors: rebuild **PresentMonAPI2** or
  **KernelProcess**; working directory must be `build\Debug` (or Release).
- ETW enable failures: Performance Log Users, provider install optional for basic
  present tracing.
- vcpkg baseline errors after pulling main: update vcpkg, ensure the same VS
  install as the build (`VCPKG_VISUAL_STUDIO_PATH` if needed).

More scenarios: [BUILDING.md](../BUILDING.md) troubleshooting.

---

## 3. Production release build (native payload)

This section covers CMake **PRODUCTION** profile through signed binaries in
`build\Release`. MSI/MSM packaging is Phase 6; until then use the documented
MSBuild WiX projects after the payload is signed.

### 3.1 Signing script (required before configure)

Production configure **fails** without an external EDSS PowerShell script (not
in this repository). Set the path before configuring:

```powershell
$env:PMON_EDSS_SIGN_SCRIPT = "C:\PresentMonBuilder\sign-production-payload.ps1"
```

Or pass `-DPMON_EDSS_SIGN_SCRIPT=...` on the `cmake` command line. The file must
exist at configure time. Template and policy: [Phase 4 - UI and Capture](Phase4-UiAndCapture.md)
(**Production signing closure**).

Optional: `-DPMON_EDSS_INTUNE_SIGNING=OFF` for Credential Manager / logon-probe
signing instead of default Intune signing.

There is **no** `windows-win32-production` preset. Win32 Release artifacts use
the **developer** Win32 preset into the same output root.

### 3.2 Configure production x64

```powershell
cmake --preset windows-x64-production
```

Confirm `Deployment profile: PRODUCTION` and `Signing backend: EDSS` in the
status block.

### 3.3 Restore CEF and build x64 Release payload

```powershell
cmake --build --preset windows-x64-production --config Release --target pmon_restore_cef
cmake --build --preset windows-x64-production --config Release
```

Or build specific targets if your pipeline stages steps. All maintained x64
product binaries, staged CEF, web UI, shaders, and `Intel-PresentMon.dll`
(provider) should appear under `build\Release\`.

### 3.4 Add Win32 Release artifacts (same output folder)

```powershell
cmake --preset windows-win32-developer
cmake --build --preset windows-win32-developer --config Release
```

This adds maintained x86 outputs (for example `PresentMon-<version>-x86.exe`,
`Intel-PresentMon32.dll`) beside the x64 Release tree without a second
`PMON_OUTPUT_ROOT`.

### 3.5 Sign and verify the payload

Signing is **not** automatic on every link step in production. After the Release
tree is complete:

```powershell
cmake --build --preset windows-x64-production --config Release --target pmon_sign_production_payload
```

This invokes your EDSS script with `-OutputRoot` pointing at
`build\Release`, `-Verify`, and (by default) `-IntuneSigning`. Do not mutate
binaries after a successful sign before packaging.

Developer-profile Release builds use SignTool on KernelProcess only; production
uses EDSS on the full payload list defined in your external script.

### 3.6 Installer (interim, pre-Phase 6)

CMake target `pmon_build_installer` is scaffolding only. Until Phase 6 lands,
build the WiX projects with MSBuild (for example `Release-EDSS-MSI|x64` on
`IntelPresentMon/PMInstaller/PMInstaller.wixproj`) using the **signed**
`build\Release` payload. Expect `build\Release\en-us\PresentMon.msi` when using
the legacy installer configuration.

Do not treat MSI contents or CMake-driven packaging as fully migrated until Phase
6 verification gates in [CMakeMigration.md](CMakeMigration.md) are closed.

---

## Quick reference

| Goal | Command |
| --- | --- |
| First vcpkg restore (x64) | `cmake --preset windows-x64-dependencies` |
| Daily configure (x64 dev) | `cmake --preset windows-x64-developer` |
| Build Debug (F5 + CTest binaries) | `cmake --build --preset windows-x64-developer --config Debug` |
| Restore CMake CEF | `cmake --build --preset windows-x64-developer --config Debug --target pmon_restore_cef` |
| F5 capture app | Open `build\cmake\windows-x64-developer\PresentMon.sln`, **KernelProcess**, Debug |
| All tests (CTest) | `ctest --test-dir build/cmake/windows-x64-developer -C Debug --output-on-failure` |
| CI / no-ETW subset | `ctest ... -L deterministic` |
| Production configure | Set `PMON_EDSS_SIGN_SCRIPT`, then `cmake --preset windows-x64-production` |
| Production sign | `cmake --build --preset windows-x64-production --config Release --target pmon_sign_production_payload` |

Related docs: [BUILDING.md](../BUILDING.md), [Phase 5 - Tests](Phase5-Tests.md),
[CMakePresets.json](../CMakePresets.json).
