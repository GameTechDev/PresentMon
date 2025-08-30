@echo off
setlocal

echo =================================================================
echo  PresentMon Build Script
echo =================================================================
echo.
echo This script will automate the build process for PresentMon.
echo It follows the instructions from BUILDING.md.
echo.
echo Please ensure you have the following prerequisites installed:
echo   - Visual Studio 2022 (with C++ workload)
echo   - CMake
echo   - Node.js / NPM
echo   - WiX toolset v3
echo.
echo =================================================================
echo.

REM Function to check for errors
:check_error
if %errorlevel% neq 0 (
    echo.
    echo ***************************************************************
    echo  An error occurred. Aborting script.
    echo ***************************************************************
    exit /b %errorlevel%
)
goto :eof


echo [1/6] Installing vcpkg dependencies...
echo -----------------------------------------------------------------
if not exist "build\vcpkg\vcpkg.exe" (
    echo Cloning vcpkg repository...
    git clone https://github.com/Microsoft/vcpkg.git build\vcpkg
    call :check_error

    echo Bootstrapping vcpkg...
    call build\vcpkg\bootstrap-vcpkg.bat
    call :check_error
) else (
    echo vcpkg already found. Skipping clone and bootstrap.
)

echo Integrating vcpkg...
call build\vcpkg\vcpkg.exe integrate install
call :check_error

echo Installing vcpkg packages...
call build\vcpkg\vcpkg.exe install
call :check_error
echo -----------------------------------------------------------------
echo.


echo [2/6] Building Chromium Embedded Framework (CEF)
echo -----------------------------------------------------------------
if "%~1"=="" (
  set /p "CEF_DIR=Please enter the full path to your extracted CEF directory (e.g., C:\cef_133): "
) else (
  set "CEF_DIR=%~1"
  echo Using CEF directory from command line argument: %CEF_DIR%
)

if not exist "%CEF_DIR%\CMakeLists.txt" (
    echo Error: CEF directory not found or invalid at '%CEF_DIR%'.
    echo Please download and extract the CEF minimal distribution from
    echo https://cef-builds.spotifycdn.com/index.html
    exit /b 1
)

echo Building CEF (Debug and Release)...
cmake -G "Visual Studio 17 2022" -A x64 -DUSE_SANDBOX=OFF -S "%CEF_DIR%" -B "%CEF_DIR%\build"
call :check_error

cmake --build "%CEF_DIR%\build" --config Debug
call :check_error

cmake --build "%CEF_DIR%\build" --config Release
call :check_error

echo Pulling CEF build outputs into the project...
call IntelPresentMon\AppCef\Batch\pull-cef.bat "%CEF_DIR%"
call :check_error
echo -----------------------------------------------------------------
echo.


echo [3/6] Building Web Asset Dependencies (NPM)
echo -----------------------------------------------------------------
pushd IntelPresentMon\AppCef\ipm-ui-vue
echo Installing npm packages...
npm ci
call :check_error

echo Building web assets...
npm run build
call :check_error
popd
echo -----------------------------------------------------------------
echo.


echo [4/6] Creating and Installing Test Certificate
echo -----------------------------------------------------------------
echo This step requires Administrator privileges.
echo If the script fails here, please re-run it from an
echo Administrator command prompt.
echo.
makecert -r -pe -n "CN=Test Certificate - For Internal Use Only" -ss PrivateCertStore testcert.cer > nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Failed to create certificate. This may be because
    echo          'makecert.exe' is not in your PATH or you are not
    echo          running as an Administrator. The Release build might
    echo          fail to run.
) else (
    certutil -addstore root testcert.cer
    call :check_error
    echo Certificate created and installed successfully.
)
echo -----------------------------------------------------------------
echo.


echo [5/6] Building PresentMon Solution
echo -----------------------------------------------------------------
echo Building Debug configuration...
msbuild /p:Platform=x64,Configuration=Debug PresentMon.sln
call :check_error

echo Building Release configuration...
msbuild /p:Platform=x64,Configuration=Release PresentMon.sln
call :check_error
echo -----------------------------------------------------------------
echo.


echo [6/6] Build complete!
echo =================================================================
echo.
echo Binaries can be found in the 'build\Debug' and 'build\Release'
echo directories.
echo.
echo To run the application:
echo  - Start the service (as Administrator):
echo    sc.exe create PresentMonService binPath=\"%cd%\build\Release\PresentMonService.exe\"
echo    sc.exe start PresentMonService
echo  - Run the capture app from its directory:
echo    cd build\Release
echo    PresentMon.exe
echo.
echo =================================================================

endlocal
