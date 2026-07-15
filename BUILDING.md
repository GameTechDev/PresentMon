# Building PresentMon

## Install Build Tool Dependencies

- Visual Studio 2022, or Visual Studio 2026 (see [Building for ARM64](#building-for-arm64-windows-on-arm) for the extra toolset components 2026 needs)

- [vcpkg](https://github.com/microsoft/vcpkg)

- [CMake](https://cmake.org)

- [Node.js / NPM](https://nodejs.org/en/download)

- [v3 of the WiX toolset AND VS extension](https://wixtoolset.org/docs/wix3/)

Note: if you only want to build the PresentData library, or the PresentMon Console application
you only need Visual Studio.  Ignore the other build and source dependency instructions and build
`PresentData\PresentData.vcxproj` or `PresentMon\ConsoleApplication.sln`.

## Install Source Dependencies

1. Run the repository bootstrap script:

    ```powershell
    > cd PresentMonRepoDir
    > .\bootstrap.ps1
    ```

    The bootstrap script:

    - Restores the locked Chromium Embedded Framework (CEF) payload.
    - Pulls the pinned auxiliary test data.
    - Installs and builds the AppCef web UI.

    See the detailed guides for [CEF lock management](IntelPresentMon/AppCef/ceflock.md), [AppCef web UI setup](IntelPresentMon/AppCef/webui.md), and [auxiliary test data](Tests/auxdata.md).

2. Create and install a trusted test certificate.  This is only required for the Release build.  Open a command shell as administrator and run the following:

    ```bat
    > makecert -r -pe -n "CN=Test Certificate - For Internal Use Only" -ss PrivateCertStore testcert.cer
    > certutil -addstore root testcert.cer
    ```

## Building PresentMon

Build `PresentMon.sln` in Visual Studio or msbuild.  e.g.:

```bat
> msbuild /p:Platform=x64,Configuration=Release PresentMon.sln
```

## Building for ARM64 (Windows on Arm)

PresentMon builds natively for ARM64 in addition to x64. The ARM64 build runs
natively on Windows-on-Arm devices (e.g. Snapdragon).

### Toolset

All projects pin the **v143** platform toolset. Visual Studio 2022 ships v143 by
default and needs nothing extra. Visual Studio 2026 on ARM64 ships only the newer
v145 toolset, so install the v143 build tools alongside it from the Visual Studio
Installer (*Individual components*):

- *MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools* **and** *... x64/x86 build tools*
- *C++ ATL for v143 build tools* for both *ARM64/ARM64EC* and *x64/x86* (required by ETLTrimmer)

The vcpkg dependencies are pinned to v143 as well, via the overlay triplets in
`triplets/` (`arm64-`, `x64-`, and `x86-windows-static.cmake`). This keeps the
vcpkg dependency ABI matched to the app toolset on VS 2026; it is a no-op on
VS 2022, where v143 is already the default. Changing a triplet's toolset causes
vcpkg to rebuild that architecture's dependencies on the next build.

### Source dependencies

Restore the ARM64 CEF payload (the auxiliary test data and web frontend that
`bootstrap.ps1` also restores are architecture-neutral):

```powershell
> .\bootstrap.ps1 -Platform arm64
```

The staged `IntelPresentMon\AppCef\Cef` directory holds a single architecture at
a time (the last one pulled). Re-run `IntelPresentMon\AppCef\Batch\pull-cef.ps1 -Platform <x64|arm64>`
when switching architectures on the same checkout.

### Building

```bat
> msbuild /p:Platform=ARM64,Configuration=Release PresentMon.sln
```

On an ARM64 host, use the native MSBuild at
`...\MSBuild\Current\Bin\arm64\MSBuild.exe`.

### Packaging

A `Release|ARM64` solution build produces an ARM64-targeted MSI
(`build\Release\en-us\PresentMon.msi`) that installs the native ARM64 service,
API, and Capture application.

Because the build output directory (`build\<Configuration>`) is
architecture-neutral, a single checkout holds one architecture's binaries and
MSI at a time. Produce the x64 and ARM64 packages from separate (or cleaned)
builds.

## Continuous Integration

GitHub Actions (`.github/workflows/ci.yml`) builds Release and runs the unit
tests on both native architectures, on every pull request and on pushes to
`main`:

| Runner | Toolchain | Build |
| --- | --- | --- |
| `windows-2022` | VS 2022 + WiX 3.14 | Full `PresentMon.sln` (x64) and **packages the MSI**, uploaded as the `PresentMon-x64-msi` artifact |
| `windows-11-arm` | VS 2022 (no WiX) | App, service, API and unit tests (the WiX installer projects are skipped) |

Each run bootstraps the source dependencies (CEF payload, aux test data, web
frontend), creates a throwaway code-signing certificate so the Release
`SignTool` post-build succeeds, builds, and runs the unit tests. (One known
ARM64-only metrics test is skipped on the ARM64 leg pending a fix.)

Dependencies are cached to keep runs fast — the installed vcpkg trees
(`vcpkg_installed`), the staged CEF payload, the auxiliary test data, and the
npm download cache — so a warm run reuses them instead of rebuilding. With a
warm cache the build also runs in parallel (`msbuild /m`); a cold run (when the
vcpkg manifests change) falls back to a serial build to avoid a concurrent
vcpkg-install download race.

`windows-2025` is intentionally not used: GitHub switched that image to
Visual Studio 2026 ([actions/runner-images#14017](https://github.com/actions/runner-images/issues/14017)),
and the project currently targets the VS 2022 (v143) toolchain.

## Running PresentMon

### Intel PresentMon

Intel PresentMon is the UI application, `PresentMon.exe`.

For Debug builds, the easiest IDE workflow is to set `Client/KernelProcess` as the startup project and launch `PresentMon.exe` with the service running as a child process:

```bat
> --svc-as-child --files-working --log-level verbose --middleware-dll-path .\PresentMonAPI2.dll --log-middleware-copy
```

For Release builds, either move the full Release output payload to a secure directory such as "Program Files" or "System32", or disable the secure directory check for local development. You cannot run release builds from the IDE typically. The installer is often the easier path for Release validation:

```bat
> build\Release\en-us\PresentMon.msi
```

### PresentMon Service

To start the service, open a command window as Administrator, then run the following commands (using the full binPath to your build executable):

```bat
> sc.exe create PresentMonService binPath="C:\...\PresentMonRepoDir\build\Release\PresentMonService.exe"
> sc.exe start PresentMonService
```

When you are finished, stop and remove the service with:

```bat
> sc.exe stop PresentMonService
> sc.exe delete PresentMonService
```

### PresentMon Standalone Console

The standalone console application is `PresentMon-dev-x64.exe`:

```bat
> build\Release\PresentMon-dev-x64.exe
```


## Troubleshooting

- If you are seeing vcpkg errors when updating to a new version of PresentMon (e.g., "error: while checking out baseline from commit...") then try updating your vcpkg checkout.

- Make sure vcpkg is using the same Visual Studio installation as the solution build. If needed, set `VCPKG_VISUAL_STUDIO_PATH` before running vcpkg.


- If you get an error dialog from PresentMon.exe stating "A referral was returned form the server."
  you most likely do not have the certificate that the PresentMon service was signed with installed
  into your trusted root.  Ensure that the trusted test certificate setup completed successfully.  If
  you built the installer on another PC or received it from a trusted third party, you need to
  install the certificate on the target PC as well.

- Add the development user to the Performance Log Users group to run from the IDE, run tests, etc. without launching the IDE as administrator.
