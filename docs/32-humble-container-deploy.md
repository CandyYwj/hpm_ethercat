<!-- Author: BearKing -->
# ROS2 Humble 容器部署到另一台电脑

> 适用场景：把本机的 ROS2 Humble Docker 容器环境（`hpm_gateway_humble`）搬到另一台电脑。
> 更新日期：2026-08-19

## 1. 结论先说

本机的 `hpm_gateway_humble` 容器是**无状态**的：

- 镜像只是公开的 `osrf/ros:humble-desktop`，容器本身只跑 `tail -f /dev/null` 挂起；
- 代码与 colcon 编译产物全部在**宿主机挂载目录** `host/hpm_gateway_ros2_ws`（容器内 `/workspace`）；
- 容器配置（host 网络、`ROS_DOMAIN_ID=42`、X11、挂载）都在脚本 `scripts/ros2_humble.sh` 里。

因此"部署到另一台电脑"有三种做法：

- **方案 C（推荐）**：用仓库 `host/Dockerfile` 构建镜像，把**环境修复（CMake≥3.28）+ SOEM + 工作区编译**全部打进镜像，`docker save/load` 整体搬走，目标机不用再装依赖、也不用再编译。
- **方案 A**：装 Docker + 拷贝工作区/脚本 + 拉基础镜像，在目标机现场编译。
- **方案 B**：仅当曾在容器内部安装过额外软件（apt 包等）时，`docker commit` + 搬整个镜像。

```text
┌─ 源机（本机）─────────────────────────────┐
│  host/Dockerfile                          │
│    ├─ --target dev        开发镜像         │
│    └─ --target standalone 独立部署镜像     │
│        （含 SOEM + 编译好的工作区）         │
└──────────────────────────────────────────┘
        │ docker save / load（或目标机现场 build）
        ▼
┌─ 目标机 ─────────────────────────────────┐
│  装 Docker ──> docker load 镜像 ──>        │
│  docker run 直接用，无需挂载/无需编译      │
└──────────────────────────────────────────┘
```

---

## 2. 部署前检查：容器里到底改过没有

```bash
# 查看容器的可写层是否和镜像不一致（有 "C"/"A" 说明容器内改过/加过文件）
docker diff hpm_gateway_humble

# 只看镜像，是否和标准镜像一致（如果装了东西 hash 会不同）
docker images osrf/ros:humble-desktop
```

- `docker diff` 输出为空 → 走**方案 C**（Dockerfile，推荐）或**方案 A**。
- 输出有内容（比如 `/usr/local`、`/var/lib/apt` 有改动）→ 走**方案 B**，否则改动会丢。

---

## 3. 方案 A：拷贝工作区 + 拉镜像重建

### A.1 目标机安装 Docker

```bash
# Ubuntu 24.04
sudo apt update && sudo apt install -y docker.io docker-compose-v2
sudo usermod -aG docker "$USER"   # 重新登录后生效
```

### A.2 拷贝工作区与脚本

保持**相对目录结构一致**即可（脚本会相对自身位置向上定位到 `host/hpm_gateway_ros2_ws`）：

```bash
# 方式 1：scp 直接拷（在源机执行）
scp -r host/hpm_gateway_ros2_ws scripts/ros2_humble.sh \
  user@目标机IP:/path/to/hpm_gateway/

# 方式 2：源码走 git（推荐，便于版本管理）
#   在目标机 clone 仓库，再拷贝 build/install/log 或直接重编
```

拷贝后建议**删掉目标机上的 `build/ install/ log/` 重新编译**，因为它们是源机镜像生成的绑定，跨机复用可能不匹配：

```bash
cd host/hpm_gateway_ros2_ws
rm -rf build install log
```

### A.3 目标机拉取镜像

国内网络直连 Docker Hub 很慢，用镜像源拉完改回标准名：

```bash
docker pull docker.1ms.run/osrf/ros:humble-desktop
docker tag docker.1ms.run/osrf/ros:humble-desktop osrf/ros:humble-desktop
```

> 其它可用镜像源（实测可探测）：`docker.m.daocloud.io/<image>`、`dockerproxy.net/<image>`。

### A.4 编译并验证

```bash
cd <仓库根目录>
./scripts/ros2_humble.sh build hpm_gateway_msgs hpm_gateway_control
./scripts/ros2_humble.sh exec "ros2 pkg list | grep hpm_gateway"
```

脚本会自动完成：创建容器（host 网络）→ 挂载工作区 → 设置 `ROS_DOMAIN_ID=42` → source ROS → colcon 编译。

### A.5 日常使用

```bash
./scripts/ros2_humble.sh shell        # 进容器交互 shell
./scripts/ros2_humble.sh send --duration-sec 5   # 发电机参考指令
./scripts/ros2_humble.sh rqt          # 图形界面看话题（带 X11）
./scripts/ros2_humble.sh logs         # 容器日志
```

---

## 4. 方案 B：容器内安装过东西时，搬整个镜像

仅当你确认容器可写层有自定义内容（`docker diff` 有输出）时使用：

```bash
# 源机：提交容器为镜像并导出
docker commit hpm_gateway_humble hpm_gateway_humble:custom
docker save hpm_gateway_humble:custom | gzip > humble_custom.tar.gz

# 传到目标机后导入
docker load < humble_custom.tar.gz
docker tag hpm_gateway_humble:custom osrf/ros:humble-desktop
```

之后 `scripts/ros2_humble.sh` 里 `IMAGE="osrf/ros:humble-desktop"` 无需改动。
注意：`docker save` 镜像可能好几个 GB，传输走内网/移动硬盘更合适。

---

## 5. 方案 C（Dockerfile，推荐）：构建镜像整体搬走

仓库提供 `host/Dockerfile`（双构建目标），把环境修复、SOEM 编译、工作区编译全部固化到镜像里：

| 构建目标 | 镜像名 | 内容 | 适用 |
| --- | --- | --- | --- |
| `dev` | `hpm_gateway_humble:dev` | 基础环境 + CMake≥3.28 + 本地 SOEM 编译；工作区仍运行时挂载 | 日常开发（兼容 `ros2_humble.sh`） |
| `standalone` | `hpm_gateway_humble:standalone` | 在 dev 基础上把 `src/` + colcon 产物编进镜像，无需挂载 | 跨机部署 / 交付 |

> **为什么需要 Dockerfile 修环境**：SOEM 的 `CMakeLists.txt` 要求 `cmake_minimum_required(3.28)`，而 Humble 自带 3.22 编不过——这就是之前 `hpm_gateway_control` 编不了的原因。Dockerfile 用 `pip install "cmake>=3.28"` 解决；SOEM 直接用仓库内 `host/tools/SOEM` 源码编译（Linux 版不需要 libpcap），装到 `/root/Tools/EtherCat/SOEM`（`hpm_gateway_control` 默认查找路径）。

> **前置条件（重要）**：Dockerfile 用 `COPY tools/SOEM` 把仓库内的 SOEM 源码编进镜像。但 `host/tools/SOEM` 目前是**嵌套 git 仓库**（`OpenEtherCATsociety/SOEM`），未纳入主仓库版本管理——在干净 clone 的机器上构建镜像前需先准备好它：
>
> ```bash
> # 方式 1：从开发机拷贝（推荐，保证与联调版本一致）
> scp -r user@开发机:/path/to/hpm_gateway/host/tools/SOEM host/tools/
>
> # 方式 2：重新克隆
> git clone https://github.com/OpenEtherCATsociety/SOEM.git host/tools/SOEM
> ```

### C.1 源机构建

```bash
# 开发镜像（本机日常用）
./scripts/ros2_humble.sh image

# 独立部署镜像（搬去别的机器用，依赖 dev 镜像）
./scripts/ros2_humble.sh standalone
```

等价的原生 docker 命令（在仓库根目录）：

```bash
docker build -f host/Dockerfile --target dev        -t hpm_gateway_humble:dev        host/
docker build -f host/Dockerfile --target standalone -t hpm_gateway_humble:standalone host/
```

### C.2 搬运到目标机

```bash
# 源机导出（standalone 纯镜像即可用，不依赖宿主机工作区）
docker save hpm_gateway_humble:standalone | gzip > humble_standalone.tar.gz

# 传到目标机后导入
docker load < humble_standalone.tar.gz
```

目标机**不需要 Docker Hub**（镜像内已含全部依赖）；或者直接在目标机 `git clone` 仓库后
`./scripts/ros2_humble.sh standalone` 现场构建（国内网络需先配镜像源拉基础镜像，见 A.3）。

### C.3 目标机运行（standalone）

```bash
# 常驻容器（与开发流程一致的 host 网络 + ROS_DOMAIN_ID=42）
docker run -d --name hpm_gateway_humble --network host \
  -e ROS_DOMAIN_ID=42 hpm_gateway_humble:standalone

# 一次性命令（entrypoint 已自动 source ROS 与 /workspace/install）
docker run --rm --network host hpm_gateway_humble:standalone ros2 topic list

# 进容器（/workspace 已是编译好的工作区）
docker exec -it hpm_gateway_humble bash
```

### C.4 本机切换 dev 镜像

`ros2_humble.sh` 检测到本地有 `hpm_gateway_humble:dev` 时会自动优先使用。
老容器是官方镜像创建的，切换需重建一次：

```bash
docker rm -f hpm_gateway_humble
./scripts/ros2_humble.sh start     # 用新 dev 镜像重建容器
```

### C.5 日常开发（dev 镜像）

与原来完全一致，`build/shell/send/rqt` 照常用；现在容器内 CMake 是 3.28+、SOEM 已就位，可编 `hpm_gateway_control`：

```bash
./scripts/ros2_humble.sh build hpm_gateway_msgs hpm_gateway_control
```

---

## 6. 注意事项（踩坑记录）

| 项目 | 说明 |
|------|------|
| 容器无状态 | 容器可随时删除重建；别把代码放容器里，放挂载的工作区 |
| host 网络 | 容器直接用宿主机网卡，DDS 与 EtherCAT 共享本机网络栈；目标机无需额外端口映射 |
| `ROS_DOMAIN_ID=42` | 所有 ROS 命令都已注入，多机协同需保证同一 domain |
| X11/rqt | 脚本已挂载 `/tmp/.X11-unix` 与 Xauthority；目标机需有图形会话，否则 `rqt` 打不开 |
| 跨机发命令 | 电机命令必须在电机控制本机跑（localhost DDS）。从 PC 跨局域网发 100-200Hz 命令会因 DDS 突发丢帧 + 100ms 超时清零 PDO，导致电机抽搐/强转一下停。跨机发命令需把命令订阅 QoS 改 Reliable 并加大 `command_timeout_ms`（实测 2026-08-17） |
| `hpm_gateway_control` | 依赖 SOEM，SOEM 要求 CMake ≥ 3.28（Humble 自带 3.22 编不过）。用方案 C 的 dev 镜像后，容器内已是 CMake 3.28+ 且 SOEM 预编译在 `/root/Tools/EtherCat/SOEM`，可直接 `./scripts/ros2_humble.sh build hpm_gateway_msgs hpm_gateway_control` |

---

## 7. 目标机验证清单

- [ ] `docker info` 正常（无需 sudo）
- [ ] `docker images` 里有 `osrf/ros:humble-desktop` 或已构建的 `hpm_gateway_humble:dev`
- [ ] （方案 C）`./scripts/ros2_humble.sh image` 构建 dev 镜像成功
- [ ] （方案 C）`./scripts/ros2_humble.sh standalone` 构建 standalone 镜像成功
- [ ] `./scripts/ros2_humble.sh start` 成功创建并启动容器
- [ ] `./scripts/ros2_humble.sh exec "ros2 --version"` 输出 Humble
- [ ] `./scripts/ros2_humble.sh build hpm_gateway_msgs` 编译通过
- [ ] （dev 镜像）`./scripts/ros2_humble.sh build hpm_gateway_msgs hpm_gateway_control` 编译通过
- [ ] `ros2 topic list` 能看到 `hpm_gateway` 相关话题
- [ ] （图形会话）`./scripts/ros2_humble.sh rqt` 能打开界面
- [ ] （联硬件）`./scripts/ros2_humble.sh send --duration-sec 5` 电机正常响应
