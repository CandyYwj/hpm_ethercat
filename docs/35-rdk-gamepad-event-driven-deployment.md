<!-- Author: BearKing -->
# RDK 事件驱动手柄节点：C++ 实现与山寨 XBOX360 接收器排障总结

> 更新日期：2026-08-20
>
> 本文总结在 RDK（TROS / ROS2 Humble）上部署 **C++ 事件驱动手柄节点**
> 的完整过程，包括：事件驱动架构设计、节点实现、以及一个硬件的坑——
> **山寨 XBOX360 2.4G 接收器在 RDK 内核上识别不到，最终用 xboxdrv 解决**。
>
> 关联文档：[`docs/34-rdk-gamepad-node.md`](34-rdk-gamepad-node.md)（Python evdev 版方案）。

---

## 1. 结论速览

| 项目 | 结论 |
| --- | --- |
| 手柄节点 | C++ / rclcpp，**纯事件驱动**（udev + epoll + signalfd + timerfd），发布标准 `sensor_msgs/msg/Joy` 到 `/gamepad/joy` |
| 包 | `host/hpm_gateway_ros2_ws/src/hpm_gateway_gamepad/` |
| 设备识别 | 双通道：udev 标签（`ID_INPUT_JOYSTICK`/`ID_INPUT_GAMEPAD`）**或**能力探测（`EVIOCGBIT`），自动排除鼠标/键盘 |
| 硬件坑 | 山寨接收器 `0x413d:0x2104` 在 RDK 内核 6.1.83 的 xpad 里**没有条目** → 只被当键盘 |
| 硬件解法 | **xboxdrv 0.8.8 用户态驱动**，命令见 §5，已实测出 `/dev/input/js0` |
| 部署 | `deploy_rdk_ros2_workspace.sh` 同步 → colcon 编译 → systemd 开机自启 |

---

## 2. 为什么用事件驱动（对比 Python evdev 轮询版）

| 能力 | C++ 事件驱动节点 | Python evdev 轮询版 |
| --- | --- | --- |
| 热插拔感知 | udev netlink，内核事件即时到达 | 定时轮询 `/dev/input` 或 inotify |
| 输入读取 | epoll 阻塞等待，事件到达才醒 | 固定间隔轮询 |
| 空闲 CPU | 无输入时线程睡在 `epoll_wait` | 周期性空转 |
| 关闭 | signalfd 事件驱动优雅退出 | 信号标志位 |
| 发布 | 事件到达即发布（含可选 autorepeat/heartbeat） | 轮询时发布 |

**核心：一个 epoll 循环复用 4 类文件描述符，不轮询、不忙等。**

```mermaid
flowchart LR
    U[udev netlink fd] -->|热插拔| E[epoll]
    J[evdev fd /dev/input/eventX] -->|输入事件| E
    S[signalfd SIGINT/SIGTERM] -->|关闭| E
    T[timerfd autorepeat/heartbeat] -->|周期| E
    E --> N[GamepadNode::run 事件分发]
    N --> P[/gamepad/joy sensor_msgs/msg/Joy]
```

---

## 3. 节点实现（hpm_gateway_gamepad）

### 3.1 目录结构

```
host/hpm_gateway_ros2_ws/src/hpm_gateway_gamepad/
├── CMakeLists.txt
├── package.xml                  # 依赖 rclcpp + sensor_msgs
├── README.md
├── config/gamepad.yaml          # 节点参数
├── launch/gamepad.launch.py
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

### 3.2 三个关键模块

**① `evdev_joystick.cpp`（事件驱动核心，不依赖 ROS2）**
- `open()` 时用 `EVIOCGBIT`/`EVIOCGABS` **探测一次**轴/按键能力，建成查表；
- `drain()` 热路径只做 `read()` + 查表，无 ioctl、无分配；
- 轴值按 absinfo min/max 归一化到 [-1,1] 并施加死区，按钮 0/1，与 `sensor_msgs/msg/Joy` 语义一致。

**② `gamepad_monitor.cpp`（udev 热插拔）**
- `udev_monitor` netlink 监听子系统 `input`；
- **双通道识别**：udev 标签 `ID_INPUT_JOYSTICK=1`/`ID_INPUT_GAMEPAD=1` **或** 能力探测
  （≥2 个摇杆轴或十字键，且带游戏按键范围/≥4 个按键），自动排除鼠标/键盘；
- 启动时 `udev_enumerate` 扫描已存在设备。

**③ `gamepad_node.cpp`（rclcpp + 事件循环）**
- `run()` 里一个 `epoll_wait` 分发 4 类 fd，发布者即主线程，**不需要 executor/spin**；
- 参数：`dev`（空=udev 自动）、`deadzone`、`autorepeat_rate`、`heartbeat_period_ms`、`frame_id`、`topic`；
- 拔出手柄会补发一帧全零 Joy，下游可立即停机。

---

## 4. 硬件坑：山寨 XBOX360 2.4G 接收器识别不到

### 4.1 现象

- RDK 上 `ls /dev/input/*` 只有 `event-kbd`（键盘）和 `hidraw/hiddev`，**没有 `/dev/input/js*`**；
- `udevadm info -q property -n /dev/input/event1`：

```
ID_VENDOR_ID=413d  ID_MODEL_ID=2104  ID_MODEL=XBOX_360_For_Windows
ID_USB_INTERFACES=:ff5d01:030101:030000:     ← 接口0=ff5d01 是 XBOX360 厂商协议
ID_USB_DRIVER=usbhid                          ← 被 usbhid 抢走，当成键盘
```

- `dmesg` 显示设备在 `0x2106 "HID zk"` ↔ `0x2104` 之间反复重枚举，且 `can't set config error -71`。

### 4.2 根因（逐层排查结论）

| 层 | 结论 |
| --- | --- |
| 设备本身 | **没问题**——同一接收器插在主机（内核 7.0）上直接出 `/dev/input/js0` |
| RDK 内核 | 6.1.83 的 xpad **缺 `0x413d` 条目**（"Black Shark Green Ghost" 是 6.2+ 才加进 xpad，LTS 分支不回灌） |
| 结果 | RDK 的 xpad 按 `vendor + class 0xff/subclass 93/protocol 1` 匹配不到接口 0（ff5d01），usbhid 只把键盘接口注册成 input 设备 |

> 佐证：`modprobe xpad` 后重新插拔，dmesg 仍无 xpad 认领日志；强制
> `echo 1-1.2:1.0 > /sys/bus/usb/drivers/xpad/bind` 也会因 id_table 无匹配而失败。

### 4.3 解法：xboxdrv 用户态驱动（已实测通过）

xboxdrv 用 libusb **直接抓接口 0（ff5d01）**，按标准 XBOX360 协议解码，再生成 uinput
虚拟手柄 `/dev/input/js0`——**完全不依赖内核 xpad 是否有该条目**。

```bash
sudo apt install -y xboxdrv
sudo xboxdrv --device-by-id 413d:2104 --type xbox360 --mimic-xpad --detach-kernel-driver
# 另开终端：
ls /dev/input/js0
sudo apt install -y joystick && jstest /dev/input/js0
```

> ⚠️ xboxdrv 0.8.8 参数坑（实测）：
> - **没有** `--wid/--wpid` 选项；
> - `--device-by-id <vid>:<pid>` **必须搭配** `--type xbox360`，否则报错。

### 4.4 备用方案（不推荐）

- **把 RDK 内核升到 6.2+**：xpad 原生支持 `0x413d`，但为手柄动系统内核风险高；
- **给 6.1 的 xpad 打补丁重编模块**：需要内核头 + DKMS，工作量大；
- **换正版设备**：微软 XBOX360 无线接收器 + 手柄、有线 XBOX360 手柄、蓝牙手柄，内核原生支持即插即用；
  若山寨接收器反复 `error -71` 重枚举（供电/接触/配对问题），也建议直接换。

---

## 5. 部署到 RDK

### 5.1 同步代码

```bash
cd /home/bear/Work/HPM32/hpm_gateway/hpm_gateway
./scripts/deploy_rdk_ros2_workspace.sh          # 默认 root@192.168.108.178，同步整个 src/
# Windows 用 .\scripts\deploy_rdk_ros2_workspace.ps1
```

### 5.2 编译

```bash
ssh root@192.168.108.178
sudo apt install -y libudev-dev
cd /root/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hpm_gateway_gamepad --symlink-install
source install/setup.bash
```

### 5.3 运行验证

```bash
# 终端 A：先确保 js0（xboxdrv 前台跑）
sudo xboxdrv --device-by-id 413d:2104 --type xbox360 --mimic-xpad --detach-kernel-driver
# 终端 B：起手柄节点
ros2 run hpm_gateway_gamepad gamepad_node
# 终端 C：看话题
ros2 topic echo /gamepad/joy
```

### 5.4 开机自启（systemd）

仓库 `scripts/` 已提供 5 个文件：

| 文件 | 安装到 | 说明 |
| --- | --- | --- |
| `rdk_xboxdrv.sh` | `/usr/local/bin/` | **看门狗循环**：xboxdrv 退出后每 2s 重试，热拔插自动重建 `js0` |
| `rdk_xboxdrv.service` | `/etc/systemd/system/` | xboxdrv 服务，`Restart=on-failure` |
| `90-rdk-xbox360.rules` | `/etc/udev/rules.d/` | 插上接收器用 `SYSTEMD_WANTS` 唤醒 xboxdrv 服务 |
| `hpm_gateway_gamepad.sh` | `/usr/local/bin/` | 启动手柄节点 |
| `hpm_gateway_gamepad.service` | `/etc/systemd/system/` | `After/Wants=rdk_xboxdrv.service`、`After=hpm_gateway_node.service` |

```bash
scp scripts/rdk_xboxdrv.sh             root@192.168.108.178:/usr/local/bin/
scp scripts/rdk_xboxdrv.service        root@192.168.108.178:/etc/systemd/system/
scp scripts/90-rdk-xbox360.rules       root@192.168.108.178:/etc/udev/rules.d/
scp scripts/hpm_gateway_gamepad.sh     root@192.168.108.178:/usr/local/bin/
scp scripts/hpm_gateway_gamepad.service root@192.168.108.178:/etc/systemd/system/

ssh root@192.168.108.178
chmod +x /usr/local/bin/rdk_xboxdrv.sh /usr/local/bin/hpm_gateway_gamepad.sh
udevadm control --reload && udevadm trigger   # 生效 udev 规则
systemctl daemon-reload
systemctl enable --now rdk_xboxdrv
systemctl enable --now hpm_gateway_gamepad
journalctl -u rdk_xboxdrv -f
journalctl -u hpm_gateway_gamepad -f
```

服务启动顺序：`rdk_xboxdrv`（先出 js0）→ `hpm_gateway_gamepad`（再发 `/gamepad/joy`）
→ 跟在 `hpm_gateway_node`（控制节点）之后。

> 🔌 **热拔插说明**：`js0` 是 xboxdrv 用 uinput 虚拟出来的——接收器一拔 xboxdrv 退出、
> `js0` 消失。`rdk_xboxdrv.sh` 的看门狗循环保证再插时自动重建 `js0`（无需重启）；
> `90-rdk-xbox360.rules` 保证即使服务曾被停止，插上接收器也会唤醒它。手柄节点本身的
> udev 热插拔逻辑（add/remove → 连/断）无需改动。
>
> 💡 **更优雅的替代（推荐）**：xboxdrv 是"绕过内核"的方案。若 RDK 有内核头
> （`/lib/modules/$(uname -r)/build`），可用 [`docs/36-xpad-413d-module-rebuild.md`](36-xpad-413d-module-rebuild.md)
> 给 6.1 内核的 xpad 补上 `0x413d` 两行并重编模块，让内核**原生**认领设备
> （`js0` 直出、原生热拔插），随后可**下线 xboxdrv 服务与 udev 规则**。

---

## 6. 验证清单

1. 无手柄时：xboxdrv 前台等设备，节点阻塞在 `epoll_wait`（CPU≈0）无报错。
2. 手柄开机/配对：`ls /dev/input/js0` 出现，`jstest /dev/input/js0` 拨杆/按键有反应。
3. `ros2 topic echo /gamepad/joy`：`axes`/`buttons` 实时变化。
4. 拔出手柄：节点日志 `gamepad disconnected`，随后补发一帧全零 Joy。
5. 再插入：自动重新连接，无需重启节点。
6. `kill -INT` 手柄节点：日志 `signal 2 received, shutting down`，干净退出。
7. systemd：重启 RDK 后 `js0` 与 `/gamepad/joy` 自动恢复。

---

## 7. 关键备忘（本次实测沉淀）

- RDK 内核 `6.1.83` 的 xpad **没有 `0x413d`**；xboxdrv 是纯用户态正解。
- xboxdrv 0.8.8 正确参数：`--device-by-id 413d:2104 --type xbox360 --mimic-xpad`；
  **没有 `--wpid`，`--device-by-id` 必须配 `--type`**。
- 手柄节点识别是双通道（udev 标签 OR 能力探测），`evdev_joystick.cpp` 可在任意 Linux
  主机独立 `g++ -fsyntax-only` 验证（本机零警告通过）。
- 下游电机控制：`hpm_gateway_control` 目前**尚未订阅** `/gamepad/joy`，如需"手柄→电机"
  整条链路，需在控制节点加 Joy 订阅并做轴映射（下一步工作）。
