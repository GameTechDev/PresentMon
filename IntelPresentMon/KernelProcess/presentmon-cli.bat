@echo off
setlocal

rem Resolve PresentMon.exe relative to this script.
set "SCRIPT_DIR=%~dp0"
set "EXE=%SCRIPT_DIR%PresentMon.exe"

rem When launched from the pathed\ subdirectory, the exe is in parent.
if not exist "%EXE%" set "EXE=%SCRIPT_DIR%..\PresentMon.exe"

if not exist "%EXE%" (
  echo PresentMon.exe not found relative to the script:
  echo   "%SCRIPT_DIR%PresentMon.exe"
  echo   "%SCRIPT_DIR%..\PresentMon.exe"
  exit /b 9009
)

rem Launch and WAIT so cmd stays blocked; forward all args.
start "" /wait "%EXE%" %*

rem Propagate child exit code to the caller.
set "RC=%ERRORLEVEL%"
exit /b %RC%
