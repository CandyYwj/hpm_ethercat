#!/usr/bin/env bash
# deploy_rdk_ros2_workspace.sh - Deploy the ROS 2 workspace src/ to the RDK over SSH.
#
# Linux 版，行为与 scripts/deploy_rdk_ros2_workspace.ps1 一致：把本仓库的
# host/hpm_gateway_ros2_ws/src 打包上传到 RDK，远端先解压到临时目录，
# 成功后将现有 src 重命名为带时间戳的备份（工作区外），再原子切换新目录；
# 切换失败会恢复旧目录。build/install/log 不传输，远端旧目录不被删除。
#
# Usage:
#   ./scripts/deploy_rdk_ros2_workspace.sh [RDK_IP] [--dry-run]
#
# Example:
#   ./scripts/deploy_rdk_ros2_workspace.sh 192.168.108.178
#   ./scripts/deploy_rdk_ros2_workspace.sh            # 默认 192.168.108.178
#
# Environment overrides:
#   RDK_USER               SSH 用户名（默认 root）
#   REMOTE_WORKSPACE       远端工作区路径（默认 /root/hpm_gateway_ros2_ws）

set -euo pipefail

RDK_IP="${RDK_IP:-192.168.108.178}"
RDK_USER="${RDK_USER:-root}"
REMOTE_WORKSPACE="${REMOTE_WORKSPACE:-/root/hpm_gateway_ros2_ws}"
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
LOCAL_WORKSPACE="$REPO_ROOT/host/hpm_gateway_ros2_ws"
LOCAL_SRC="$LOCAL_WORKSPACE/src"

for tool in tar scp ssh; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "error: required tool not found in PATH: $tool" >&2
        exit 1
    }
done

if [[ ! -d "$LOCAL_SRC" ]]; then
    echo "error: local ROS 2 src directory not found: $LOCAL_SRC" >&2
    echo "       Run scripts/pull_rdk_ros2_workspace.ps1 first." >&2
    exit 1
fi
if ! find "$LOCAL_SRC" -type f -name package.xml -print -quit | grep -q .; then
    echo "error: no ROS 2 package.xml found under: $LOCAL_SRC" >&2
    exit 1
fi
if [[ ! -f "$LOCAL_SRC/hpm_gateway_msgs/package.xml" ]]; then
    echo "error: required package not found: $LOCAL_SRC/hpm_gateway_msgs/package.xml" >&2
    echo "       Run scripts/pull_rdk_ros2_workspace.ps1 before deploying." >&2
    exit 1
fi

TARGET="${RDK_USER}@${RDK_IP}"
REMOTE_SRC="$REMOTE_WORKSPACE/src"
echo "Source: $LOCAL_SRC"
echo "Target: $TARGET:$REMOTE_SRC"
echo 'Scope: src only (build, install, and log are excluded)'

if [[ $DRY_RUN -eq 1 ]]; then
    echo "[DRY RUN] No archive is created and no files are transferred."
    exit 0
fi

ARCHIVE="$(mktemp --suffix=.tar.gz)"
REMOTE_ARCHIVE="/tmp/.hpm-gateway-ros2-src-sync-$(date +%s%N).tar.gz"
trap 'rm -f "$ARCHIVE"' EXIT

echo "Creating ROS 2 source archive..."
tar -czf "$ARCHIVE" -C "$LOCAL_WORKSPACE" src

echo "Uploading archive to RDK..."
scp -o BatchMode=no -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 \
    "$ARCHIVE" "$TARGET:$REMOTE_ARCHIVE"

echo "Switching remote ROS 2 source directory..."
ssh -o BatchMode=no -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 \
    "$TARGET" "workspace='$REMOTE_WORKSPACE' target='$REMOTE_SRC' archive='$REMOTE_ARCHIVE' bash -s" <<'REMOTE'
set -eu
backup_root="$workspace.backups"
stamp="$(date +%Y%m%d-%H%M%S)"
stage="$workspace/.src-stage-$stamp-$$"
backup="$backup_root/src.backup-$stamp-$$"
had_target=0

cleanup() {
    status=$?
    rm -f "$archive"
    if test -d "$stage"; then
        rm -rf "$stage"
    fi
    exit "$status"
}
trap cleanup EXIT

mkdir -p "$workspace"
mkdir -p "$backup_root"
mkdir "$stage"
tar -xzf "$archive" -C "$stage"
test -d "$stage/src"
test -n "$(find "$stage/src" -type f -name package.xml -print -quit)"

if test -e "$target"; then
    mv "$target" "$backup"
    had_target=1
else
    backup=none
fi

if ! mv "$stage/src" "$target"; then
    if test "$had_target" -eq 1; then
        mv "$backup" "$target"
    fi
    exit 1
fi

# 确保远端脚本可执行（tar 跨平台时保留 exec bit 不可靠）。
chmod +x "$target/hpm_gateway_control/scripts/send_reference_motor_commands.py"

rmdir "$stage"
rm -f "$archive"
trap - EXIT

printf 'backup=%s\n' "$backup"
printf 'packages='
find "$target" -type f -name package.xml | wc -l
REMOTE

echo "[OK] ROS 2 workspace source deployed. The previous remote src was retained outside the workspace."
