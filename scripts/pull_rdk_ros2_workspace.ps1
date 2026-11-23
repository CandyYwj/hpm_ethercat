# Author: BearKing
[CmdletBinding()]
param(
    [string]$RdkIp = '192.168.108.178',
    [string]$UserName = 'root',
    [string]$RemoteWorkspace = '/root/hpm_gateway_ros2_ws',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$localWorkspace = Join-Path $repositoryRoot 'host\hpm_gateway_ros2_ws'
$localSourceDirectory = Join-Path $localWorkspace 'src'

foreach ($toolName in @('tar.exe', 'scp.exe', 'ssh.exe')) {
    if (-not (Get-Command $toolName -ErrorAction SilentlyContinue)) {
        throw "Required tool was not found in PATH: $toolName"
    }
}

$RemoteWorkspace = $RemoteWorkspace.TrimEnd('/')
if ($RemoteWorkspace -notmatch '^/(?:[A-Za-z0-9._-]+/)*[A-Za-z0-9._-]+$' -or
    $RemoteWorkspace -match '(?:^|/)\.{1,2}(?:/|$)') {
    throw "RemoteWorkspace must be a normalized absolute Linux path without spaces or '..': $RemoteWorkspace"
}

$transferId = [Guid]::NewGuid().ToString('N')
$remoteSourceDirectory = "$RemoteWorkspace/src"
$remoteArchive = "/tmp/.hpm-gateway-ros2-src-pull-$transferId.tar.gz"
$sshTarget = "${UserName}@${RdkIp}"
$sshOptions = @(
    '-o', 'BatchMode=no',
    '-o', 'StrictHostKeyChecking=accept-new',
    '-o', 'ConnectTimeout=10'
)

Write-Host "Source: $sshTarget`:$remoteSourceDirectory"
Write-Host "Target: $localSourceDirectory"
Write-Host 'Scope: src only (build, install, and log are excluded)'

if ($DryRun) {
    Write-Host '[DRY RUN] No remote command is run and no files are transferred.'
    exit 0
}

if (-not (Test-Path -LiteralPath $localWorkspace -PathType Container)) {
    New-Item -ItemType Directory -Path $localWorkspace | Out-Null
}

$archivePath = Join-Path ([System.IO.Path]::GetTempPath()) "hpm_gateway_ros2_src_pull_$transferId.tar.gz"
$extractRoot = Join-Path ([System.IO.Path]::GetTempPath()) "hpm_gateway_ros2_src_pull_$transferId"
$remoteArchiveCreated = $false

try {
    $remoteCommand = @"
set -eu
source_dir='$remoteSourceDirectory'
archive='$remoteArchive'

cleanup() {
    status=`$?
    if test "`$status" -ne 0; then
        rm -f "`$archive"
    fi
    exit "`$status"
}
trap cleanup EXIT

test -d "`$source_dir"
test -n "`$(find "`$source_dir" -type f -name package.xml -print -quit)"
tar -czf "`$archive" -C '$RemoteWorkspace' src
printf 'archive=%s\n' "`$archive"
printf 'packages='
find "`$source_dir" -type f -name package.xml | wc -l
trap - EXIT
"@

    Write-Host 'Creating remote ROS 2 source archive...'
    & ssh.exe @sshOptions $sshTarget $remoteCommand
    if ($LASTEXITCODE -ne 0) {
        throw "ssh failed with exit code $LASTEXITCODE while creating $remoteArchive."
    }
    $remoteArchiveCreated = $true

    Write-Host 'Downloading archive from RDK...'
    & scp.exe @sshOptions "${sshTarget}:$remoteArchive" $archivePath
    if ($LASTEXITCODE -ne 0) {
        throw "scp failed with exit code $LASTEXITCODE while downloading $remoteArchive."
    }

    New-Item -ItemType Directory -Path $extractRoot | Out-Null

    Write-Host 'Extracting workspace source locally...'
    & tar.exe -xzf $archivePath -C $extractRoot
    if ($LASTEXITCODE -ne 0) {
        throw "tar failed with exit code $LASTEXITCODE"
    }

    $extractedSourceDirectory = Join-Path $extractRoot 'src'
    if (-not (Test-Path -LiteralPath $extractedSourceDirectory -PathType Container)) {
        throw 'Downloaded archive did not contain src.'
    }
    if (-not (Get-ChildItem -LiteralPath $extractedSourceDirectory -Recurse -Filter package.xml -File | Select-Object -First 1)) {
        throw 'Downloaded src did not contain a ROS 2 package.xml.'
    }

    $backupPath = $null
    if (Test-Path -LiteralPath $localSourceDirectory) {
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
        $backupPath = Join-Path $localWorkspace "src.backup-$stamp-$($transferId.Substring(0, 8))"
        Move-Item -LiteralPath $localSourceDirectory -Destination $backupPath
    }

    Move-Item -LiteralPath $extractedSourceDirectory -Destination $localSourceDirectory

    Write-Host '[OK] ROS 2 workspace source pulled from RDK.'
    if ($backupPath) {
        Write-Host "Local backup: $backupPath"
    }
}
finally {
    if ($remoteArchiveCreated) {
        & ssh.exe @sshOptions $sshTarget "rm -f '$remoteArchive'" | Out-Null
    }
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }
    if (Test-Path -LiteralPath $extractRoot) {
        Remove-Item -LiteralPath $extractRoot -Recurse -Force
    }
}
