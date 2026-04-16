@echo off
setlocal enabledelayedexpansion

echo Generating AllActions.h

REM Define the output file
set outputFile=AllActions.h

REM Generate a temporary file for comparison
set tempFile=%TEMP%\AllActions_tmp.h

REM Create or overwrite the temporary file and write the preamble
echo // GENERATED HEADER>%tempFile%
echo #pragma once>>%tempFile%

REM Iterate over all .h files in the .\acts directory
for %%f in (.\acts\*.h) do (
    echo #include "acts/%%~nxf">>%tempFile%
)

REM Compare the temporary file with the existing AllActions.h
fc %tempFile% %outputFile% > nul
if errorlevel 1 (
    echo Changes detected, updating %outputFile%
    move /Y %tempFile% %outputFile%
) else (
    echo No changes detected, no update needed.
    del %tempFile%
)

endlocal
