@echo off
setlocal

rem Resolve the intel presentmon exe beside this script
set "EXE=%~dp0PresentMon.exe"
if not exist "%EXE%" (
  echo PresentMon.exe not found next to the script: "%EXE%"
  exit /b 9009
)

rem Launch and WAIT so cmd stays blocked; forward all args
start "" /wait "%EXE%" %*

rem Propagate child's exit code to the caller (CI-friendly)
set "RC=%ERRORLEVEL%"
exit /b %RC%
