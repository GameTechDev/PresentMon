:: Copyright (C) 2017-2024 Intel Corporation
:: SPDX-License-Identifier: MIT

@echo off
setlocal enabledelayedexpansion
set presentmon=%1
set rootdir=%~2
set force=0
if "%~1" equ "" goto usage
if "%~2" equ "" goto usage
if not exist %presentmon% goto usage
if not exist "%rootdir%\." goto usage
if "%~3"=="force" (
    set force=1
) else (
    if not "%~3"=="" goto usage
)
goto args_ok
:usage
    echo usage: create_gold_csvs.cmd PresentMonPath GoldEtlCsvRootDir [force]
    exit /b 1
:args_ok
set already_exists=0

for /f "tokens=*" %%a in ('dir /s /b /a-d "%rootdir%\*.etl"') do call :create_csv "%%a"

if %already_exists% neq 0 echo Use 'force' command line argument to overwrite.
exit /b 0

:create_csv
    call :create_csv_2 %1 "%~dpn1.csv"    " "
    call :create_csv_2 %1 "%~dpn1_v1.csv" "--v1_metrics"
    call :create_csv_2 %1 "%~dpn1_v2.csv" "--v2_metrics"
    exit /b 0

:create_csv_2
    if exist %2 if %force% neq 1 (
        echo Already exists: %~2
        set already_exists=1
        exit /b 0
    )

    echo %presentmon% --no_console_stats --stop_existing_session --qpc_time --track_frame_type --track_app_timing --track_gpu_video --track_pc_latency %~3 --etl_file %1 --output_file %2
         %presentmon% --no_console_stats --stop_existing_session --qpc_time --track_frame_type --track_app_timing --track_gpu_video --track_pc_latency %~3 --etl_file %1 --output_file %2 >NUL 2>&1
    exit /b 0
