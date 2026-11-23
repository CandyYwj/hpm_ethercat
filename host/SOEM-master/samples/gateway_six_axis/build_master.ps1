# Author: BearKing
# Build the six-axis bench master against the SOEM sources in this checkout.
# Requires MinGW-w64 gcc on PATH and Npcap installed (wpcap.dll / Packet.dll).
$ErrorActionPreference = "Stop"
$sampleDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent (Split-Path -Parent $sampleDir)
$out = Join-Path $root "build_win"
$shim = Join-Path $out "shim"
New-Item -ItemType Directory -Force -Path $shim | Out-Null

# MinGW 8.x lacks C11 timespec_get and timeapi.h used by SOEM win32 osal.
$shimLines = @(
  "#pragma once",
  "#include <time.h>",
  "#ifndef TIME_UTC",
  "#define TIME_UTC 1",
  "#endif",
  "#if defined(__MINGW32__) || defined(__MINGW64__)",
  "static inline int timespec_get(struct timespec *ts, int base)",
  "{",
  "    (void)base;",
  "    if (ts == NULL) { return 0; }",
  "    ts->tv_sec = (time_t)time(NULL);",
  "    ts->tv_nsec = 0;",
  "    return 1;",
  "}",
  "#endif"
)
Set-Content -LiteralPath (Join-Path $shim "timespec_shim.h") -Value $shimLines -Encoding utf8
Set-Content -LiteralPath (Join-Path $shim "timeapi.h") -Value "#pragma once", "#include <mmsystem.h>" -Encoding utf8

$sources = @()
$sources += (Get-ChildItem (Join-Path $root "src") -Filter *.c | ForEach-Object { $_.FullName })
$sources += (Join-Path $root "osal\win32\osal.c")
$sources += (Join-Path $root "oshw\win32\oshw.c")
$sources += (Join-Path $root "oshw\win32\nicdrv.c")
$sources += (Join-Path $sampleDir "gateway_master.c")

$exe = Join-Path $out "gateway_master.exe"
$compileArgs = @(
    "-std=c11", "-O2", "-D_WIN32_WINNT=0x0601",
    "-I", $shim, "-include", (Join-Path $shim "timespec_shim.h"),
    "-I", (Join-Path $root "include"),
    "-I", (Join-Path $root "osal"),
    "-I", (Join-Path $root "osal\win32"),
    "-I", (Join-Path $root "oshw\win32"),
    "-I", (Join-Path $root "oshw\win32\wpcap\Include")
) + $sources + @(
    "-lws2_32", "-lwinmm", "-liphlpapi", "-o", $exe,
    "C:\Windows\System32\wpcap.dll", "C:\Windows\System32\Packet.dll"
)

Write-Host "Building $exe"
& gcc @compileArgs
if ($LASTEXITCODE -ne 0) { throw "build failed" }
Write-Host "OK: $exe"
