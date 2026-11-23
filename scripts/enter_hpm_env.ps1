<#
.SYNOPSIS
    在当前 PowerShell 会话中配置 HPM 开发环境变量（临时）。

.DESCRIPTION
    将仓库相对位置的 HPM SDK、RISC-V 工具链和工具目录加入环境变量，
    以便直接运行仓库内的构建与烧写脚本（例如 `build_firmware.ps1` 和
    `download_firmware.ps1`）。

.PARAMETER Persist
    将变量写入用户环境（使用 `setx`），使其在新打开的终端中永久生效。
    注意：`setx` 不会影响当前会话，只影响今后启动的进程。

USAGE
    # 在当前 PowerShell 会话设置（临时）：
    .\scripts\enter_hpm_env.ps1

    # 并将变量持久化到当前用户环境：
    .\scripts\enter_hpm_env.ps1 -Persist
#>

param(
    [switch]$Persist,
    [string]$SdkRoot
)

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")

# Allow explicit SdkRoot parameter, or respect existing HPM_SDK_BASE env var,
# otherwise infer from repository layout (two levels up).
if ($SdkRoot) {
    $sdkEnvRoot = Resolve-Path $SdkRoot
} elseif ($env:HPM_SDK_BASE) {
    # If user already set HPM_SDK_BASE, use its parent as sdkEnvRoot when possible
    $sdkBase = Resolve-Path $env:HPM_SDK_BASE
    $sdkEnvRoot = Split-Path $sdkBase -Parent
} else {
    $sdkEnvRoot = Resolve-Path (Join-Path $repoRoot "..\..")
}

if (-not $sdkBase) {
    $sdkBase = Join-Path $sdkEnvRoot "hpm_sdk"
}

if ($env:GNURISCV_TOOLCHAIN_PATH) {
    $toolchain = $env:GNURISCV_TOOLCHAIN_PATH
} else {
    $toolchain = Join-Path $sdkEnvRoot "toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
}
$cmakeBin = Join-Path $sdkEnvRoot "tools\cmake\bin"
$ninjaBin = Join-Path $sdkEnvRoot "tools\ninja"
$pythonBin = Join-Path $sdkEnvRoot "tools\python3"

if (!(Test-Path $sdkBase)) {
    Write-Error "HPM SDK not found: $sdkBase"
    Write-Host "如果 HPM SDK 在其它位置，先设置环境变量:`n  $env:HPM_SDK_BASE = 'C:\path\to\hpm_sdk'`" -ForegroundColor Yellow
    exit 2
}
if (!(Test-Path (Join-Path $toolchain "bin\riscv32-unknown-elf-gcc.exe"))) {
    Write-Error "RISC-V toolchain not found: $toolchain"
    Write-Host "如果工具链路径不同，请先设置 `GNURISCV_TOOLCHAIN_PATH` 环境变量或用 -SdkRoot 参数指定 SDK 根目录。" -ForegroundColor Yellow
    exit 2
}

Write-Host "Configuring HPM environment for this session..."
$env:HPM_SDK_BASE = (Resolve-Path $sdkBase).Path
$env:GNURISCV_TOOLCHAIN_PATH = (Resolve-Path $toolchain).Path
$env:PATH = "$(Resolve-Path $cmakeBin).Path;$(Resolve-Path (Join-Path $toolchain 'bin')).Path;$pythonBin;$(Resolve-Path $ninjaBin).Path;$env:PATH"

Write-Host "HPM_SDK_BASE=$env:HPM_SDK_BASE"
Write-Host "GNURISCV_TOOLCHAIN_PATH=$env:GNURISCV_TOOLCHAIN_PATH"

if ($Persist) {
    Write-Host "Persisting variables to current user environment (setx)..."
    & setx HPM_SDK_BASE "$env:HPM_SDK_BASE" | Out-Null
    & setx GNURISCV_TOOLCHAIN_PATH "$env:GNURISCV_TOOLCHAIN_PATH" | Out-Null
    # Persisting PATH is intrusive; only append the tools if not already present.
    $currentPath = [Environment]::GetEnvironmentVariable('PATH','User')
    $addPaths = "$(Resolve-Path $cmakeBin).Path;$(Resolve-Path (Join-Path $toolchain 'bin')).Path;$(Resolve-Path $ninjaBin).Path;$(Resolve-Path $pythonBin).Path"
    if ($currentPath -notlike "*$addPaths*") {
        & setx PATH "$currentPath;$addPaths" | Out-Null
    }
    Write-Host "Persisted. 新打开的终端会话将看到这些环境变量。"
}

Write-Host "Done. 现在可以运行: .\scripts\build_firmware.ps1; .\scripts\download_firmware.ps1"
