@echo off
setlocal
set csvpath=%~1
set processes=
set processcount=0
:gather_processes
    shift
    if "%~1"=="" goto done_gathering_processes
    set processes=%processes%,"%~1"
    set /a processcount=%processcount%+1
    goto gather_processes
:done_gathering_processes

if not exist "%csvpath%" goto usage
if %processcount% equ 0 goto usage
goto args_ok
:usage
    echo usage: prune_processes.cmd path_to_csv.csv process_name [process2_name ...] 1>&2
    echo. 1>&2
    echo CSV will be output to stdout with data from non-listed processes excluded. 1>&2
    echo. 1>&2
    echo e.g.: prune_processes.cmd PresentMon.csv foo.exe bar.exe ^> PresentMon-pruned.csv 1>&2
    exit /b 1
:args_ok

echo Pruning processes from %csvpath% except:1>&2
for %%a in (%processes%) do (
    echo.     %%a 1>&2
)
set lineno=0
for /f "tokens=*" %%a in (%csvpath%) do call :handle_line "%%a"
exit /b 0

:handle_line
    set /a lineno=%lineno%+1
    if %lineno% equ 1 (
        echo %~1
    ) else (
        for /f "tokens=1 delims=," %%a in (%1) do call :check_line %1 "%%a"
    )
    exit /b 0

:check_line
    for %%a in (%processes%) do (
        if %%a equ %2 (
            call :print_line %1
            exit /b 0
        )
    )
    exit /b 0

:print_line
    set line=##%1##
    set line=%line:<=^<%
    set line=%line:>=^>%
    set line=%line:"##=%
    echo %line:##"=%
    exit /b 0
