# Author: BearKing
[CmdletBinding()]
param(
    [string]$RdkIp = '192.168.108.178',
    [string]$UserName = 'root',
    [string]$RemoteRoot = '/root/hpm_gateway',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$localHostDirectory = Join-Path $repositoryRoot 'host'

if (-not (Test-Path -LiteralPath $localHostDirectory -PathType Container)) {
    throw "Local host directory was not found: $localHostDirectory"
}

foreach ($toolName in @('tar.exe', 'scp.exe', 'ssh.exe')) {
    if (-not (Get-Command $toolName -ErrorAction SilentlyContinue)) {
        throw "Required tool was not found in PATH: $toolName"
    }
}

if (-not $RemoteRoot.StartsWith('/')) {
    throw "RemoteRoot must be an absolute Linux path: $RemoteRoot"
}

$transferId = [Guid]::NewGuid().ToString('N')
$remoteArchive = "$RemoteRoot/.host-sync-$transferId.tar.gz"
$sshTarget = "${UserName}@${RdkIp}"
$sshOptions = @(
    '-o', 'BatchMode=no',
    '-o', 'StrictHostKeyChecking=accept-new',
    '-o', 'ConnectTimeout=10'
)

Write-Host "Source: $localHostDirectory"
Write-Host "Target: $sshTarget`:$RemoteRoot/host"

if ($DryRun) {
    Write-Host '[DRY RUN] No archive is created and no files are transferred.'
    exit 0
}

$archivePath = Join-Path ([System.IO.Path]::GetTempPath()) "hpm_gateway_host_$transferId.tar.gz"

try {
    Write-Host 'Creating host source archive...'
    & tar.exe -czf $archivePath -C $repositoryRoot host
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
base='$RemoteRoot'
archive="`$base/.host-sync-$transferId.tar.gz"
stamp="`$(date +%Y%m%d-%H%M%S)"
stage="`$base/.host-stage-`$stamp-`$$"
backup="`$base/host.backup-`$stamp"

mkdir "`$stage"
tar -xzf "`$archive" -C "`$stage"
test -d "`$stage/host"

if test -e "`$base/host"; then
    mv "`$base/host" "`$backup"
else
    backup=none
fi

mv "`$stage/host" "`$base/host"
rmdir "`$stage"
rm -f "`$archive"

printf 'backup=%s\n' "`$backup"
printf 'directories='
find "`$base/host" -mindepth 1 -maxdepth 1 -type d -printf '%f '
echo
printf 'files='
find "`$base/host" -type f | wc -l
"@

    Write-Host 'Switching remote host directory...'
    & ssh.exe @sshOptions $sshTarget $remoteCommand
    if ($LASTEXITCODE -ne 0) {
        throw "ssh failed with exit code $LASTEXITCODE. The remote archive may remain at $remoteArchive."
    }

    Write-Host '[OK] Host code deployed. The previous remote host directory was retained as a timestamped backup.'
}
finally {
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }
}
