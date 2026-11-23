# Author: BearKing
param(
    [ValidateSet("cmsis_dap", "jlink", "ft2232", "ft232", "nds_aice_micro")]
    [string]$Probe = "cmsis_dap",

    [int]$Speed = 20000,

    [string]$Board = "hpm6e00evk",

    [string]$Elf = "",

    [switch]$NoVerify,

    [switch]$NoReset,

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")
$sdkEnvRoot = Resolve-Path (Join-Path $repoRoot "..\..")

$sdkBase = Join-Path $sdkEnvRoot "hpm_sdk"
$openocd = Join-Path $sdkEnvRoot "tools\openocd\openocd.exe"
$openocdTcl = Join-Path $sdkEnvRoot "tools\openocd\tcl"
$openocdCfg = Join-Path $sdkBase "boards\openocd\hpm6e00_all_in_one.cfg"
$openocdSoc = Join-Path $sdkBase "boards\openocd"

if ([string]::IsNullOrWhiteSpace($Elf)) {
    $elfPathFile = Join-Path $repoRoot "firmware\ecat_tsn_gateway\build\firmware-elf-path.txt"
    if (!(Test-Path $elfPathFile)) {
        throw "Firmware output metadata not found: $elfPathFile`nPlease configure and build firmware first."
    }
    $Elf = (Get-Content -LiteralPath $elfPathFile -Raw).Trim()
}

if (!(Test-Path $openocd)) {
    throw "OpenOCD not found: $openocd"
}

if (!(Test-Path $openocdTcl)) {
    throw "OpenOCD tcl directory not found: $openocdTcl"
}

if (!(Test-Path $openocdCfg)) {
    throw "OpenOCD config not found: $openocdCfg"
}

if (!(Test-Path $Elf)) {
    throw "ELF not found: $Elf`nPlease build firmware first."
}

$elfPath = (Resolve-Path $Elf).Path.Replace("\", "/")
$sdkBasePath = (Resolve-Path $sdkBase).Path.Replace("\", "/")

$verifyPart = ""
if (!$NoVerify) {
    $verifyPart = " verify"
}

$resetPart = " reset"
if ($NoReset) {
    $resetPart = ""
}

$programCmd = "program {$elfPath}$verifyPart$resetPart exit"

Write-Host "OpenOCD : $openocd"
Write-Host "SDK     : $sdkBasePath"
Write-Host "Board   : $Board"
Write-Host "Probe   : $Probe"
Write-Host "Speed   : ${Speed}kHz"
Write-Host "ELF     : $elfPath"
Write-Host ""
Write-Host "Command :"
Write-Host "`"$openocd`" -s `"$openocdTcl`" -s `"$openocdSoc`" -c `"set HPM_SDK_BASE $sdkBasePath; set BOARD $Board; set PROBE $Probe`" -f `"$openocdCfg`" -c `"adapter speed $Speed`" -c `"$programCmd`""
Write-Host ""

if ($DryRun) {
    Write-Host "Dry run only. Nothing was downloaded."
    exit 0
}

& $openocd `
    -s $openocdTcl `
    -s $openocdSoc `
    -c "set HPM_SDK_BASE $sdkBasePath; set BOARD $Board; set PROBE $Probe" `
    -f $openocdCfg `
    -c "adapter speed $Speed" `
    -c $programCmd

if ($LASTEXITCODE -ne 0) {
    throw "Firmware download failed. OpenOCD exit code: $LASTEXITCODE"
}

Write-Host ""
Write-Host "Firmware download completed."
