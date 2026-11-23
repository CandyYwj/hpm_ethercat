#!/usr/bin/env bash
# Author: BearKing
# ros2_humble.sh - ROS 2 Humble Docker 容器工具脚本
#
# 用法:
#   ./ros2_humble.sh build [包名...]   在容器内 colcon 编译
#                                      (不带参数默认只编译 hpm_gateway_msgs;
#                                       可传多个包名, 如 build hpm_gateway_msgs hpm_gateway_control)
#   ./ros2_humble.sh shell             进入容器交互式 shell (已 source ROS)
#   ./ros2_humble.sh exec <命令...>    在容器内执行任意命令 (已 source ROS)
#   ./ros2_humble.sh send [参数...]    运行命令发送脚本 send_reference_motor_commands.py
#   ./ros2_humble.sh rqt               打开 rqt 图形界面查看话题/节点 (带 X11, 一次性容器)
#   ./ros2_humble.sh image             构建 dev 镜像 (CMake>=3.28 + 本地 SOEM)
#   ./ros2_humble.sh standalone        构建独立部署镜像 (工作区编进镜像, 可搬去别的机器)
#   ./ros2_humble.sh logs              跟踪容器日志
#   ./ros2_humble.sh start             启动容器 (不存在则先创建并挂载工作区)
#   ./ros2_humble.sh stop              停止容器
#
# 说明:
#   - 容器名: hpm_gateway_humble, 镜像: osrf/ros:humble-desktop
#   - 使用 host 网络 (DDS 与 EtherCAT 直接使用主机网卡)
#   - 挂载 ROS2 工作区到容器内 /workspace
#   - 容器内所有 ROS 命令注入 ROS_DOMAIN_ID=42

set -euo pipefail

CONTAINER="hpm_gateway_humble"
IMAGE="osrf/ros:humble-desktop"
IMAGE_DEV="hpm_gateway_humble:dev"          # ./ros2_humble.sh image 构建
IMAGE_STANDALONE="hpm_gateway_humble:standalone"  # ./ros2_humble.sh standalone 构建

# 主机工作区路径 (脚本所在目录向上定位到 host/hpm_gateway_ros2_ws)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS_HOST="$(cd "${SCRIPT_DIR}/../host/hpm_gateway_ros2_ws" && pwd)"
WS_GUEST="/workspace"
HOST_DIR="$(cd "${SCRIPT_DIR}/../host" && pwd)"

# 若本地已构建 dev 镜像 (含 CMake>=3.28 + SOEM), 则优先使用, 否则回退官方基础镜像
ensure_image() {
  if docker image inspect "${IMAGE_DEV}" >/dev/null 2>&1; then
    IMAGE="${IMAGE_DEV}"
  else
    IMAGE="osrf/ros:humble-desktop"
  fi
}

ensure_container() {
  ensure_image
  if ! docker ps -a --format '{{.Names}}' | grep -qx "${CONTAINER}"; then
    echo "[ros2_humble] 创建容器 ${CONTAINER} ..."
    # 带 X11 挂载创建常驻容器：进入 shell 后可直接 rqt & 开 GUI（不占终端）
    local xauthority="${XAUTHORITY:-${HOME}/.Xauthority}"
    docker run -d --name "${CONTAINER}" --network host \
      -e ROS_DOMAIN_ID=42 \
      -e DISPLAY="${DISPLAY:-:0}" \
      -e XAUTHORITY=/tmp/.Xauthority \
      -v /tmp/.X11-unix:/tmp/.X11-unix \
      -v "${xauthority}:/tmp/.Xauthority:ro" \
      -v "${WS_HOST}:${WS_GUEST}" \
      "${IMAGE}" tail -f /dev/null
  fi
  if [ "$(docker inspect -f '{{.State.Running}}' "${CONTAINER}")" != "true" ]; then
    echo "[ros2_humble] 启动容器 ${CONTAINER} ..."
    docker start "${CONTAINER}" >/dev/null
  fi
}

cmd_build() {
  ensure_container
  local pkgs=("$@")
  if [ "${#pkgs[@]}" -eq 0 ]; then
    pkgs=(hpm_gateway_msgs)
  fi
  local sel
  sel=$(IFS=' '; echo "${pkgs[*]}")
  echo "[ros2_humble] colcon build --packages-select ${sel}"
  docker exec -w "${WS_GUEST}" "${CONTAINER}" bash -lc "
    export ROS_DOMAIN_ID=42
    source /opt/ros/humble/setup.bash
    colcon build --symlink-install --packages-select ${sel}
  "
}

cmd_shell() {
  ensure_container
  docker exec -it -w "${WS_GUEST}" "${CONTAINER}" bash -lc "
    export ROS_DOMAIN_ID=42
    source /opt/ros/humble/setup.bash
    if [ -f ${WS_GUEST}/install/setup.bash ]; then source ${WS_GUEST}/install/setup.bash; fi
    exec bash
  "
}

cmd_exec() {
  ensure_container
  docker exec -w "${WS_GUEST}" "${CONTAINER}" bash -lc "
    export ROS_DOMAIN_ID=42
    source /opt/ros/humble/setup.bash
    if [ -f ${WS_GUEST}/install/setup.bash ]; then source ${WS_GUEST}/install/setup.bash; fi
    $*"
}

cmd_send() {
  ensure_container
  docker exec -w "${WS_GUEST}" "${CONTAINER}" bash -lc "
    export ROS_DOMAIN_ID=42
    source /opt/ros/humble/setup.bash
    source ${WS_GUEST}/install/setup.bash
    exec python3 ${WS_GUEST}/src/hpm_gateway_control/scripts/send_reference_motor_commands.py $*"
}

cmd_rqt() {
  ensure_image
  local xauthority="${XAUTHORITY:-${HOME}/.Xauthority}"
  # 一次性容器打开 rqt (X11 转发), 不干扰常驻容器 hpm_gateway_humble
  # 同时挂载 Xauthority, 否则宿主要求授权时容器报
  # "Authorization required, but no authorization protocol specified"
  docker run --rm -it --network host \
    -e DISPLAY="${DISPLAY:-:0}" \
    -e XAUTHORITY=/tmp/.Xauthority \
    -e ROS_DOMAIN_ID=42 \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "${xauthority}:/tmp/.Xauthority:ro" \
    -v "${WS_HOST}:${WS_GUEST}" \
    "${IMAGE}" bash -lc "
      export ROS_DOMAIN_ID=42
      source /opt/ros/humble/setup.bash
      source ${WS_GUEST}/install/setup.bash
      exec rqt"
}

# 构建 dev 镜像: 基础环境 + CMake>=3.28 + 本地 SOEM (host/Dockerfile --target dev)
cmd_image() {
  echo "[ros2_humble] 构建开发镜像 ${IMAGE_DEV} (CMake>=3.28 + 本地 SOEM) ..."
  docker build -f "${HOST_DIR}/Dockerfile" --target dev -t "${IMAGE_DEV}" "${HOST_DIR}"
  echo "[ros2_humble] 完成: ${IMAGE_DEV}"
}

# 构建 standalone 镜像: 把工作区源码+colcon 产物编进镜像, 可独立部署
cmd_standalone() {
  echo "[ros2_humble] 构建独立部署镜像 ${IMAGE_STANDALONE} (依赖 ${IMAGE_DEV}) ..."
  docker build -f "${HOST_DIR}/Dockerfile" --target standalone -t "${IMAGE_STANDALONE}" "${HOST_DIR}"
  echo "[ros2_humble] 完成: ${IMAGE_STANDALONE}"
}

case "${1:-}" in
  build)
    shift
    cmd_build "$@"
    ;;
  shell)
    cmd_shell
    ;;
  exec)
    shift
    cmd_exec "$@"
    ;;
  send)
    shift
    cmd_send "$@"
    ;;
  rqt)
    cmd_rqt
    ;;
  image)
    cmd_image
    ;;
  standalone)
    cmd_standalone
    ;;
  logs)
    docker logs -f "${CONTAINER}"
    ;;
  start)
    ensure_container
    echo "[ros2_humble] 容器 ${CONTAINER} 运行中"
    ;;
  stop)
    docker stop "${CONTAINER}"
    ;;
  *)
    echo "用法: $0 {build|shell|send|rqt|image|standalone|exec|logs|start|stop}"
    exit 1
    ;;
esac
