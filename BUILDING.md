# Building PresentMon

## Install Build Tool Dependencies

- Visual Studio 2022

- [CMake](https://cmake.org)

- [Node.js / NPM](https://nodejs.org/en/download)

- [v3 of the WiX toolset and VS extension](https://wixtoolset.org/docs/wix3/)

Note: if you only want to build the PresentData library, or the PresentMon Console application
you only need Visual Studio.  Ignore the other build and source dependency instructions and build
`PresentData\PresentData.vcxproj` or `PresentMon\ConsoleApplication.sln`.

## Automated Build Script

For convenience, a batch script `build.bat` is provided to automate the entire build process. This script will perform all the steps described below, including installing vcpkg dependencies, building CEF, building the web assets, and building the final PresentMon solution.

To use the script:
1. Open a Command Prompt or PowerShell terminal.
2. Run the script from the root of the repository: `.\build.bat`
3. The script will pause and ask you to provide the full path to your downloaded and extracted CEF (Chromium Embedded Framework) directory.
4. The script will attempt to create and install a test certificate, which requires administrator privileges. It is recommended to run the script in a terminal with Administrator rights.

The manual steps are still documented below if you prefer to execute the build process step-by-step.

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

    1. Download the CEF distribution and extract it to a local folder (e.g. C:\cef_133): https://cef-builds.spotifycdn.com/index.html

        - Most recently tested release is version 136

        - Proximal versions will most likely be compatible, but are not officially supported.

        - The "Minimal Distribution" is sufficient.

    2. Build 64-bit Debug and Release configurations (replace "CefDir" with the full path to the directory you downloaded into):

        ```bat
        > cmake -G "Visual Studio 17" -A x64 -DUSE_SANDBOX=OFF -S CefDir -B CefDir\build
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
    > pushd IntelPresentMon\AppCef\ipm-ui-vue
    > npm ci
    > npm run build
    > popd
    ```

    Note: instead of using the production build as described above, you can use a development process with a local server with hotloading support.  To do this, use `npm run dev` instead, and use the `--ui-option url "http://localhost:5173/"` command line argument when running the *PresentMon Capture Application*.  This causes the app to load web content from localhost rather than the files in ipm-ui-vue/.

4. Create and install a trusted test certificate.  This is only required for the Release build.  Open a command shell as administrator and run the following:

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

Further, for the Release build, the application must be run from a secure location (e.g. "Program Files" or "System32") so it will need to be copied there first. The Release build also cannot be started from Visual Studio, irregardless of whether the debugger is attached, and even if VS is running with admin privilege.

## Troubleshooting

- If you are seeing vcpkg errors when updating to a new version of PresentMon (e.g., "error: while checking out baseline from commit...") then try updating or removing and re-adding vcpkg:

    ```bat
    > cd PresentMonRepoDir
    > build\vcpkg\vcpkg.exe remove
    > build\vcpkg\vcpkg.exe integrate remove
    > rmdir /s /q build\vcpkg
    > git clone https://github.com/Microsoft/vcpkg.git build\vcpkg
    > build\vcpkg\bootstrap-vcpkg.bat
    > build\vcpkg\vcpkg.exe integrate install
    > build\vcpkg\vcpkg.exe install
    ```

- If you get an error dialog from PresentMon.exe stating "A referral was returned form the server."
  you most likely do not have the certificate that the PresentMon service was signed with installed
  into your trusted root.  Ensure that the above step 4 completed successfully.  If you built the
  installer on another PC or received it from a trusted third party, you need to install the
  certificate on the target PC as well.
