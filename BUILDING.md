# Building PresentMon

Note: if you only want to build the PresentData/ library, or the PresentMon Console application you only need Visual Studio: open or build `PresentMon.sln`.

## Install Build Tool Dependencies

- Visual Studio 2022

- [CMake](https://cmake.org)

- [Node.js / NPM](https://nodejs.org/en/download)

- [v3 of the WiX toolset and VS extension](https://wixtoolset.org/docs/wix3/)

## Install Source Dependencies

1. Download and install *vcpkg*, which will be used to obtain source package dependencies during the build:

    ```bat
    > cd PresentMonRepoDir
    > git clone https://github.com/Microsoft/vcpkg.git build\vcpkg
    > build\vcpkg\bootstrap-vcpkg.bat
    > build\vcpkg\vcpkg.exe integrate install
    > build\vcpkg\vcpkg.exe install
    ```

2. Build the Chromium Embedded Framework (CEF)

    1. Download the CEF 114.2.11 distribution and extract it to a local folder (e.g., CefDir): https://cef-builds.spotifycdn.com/index.html

        - The "Minimal Distribution" is sufficient.

        - Proximal versions will most likely be compatible, but are not officially supported.

    2. Build 64-bit Debug and Release configurations (replace "CefDir" with the directory you downloaded into):

        ```bat
        > cmake -G "Visual Studio 17 2022" -A x64 -DUSE_SANDBOX=OFF -S CefDir -B CefDir\build
        > cmake --build CefDir\build --config Debug
        > cmake --build CefDir\build --config Release
        ```

    3. Copy the required build outputs into AppCef by running the following:

        ```bat
        > IntelPresentMon\AppCef\Batch\pull-cef.bat CefDir
        ```

    4. You can now delete the local cef directory if you wish.

3. Download and build the web asset dependencies via NPM.  This only needs to be run once on fresh clone, or after new packages are added:

    ```bat
    > pushd IntelPresentMon\AppCef\Web
    > npm ci
    > npm run build
    > popd
    ```

    Note: instead of using the production build as described above, you can use a development process with a local server with hotloading support.  To do this, use `npm run serve` instead, and use the `--p2c-url=http://localhost:8080/` command line argument when running the *PresentMon Capture Application*.  This causes the app to load web content from localhost rather than the files in Web/.

4. Create and install a trusted test certificate.  This is only required for the Release build.  Open the `Developer Command Prompt for VS 2022` or `Developer PowerShell for VS 2022` as administrator and run the following:

    ```bat
    > makecert -r -pe -n "CN=Test Certificate - For Internal Use Only" -ss PrivateCertStore testcert.cer
    > certutil -addstore root testcert.cer
    ```

## Building PresentMon

Build `PresentMon2.sln` in Visual Studio or msbuild.  e.g.:

```bat
> msbuild /p:Platform=x64,Configuration=Release PresentMon2.sln
```

## Running PresentMon

- PresentMon Console Application: `build\Release\PresentMon-dev-x64.exe`

- PresentMon Console Tests: `build\Release\PresentMonTests-dev-x64.exe`

- PresentMon Service Command Line Interface: `build\Release\PresentMonCli.exe`

- PresentMon Installer: `build\Release\en-us\PresentMon.msi`

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

### PresentMon Capture Application

You must run the PresentMon Capture Application from its directory, with the *PresentMon Service* already running.  e.g.:

```bat
> cd build\Debug
> PresentMon.exe
```

## Capture Application Release Build Notice

Further, for the Release build, the application must be run from a secure location (e.g. `"Program Files"` or `"System32"`) so it will need to be copied there first. The Release build also cannot be started from Visual Studio, irregardless of whether the debugger is attached, and even if VS is running with admin privilege.

# Running PresentMon on other systems

You will encounter an error message such as: `"A referral was returned from the server"`. This usually means a certificate mismatch or not exist. The same certificate that was used to sign the application must be installed on the other machine. Open command prompt as administrator and run the following:

```bat
> certutil -addstore root testcert.cer
```
