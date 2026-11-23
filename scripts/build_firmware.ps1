# Author: BearKing
param(
    [switch]$Clean,
    [switch]$DryRun
)

# Build the HPM6E80 EtherCAT gateway firmware into the single `build`
# directory with O2 (GATEWAY_PERFORMANCE_BUILD=ON, keeps debug symbols).
# Usage (from the repository root):
#   .\scripts\build_firmware.ps1          # configure + build
#   .\scripts\build_firmware.ps1 -Clean   # clean + rebuild
#   .\scripts\build_firmware.ps1 -DryRun  # print commands only

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
$sdkEnvRoot = Resolve-Path (Join-Path $repoRoot "..\..")

$sdkBase = Join-Path $sdkEnvRoot "hpm_sdk"
$toolchain = Join-Path $sdkEnvRoot "toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
$boardSearchPath = Join-Path $sdkEnvRoot "hpm_app\boards"
$cmake = Join-Path $sdkEnvRoot "tools\cmake\bin\cmake.exe"
$ninja = Join-Path $sdkEnvRoot "tools\ninja\ninja.exe"
$python = Join-Path $sdkEnvRoot "tools\python3"

$fwSrc = Join-Path $repoRoot "firmware\ecat_tsn_gateway"
$fwBuild = Join-Path $fwSrc "build"

foreach ($tool in @($cmake, $ninja)) {
    if (!(Test-Path $tool)) {
        throw "Tool not found: $tool"
    }
}
if (!(Test-Path $sdkBase)) {
    throw "HPM SDK not found: $sdkBase"
}
if (!(Test-Path (Join-Path $toolchain "bin\riscv32-unknown-elf-gcc.exe"))) {
    throw "RISC-V toolchain not found: $toolchain"
}

# HPM SDK environment required by the SDK's cmake toolchain files.
$env:HPM_SDK_BASE = $sdkBase
$env:GNURISCV_TOOLCHAIN_PATH = $toolchain
$env:PATH = "$(Join-Path $sdkEnvRoot 'tools\cmake\bin');$(Join-Path $toolchain 'bin');$python;$(Join-Path $sdkEnvRoot 'tools\ninja');$env:PATH"

$configureArgs = @(
    "-S", $fwSrc,
    "-B", $fwBuild,
    "-G", "Ninja",
    "-DBOARD=hpm6e00_full_port",
    "-DBOARD_SEARCH_PATH=$boardSearchPath",
    "-DHPM_BUILD_TYPE=flash_xip",
    "-DCMAKE_BUILD_TYPE=debug",
    "-DGATEWAY_PERFORMANCE_BUILD=ON"
)

Write-Host "CMake  : $cmake"
Write-Host "Ninja  : $ninja"
Write-Host "SDK    : $sdkBase"
Write-Host "Board  : hpm6e00_full_port (O2, flash_xip, debug symbols)"
Write-Host "Source : $fwSrc"
Write-Host "Build  : $fwBuild"
Write-Host ""

if ($DryRun) {
    Write-Host "[dry-run] cmake $($configureArgs -join ' ')"
    if ($Clean) {
        Write-Host "[dry-run] $ninja -C `"$fwBuild`" -t clean"
    }
    Write-Host "[dry-run] $ninja -C `"$fwBuild`""
    exit 0
}

& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed. Exit code: $LASTEXITCODE"
}

if ($Clean) {
    Write-Host ""
    Write-Host "Cleaning build directory..."
    & $ninja -C $fwBuild -t clean
    if ($LASTEXITCODE -ne 0) {
        throw "ninja clean failed. Exit code: $LASTEXITCODE"
    }
}

Write-Host ""
Write-Host "Building firmware..."
& $ninja -C $fwBuild
if ($LASTEXITCODE -ne 0) {
    throw "Firmware build failed. Exit code: $LASTEXITCODE"
}

$elfPathFile = Join-Path $fwBuild "firmware-elf-path.txt"
if (!(Test-Path $elfPathFile)) {
    throw "Firmware output metadata not found: $elfPathFile"
}
$elf = (Get-Content -LiteralPath $elfPathFile -Raw).Trim()
if (!(Test-Path $elf)) {
    throw "Build succeeded but ELF not found: $elf"
}

Write-Host ""
Write-Host "Build OK: $elf"
Write-Host "Flash with: .\scripts\download_firmware.ps1"
