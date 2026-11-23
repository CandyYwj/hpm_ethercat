# hpm_gateway_power

DOG 电源板（STM32F103C8T6）PowerLink v1 控制节点（ROS2 Humble / Python）。

- 通过 USART1 / USB-TTL（PA9 TX, PA10 RX, 115200-8-N-1）与电源板通信。
- 周期心跳（默认 500 ms）+ GET_STATUS 轮询（默认 1000 ms）。
- 订阅 `PowerCommand`，执行单通道 `SET_POWER` / 批量 `SET_POWER_ALL`。
- 发布 `PowerStatus` 遥测（电池电压 / NTC 温度 / 输出状态 / 告警）。
- 主机侧链路看门狗：>3 s 无合法帧置 `comm_lost`。

## 目录

```
hpm_gateway_power/
  hpm_gateway_power/
    power_link_protocol.py   # 纯 Python 协议客户端（无 ROS 依赖）
    power_node.py            # ROS2 节点
    power_board_cli.py       # 无 ROS 命令行工具
  launch/power_node.launch.py
  config/power_node.yaml
  tests/                     # 协议 + 固件仿真单测
```

## 依赖

- ROS2 Humble（RDK TROS 或容器），`hpm_gateway_msgs`（含新增 PowerStatus/PowerCommand）
- `python3-serial`（pyserial）：`pip install pyserial` 或 `apt install python3-serial`

## 构建与运行

```bash
# 1) 先编译消息包（PowerStatus/PowerCommand 已加入 hpm_gateway_msgs）
colcon build --packages-select hpm_gateway_msgs

# 2) 再安装本包（纯 Python，--symlink-install 便于开发）
colcon build --packages-select hpm_gateway_power --symlink-install
source install/setup.bash

# 3) 运行（串口按实际修改，RDK 上通常为 /dev/ttyUSB0）
ros2 launch hpm_gateway_power power_node.launch.py
# 或指定串口：
ros2 run hpm_gateway_power power_node --ros-args \
  -p serial_port:=/dev/ttyUSB0 -p status_period_ms:=1000

# 4) 观察遥测 / 发送命令
ros2 topic echo /hpm_gateway/power_status
ros2 topic pub --once /hpm_gateway/power_command hpm_gateway_msgs/msg/PowerCommand \
  "{channel: 7, enable: true, state_mask: 0}"
```

## 通道映射（以硬件/固件为准）

| CH | 通道 | 说明 |
| --- | --- | --- |
| 0..5 | 12V_1, 12V_2, 12V_FAN, 12V_4, 5V, 12V_3 | 低压 DC-DC |
| 6 | 48V_M4 | PA8 PWR_ON（低有效） |
| 7 | 48V_M1 | PB13 PWR_ON（低有效） |
| 8 | 48V_M2 | PB14 PWR_ON（低有效） |
| 9 | 48V_M3 | PB15 PWR_ON（低有效） |

> ⚠️ 协议文档初稿（comm_protocol_project/protocol-spec.md §6.1）的 48V 通道顺序
> 与硬件/固件不一致，本节点以硬件/固件为准（M4=ch6，M1=ch7，M2=ch8，M3=ch9）。

## CLI（无 ROS，bring-up 用）

```bash
# 查询状态
python3 hpm_gateway_power/power_board_cli.py --port /dev/ttyUSB0 status
# 开关单路 / 全部
python3 hpm_gateway_power/power_board_cli.py --port /dev/ttyUSB0 on M1
python3 hpm_gateway_power/power_board_cli.py --port /dev/ttyUSB0 off all
# 位图批量
python3 hpm_gateway_power/power_board_cli.py --port /dev/ttyUSB0 mask 0x03FF
# 周期监视
python3 hpm_gateway_power/power_board_cli.py --port /dev/ttyUSB0 monitor
```

## 测试

```bash
python3 -m unittest discover -s tests -v
```

详见 `docs/40-powerlink-power-control-node.md`。
