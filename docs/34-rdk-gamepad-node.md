<!-- Author: BearKing -->
# RDK 手柄节点：动态感知热插拔 + 发布手柄消息

> 目标：在 RDK（TROS / ROS2 Humble）上跑一个手柄节点，满足两个核心需求——
> ① **动态感知手柄的插入/拔出**（不重启节点、不依赖固定设备名）；
> ② **持续发布手柄状态消息**（标准 `sensor_msgs/msg/Joy`，方便直接接电机控制）。
>
> 更新日期：2026-08-20
>
> 📌 **最终实现见 [`docs/35-rdk-gamepad-event-driven-deployment.md`](35-rdk-gamepad-event-driven-deployment.md)**：
> 已落地为 C++ 事件驱动节点 `hpm_gateway_gamepad`（udev+epoll，发布 `/gamepad/joy`），
> 并含山寨 XBOX360 接收器在 RDK 上需用 xboxdrv 出 `js0` 的排障记录。本文档保留为方案对比背景。

## 1. 需求拆解

```text
USB 手柄
  └─ /dev/input/js0  (Linux joystick API)    或  /dev/input/event*  (evdev)
       │  热插拔：插入/拔出动态变化
       ▼
手柄节点 (RDK / TROS)
  ├─ 感知存在：设备出现 → 打开并开始读；设备消失 → 关闭并等待
  └─ 发布消息：sensor_msgs/msg/Joy { axes[], buttons[], header }
       ▼
话题 /gamepad/joy
       ▼
(可选) 电机控制节点 hpm_gateway_control 订阅 → 映射为电机命令
```

- **消息类型**：用 ROS2 标准 `sensor_msgs/msg/Joy`（`axes: float32[]`、`buttons: int32[]`、`header`），无需自定义消息，任何节点都能直接订阅。
- **热插拔**是重点：设备路径可能随插拔变化（`/dev/input/js0` 可能变 `js1`），节点不能假设路径固定。

---

## 2. 方案总览

| 方案 | 实现方式 | 热插拔 | 上手成本 | 适用 |
| --- | --- | --- | --- | --- |
| **A. `ros-humble-joy`** | 官方 `joy_node`（Linux 后端） | 启动时无手柄会等待；运行时掉线自动重连**视内核/驱动而定** | 零代码，一条命令 | 先快速验证、标准手柄 |
| **B. SDL2 `game_controller`** | SDL2 后端，多手柄/按键映射 | 一般，不如 evdev 稳 | 低 | 需要 SDL 手柄映射 |
| **C. evdev 自研节点** ⭐ | Python `evdev` + 轮询/udev 监听 | **最稳、可控** | 中（几十行代码） | 长期方案、需要可靠插拔感知 |

> ⚠️ **实测（2026-08-20）**：TROS 上未配置 ROS2 官方 apt 源，`apt install ros-humble-joy` 报
> `Unable to locate package`。**官方 `joy` 节点装不上，所以直接走方案 C（evdev）**——
> 它只依赖系统包 `python3-evdev`，与 ROS 仓库无关；且发布的消息仍是标准
> `sensor_msgs/msg/Joy`，**下游电机控制代码不用改**。
>
> 若确实想要官方 `joy` 节点，可走 3.4 的源码编译（成本较高，非必需）。

---

## 3. 方案 A：`ros-humble-joy`（零代码，先试）

### 3.1 安装（TROS 实测装不上 → 见 3.4 备选）

```bash
# RDK 上（root）—— 实测结果：
apt-cache search ros-humble-joy        # 空（TROS 未配置 ROS2 apt 源）
apt install -y ros-humble-joy          # E: Unable to locate package
```

> 说明：`sensor_msgs/msg/Joy` 消息类型 TROS 自带（`hpm_gateway_control` 已依赖
> `sensor_msgs`），缺的只是官方 `joy` 可执行节点。**不折腾官方包，直接用方案 C**。

### 3.2 运行

```bash
source /opt/tros/humble/setup.bash
ros2 run joy joy_node
```

常用参数（launch 或 yaml 里配）：

| 参数 | 默认 | 说明 |
| --- | --- | --- |
| `dev` | `/dev/input/js0` | 手柄设备路径；留空让后端自动找第一个 |
| `deadzone` | `0.05` | 摇杆死区 |
| `autorepeat_rate` | `20.0` | 按钮自动重复频率 |
| `coalesce_interval_ms` | `50` | 事件合并间隔 |

```bash
ros2 run joy joy_node --ros-args -p deadzone:=0.1 -p autorepeat_rate:=20.0
```

### 3.3 热插拔表现（如实说明）

- **启动时没插手柄**：`joy_linux` 后端会轮询设备、等待插入 → 插上后自动开始发布；
- **运行时拔掉**：设备关闭，节点通常能感知；**能否自动重连到新设备**在不同内核/
  驱动上表现不一，个别情况需要重启节点。
- 兜底：用 `systemd` 的 `Restart=on-failure`（见第 7 节）保证异常时自动拉起。

### 3.4 备选：源码编译官方 `joy`（可选）

实在要官方节点，可在工作区里把 `ros-drivers/joystick_drivers` 的 `joy`、`joy_linux`
拉下来 colcon 编译（需 ROS2 源/工具链，成本高）。热插拔体验并不优于方案 C，
一般没必要。

---

## 4. 方案 B：SDL2 `game_controller`

```bash
source /opt/tros/humble/setup.bash
ros2 run joy game_controller
```

- 优点：SDL2 支持多手柄、自带按键映射（`SDL_GAMECONTROLLERCONFIG`）。
- 缺点：热插拔与 `joy_node` 类似，依赖 SDL 后端；对"动态感知插拔"这一核心需求
  不是最稳的。

---

## 5. 方案 C（推荐长期）：evdev 自研节点

### 5.1 原理

- 用 Python `evdev` 直接读 `/dev/input/event*`（比 `js*` 更通用、信息更全）；
- **热插拔 = 每次轮询重新枚举设备**（`evdev.list_devices()`），插上就打开、
  拔出就 `OSError` → 关闭等待。零 udev 依赖、行为可预测；
- 发布标准 `sensor_msgs/msg/Joy`，与方案 A 完全兼容。

### 5.2 依赖

```bash
apt install -y python3-evdev          # 或 pip install evdev
```

### 5.3 代码骨架（示意，按你的手柄按键/轴号调整映射）

```python
#!/usr/bin/env python3
import evdev
from evdev import ecodes, list_devices
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy

# 摇杆/扳机轴的 ecodes 到 Joy.axes[] 下标映射（不同手柄要核对）
AX_MAP = {ecodes.ABS_X: 0, ecodes.ABS_Y: 1, ecodes.ABS_RX: 2, ecodes.ABS_RY: 3,
          ecodes.ABS_Z: 4, ecodes.ABS_RZ: 5, ecodes.ABS_HAT0X: 6, ecodes.ABS_HAT0Y: 7}


class GamepadNode(Node):
    def __init__(self):
        super().__init__('gamepad_node')
        self.pub = self.create_publisher(Joy, 'gamepad/joy', 10)
        self.timer = self.create_timer(0.01, self.poll)   # 100 Hz
        self.dev = None
        self.axes = [0.0] * 8
        self.buttons = [0] * 16

    def find_gamepad(self):
        """每次调用都重新枚举 -> 天然感知热插拔"""
        for path in list_devices():
            try:
                dev = evdev.InputDevice(path)
                has_stick = ecodes.EV_ABS in dev.capabilities()
                dev.close()
                if has_stick:
                    return path
            except OSError:
                continue
        return None

    def poll(self):
        if self.dev is None:
            path = self.find_gamepad()
            if path is not None:
                self.dev = evdev.InputDevice(path)
                self.dev.grab()
                self.get_logger().info(f'手柄接入: {self.dev.name} ({path})')
            return

        try:
            while True:
                event = self.dev.read_one()          # 非阻塞
                if event is None:
                    break
                if event.type == ecodes.EV_ABS and event.code in AX_MAP:
                    # 值归一化到 [-1, 1]（按手柄的 ABS range）
                    rng = self.dev.absinfo(event.code)
                    self.axes[AX_MAP[event.code]] = \
                        2.0 * (event.value - rng.min) / (rng.max - rng.min) - 1.0
                elif event.type == ecodes.EV_KEY and 0 < event.code <= len(self.buttons):
                    self.buttons[event.code - 1] = event.value
        except OSError:
            self.get_logger().warn('手柄拔出，等待重新插入')
            self.dev.close()
            self.dev = None
            return

        msg = Joy(header=self.get_clock().now().to_msg(),
                  axes=list(self.axes), buttons=list(self.buttons))
        self.pub.publish(msg)


def main():
    rclpy.init()
    rclpy.spin(GamepadNode())


if __name__ == '__main__':
    main()
```

> 说明：上面的轴/按钮映射是**示意**，必须用 `evtest` 或打印 `event` 的方式按你的
> 手柄实际 ecodes 校准。`grab()` 独占设备，避免和其它进程抢读。

### 5.4 更强的事件驱动写法（可选）

想要"零轮询、事件驱动"，用 `evdev` 的 udev monitor 监听设备插拔：

```python
import pyudev
context = pyudev.Context()
monitor = pyudev.Monitor.from_netlink(context)
monitor.filter_by(subsystem='input')
# 收到 'add'/'remove' 事件时打开/关闭设备
```

需要 `python3-pyudev`。轮询写法已经够稳，这里作为进阶参考。

---

## 6. 热插拔检测的三种实现方式（原理对照）

| 方式 | 原理 | 说明 |
| --- | --- | --- |
| **Linux joystick API 轮询** | `joy_node` 反复尝试打开 `/dev/input/jsX` | 简单；重连稳定性看后端 |
| **evdev 设备枚举轮询** | 每次 `list_devices()` 重扫 | 方案 C，简单可靠 |
| **udev 事件驱动** | 订阅内核热插拔事件（`add`/`remove`） | 零轮询、最及时；代码略多 |

三者可任选；对 RDK 上"手柄热插拔感知"这一需求，**方案 C 的轮询枚举**性价比最高。

---

## 7. RDK 部署步骤（systemd 自启，复用现有模式）

### 7.1 设备权限

两种方式任选：

```bash
# 方式 1：节点以 root 跑（systemd User=root，最简单，和 hpm_gateway_node 一致）
# 方式 2：加 input 组 / udev 规则（非 root 用户可用）
sudo usermod -aG input $USER
# /etc/udev/rules.d/99-gamepad.rules
#   SUBSYSTEM=="input", KERNEL=="js*", GROUP="input", MODE="0660"
# udevadm control --reload && udevadm trigger
```

### 7.2 启动 wrapper 脚本 + systemd 服务（照抄 hpm_gateway_node 的模式）

`/usr/local/bin/gamepad_node.sh`：

```bash
#!/usr/bin/env bash
set -e
source /opt/tros/humble/setup.bash
[ -f /root/hpm_gateway_ros2_ws/install/setup.bash ] && source /root/hpm_gateway_ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=42
# 方案 A：
exec ros2 run joy joy_node --ros-args -p dev:="/dev/input/js0"
# 方案 C：换成你的节点
# exec ros2 run hpm_gateway_control gamepad_node
```

`/etc/systemd/system/gamepad_node.service`：

```ini
[Unit]
Description=Gamepad hotplug node (ROS2 Humble / TROS)
After=network-online.target hpm_gateway_node.service
Wants=network-online.target

[Service]
Type=simple
User=root
Environment=ROS_DOMAIN_ID=42
ExecStart=/usr/local/bin/gamepad_node.sh
Restart=on-failure
RestartSec=5
TimeoutStopSec=20
KillMode=control-group
KillSignal=SIGINT
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

安装与启停：

```bash
install -m 0755 /root/gamepad_node.sh /usr/local/bin/gamepad_node.sh
install -m 0644 /root/gamepad_node.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now gamepad_node.service
systemctl status gamepad_node.service
journalctl -u gamepad_node.service -f
```

> 细节与 systemd 原理见 `docs/33-systemd-guide.md` 和 `docs/29` §8.3。

---

## 8. 验证清单

- [ ] 插入手柄前：`ls /dev/input/` 无 `js*`/新 `event*`；`ros2 topic hz /gamepad/joy` 无输出
- [ ] 插入手柄：`dmesg | tail` 出现 USB 手柄枚举；`evtest /dev/input/eventX` 能读到按键/摇杆
- [ ] 节点日志出现"手柄接入"；`ros2 topic echo /gamepad/joy` 能看到 `axes`/`buttons` 变化
- [ ] 摇动摇杆/按键：`axes`/`buttons` 对应变化（用 evtest 校准映射）
- [ ] **拔掉手柄**：节点日志出现"手柄拔出/等待"；话题停止发布；**再次插入**：节点自动恢复发布（热插拔核心）
- [ ] 拔插过程中节点进程没有崩溃（systemd 无反复重启）

---

## 9.（可选）接入手柄 → 电机命令

手柄节点只负责"感知 + 发布 Joy"。要把手柄变成电机控制，在下游加一个映射节点
（订阅 `/gamepad/joy`，把 `axes` 映射成速度/位置命令，发到现有
`/hpm_gateway/motor_command`）：

```text
/gamepad/joy  →  joy_to_motor 节点  →  /hpm_gateway/motor_command (motor_id, v_des, kd)
```

安全要点（务必遵守）：
- **必须持续发布**：电机命令有 100 ms 超时，映射节点要用 ≥50 Hz 连续发布，
  摇杆回中时发零命令（不是停发）；
- **死区**：摇杆加 deadzone，避免漂移触发微动；
- **方向/限速**：映射前确认轴方向，限制最大速度，第一次用手动/空载试；
- 单轴话题是"单轴独占"，发一个 `motor_id` 会停掉其它轴；要同时控制多轴用
  数组话题（见 `docs/29` §9.5）。

---

## 10. 相关文件与参考

```text
docs/33-systemd-guide.md               systemd 服务原理与写法
docs/29-rdkx5-ros2-deployment.md       RDK 部署、systemd 自启（§8.3）、电机命令（§9.5）
scripts/hpm_gateway_node.sh/.service   可复制的 systemd 自启模板
```
