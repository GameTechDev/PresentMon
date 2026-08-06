# Building PresentMon

## Install Build Tool Dependencies

- Visual Studio 2022

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

## Running PresentMon

### Intel PresentMon

Intel PresentMon is the UI application, `PresentMon.exe`.

For Debug builds, set the **KernelProcess** project (`PresentMon.exe`) as the startup project in `PresentMon.sln` and launch with the service as a child process. The project lives under `IntelPresentMon/KernelProcess` in the repository layout. Use the **VS-Debug-Run** argument preset in `IntelPresentMon/KernelProcess/KernelProcess.args.json`, or the equivalent command line:

```bat
> --svc-as-child --files-working --log-level verbose --middleware-dll-path .\PresentMonAPI2.dll --log-middleware-copy
```

With `--svc-as-child`, `PresentMon.exe` spawns `PresentMonService.exe` in the install output folder (not via `sc.exe`). The child runs the same realtime path as an installed service: `PresentMon` with `isRealtime=true` constructs `RealtimePresentMonSession`, which calls `StartEtwSession()` at startup (`StartTraceW` on the configured session name). ETW **providers** stay off until you attach and track a process; then `UpdateTracking` calls `StartProviders()` when targets go from none to some. That provider enable step requires membership in **Performance Log Users** (sign out and back in after adding the group). The Intel-PresentMon provider manifest (`Provider\install_provider.cmd`, Administrator) is optional for basic present tracing but documents custom provider events.

For synthetic GPU/system metrics only (for example **PresentMon Mock GPU**), add `--svc-flag enable-mock-telemetry` manually or pick the separate args-json entry; it is not part of **VS-Debug-Run**.

Working directory must be the configuration output folder (`build\<Configuration>\`, or `$(OutDir)` in MSBuild).

**Debug F5 vs MSI install:** F5 can match installed realtime ETW and tracking when PLU is configured. It does not match Release-only behavior: Debug `PresentMon.exe` uses `uiAccess=false`, binaries run from the build output folder (no secure-directory requirement), the service is a child console process rather than the Windows service control manager, and production signing/trusted-root validation for the service binary is not exercised. Provider registration and PLU are the same requirements as a manual install from the same build output.

### CMake + Visual Studio (Debug UI)

After `bootstrap.ps1` (web UI) and CMake CEF restore, you can F5 from the solution CMake generates:

```powershell
cmake --preset windows-x64-dependencies
cmake --preset windows-x64-developer
cmake --build --preset windows-x64-developer --config Debug --target pmon_restore_cef
cmake --build --preset windows-x64-developer --config Debug --target PresentMonUI KernelProcess PresentMonAPI2
```

Building **KernelProcess** also builds **PresentMonService**, **PresentMonUI**, and **PresentMonAPI2** (CMake dependency). For VS F5, `build\Debug\` must contain at least `PresentMon.exe`, `PresentMonService.exe`, `PresentMonUI.exe`, `PresentMonAPI2.dll`, CEF binaries (from `pmon_restore_cef`), and staged web assets (`ipm-ui-vue\`, `Presets\`, etc.).

Open `build\cmake\windows-x64-developer\PresentMon.sln`, set startup project **KernelProcess**, configuration **Debug**, and start debugging. CMake sets the debugger working directory to `build\Debug` and the default command line to match **VS-Debug-Run**. The legacy root `PresentMon.sln` plus `bootstrap.ps1` remains supported; use **VS-Debug-Run** there via the args JSON extension when installed. Build the **PresentMonAPI2** project once so `PresentMonAPI2.dll` is in the same output folder as `PresentMon.exe`.

CEF for CMake builds comes from `pmon_restore_cef` (`build/ThirdParty/cef`), not from `bootstrap.ps1` alone.

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

- If F5 on **KernelProcess** logs `PM_STATUS_NONEXISTENT_FILE_PATH` from the middleware loader or `error opening session ctrl=\\.\pipe\pm-ctrl`, ensure `build\Debug\PresentMonAPI2.dll` exists (build target **PresentMonAPI2** or rebuild **KernelProcess** after the CMake dependency is in place) and that **PresentMonService.exe** is in the same folder.

- If `pmStartTrackingProcess` / **StartTracking** returns `PM_STATUS_FAILURE` (service log: `Enabling of ETW Providers failed` or `Start tracking call failed`), add your user to **Performance Log Users**, sign out and back in, then retry. For frame data without live ETW, run the service with `--etl-test-file` (playback). For mock GPU metrics without real telemetry hardware, pass `--svc-flag enable-mock-telemetry` to the child service via kernel CLI.

- If you are seeing vcpkg errors when updating to a new version of PresentMon (e.g., "error: while checking out baseline from commit...") then try updating your vcpkg checkout.

- Make sure vcpkg is using the same Visual Studio installation as the solution build. If needed, set `VCPKG_VISUAL_STUDIO_PATH` before running vcpkg.


- If you get an error dialog from PresentMon.exe stating "A referral was returned form the server."
  you most likely do not have the certificate that the PresentMon service was signed with installed
  into your trusted root.  Ensure that the trusted test certificate setup completed successfully.  If
  you built the installer on another PC or received it from a trusted third party, you need to
  install the certificate on the target PC as well.

- Add the development user to the Performance Log Users group to run from the IDE, run tests, etc. without launching the IDE as administrator.
