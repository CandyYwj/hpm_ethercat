#!/usr/bin/env bash
# deploy_host_to_rdk.sh - Deploy the host/ source tree to the RDK over SSH.
#
# Linux 版，行为与 scripts/deploy_host_to_rdk.ps1 一致：用 tar 打包 host/，
# scp 上传后原子替换远端目录，旧目录保留为带时间戳的备份，不删除。
#
# Usage:
#   ./scripts/deploy_host_to_rdk.sh [RDK_IP] [--dry-run]
#
# Example:
#   ./scripts/deploy_host_to_rdk.sh 192.168.108.118
#   ./scripts/deploy_host_to_rdk.sh            # 默认 192.168.108.178
#
# Environment overrides:
#   RDK_USER      SSH 用户名（默认 root）
#   REMOTE_ROOT   远端父目录（默认 /root/hpm_gateway）

set -euo pipefail

RDK_IP="${RDK_IP:-192.168.108.178}"
RDK_USER="${RDK_USER:-root}"
REMOTE_ROOT="${REMOTE_ROOT:-/root/hpm_gateway}"
DRY_RUN=0

# 允许 `--dry-run` 出现在第一或第二参数，避免被误当成 IP。
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=1
    shift 2>/dev/null || true
fi
RDK_IP="${1:-$RDK_IP}"
if [[ "${2:-}" == "--dry-run" ]]; then
    DRY_RUN=1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCAL_HOST_DIR="$REPO_ROOT/host"

for tool in tar scp ssh; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "error: required tool not found in PATH: $tool" >&2
        exit 1
    }
done

if [[ ! -d "$LOCAL_HOST_DIR" ]]; then
    echo "error: local host directory not found: $LOCAL_HOST_DIR" >&2
    exit 1
fi

TARGET="${RDK_USER}@${RDK_IP}"
echo "Source: $LOCAL_HOST_DIR"
echo "Target: $TARGET:$REMOTE_ROOT/host"

if [[ $DRY_RUN -eq 1 ]]; then
    echo "[DRY RUN] No archive is created and no files are transferred."
    exit 0
fi

ARCHIVE="$(mktemp --suffix=.tar.gz)"
REMOTE_ARCHIVE="$REMOTE_ROOT/.host-sync-$(date +%s%N).tar.gz"
trap 'rm -f "$ARCHIVE"' EXIT

echo "Creating host source archive..."
tar -czf "$ARCHIVE" -C "$REPO_ROOT" host

echo "Uploading archive to RDK..."
scp -o BatchMode=no -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 \
    "$ARCHIVE" "$TARGET:$REMOTE_ARCHIVE"

echo "Switching remote host directory..."
ssh -o BatchMode=no -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 \
    "$TARGET" "base='$REMOTE_ROOT' archive='$REMOTE_ARCHIVE' bash -s" <<'REMOTE'
set -eu
stamp="$(date +%Y%m%d-%H%M%S)"
stage="$base/.host-stage-$stamp-$$"
backup="$base/host.backup-$stamp"

mkdir "$stage"
tar -xzf "$archive" -C "$stage"
test -d "$stage/host"

if test -e "$base/host"; then
    mv "$base/host" "$backup"
else
    backup=none
fi

mv "$stage/host" "$base/host"
rmdir "$stage"
rm -f "$archive"

printf 'backup=%s\n' "$backup"
printf 'directories='
find "$base/host" -mindepth 1 -maxdepth 1 -type d -printf '%f '
echo
printf 'files='
find "$base/host" -type f | wc -l
REMOTE

echo "[OK] Host code deployed to $TARGET:$REMOTE_ROOT/host"
