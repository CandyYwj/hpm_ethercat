#!/usr/bin/env bash
# 容器入口: source ROS 环境; 若存在工作区 install 一并 source。
# 这样 standalone 镜像跑 `docker run ... ros2 topic list` 直接可用。
set -e

source /opt/ros/humble/setup.bash
if [ -f /workspace/install/setup.bash ]; then
  source /workspace/install/setup.bash
fi

exec "$@"
