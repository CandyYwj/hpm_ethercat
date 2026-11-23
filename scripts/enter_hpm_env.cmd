@echo off
rem enter_hpm_env.cmd — 在当前 cmd 会话中设置 HPM 环境变量
rem 用法: enter_hpm_env.cmd [--persist]

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"

if defined HPM_SDK_BASE (
    set "HPM_SDK_BASE=%HPM_SDK_BASE%"
    for %%I in ("%HPM_SDK_BASE%\..") do set "SDK_ENV_ROOT=%%~fI"
) else (
    for %%I in ("%SCRIPT_DIR%..\..") do set "SDK_ENV_ROOT=%%~fI"
    set "HPM_SDK_BASE=%SDK_ENV_ROOT%\hpm_sdk"
)

if defined GNURISCV_TOOLCHAIN_PATH (
    set "GNURISCV_TOOLCHAIN_PATH=%GNURISCV_TOOLCHAIN_PATH%"
) else (
    set "GNURISCV_TOOLCHAIN_PATH=%SDK_ENV_ROOT%\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
)
set "CMK_BIN=%SDK_ENV_ROOT%\tools\cmake\bin"
set "NINJA_BIN=%SDK_ENV_ROOT%\tools\ninja"
set "PY_BIN=%SDK_ENV_ROOT%\tools\python3"

if not exist "%HPM_SDK_BASE%" (
    echo [ERROR] HPM SDK not found: %HPM_SDK_BASE%
    echo If your SDK resides elsewhere, set the environment variable first and re-run:
    echo     set HPM_SDK_BASE=C:\path\to\hpm_sdk
    exit /b 2
)
if not exist "%GNURISCV_TOOLCHAIN_PATH%\bin\riscv32-unknown-elf-gcc.exe" (
    echo [ERROR] RISC-V toolchain not found: %GNURISCV_TOOLCHAIN_PATH%
    echo If your toolchain is elsewhere, set GNURISCV_TOOLCHAIN_PATH and re-run.
    exit /b 2
)

set "PATH=%CMK_BIN%;%GNURISCV_TOOLCHAIN_PATH%\bin;%PY_BIN%;%NINJA_BIN%;%PATH%"

echo HPM_SDK_BASE=%HPM_SDK_BASE%
echo GNURISCV_TOOLCHAIN_PATH=%GNURISCV_TOOLCHAIN_PATH%

if /I "%~1"=="--persist" (
    echo Persisting to user environment via SETX (affects new shells)...
    setx HPM_SDK_BASE "%HPM_SDK_BASE%" >nul
    setx GNURISCV_TOOLCHAIN_PATH "%GNURISCV_TOOLCHAIN_PATH%" >nul
    rem 修改 PATH 需谨慎，这里仅追加 tool paths
    for /f "delims=" %%P in ('powershell -NoProfile -Command "[Environment]::GetEnvironmentVariable('PATH','User')"') do set "USER_PATH=%%P"
    set "ADD_PATH=%CMK_BIN%;%GNURISCV_TOOLCHAIN_PATH%\bin;%NINJA_BIN%;%PY_BIN%"
    echo %USER_PATH%|findstr /C:"%ADD_PATH%" >nul || (
        setx PATH "%USER_PATH%;%ADD_PATH%" >nul
    )
    echo Persist complete. 新开启的终端会看到这些变量。
)

endlocal & (
    set "HPM_SDK_BASE=%HPM_SDK_BASE%"
    set "GNURISCV_TOOLCHAIN_PATH=%GNURISCV_TOOLCHAIN_PATH%"
    set "PATH=%PATH%"
)

echo Done. 现在可以运行: .\scripts\build_firmware.ps1 或 .\scripts\download_firmware.ps1
