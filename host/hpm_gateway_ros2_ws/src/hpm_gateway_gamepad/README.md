# hpm_gateway_gamepad

RDK（TROS / ROS2 Humble）上的**事件驱动**手柄节点：C++ 实现，发布标准
`sensor_msgs/msg/Joy`，用于直接对接 `hpm_gateway_control` 的电机控制。

## 为什么是事件驱动（相比 Python evdev 轮询版）

| 能力 | 本节点（C++ 事件驱动） | Python evdev 轮询版 |
| --- | --- | --- |
| 热插拔感知 | udev netlink，内核事件即时到达 | 定时轮询 `/dev/input` 或 inotify |
| 输入读取 | epoll 阻塞等待，事件到达才醒 | 固定间隔轮询 |
| 空闲 CPU | 无输入时线程睡在 `epoll_wait` | 周期性空转 |
| 关闭 | signalfd 事件驱动优雅退出 | 信号标志位 |
| 发布 | 事件到达即发布（含可选 autorepeat/heartbeat） | 轮询时发布 |

**核心思路**：一个 epoll 循环复用 4 类文件描述符，不轮询、不忙等：

```mermaid
flowchart LR
    U[udev netlink fd] -->|热插拔| E[epoll]
    J[evdev fd /dev/input/eventX] -->|输入事件| E
    S[signalfd SIGINT/SIGTERM] -->|关闭| E
    T[timerfd autorepeat/heartbeat] -->|周期| E
    E --> N[GamepadNode::run 事件分发]
    N --> P[/gamepad/joy sensor_msgs/msg/Joy]
```

## 目录

```
src/hpm_gateway_gamepad/
├── CMakeLists.txt
├── package.xml
├── README.md
├── config/gamepad.yaml          # 节点参数
├── launch/gamepad.launch.py     # launch 入口
├── include/hpm_gateway_gamepad/
│   ├── evdev_joystick.hpp       # evdev 设备封装（RAII）
│   ├── gamepad_monitor.hpp      # udev 热插拔监视
│   └── gamepad_node.hpp         # rclcpp 节点 + 事件循环
└── src/
    ├── evdev_joystick.cpp
    ├── gamepad_monitor.cpp
    ├── gamepad_node.cpp
    └── gamepad_main.cpp
```

## 构建（RDK 上）

```bash
# 依赖：libudev（系统库，一般已装；缺失时）
sudo apt install -y libudev-dev

cd /root/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hpm_gateway_gamepad --symlink-install
source install/setup.bash
```

> 本仓库内从 PC 同步代码用 `scripts/deploy_rdk_ros2_workspace.sh <RDK_IP>`。

## 运行

```bash
# 手动
ros2 run hpm_gateway_gamepad gamepad_node

# 或 launch（加载 config/gamepad.yaml）
ros2 launch hpm_gateway_gamepad gamepad.launch.py

# 验证发布
ros2 topic echo /gamepad/joy
```

插拔手柄应看到日志 `gamepad connected/disconnected`，无需重启节点。

## 参数（config/gamepad.yaml）

| 参数 | 默认 | 说明 |
| --- | --- | --- |
| `dev` | `""` | 空 = udev 自动检测（推荐）；固定 `/dev/input/eventX` 可跳过检测 |
| `deadzone` | `0.05` | 摇杆死区（归一化 [-1,1]） |
| `autorepeat_rate` | `20.0` | 按住按键时按此频率重复发布；`0` 关闭 |
| `heartbeat_period_ms` | `0` | >0 时周期性重发当前状态（下游看门狗用）；`0` 关闭 |
| `frame_id` | `gamepad` | Joy.header.frame_id |
| `topic` | `/gamepad/joy` | 发布话题 |

命令行覆盖示例：

```bash
ros2 run hpm_gateway_gamepad gamepad_node --ros-args \
  -p dev:=/dev/input/event2 -p deadzone:=0.1 -p heartbeat_period_ms:=1000
```

## 开机自启（systemd）

```bash
# 仓库 scripts/ 下已提供：
#   hpm_gateway_gamepad.sh      -> /usr/local/bin/hpm_gateway_gamepad.sh
#   hpm_gateway_gamepad.service -> /etc/systemd/system/hpm_gateway_gamepad.service
sudo cp scripts/hpm_gateway_gamepad.sh /usr/local/bin/
sudo cp scripts/hpm_gateway_gamepad.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now hpm_gateway_gamepad
sudo systemctl status hpm_gateway_gamepad   # 验证
```

服务 `After=hpm_gateway_node.service`，确保控制节点已就绪后再发布手柄数据。

## 验证清单

1. 无手柄启动：进程存活，`epoll_wait` 阻塞（CPU≈0），无报错。
2. 插入手柄：日志出现 `gamepad connected: /dev/input/eventX (设备名)`。
3. `ros2 topic echo /gamepad/joy`：拨动摇杆/按按键，`axes`/`buttons` 实时变化。
4. 拔出手柄：日志出现 `gamepad disconnected`，随后发布一帧全零 Joy。
5. 再插入：自动重新连接，无需重启节点。
6. `kill -INT`：日志 `signal 2 received, shutting down`，进程干净退出。

## 说明

- 设备识别**双通道**：udev 标签 `ID_INPUT_JOYSTICK=1`/`ID_INPUT_GAMEPAD=1`，或
  能力探测（`EVIOCGBIT` 探测摇杆轴 + 游戏按键，自动排除鼠标/键盘）。因此即使
  某接收器暂时只以键盘接口出现，等真正的手柄接口出现后也会被识别。
- 轴值按 `EVIOCGABS` 的 min/max 归一化到 [-1,1] 并施加死区，按钮 0/1，
  与 `sensor_msgs/msg/Joy` 语义一致，下游控制无需改动。
- 事件驱动核心（`evdev_joystick.cpp`）不依赖 ROS2，可在任意 Linux 主机单独编译。
- 该节点是发布者，不需要 executor/spin；无订阅，发布线程即主线程。
- 若手柄只有键盘/鼠标接口（如 2.4G XBOX360 接收器未配对），先按住手柄导航键
  配对开机，直到出现 `/dev/input/js*` 或具备摇杆轴的手柄事件节点。
