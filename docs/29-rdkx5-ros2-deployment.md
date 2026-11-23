# 在 RDK X5（已有 ROS2）上部署可用的电机控制环境

本文面向一台**已经装有 TROS / ROS2 Humble 的地瓜派 RDK X5**，给出把它变成
“可用联调环境”的完整步骤：网络打通、源码部署、SOEM 安装、ROS2 工作区编译、
实时配置、启动节点、发命令验证。读完本文即可在 RDK 上真正驱动 HPM 网关和电机。

> RDK 的 ROS2 是 TROS（地瓜派系统），环境脚本是 `/opt/tros/humble/setup.bash`，
> **不是** `/opt/ros/humble/setup.bash`。本文所有 `source` 都以 TROS 路径为准。

## 1. 目标与拓扑

```text
个人 PC (Ubuntu 24.04)                       RDK X5 (TROS / ROS2 Humble)
├─ eno1  192.168.108.156/22 (DHCP)  ──网线──  eth0  192.168.108.118/24 (管理口)
└─ Docker: hpm_gateway_humble (ROS2 容器)     ├─ ROS2 工作区 /root/hpm_gateway_ros2_ws
                                              ├─ SOEM   /root/Tools/EtherCat/SOEM
                                              └─ EtherCAT 口 (raw socket) ── HPM 网关 ── CAN0/CAN1 ── 电机
```

- 管理网：PC ⇄ RDK 走 `192.168.108.0/24` 同网段，用于 SSH / 部署 / ROS2 DDS。
- 数据面：RDK 另一块网口以 raw socket 跑 SOEM 主站，**不配 IP**，直连 HPM 网关。
- 跨机 DDS：PC 容器与 RDK 都设 `export ROS_DOMAIN_ID=42` 即可互通。

## 2. 前置条件

在 RDK 上确认（SSH 或串口登录，用户 `root`）：

```bash
# 1. TROS 存在
ls /opt/tros/humble/setup.bash

# 2. 网卡清单（找哪块是管理口、哪块是 EtherCAT 口）
ip -br link
nmcli device status

# 3. 编译工具链可用（colcon、cmake、g++）
source /opt/tros/humble/setup.bash
which colcon && colcon version-check
cmake --version | head -1
```

> **colcon 缺失**：TROS 的 `setup.bash` 不保证自带 colcon。若 `which colcon` 无输出
> （实测 RDK 上即缺失），先装：
>
> ```bash
> # 确保 pip 存在
> python3 -m pip --version || { sudo apt update && sudo apt install -y python3-pip; }
> # 安装 colcon（实测 RDK：Ubuntu 22.04 / pip 22.0.2，无 PEP 668，直接装即可）
> python3 -m pip install colcon-common-extensions
> which colcon && colcon version-check   # 验证
> ```
>
> 若报 `no such option: --break-system-packages`，说明是旧 pip（<23），**去掉该参数**直接装。
> 只有 Ubuntu 24.04 / pip≥23 的 PEP 668 限制才需要 `--break-system-packages`。
>
> 若装完仍找不到，多半是 Python bin 目录不在 PATH，`which colcon || ls -l /usr/local/bin/colcon ~/.local/bin/colcon 2>/dev/null` 定位后补 PATH。
>
> **CMake 版本**：RDK 自带 3.22.1，SOEM master 需要 ≥ 3.28（`CMake 3.28 or higher is required`）。先不用急，第 5.3 节会统一升级；这里只需确认存在即可。

## 3. 网络打通（管理口）

RDK 管理口 IP 以实测为准（本文实机为 `192.168.108.118/24`）。详见
`docs/26-rdk-usb-ethernet-link.md` 与 `docs/27-rdk-usb-eth-to-pc.md`，要点：

```bash
# PC 端验证（应能 ping 通并 SSH）
ping -c 4 192.168.108.118
ssh root@192.168.108.118
```

> 注意：仓库部署脚本默认 RDK IP 是 `192.168.108.178`，本文实机是
> `192.168.108.118`，用脚本时记得加 `-RdkIp 192.168.108.118`。

> 关键：EtherCAT 口**不要配管理 IP**，否则与 SOEM raw socket 冲突。
> 网线两端 `carrier=1` 只代表链路 UP，**网段对齐才是连通前提**。

## 4. 部署源码到 RDK

源码分两部分，都在本仓库的 `scripts/` 下用 PowerShell 脚本同步（只同步源码，
不含可重生成的 `build/install/log`）。

### 4.1 部署 host 代码（SOEM 周期主站 + 辅助）

```powershell
# 仓库根目录，Windows PowerShell
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_host_to_rdk.ps1 -RdkIp 192.168.108.118
# 目标: root@192.168.108.118:/root/hpm_gateway/host
```

### 4.2 部署 ROS2 工作区源码

```powershell
# 首次：先拉取 RDK 当前源码（避免覆盖）
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1 -RdkIp 192.168.108.118

# 本地修改确认后：推送到 RDK
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 -RdkIp 192.168.108.118
# 目标: root@192.168.108.118:/root/hpm_gateway_ros2_ws/src
```

> 两脚本都是可回退的：旧 `src`/`host` 会重命名保留，不会删除。
> 也可以手动 scp（先建目录，否则报 `path canonicalization failed`）：

```bash
ssh root@192.168.108.118 'mkdir -p /root/hpm_gateway_ros2_ws'
scp -r host/hpm_gateway_ros2_ws/src root@192.168.108.118:/root/hpm_gateway_ros2_ws/
```

### 4.3 部署常见问题（实测）

**a. `WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!` / host key verification failed**

RDK 重装系统或重新烧录后主机密钥变化，本机 `known_hosts` 里还是旧密钥，OpenSSH
出于防中间人攻击会拒绝连接。删除旧记录后重试即可：

```powershell
# 本机 PowerShell
ssh-keygen -R 192.168.108.118
# 重跑部署脚本，首次会提示接受新密钥（本机 known_hosts 会自动更新）
```

> 出现该提示前请先确认连接对象确是你的 RDK，避免误连被中间人替换的主机。

**b. `scp: ...: No such file or directory`（归档上传到不存在的目录）**

部署脚本把归档写到远端 `$RemoteRoot`（默认 `/root/hpm_gateway`）下，但该目录
**不会自动创建**。RDK 全新/重刷后目录为空，scp 就会报错。先手动建目录：

```powershell
# 本机 PowerShell
ssh root@192.168.108.118 "mkdir -p /root/hpm_gateway"
# 再重跑 deploy_host_to_rdk.ps1
```

> 同理，推送 ROS2 工作区前确保 `/root/hpm_gateway_ros2_ws` 存在；手动 scp 也要先
> `mkdir -p` 目标父目录，否则报 `path canonicalization failed`。

## 5. 安装 / 验证 SOEM

`hpm_gateway_control`（EtherCAT 主站节点）依赖 SOEM 库。RDK 默认路径为
`/root/Tools/EtherCat/SOEM`，包会在该目录、`/usr/local`、常见 build 输出目录中搜索。

### 5.1 确认现有 SOEM

```bash
find /root/Tools/EtherCat/SOEM /usr/local \( -name 'libsoem.so*' -o -name 'libsoem.a' \) -print
ls /usr/local/include/soem/soem.h 2>/dev/null
```

### 5.2 没有则本地克隆并上传（本文采用的方式）

SOEM 源码已克隆在本仓库 `host/tools/SOEM`。本地克隆（如已存在可跳过）：

```bash
cd host/tools
git clone --depth 1 https://github.com/OpenEtherCATsociety/SOEM.git
```

上传到 RDK（在本地仓库根目录执行，会提示输入 root 密码）：

```bash
ssh root@192.168.108.118 'mkdir -p /root/Tools/EtherCat'
scp -r host/tools/SOEM root@192.168.108.118:/root/Tools/EtherCat/
# 最终落在 RDK: /root/Tools/EtherCat/SOEM
```

> 注意：先 `mkdir -p /root/Tools/EtherCat`，否则 scp 会报
> `path canonicalization failed`（目标父目录不存在）。

### 5.3 升级 CMake（SOEM master 要求 ≥ 3.28）

RDK 自带 CMake 3.22.1，SOEM master 会报
`CMake 3.28 or higher is required`。两种解法（本文选用 B）：

**A. PC 下载官方 aarch64 预编译包并 scp（离线可用）**

```bash
# PC（有网）
wget https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-linux-aarch64.tar.gz
scp cmake-3.30.5-linux-aarch64.tar.gz root@192.168.108.118:/root/

# RDK
cd /opt
sudo tar xzf /root/cmake-3.30.5-linux-aarch64.tar.gz
sudo ln -sf /opt/cmake-3.30.5-linux-aarch64/bin/cmake  /usr/local/bin/cmake
sudo ln -sf /opt/cmake-3.30.5-linux-aarch64/bin/ctest  /usr/local/bin/ctest
sudo ln -sf /opt/cmake-3.30.5-linux-aarch64/bin/cpack  /usr/local/bin/cpack
cmake --version                       # 应为 3.30.x
```

**B. pip 安装（已选用，RDK 有网时最省事）**

```bash
# 确保 pip 存在
python3 -m pip --version || { sudo apt update && sudo apt install -y python3-pip; }

# 安装新版 cmake（root 装到系统）
# 实测 RDK：Ubuntu 22.04 / pip 22.0.2，无 PEP 668 限制，直接装；
# 仅 Ubuntu 24.04 / pip>=23 才需追加 --break-system-packages
python3 -m pip install cmake

# 若不想动系统包，退回 --user 方式：
# python3 -m pip install --user cmake
# ln -sf "$HOME/.local/bin/cmake" /usr/local/bin/cmake

# 确认新版本生效（/usr/local/bin 在 PATH 中先于 /usr/bin）
hash -r
cmake --version                        # 应为 3.28+
```

### 5.4 在 RDK 上编译并安装

```bash
cd /root/Tools/EtherCat/SOEM
rm -rf build && mkdir build && cd build   # 清掉上次 CMake 3.22 失败留下的缓存
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DSOEM_BUILD_SAMPLES=OFF
make -j$(nproc)
sudo make install                     # 装到 /usr/local/include/soem + /usr/local/lib

# 关键头文件应齐全（ec_options.h 是 configure 时生成的，nicdrv.h 是 Linux 平台头）
ls /usr/local/include/soem/ec_options.h /usr/local/include/soem/nicdrv.h
ls /usr/local/lib/libsoem.a
```

> **务必加 `-DSOEM_BUILD_SAMPLES=OFF`**：SOEM master 顶层 CMake 引用了从未定义的
> `SOEM_SOURCE_DIR`，构建 samples（`eni_test`）时会报
> `Could not find ENICONV using the following names: eniconv.py`。本项目只需要
> `libsoem.a` 和头文件，不需要 samples，关掉即可绕过该坑。

> 必须显式 `-DCMAKE_INSTALL_PREFIX=/usr/local`：SOEM 默认装到 `build/install`，
> ROS2 包的默认查找路径会漏掉它。
> 若漏掉这步，`colcon build` 会因找不到生成的 `soem/ec_options.h` 报
> `fatal error: soem/ec_options.h`。

装完后 `hpm_gateway_control` 的 `find_path`/`find_library` 会在默认路径
`/root/Tools/EtherCat/SOEM` 与 `/usr/local` 自动找到；若装在别处，编译前
`export SOEM_DIR=/path/to/SOEM` 即可。

## 6. 编译 ROS2 工作区

```bash
cd ~/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
export SOEM_DIR=/usr/local          # 用第 5 节安装后的头文件/库，避开不完整的源码 include
rm -rf build/hpm_gateway_control install/hpm_gateway_control   # 清掉失败留下的 CMake 缓存
colcon build --symlink-install
source install/setup.bash
```

- `--symlink-install` 让 `scripts/` 和 `config/` 的修改即时生效，联调方便。
- `export SOEM_DIR=/usr/local` 让包用安装后的头文件（含生成的 `ec_options.h`、
  平台 `nicdrv.h`）；不设置时会指向 `/root/Tools/EtherCat/SOEM/include`
  （源码目录，缺生成头），报 `fatal error: soem/ec_options.h`。
- 改过 `SOEM_DIR` 或换过 SOEM 后，要删 `build/hpm_gateway_control`、
  `install/hpm_gateway_control` 再重编，否则旧路径仍缓存在 CMake 里。
- 若报 `SOEM was not found`：回第 5 节确认库已装。

## 7. 实时配置（可选但推荐）

EtherCAT 1 kHz 周期的实时性依赖“NIC 中断”与“EtherCAT 工作线程”落在同一核。
仓库提供开机自启脚本完成中断绑定：

```bash
# 本机 PowerShell：拷到 RDK
scp scripts/rdk_rt_setup.sh scripts/rdk_rt_setup.service root@192.168.108.118:/root/

# RDK 上：安装并启用
install -m 0755 /root/rdk_rt_setup.sh /usr/local/bin/rdk_rt_setup.sh
install -m 0644 /root/rdk_rt_setup.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now rdk_rt_setup.service

# 验证（默认绑定核 3，接口 eth0）
systemctl status rdk_rt_setup.service
cat /proc/interrupts | grep eth0
```

> 注意：脚本默认绑定 `eth0`。**先确认哪块网口是 EtherCAT 口**，若不是 eth0，
> 改 service 里的 `IFACE=eth0`（及 `hpm_gateway_control_eth0.yaml` 的 `interface_name`）。

若同时要求硬实时，把 `config/hpm_gateway_control_eth0.yaml` 的
`rt_priority` 与 `rt_cpu` 配成与脚本一致（例如 `rt_priority: 80`、`rt_cpu: 3`）。

### 7.1 换一块新板子时，怎么确定该绑哪个核

绑核的原则：**网卡 IRQ 和 EtherCAT 工作线程必须落在同一个、且尽量“干净”的核**。
“干净”= 负载低、不是核 0、没有 SMT 兄弟核抢资源。按以下步骤确定核号 `N`：

1. **确认 EtherCAT 网口名**（不是管理口）：
   ```bash
   ip -br link           # 找直连网关、没有管理 IP 的那块口，记下名字（如 eth0 / enp4s0）
   nmcli device status
   ```

2. **查看 CPU 拓扑与核数**：
   ```bash
   lscpu                 # CPU(s)=核数；若 Thread(s) per core > 1 说明开了 SMT（超线程）
   nproc
   ```
   > 若开了 SMT：同一物理核的两个逻辑核（如 0/1、2/3…）共享执行单元，**不要**用它们的兄弟核，
   > 避免 RT 线程和兄弟核上的负载互相拖慢。

3. **观察各核当前负载，选最空闲的核**：
   ```bash
   mpstat -P ALL 1       # 需要 sysstat；每秒打印每核 %idle，选最闲的核
   # 或
   top   # 然后按数字键 1 看每核 %us / %idle
   ```

4. **避开核 0**：内核 tick、定时器、多数外设中断默认都压到核 0，不干净。

5. **定下核号 N 后，在两处保持完全一致**：
   - `config/hpm_gateway_control_eth0.yaml`：
     ```yaml
     rt_priority: 80
     rt_cpu: 3               # ← 改成你选的 N
     ```
   - `rdk_rt_setup.service`：
     ```ini
     Environment=CORE=3        # ← 改成 N
     Environment=IFACE=eth0    # ← 改成第 1 步确认的 EtherCAT 口名
     ```
   > 两者不一致 = 中断在 A 核、工作线程在 B 核，跨核唤醒，绑核等于白做。

6. **改完重载并验证**：
   ```bash
   systemctl daemon-reload
   systemctl restart rdk_rt_setup.service hpm_gateway_node.service
   cat /proc/interrupts | grep eth0      # 目标核那一列计数持续增长、其他核基本不变
   ps -eLo pid,psr,comm | grep hpm_gateway   # 工作线程的 PSR 应显示核 N
   # 再看节点日志的 deadline_misses / max_lateness_us 是否稳定
   ```

一句话：**核号 N = 非 0、低负载、避开 SMT 兄弟核的那个核，然后让 `rt_cpu` 和 `CORE` 两处保持一致。**

### 7.2 插上 EtherCAT 网线就断 SSH（实测第二台机器 wzsu）

**现象**：管理口走 Wi-Fi（`wlo1`，192.168.111.70/22），EtherCAT 口是 `enp2s0`。
只要把 EtherCAT 网线插进 `enp2s0`，SSH 立刻断；拔线即恢复。

**原因**：`enp2s0` 起来后被 NetworkManager/netplan 自动配置上了 IP/路由（DHCP 或
遗留静态配置），抢走回包路径 → **非对称路由**（SSH 应答从 EtherCAT 口出去），
表现为"连不上/卡住"。EtherCAT 主站（SOEM）用 raw socket，只需要链路 UP，
**不需要 IP、更不能有默认路由**——这也正是第 3 节"EtherCAT 口不要配管理 IP"的由来。

**解决：把 EtherCAT 口设为"只亮不配"（link-only / unmanaged），不给 IP、不给路由。**

（a）NetworkManager（wzsu 的 Wi-Fi 即 NM 管理，推荐）——标记不受管：

```bash
# 立即生效
sudo nmcli device set enp2s0 managed no

# 永久生效（重启后仍不受管）
sudo tee /etc/NetworkManager/conf.d/99-ethercat-unmanaged.conf >/dev/null <<'EOF'
[keyfile]
unmanaged-devices=interface-name:enp2s0
EOF
sudo systemctl reload NetworkManager
```

（b）netplan（若用 networkd 渲染）——只留链路、不给 IP，在 `/etc/netplan/*.yaml` 追加：

```yaml
network:
  ethernets:
    enp2s0:
      dhcp4: false
      addresses: []
  version: 2
```

```bash
sudo netplan apply
```

**验证**：

```bash
ip -br addr show enp2s0    # 应无 IPv4 地址
# 插上 EtherCAT 网线后，SSH 不再断；SOEM 可在该无 IP 口上跑 raw socket
```

**配套脚本**：`scripts/ethercat_port_linkonly.sh [接口名]`（默认 `enp2s0`），
一键应用上面的 (a) 方案并做验证（需 root 执行）。

## 8. 启动节点

### 8.1 安全模式（不开 EtherCAT，只测 ROS2 通信）

```bash
export ROS_DOMAIN_ID=42
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
ros2 launch hpm_gateway_control hpm_gateway_control.launch.py
```

### 8.2 EtherCAT 模式（真正驱动电机，需 root 的 raw socket）

```bash
sudo -E bash -lc '
export ROS_DOMAIN_ID=42
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
ros2 launch hpm_gateway_control hpm_gateway_control_eth0.launch.py
'
```

EtherCAT 参数在 `config/hpm_gateway_control_eth0.yaml`：

| 参数 | 值 | 说明 |
| --- | ---: | --- |
| `start_ethercat` | `true` | 打开 EtherCAT 主站 |
| `arm_all_axes` | `true` | 首帧命令后使能全部已配置轴（当前固件行为） |
| `interface_name` | `eth0` | EtherCAT 网口名（以 `ip -br link` 实测为准） |
| `cycle_us` | `1000` | 1 kHz PDO 周期 |
| `command_timeout_ms` | `100` | 命令超时，清除命令快照（通信兜底，非急停） |

### 8.3 开机自启 ROS 节点（systemd 服务）

需要 RDK 上电后自动启动 EtherCAT 主站节点时，用仓库提供的
`scripts/hpm_gateway_node.sh` + `scripts/hpm_gateway_node.service`。

服务做的事：以 root 运行（raw socket 需要）→ source TROS → source 工作区
`install/setup.bash` → 设 `ROS_DOMAIN_ID=42` →
`ros2 launch hpm_gateway_control hpm_gateway_control_eth0.launch.py`，并
**在 `rdk_rt_setup.service`（IRQ 绑核）之后启动**。

#### 安装（在 RDK 上执行，root）

```bash
# 1. 拷到 RDK（本机 PowerShell）
scp scripts/hpm_gateway_node.sh scripts/hpm_gateway_node.service root@192.168.108.118:/root/

# 2. 安装并启用
install -m 0755 /root/hpm_gateway_node.sh /usr/local/bin/hpm_gateway_node.sh
install -m 0644 /root/hpm_gateway_node.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now hpm_gateway_node.service

# 3. 验证 / 看日志
systemctl status hpm_gateway_node.service
journalctl -u hpm_gateway_node.service -f
```

#### 管理命令

```bash
systemctl restart hpm_gateway_node.service        # 重启节点
systemctl stop hpm_gateway_node.service           # 停止（断开 EtherCAT）
systemctl disable --now hpm_gateway_node.service  # 取消开机自启
```

#### 切换启动模式

默认启动 EtherCAT 模式（驱动电机）。改成只跑 ROS 通信的安全模式，覆盖
`HPM_GATEWAY_LAUNCH` 环境变量（或编辑 `/usr/local/bin/hpm_gateway_node.sh`）：

```bash
HPM_GATEWAY_LAUNCH=hpm_gateway_control.launch.py /usr/local/bin/hpm_gateway_node.sh
```

#### 注意（务必阅读）

- **电机使能时机**：节点只在收到**第一条命令帧**后才使能全部已配置轴
  （`arm_all_axes: true` + 固件 auto-enable）。开机后电机不会自转；但服务常驻
  意味着**任何时刻发布的命令都会被电机执行**，安全纪律不变。
- **接口名**：EtherCAT 网口名以 `ip -br link` 实测为准，改
  `config/hpm_gateway_control_eth0.yaml` 的 `interface_name`（默认 `eth0`）；
  绑错口会导致 raw socket 失败、服务反复重启。
- **工作区必须已编译**：`/root/hpm_gateway_ros2_ws/install/setup.bash` 不存在时
  启动脚本直接退出（journal 里有 error），服务每 5 s 重试。先按第 6 节
  `colcon build` 一次。
- **依赖 RT 服务**：服务声明了 `After=rdk_rt_setup.service`，请先按第 7 节装好，
  否则中断未绑核、实时性不保证。
- 停止/重启节点会断开 EtherCAT 周期 → 固件 100 ms 命令超时后电机走协议停机/刹车。

## 9. 发送命令与验证

### 9.1 在 RDK 上发命令

```bash
export ROS_DOMAIN_ID=42
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash

# 速度模式：电机 1 以 0.5 rad/s 转 5 秒
ros2 run hpm_gateway_control send_reference_motor_commands.py \
  --motor-ids 1 --mode velocity --velocity-rad-s 0.5 --kd 2 --duration-sec 5

# 位置模式：电机 1 转到 0.3 rad 并保持 5 秒
ros2 run hpm_gateway_control send_reference_motor_commands.py \
  --motor-ids 1 --mode position --position-rad 0.3 --kp 20 --kd 5 --duration-sec 5
```

参数与 PC 容器 `send` 完全一致，详见 `docs/28-ros2-container-motor-control.md`
第 3 节（`--motor-ids`、`--mode`、`--position-rad`、`--velocity-rad-s`、`--kp`、
`--kd`、`--rate-hz`、`--duration-sec`、`--topic`）。

> **调参提示（ENCOS）**：MIT 帧 KD 上限为 5.0。速度模式 `--kd` 太低（<2）时速度环
> 易震荡、可能触发过流/超速保护跳闸；位置模式可从 `--kp 5~20 --kd 5` 起步，
> 第一次务必用小角度（`--position-rad 0.1~0.3`）确认转向与方向。`--duration-sec`
> 结束后命令停止、电机松开（不再保持）。

### 9.2 查看话题与反馈

```bash
ros2 topic list
ros2 topic hz /hpm_gateway/motor_feedback
ros2 topic echo /hpm_gateway/motor_feedback
ros2 topic echo /imu/data
```

### 9.3 跨机互通（PC 容器发、RDK 跑主站）

PC 容器与 RDK 都 `export ROS_DOMAIN_ID=42` 且网络可达时，DDS 互通。典型用法：

- **RDK 上跑 EtherCAT 节点**，**PC 容器里 `./ros2_humble.sh send ...` 发命令**；
- 或 RDK 发命令、PC 容器 `./ros2_humble.sh exec ros2 topic echo ...` 看反馈。

> ⚠️ **跨局域网发命令务必降频（实测结论）**：从 PC 跨网以 100~200 Hz 发命令时，
> DDS 是突发式到达，叠加节点 `KeepLast(1).best_effort` 订阅丢帧和 `run_cyclic`
> 的 100 ms 命令超时清零 PDO，会造成命令断续，电机表现为抽搐/发疯/强转一下停。
> **控制命令流应尽量在 RDK 本机发**；确需跨机时给 `send` 加 `--rate-hz 20`
> （50 ms 间隔 < 100 ms 超时，可稳定工作）。

### 9.4 不经过 ROS2 的底层主站（更贴近联调）

```bash
cd /root/hpm_gateway/host/rdk_ecat_motor_rt && make
sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --rx-timeout-us 500 --print-feedback
```

> 接口名以 `ip -br link` 实测为准（本文示例 `enp4s0` 与 ROS2 配置的 `eth0`
> 对应同一块 EtherCAT 口，不同 RDK 镜像命名可能不同，务必先核对）。
> `--print-feedback` 每秒打印 IMU 与 16 轴反馈，可快速判断**哪些电机在线**
> （反馈非零且在变）以及 EtherCAT 周期健康度（`cycles/s`、`wkc_errors`、
> `max_exchange_us`）。注意该主站只发零命令，**不会让电机转动**，仅用于
> 链路与在线排查。

### 9.5 单电机控制与命令接受逻辑（参考）

节点 `hpm_gateway_control_node` 订阅**两个**命令话题：

| 话题 | 类型 | 语义 |
| --- | --- | --- |
| `/hpm_gateway/motor_command` | `MotorMitCommand` | 单轴独占：只驱动该轴，其余 15 轴清零 |
| `/hpm_gateway/motor_commands` | `MotorMitCommandArray` | 16 轴整体替换：整帧覆盖，未选轴清零 |

`MotorMitCommand` 字段：`motor_id`（1..16）、`position_rad`、`velocity_rad_s`、
`kp`、`kd`、`torque_nm`（`stamp` 可忽略）。

控制单个电机（**必须持续发布**，命令有 100 ms 超时）：

```bash
# 位置模式：电机 1 保持 0.5 rad
ros2 topic pub --rate 100 /hpm_gateway/motor_command \
  hpm_gateway_msgs/msg/MotorMitCommand \
  "{motor_id: 1, position_rad: 0.5, velocity_rad_s: 0.0, kp: 5.0, kd: 1.0, torque_nm: 0.0}"

# 速度模式：电机 1 以 0.5 rad/s 转（MIT 约定 kp=0，kd 当速度增益）
ros2 topic pub --rate 100 /hpm_gateway/motor_command \
  hpm_gateway_msgs/msg/MotorMitCommand \
  "{motor_id: 1, position_rad: 0.0, velocity_rad_s: 0.5, kp: 0.0, kd: 2.0, torque_nm: 0.0}"
```

端到端接受链路：

```text
ROS2 话题 (motor_command / motor_commands)
  -> hpm_gateway_control_node (handle_single / handle_aggregate)
  -> LatestBuffer 单槽 command_frame (16 轴)
  -> EtherCAT 线程 1 kHz run_cyclic
  -> 320 B RxPDO (16×5 REAL32)
  -> 固件 APPL_OutputMapping (每周期复制 + sequence++)
  -> motor_manager 1 ms poll (100 ms 超时窗口)
  -> CAN 运动帧 (ENCOS / RS06 MIT)
```

逐级逻辑：

1. **ROS2 节点**：单轴话题校验 `motor_id` 在 1..16 且数值有限，**先清空整帧
   再只填该轴**（其余 15 轴清零），`valid_mask` 只置对应位、`sequence++`；
   聚合话题整帧重填（16 轴先清零再按 `msg.commands[i]` 覆盖）。两个话题共用
   同一互斥回调组，避免并发写 `command_frame_` 产生数据竞争。
2. **EtherCAT 线程**：每 1 ms 从单槽 `LatestBuffer` 取最新帧；若从未收到命令或距
   上次收到命令超过 `command_timeout_ms`（100 ms），输出 PDO 清零（安全态）。
3. **固件**：`APPL_OutputMapping` 把 16×5 个 `REAL32` 拷进对象字典并
   `sync_command_cache()`，随后 `s_motor_command_sequence++`。
4. **motor_manager**：发现 sequence 变化即刷新 `last_command_ms`、标记已收到命令；
   每个 1 ms 周期内若「收到过命令且 `now - last_command_ms ≤ 100 ms`」→
   `send_motion_controls()`，否则 `stop_all()`（发协议 stop/brake）。只驱动
   enabled 且无故障的轴，首帧自动 enable。

约束（务必注意）：

- 当前只有 8 个 enabled 轴：CAN0 = ENCOS ID 1/2/3 + RS06 MIT ID 13；
  CAN1 = ENCOS ID 4/5/6 + RS06 MIT ID 14；其余轴 disabled（发命令也不驱动）。
- PDO 轴 n ↔ 全局 CAN 电机 ID n+1；`motor_id=13` 才到 RS06 MIT。
- 没有逐轴 armed 位进 PDO：固件只认「命令序列是否在 100 ms 内刷新」，主机侧还有
  一层 100 ms 超时清零 PDO。停止发布 ~100 ms 后轴会被协议停机/刹车。
- 单轴话题是**单轴独占**（只驱动该轴，其余轴清零）、聚合话题是**整体替换**；
  两者混用时后到者覆盖先到者，注意单轴命令会隐式停掉其他所有轴。

## 10. 常用调试命令速查

```bash
# 环境
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=42

# 节点 / 话题 / 频率
ros2 node list
ros2 topic list
ros2 topic type /hpm_gateway/motor_feedback
ros2 topic hz /hpm_gateway/motor_feedback

# 实时性观测（主站进程输出每秒速率 + WKC 失败 / deadline miss / 最大交换时间）
# 直接看启动节点的 stdout
```

## 11. 已知限制（务必阅读）

- **`hpm_gateway_control` C++ 节点必须在 RDK 上编译运行**：它依赖 SOEM，PC 容器内的
  CMake（3.22）不兼容，容器里只有发命令的 Python 脚本可用（不碰 EtherCAT）。
- **SOEM master 要求 CMake ≥ 3.28**：RDK 自带 3.22.1，直接 `cmake` 会报
  `CMake 3.28 or higher is required`，先按第 5.3 节升级 CMake。
- 发命令脚本默认全 16 轴有效——**请用 `--motor-ids` 限定到实际接线的轴**，
  避免向未接线轴发意外指令。
- `command_timeout_ms` 只是通信兜底，不是独立物理急停；请保证机械限位与人员安全。
- EtherCAT 口不要配管理 IP，否则与 SOEM raw socket 冲突。

## 12. 相关文件

```text
scripts/deploy_host_to_rdk.ps1                    部署 host 目录到 RDK
scripts/deploy_rdk_ros2_workspace.ps1             推送 ROS2 工作区 src 到 RDK
scripts/pull_rdk_ros2_workspace.ps1               拉取 RDK 现有 ROS2 src
scripts/rdk_rt_setup.sh / rdk_rt_setup.service    NIC 中断绑核 + 开机自启
scripts/hpm_gateway_node.sh / hpm_gateway_node.service  RDK 开机自启 ROS 节点（systemd）
scripts/ros2_humble.sh                            PC 侧 ROS2 容器工具
host/hpm_gateway_ros2_ws/src/hpm_gateway_control/ RDK 上的 EtherCAT 主站节点
host/hpm_gateway_ros2_ws/src/hpm_gateway_msgs/    消息定义
host/rdk_ecat_motor_rt/                           底层 SOEM 周期主站
host/tools/SOEM/                                  SOEM 源码（本地克隆，scp 到 RDK）
docs/26-rdk-usb-ethernet-link.md                  PC 侧 USB 网口直连 RDK
docs/27-rdk-usb-eth-to-pc.md                      RDK 侧 USB 网口直连 PC
docs/28-ros2-container-motor-control.md           容器发命令完整命令手册
```
