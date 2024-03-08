:: Copyright (C) 2022 Intel Corporation
:: SPDX-License-Identifier: MIT
@echo off
if "%~1"=="" goto usage
if "%~2"=="" goto usage
if not exist %1 goto usage
goto ok
:usage
    echo usage: generate_options_header.cmd path_to_readme.md path_to_output_header.h
    exit /b 1
:ok

if exist %2 echo.>%2

setlocal enabledelayedexpansion

pushd "%~dp0."

set done=0
for /f "tokens=1,2 delims=|" %%a in ('sed.exe -nr "/^\|[^|]*\|[^|]*\|$/p" %1') do (
    if !done! equ 0 call :process "%%a" "%%b" %2
)

popd
exit /b 0

:process
    set a1=%1
    set a2=%2
    set a1=%a1:`=%
    set a2=%a2:`=%

    set c1=""
    set c2=""
    for /f "tokens=*" %%a in ('echo^| set /p^=%a1% ^| awk.exe "{$1=$1};1"') do set c1="%%a"
    for /f "tokens=*" %%a in ('echo^| set /p^=%a2% ^| awk.exe "{$1=$1};1"') do set c2="%%a"

    if %c1% equ "Column Header" (
        set done=1
        exit /b 0
    )

    if %c1:-=% neq "" (
        if %c2% equ "" (
            echo.>> %3
            echo L%c1%, nullptr,>> %3
        ) else (
            echo L%c1%, LR"(%c2:~1,-1%)",>> %3
        )
    )

    exit /b 0

