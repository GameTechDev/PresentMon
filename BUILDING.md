# Building PresentMon

## Install Build Tool Dependencies

- Visual Studio 2022

- [vcpkg](https://github.com/microsoft/vcpkg)

- [CMake](https://cmake.org)

- [Node.js / NPM](https://nodejs.org/en/download)

- [v3 of the WiX toolset and VS extension](https://wixtoolset.org/docs/wix3/)

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

For Debug builds, the easiest IDE workflow is to set `p2c/KernelProcess` as the startup project and launch `PresentMon.exe` with the service running as a child process:

```bat
> --svc-as-child --files-working --log-level verbose --middleware-dll-path .\PresentMonAPI2.dll --log-middleware-copy
```

For Debug builds from the output directory, run from the build output folder so the application can find the staged binaries and web assets:

```bat
> cd build\Debug
> PresentMon.exe --svc-as-child --files-working
```

For Release builds, either move the full Release output payload to a secure directory such as "Program Files" or "System32", or disable the secure directory check for local development. The installer is often the easier path for Release validation:

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

During development, Intel PresentMon can launch the service as a child process with `--svc-as-child` instead of using a separately installed service.

### PresentMon Standalone Console

The standalone console application is `PresentMon-dev-x64.exe`:

```bat
> build\Release\PresentMon-dev-x64.exe
```


## Troubleshooting

- If you are seeing vcpkg errors when updating to a new version of PresentMon (e.g., "error: while checking out baseline from commit...") then try updating your vcpkg checkout. Keep vcpkg wherever you normally install development tools, outside the PresentMon repository:

    ```bat
    > set VCPKG_ROOT=C:\dev\vcpkg
    > cd %VCPKG_ROOT%
    > git pull
    > bootstrap-vcpkg.bat
    > vcpkg.exe integrate install
    > cd PresentMonRepoDir
    > %VCPKG_ROOT%\vcpkg.exe install
    ```

    If the checkout needs to be recreated, clone vcpkg back to your external tools location and rerun the bootstrap and install commands.

    Make sure vcpkg is using the same Visual Studio installation as the solution build. If needed, set `VCPKG_VISUAL_STUDIO_PATH` before running vcpkg:

    ```bat
    > set VCPKG_VISUAL_STUDIO_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
    ```

- If you get an error dialog from PresentMon.exe stating "A referral was returned form the server."
  you most likely do not have the certificate that the PresentMon service was signed with installed
  into your trusted root.  Ensure that the trusted test certificate setup completed successfully.  If
  you built the installer on another PC or received it from a trusted third party, you need to
  install the certificate on the target PC as well.

- Add the development user to the Performance Log Users group to run from the IDE, run tests, etc. without launching the IDE as administrator.

- If debugging `KernelProcess` from Visual Studio fails to find the staged runtime files, set the project's debugging working directory to `$(OutDir)`.
