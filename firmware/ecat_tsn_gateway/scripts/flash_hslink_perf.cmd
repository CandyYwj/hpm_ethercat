@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Flash the O2+debug performance build through HSLINK/CMSIS-DAP.
rem Usage: flash_hslink_perf.cmd [--dry-run]

set "SCRIPT_DIR=%~dp0"

if "%~1"=="" goto flash
if /I "%~1"=="--dry-run" goto flash
if /I "%~1"=="--help" goto help
if /I "%~1"=="-h" goto help

echo [ERROR] Unsupported argument: %~1
call :print_help
exit /b 2

:flash
set "PERF_ELF_PATH="
if not exist "%SCRIPT_DIR%..\build_perf\firmware-elf-path.txt" (
    echo [ERROR] Configure and build the performance firmware first.
    exit /b 2
)
set /p "PERF_ELF_PATH=" < "%SCRIPT_DIR%..\build_perf\firmware-elf-path.txt"
call "%SCRIPT_DIR%flash_hslink.cmd" %* "%PERF_ELF_PATH%"
exit /b %ERRORLEVEL%

:help
call :print_help
exit /b 0

:print_help
echo Usage: %~nx0 [--dry-run]
echo.
echo Reads the ELF path from ..\build_perf\firmware-elf-path.txt.
echo Run from any directory after building the performance firmware.
exit /b 0
