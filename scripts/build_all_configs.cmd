@echo off

echo x64-debug
msbuild /nologo /verbosity:quiet /p:Configuration=debug /p:Platform=x64 "%~dp0..\source\PresentMon.sln"
if not "%errorlevel%"=="0" goto fail

echo x64-release
msbuild /nologo /verbosity:quiet /p:Configuration=release /p:Platform=x64 "%~dp0..\source\PresentMon.sln"
if not "%errorlevel%"=="0" goto fail

echo x86-debug
msbuild /nologo /verbosity:quiet /p:Configuration=debug /p:Platform=x86 "%~dp0..\source\PresentMon.sln"
if not "%errorlevel%"=="0" goto fail

echo x86-release
msbuild /nologo /verbosity:quiet /p:Configuration=release /p:Platform=x86 "%~dp0..\source\PresentMon.sln"
if not "%errorlevel%"=="0" goto fail

exit /b 0

:fail
    echo FAILED!
    exit /b 1
