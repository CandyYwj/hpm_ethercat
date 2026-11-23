# 失力排查轻量方案（topic_monitor 话题观测）

- 文档编号：39
- 日期：2026-08-24
- 状态：`implemented`（轻量版）
- 关联调查：`debug_project/cases/all-motors-force-loss/`
- 说明：本页取代早期复杂架构草案（v2/v3 的 EventRing、monitor 节点、消息扩展等均已废弃）。

## 1. 背景与目标

四足运行中**所有已使能电机突然同时失去力矩（失力）**。本方案用**独立只读的话题观测**
落盘每个话题的**频率 + 内容摘要**，不改任何现有代码/消息/PDO，即可在日志里看到：
命令/反馈/手柄哪个先停发、电机扭矩是否全零，从而定位失力根因。

## 2. 核心工具：`scripts/topic_monitor.py`

一个独立的 ROS2 Python 观测者：

- 只订阅现有话题，**不发布任何数据**，失败不影响被观测节点；
- 每秒把每个话题的**频率（Hz）+ 最近内容摘要**写入 CSV；
- 自动通过话题图解析消息类型（任意话题都能监听，复用性强）；
- 话题从“有数据”骤降到 0 时写一行 `!DROPPED <topic>` 标记；
- 内容摘要：`motor_feedback`→全部 16 轴 pos/vel/torque、`imu`→加速度、`joy`→摇杆、`motor_command`→命令值。

## 3. 使用

```bash
# RDK（需 source 工作区）
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash

python3 scripts/topic_monitor.py \
  --topics /hpm_gateway/motor_feedback@best_effort /imu/data@best_effort \
           /hpm_gateway/motor_commands /hpm_gateway/motor_command /gamepad/joy \
  --log /var/log/hpm_gateway/topic_monitor.csv --interval 1
```

> feedback/imu 是 best_effort，必须用 `@best_effort`；其余默认 reliable。

输出示例：

```text
time, /hpm_gateway/motor_feedback_hz, /hpm_gateway/motor_feedback_last, ...
2026-08-24 11:30:00, 200.0, pos[0.100 0.200 ...] vel[...] t[0.010 0.020 ...], ...
2026-08-24 11:30:01, 200.0, pos[...] vel[...] t[0.000 0.000 ...], ...   ← 扭矩全零
```

## 4. 失力排查方法

失力后看 CSV，用 4 个信号定位根因：

| 信号 | 现象 | 指向 |
| --- | --- | --- |
| 命令话题 `_hz` 归零 | `/motor_commands`/`/motor_command` 停发 | H1 命令源停发（含 DDS 丢帧） |
| 手柄 `_hz` 归零 | `/gamepad/joy` 停发，js0/xboxdrv 异常 | H2 手柄链路 |
| 反馈 `_hz` 归零 | `/motor_feedback` 停发 | H3 EtherCAT 链路 / H5 网关复位 |
| 反馈 `t[...]` 全零 | 频率正常但扭矩全零（`t[0.000 ...]`） | H6 电源/驱动共因（或电机未使能） |

时间轴对齐：`torque_nonzero=NONE` 出现时刻与命令 `_hz` 归零时刻相差约 100 ms → 命令源先停（H1/H2）；
若扭矩归零但所有 `_hz` 正常 → 指向网关/电源（H5/H6）。

## 5. 可选补充手段（不改 PDO）

- **bag 取证**：诊断期 `ros2 bag record`（feedback/imu 用 QoS override）完整回放因果链。
- **固件 UART**：失力时人工记录固件 `timeout_stop_count` / 复位日志，与 CSV 时间对齐，确认 H5。
- **系统层**：journald 持久化 + sysstat(`sar`) 提供 CPU/网络历史，排查 H4 抖动。

## 6. 约束

- **不改 PDO、不改现有代码/消息**；观测只读、低开销；
- 只适用于“能通过话题数据判断”的排查；需要 host 内部计数（command_timeouts 等）时属后续增强，另行立项。
