# 使用 ROS2 容器控制电机（完整命令手册）

本文说明如何通过 ROS 2 Humble Docker 容器（`hpm_gateway_humble`）发布电机
控制命令、查看话题，以及当前所有已支持的命令和参数。

## 1. 环境与容器

| 项 | 值 |
| --- | --- |
| 容器名 | `hpm_gateway_humble` |
| 镜像 | `osrf/ros:humble-desktop` |
| 网络 | `--network host`（DDS 直接使用主机网卡） |
| 挂载 | 主机 `host/hpm_gateway_ros2_ws` → 容器 `/workspace` |
| ROS_DOMAIN_ID | `42` |
| 入口脚本 | `scripts/ros2_humble.sh` |

容器只依赖 `rclpy` + `hpm_gateway_msgs`，**不需要 SOEM** 就能发电机命令
（`hpm_gateway_control` 的 C++ EtherCAT 节点依赖 SOEM，容器内暂未编译，见第 7 节）。

## 2. 容器工具脚本（`scripts/ros2_humble.sh`）全部子命令

在仓库根目录运行 `./scripts/ros2_humble.sh <子命令>`：

| 子命令 | 作用 | 示例 |
| --- | --- | --- |
| `build [包名...]` | 在容器内 colcon 编译；无参数默认只编译 `hpm_gateway_msgs` | `./ros2_humble.sh build` |
| `build <pkg1> <pkg2>` | 编译指定多个包 | `./ros2_humble.sh build hpm_gateway_msgs hpm_gateway_control` |
| `shell` | 进入容器交互式 bash（已 source ROS + 工作区） | `./ros2_humble.sh shell` |
| `exec <命令...>` | 在容器内执行任意命令（已 source ROS） | `./ros2_humble.sh exec ros2 topic list` |
| `send [参数...]` | 运行电机命令发送脚本（见第 3 节） | `./ros2_humble.sh send --motor-ids 1,2 --mode velocity` |
| `rqt` | 打开 rqt 图形界面（X11 转发，一次性容器） | `./ros2_humble.sh rqt` |
| `logs` | 跟踪容器日志 | `./ros2_humble.sh logs` |
| `start` | 启动容器（不存在则先创建并挂载工作区） | `./ros2_humble.sh start` |
| `stop` | 停止容器 | `./ros2_humble.sh stop` |

> 大部分命令会自动 `ensure_container`（不存在则创建、未运行则启动），
> 所以通常直接跑 `send` / `exec` 即可，不用先手动 `start`。

## 3. 发送电机命令（`send`）— 所有参数

`./ros2_humble.sh send` 实际运行
`src/hpm_gateway_control/scripts/send_reference_motor_commands.py`。
它发布 16 轴 `MotorMitCommandArray`，默认发到 `/hpm_gateway/motor_commands` 话题。

### 3.1 全部参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `--topic` | `/hpm_gateway/motor_commands` | 发布到的 `MotorMitCommandArray` 话题 |
| `--rate-hz` | `200.0` | 命令发布频率（Hz），必须 > 0 |
| `--duration-sec` | `2.0` | 有效命令持续时间（秒），必须 >= 0 |
| `--motor-ids` | `1,2,3,...,16`（全部） | 要控制的电机 ID（1..16，逗号分隔） |
| `--mode` | `position` | 控制模式：`position` 或 `velocity` |
| `--position-rad` | `0.0` | 位置模式目标位置（弧度） |
| `--velocity-rad-s` | `0.3` | 速度模式目标速度（rad/s，负数反转） |
| `--kp` | `1.0` | 位置刚度（Nm/rad），必须 >= 0 |
| `--kd` | `1.0` | 阻尼（Nm/(rad/s)），必须 >= 0 |

### 3.2 两种控制模式

- **position（位置模式）**：对选中的电机设置 `position_rad` + `kp` + `kd`，
  主动保持到目标位置。
  - 例如让电机 1 保持 1.0 rad：`--motor-ids 1 --mode position --position-rad 1.0 --kp 5 --kd 1`
- **velocity（速度模式）**：对选中的电机设置 `velocity_rad_s` + `kd`（kd 作为速度增益），kp 置 0。
  - 例如让电机 1、2 以 0.5 rad/s 转：`--motor-ids 1,2 --mode velocity --velocity-rad-s 0.5 --kd 2`

### 3.3 安全行为

- 选中的电机 `valid_mask` 置位；**未选中的轴全部为 0**。
- 命令结束后脚本会**连续发布 10 帧 inactive**（valid_mask=0），让固件释放轴，
  由固件 100 ms 命令超时触发协议停机/刹车。
- 运行时日志会有运动安全警告，请确保机械限位和人员安全。

## 4. 常用示例（直接复制）

```bash
# 1. 位置模式：电机 1、2、3 保持到 0.5 rad，跑 5 秒
./ros2_humble.sh send --motor-ids 1,2,3 --mode position --position-rad 0.5 --kp 5 --kd 1 --duration-sec 5

# 2. 速度模式：电机 1 正转 0.5 rad/s，5 秒
./ros2_humble.sh send --motor-ids 1 --mode velocity --velocity-rad-s 0.5 --kd 2 --duration-sec 5

# 3. 速度模式：电机 1 反转 -0.5 rad/s，3 秒
./ros2_humble.sh send --motor-ids 1 --mode velocity --velocity-rad-s -0.5 --kd 2 --duration-sec 3

# 4. 位置模式：控制 RS06（ID 13）到 0 rad，8 秒（注意该轴协议是 MIT 型）
./ros2_humble.sh send --motor-ids 13 --mode position --position-rad 0.0 --kp 10 --kd 2 --duration-sec 8

# 5. 全 16 轴位置 0（释放），2 秒
./ros2_humble.sh send --mode position --position-rad 0.0 --kp 1 --kd 1 --duration-sec 2

# 6. 只发布到自定义话题（配合自己的监听/控制节点）
./ros2_humble.sh send --topic /my_commands --motor-ids 1 --mode velocity --velocity-rad-s 0.2 --duration-sec 4
```

## 5. 查看与调试（`exec` 内可用的 ROS2 命令）

```bash
# 列出话题
./ros2_humble.sh exec ros2 topic list

# 查看话题类型
./ros2_humble.sh exec ros2 topic type /hpm_gateway/motor_commands

# 回显命令/反馈话题（echo 前先运行 send，或单独开一个终端）
./ros2_humble.sh exec ros2 topic echo /hpm_gateway/motor_commands
./ros2_humble.sh exec ros2 topic echo /hpm_gateway/motor_feedback

# 查看话题发布频率
./ros2_humble.sh exec ros2 topic hz /hpm_gateway/motor_commands

# 图形界面查看
./ros2_humble.sh rqt

# 进容器手动操作
./ros2_humble.sh shell
```

> 提示：容器使用 `--network host` + `ROS_DOMAIN_ID=42`，**同一台主机上**的 ROS2
> 节点（例如 RDK 上跑的）也要设 `export ROS_DOMAIN_ID=42` 才能互通。

## 6. 在 RDK 上启动（真正驱动电机）

容器里的 `send` 只是发布 ROS2 话题命令，**不会打开 EtherCAT / 发 PDO**。
真正把命令送到 HPM 网关和电机，需要在 **RDK** 上编译并运行
`hpm_gateway_control` 节点（EtherCAT 主站模式）。RDK 使用 TROS（地瓜派系统），
环境是 `/opt/tros/humble/setup.bash`（不是 `/opt/ros/humble`）。

### 6.1 部署源码到 RDK

在 Windows PowerShell（仓库根目录）执行，把本地 `src` 同步到 RDK 的
`/root/hpm_gateway_ros2_ws/src`（只同步 src，不含 build/install/log）：

```powershell
# 首次先拉取 RDK 当前源码（或直接推送本地源码）
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1
# 本地修改确认后推送到 RDK
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1
```

也可以手动 scp 到 RDK：`root@192.168.108.178:/root/hpm_gateway_ros2_ws/src`。

### 6.2 在 RDK 上编译（需要 SOEM）

RDK 默认 SOEM 路径 `/root/Tools/EtherCat/SOEM`（若无，先 `find /root/Tools/EtherCat/SOEM /usr/local \( -name 'libsoem.so*' -o -name 'libsoem.a' \) -print` 确认）：

```bash
cd ~/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```

### 6.3 启动节点（两种模式）

**A. EtherCAT 模式（真正驱动电机）** —— 使用 `hpm_gateway_control_eth0.launch.py`，
配置在 `config/hpm_gateway_control_eth0.yaml`（接口 `eth0`、`arm_all_axes: true`、
1 kHz 周期）。需要 root（raw socket）：

```bash
sudo -E bash -lc '
export ROS_DOMAIN_ID=42
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
ros2 launch hpm_gateway_control hpm_gateway_control_eth0.launch.py
'
```

**B. 安全模式（不打开 EtherCAT）** —— 用于只测 ROS2 通信：

```bash
export ROS_DOMAIN_ID=42
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
ros2 launch hpm_gateway_control hpm_gateway_control.launch.py
```

### 6.4 在 RDK 上发命令

在 RDK 上（另一个终端，已 source 环境 + `export ROS_DOMAIN_ID=42`）运行同一个
发送脚本（参数与第 3 节完全相同）：

```bash
ros2 run hpm_gateway_control send_reference_motor_commands.py \
  --motor-ids 1 --mode velocity --velocity-rad-s 0.5 --kd 2 --duration-sec 5
```

### 6.5 RDK 与容器配合（跨机器通信）

- 容器（PC）和 RDK 使用 `--network host` + `ROS_DOMAIN_ID=42`，只要两台机器
  网络可达且域 ID 相同，DDS 就能互通。
- 典型用法：**RDK 上跑 EtherCAT 节点**，**PC 容器里用 `send` 发命令**，
  或反过来在 RDK 上用 `ros2 run` 发、容器里 `exec ros2 topic echo` 看反馈。
- 两者必须都用 `export ROS_DOMAIN_ID=42`。

> ⚠️ **跨局域网发命令请降频（实测结论）**：跨网以 100~200 Hz 发命令时，
> DDS 突发到达 + 节点 `KeepLast(1).best_effort` 订阅丢帧 + 100 ms 命令超时
> 清零 PDO，会导致电机抽搐/发疯/强转一下停。**控制命令流建议在 RDK 本机发**；
> 确需从 PC 跨机发时，给 `send` 加 `--rate-hz 20`。

### 6.6 RDK 上的其他主站方式

也可以不经过 ROS2，直接用 RDK 上的 SOEM 周期主站（更底层、更贴近联调）：

```bash
cd /root/hpm_gateway/host/rdk_ecat_motor_rt && make
sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --rx-timeout-us 500
```

## 7. 已知限制（务必阅读）

- **`hpm_gateway_control` C++ 节点（EtherCAT 主站）容器内未编译**：它依赖 SOEM，
  且其 CMake 需要 3.28+，容器内的 CMake（3.22）不兼容，因此容器里**只有**
  发命令的 Python 脚本可用（不碰 EtherCAT）。该节点需在 **RDK** 上编译运行（见第 6 节）。
- 容器内 `send` 只是发布 ROS2 话题命令，**并不会打开 EtherCAT / 发 PDO**。
  真正把命令送到 HPM 网关和电机，需要：
  - 在 RDK 上编译并运行 `hpm_gateway_control`（EtherCAT 模式），
    launch 用 `hpm_gateway_control_eth0.launch.py`（接口 `eth0`，`arm_all_axes: true`）；
  - 或者用 RDK 上的 SOEM 主站 `host/rdk_ecat_motor_rt/ecat_motor_rt`。
- 容器发命令脚本默认全 16 轴有效——**请先用 `--motor-ids` 限定到实际接线的轴**，
  避免向未接线的轴发送意外指令。

## 8. 相关文件

```text
scripts/ros2_humble.sh                                    容器工具脚本
host/hpm_gateway_ros2_ws/src/hpm_gateway_control/scripts/
  send_reference_motor_commands.py                        电机命令发送脚本
host/hpm_gateway_ros2_ws/src/hpm_gateway_msgs/msg/
  MotorMitCommand.msg / MotorMitCommandArray.msg          命令消息
  MotorFeedback.msg / MotorFeedbackArray.msg              反馈消息
host/hpm_gateway_ros2_ws/src/hpm_gateway_control/
  launch/hpm_gateway_control.launch.py                    安全模式（无 EtherCAT）
  launch/hpm_gateway_control_eth0.launch.py               EtherCAT 模式（RDK）
  config/hpm_gateway_control_eth0.yaml                    EtherCAT 参数
```

## 9. 消息与话题速查

| 话题 | 类型 | 方向 |
| --- | --- | --- |
| `/hpm_gateway/motor_commands` | `MotorMitCommandArray` | 发送脚本发布（16 轴聚合） |
| `/hpm_gateway/motor_command` | `MotorMitCommand` | 控制节点（单轴，EtherCAT 模式） |
| `/hpm_gateway/motor_feedback` | `MotorFeedbackArray` | 控制节点发布（16 轴反馈） |
| `/imu/data` | `sensor_msgs/msg/Imu` | 控制节点发布 |
| `/imu/temperature` | `sensor_msgs/msg/Temperature` | 控制节点发布 |

消息字段：

```text
MotorMitCommandArray:  stamp, sequence, valid_mask, commands[16]
MotorMitCommand:      stamp, motor_id, position_rad, velocity_rad_s, kp, kd, torque_nm
MotorFeedbackArray:   stamp, sequence, valid_mask, feedback[16]
MotorFeedback:        stamp, motor_id, position_rad, velocity_rad_s, torque_nm, status
```

> `valid_mask` 第 n 位（bit n）标记 CAN 电机 ID n+1 有效；`commands[n]` 携带该 ID。
