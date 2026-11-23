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

if (-not (Test-Path -LiteralPath $localSourceDirectory -PathType Container)) {
    throw "Local ROS 2 src directory was not found: $localSourceDirectory`nRun scripts\pull_rdk_ros2_workspace.ps1 first."
}
if (-not (Get-ChildItem -LiteralPath $localSourceDirectory -Recurse -Filter package.xml -File | Select-Object -First 1)) {
    throw "No ROS 2 package.xml was found under: $localSourceDirectory"
}
$requiredMessagePackage = Join-Path $localSourceDirectory 'hpm_gateway_msgs\package.xml'
if (-not (Test-Path -LiteralPath $requiredMessagePackage -PathType Leaf)) {
    throw "Required package was not found: $requiredMessagePackage`nRun scripts\pull_rdk_ros2_workspace.ps1 before deploying."
}

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
$remoteArchive = "/tmp/.hpm-gateway-ros2-src-sync-$transferId.tar.gz"
$sshTarget = "${UserName}@${RdkIp}"
$sshOptions = @(
    '-o', 'BatchMode=no',
    '-o', 'StrictHostKeyChecking=accept-new',
    '-o', 'ConnectTimeout=10'
)

Write-Host "Source: $localSourceDirectory"
Write-Host "Target: $sshTarget`:$remoteSourceDirectory"
Write-Host 'Scope: src only (build, install, and log are excluded)'

if ($DryRun) {
    Write-Host '[DRY RUN] No archive is created and no files are transferred.'
    exit 0
}

$archivePath = Join-Path ([System.IO.Path]::GetTempPath()) "hpm_gateway_ros2_src_sync_$transferId.tar.gz"

try {
    Write-Host 'Creating ROS 2 source archive...'
    & tar.exe -czf $archivePath -C $localWorkspace src
    if ($LASTEXITCODE -ne 0) {
        throw "tar failed with exit code $LASTEXITCODE"
    }

    Write-Host 'Uploading archive to RDK...'
    & scp.exe @sshOptions $archivePath "${sshTarget}:$remoteArchive"
    if ($LASTEXITCODE -ne 0) {
        throw "scp failed with exit code $LASTEXITCODE"
    }

    $remoteCommand = @"
set -eu
workspace='$RemoteWorkspace'
target='$remoteSourceDirectory'
archive='$remoteArchive'
backup_root="`$workspace.backups"
stamp="`$(date +%Y%m%d-%H%M%S)"
stage="`$workspace/.src-stage-`$stamp-`$$"
backup="`$backup_root/src.backup-`$stamp-`$$"
had_target=0

cleanup() {
    status=`$?
    rm -f "`$archive"
    if test -d "`$stage"; then
        rm -rf "`$stage"
    fi
    exit "`$status"
}
trap cleanup EXIT

mkdir -p "`$workspace"
mkdir -p "`$backup_root"
mkdir "`$stage"
tar -xzf "`$archive" -C "`$stage"
test -d "`$stage/src"
test -n "`$(find "`$stage/src" -type f -name package.xml -print -quit)"

if test -e "`$target"; then
    mv "`$target" "`$backup"
    had_target=1
else
    backup=none
fi

if ! mv "`$stage/src" "`$target"; then
    if test "`$had_target" -eq 1; then
        mv "`$backup" "`$target"
    fi
    exit 1
fi

# Windows tar does not preserve the Unix executable bit; restore it so
# `ros2 run hpm_gateway_control send_reference_motor_commands.py` works.
chmod +x "`$target/hpm_gateway_control/scripts/send_reference_motor_commands.py"

rmdir "`$stage"
rm -f "`$archive"
trap - EXIT

printf 'backup=%s\n' "`$backup"
printf 'packages='
find "`$target" -type f -name package.xml | wc -l
"@

    Write-Host 'Switching remote ROS 2 source directory...'
    & ssh.exe @sshOptions $sshTarget $remoteCommand
    if ($LASTEXITCODE -ne 0) {
        throw "ssh failed with exit code $LASTEXITCODE while updating $remoteSourceDirectory."
    }

    Write-Host '[OK] ROS 2 workspace source deployed. The previous remote src was retained outside the workspace.'
}
finally {
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }
}
