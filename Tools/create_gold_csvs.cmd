:: Copyright 2020 Intel Corporation
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to
:: deal in the Software without restriction, including without limitation the
:: rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
:: sell copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
:: FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
:: IN THE SOFTWARE.
@echo off
setlocal enabledelayedexpansion
set presentmon=%1
set rootdir=%~2
set force=0
if not exist %presentmon% goto usage
if not exist "%rootdir%\." goto usage
if "%~3"=="force" (
    set force=1
) else (
    if not "%~3"=="" goto usage
)
goto args_ok
:usage
    echo usage: create_fold_csvs PresentMonPath GoldEtlCsvRootDir [force]
    exit /b 1
:args_ok

set pmargs=-no_top -stop_existing_session -qpc_time -verbose -captureall
for /f "tokens=*" %%a in ('dir /s /b /a-d "%rootdir%\*.etl"') do call :create_csv %%a
exit /b 0

:create_csv
    if exist "%~dpn1.csv" if %force% neq 1 exit /b 0
    echo %presentmon% %pmargs% -etl_file %1 -output_file "%~dpn1.csv"
    %presentmon% %pmargs% -etl_file %1 -output_file "%~dpn1.csv"
    echo.
    exit /b 0

