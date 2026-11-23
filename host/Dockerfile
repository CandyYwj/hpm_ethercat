# syntax=docker/dockerfile:1
# =====================================================================
# HPM Gateway — ROS 2 Humble 容器镜像
#
# 两个构建目标:
#   --target dev        开发镜像: 基础环境 + CMake>=3.28 + 本地 SOEM 编译,
#                       工作区在运行时挂载 (scripts/ros2_humble.sh 默认流程)
#   --target standalone 独立部署镜像: 在 dev 基础上把工作区源码 + colcon 产物
#                       编进镜像, 不依赖宿主机挂载, 可直接 docker save/load 搬走
#
# 构建 (在仓库根目录执行):
#   docker build -f host/Dockerfile --target dev        -t hpm_gateway_humble:dev        host/
#   docker build -f host/Dockerfile --target standalone -t hpm_gateway_humble:standalone host/
#
# 或使用脚本:
#   ./scripts/ros2_humble.sh image         # 构建 dev
#   ./scripts/ros2_humble.sh standalone    # 构建 standalone (需先有 dev)
# =====================================================================

# ---------------------------------------------------------------------
# 目标 1: dev — 开发镜像
# ---------------------------------------------------------------------
FROM osrf/ros:humble-desktop AS dev

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DOMAIN_ID=42

# 构建工具链 + colcon
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        git \
        python3-pip \
        python3-colcon-common-extensions \
    && rm -rf /var/lib/apt/lists/*

# SOEM 的 CMakeLists 要求 CMake >= 3.28; humble 自带 3.22 编不过。
# pip 装的 cmake 落在 /usr/local/bin, PATH 优先于系统 /usr/bin/cmake (3.22)。
RUN python3 -m pip install --no-cache-dir "cmake>=3.28"

# 用仓库内的 SOEM 源码 (host/tools/SOEM) 编译, 免去容器内再下载 GitHub。
# 安装到 /root/Tools/EtherCat/SOEM —— hpm_gateway_control 的默认查找路径。
COPY tools/SOEM /root/Tools/EtherCat/SOEM
RUN cmake -S /root/Tools/EtherCat/SOEM -B /root/Tools/EtherCat/SOEM/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DSOEM_BUILD_SAMPLES=OFF \
    && cmake --build /root/Tools/EtherCat/SOEM/build -j"$(nproc)"

# 入口: 自动 source ROS; 存在 /workspace/install 时一并 source
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["tail", "-f", "/dev/null"]

# ---------------------------------------------------------------------
# 目标 2: standalone — 独立部署镜像 (把工作区编进镜像)
# ---------------------------------------------------------------------
FROM dev AS standalone

WORKDIR /workspace
COPY hpm_gateway_ros2_ws/src/ /workspace/src/

# 先编 msgs 再编 control (control 依赖 hpm_gateway_msgs 与 SOEM)
RUN bash -lc "set -e; source /opt/ros/humble/setup.bash && \
    cd /workspace && \
    colcon build --symlink-install --packages-select hpm_gateway_msgs hpm_gateway_control"

ENV ROS_DOMAIN_ID=42
