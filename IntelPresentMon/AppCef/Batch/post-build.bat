@echo off
echo Validate staged CEF payload...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0validate-cef.ps1" -Mode Stage
if errorlevel 1 exit /b %errorlevel%

echo Copy CEF binaries...
xcopy /SY "Cef\Bin\" "%~2"

echo Copy CEF resources...
xcopy /SY "Cef\Resources" "%~2"

if "%~1"=="Release" (
	echo "Build Web Assets..."
	call "%~dp0build-web.bat"
)

echo Copy web resources...
xcopy /SY "ipm-ui-vue\dist" "%~2ipm-ui-vue\"

echo Copy presets...
xcopy /SY "ipm-ui-vue\presets" "%~2Presets\"

echo Copy block list...
xcopy /SY "ipm-ui-vue\BlockLists" "%~2BlockLists\"
