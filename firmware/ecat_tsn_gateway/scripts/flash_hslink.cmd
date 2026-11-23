@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Flash ecat_tsn_gateway to an HPM6E80 board through HSLINK/CMSIS-DAP.
rem Usage: flash_hslink.cmd [--dry-run] [path-to-firmware.elf]

set "SCRIPT_DIR=%~dp0"
set "CONFIG_FILE=%SCRIPT_DIR%hslink_hpm6e00.cfg"
set "DRY_RUN=0"
set "ELF_ARGUMENT="
set "ELF_PATH="

:parse_arguments
if "%~1"=="" goto arguments_done
if /I "%~1"=="--help" goto help
if /I "%~1"=="-h" goto help
if /I "%~1"=="--dry-run" (
    set "DRY_RUN=1"
    shift
    goto parse_arguments
)
if defined ELF_ARGUMENT goto invalid_arguments
set "ELF_ARGUMENT=%~1"
shift
goto parse_arguments

:arguments_done
if defined ELF_ARGUMENT (
    for %%I in ("%ELF_ARGUMENT%") do set "ELF_PATH=%%~fI"
) else (
    if not exist "%SCRIPT_DIR%..\build\firmware-elf-path.txt" (
        echo [ERROR] Configure and build firmware first; firmware-elf-path.txt is missing.
        exit /b 2
    )
    set /p "ELF_PATH=" < "%SCRIPT_DIR%..\build\firmware-elf-path.txt"
)

if not exist "%ELF_PATH%" (
    echo [ERROR] ELF file not found: "%ELF_PATH%"
    echo Build the firmware first, or pass an explicit ELF path.
    exit /b 2
)

if not defined HPM_SDK_BASE (
    echo [ERROR] HPM_SDK_BASE is not set.
    echo Start the HPM SDK environment, then run this script again.
    exit /b 2
)

for %%I in ("%HPM_SDK_BASE%\..\tools\openocd\openocd.exe") do set "OPENOCD_EXE=%%~fI"
if not exist "%OPENOCD_EXE%" (
    echo [ERROR] OpenOCD was not found: "%OPENOCD_EXE%"
    exit /b 2
)

set "OPENOCD_SCRIPTS=%APPDATA%\OpenOCD"
if not exist "%OPENOCD_SCRIPTS%\probes\cmsis_dap.cfg" goto missing_openocd_scripts
if not exist "%OPENOCD_SCRIPTS%\soc\hpm6e80-single-core.cfg" goto missing_openocd_scripts
if not exist "%OPENOCD_SCRIPTS%\boards\hpm6e00evk.cfg" goto missing_openocd_scripts
if not exist "%CONFIG_FILE%" (
    echo [ERROR] HSLINK configuration file not found: "%CONFIG_FILE%"
    exit /b 2
)

set "ELF_PATH_OPENOCD=%ELF_PATH:\=/%"
set "OPENOCD_COMMAND=program {%ELF_PATH_OPENOCD%} verify reset exit"

echo ELF:     %ELF_PATH%
echo Config:  %CONFIG_FILE%
echo Probe:   HSLINK / CMSIS-DAP v2

if "%DRY_RUN%"=="1" (
    echo.
    echo [DRY RUN] The following command would be executed:
    echo "%OPENOCD_EXE%" -s "%OPENOCD_SCRIPTS%" -f "%CONFIG_FILE%" -c "%OPENOCD_COMMAND%"
    exit /b 0
)

echo.
echo Flashing and verifying firmware...
"%OPENOCD_EXE%" -s "%OPENOCD_SCRIPTS%" -f "%CONFIG_FILE%" -c "%OPENOCD_COMMAND%"
set "OPENOCD_STATUS=%ERRORLEVEL%"

if not "%OPENOCD_STATUS%"=="0" (
    echo [ERROR] OpenOCD failed with exit code %OPENOCD_STATUS%.
    exit /b %OPENOCD_STATUS%
)

echo [OK] Firmware programmed, verified, and reset.
exit /b 0

:missing_openocd_scripts
echo [ERROR] Required OpenOCD script files are missing under "%OPENOCD_SCRIPTS%".
echo Expected CMSIS-DAP, HPM6E80, and HPM6E00 board definitions.
exit /b 2

:invalid_arguments
echo [ERROR] Only one ELF path may be supplied.
call :print_help
exit /b 2

:help
call :print_help
exit /b 0

:print_help
echo Usage: %~nx0 [--dry-run] [path-to-firmware.elf]
echo.
echo Without an ELF path, the script reads ..\build\firmware-elf-path.txt.
echo --dry-run validates paths and prints the OpenOCD command without programming.
exit /b 0
