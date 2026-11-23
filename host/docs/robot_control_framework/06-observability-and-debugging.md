# 可观察性与调试设计

## 原则

可观察性契约与设备 API 同等重要。任何“设备不动、反馈过期、周期超时或命令无效”的问题，都应能回答：命令来自谁、何时进入系统、在哪个周期被接受/拒绝、发出了什么线缆帧、收到了什么反馈、最后一次安全决策是什么。

RT 周期不打印、不格式化字符串、不写磁盘。RT 只写固定宽度事件和原子计数，非 RT 导出日志、指标、trace 和录包。

## 统一事件与关联字段

所有跨边界记录至少包含：

```text
robot_id, device_uid, component, event_id, severity
monotonic_ns, hardware_timestamp_ns?, ros_timestamp?
sequence, trace_id, source_id, protocol_id, protocol_version
config_fingerprint, firmware_id, reason_code
```

日志采用结构化 key/value（journald JSON 或等价格式），敏感参数/令牌过滤。事件 ID 使用稳定枚举，例如 `RT_DEADLINE_MISS`、`IPC_CRC_ERROR`、`CAN_BUS_OFF`、`DEVICE_FEEDBACK_STALE`；不要依赖可变的中文日志文本做告警。

## 指标

指标名称和标签有限且可控，禁止把 CAN ID、序号等高基数字段直接当 label。建议最小集合：

| 类别 | 指标示例 | 关键标签 |
| --- | --- | --- |
| RT | `robot_rt_cycle_duration_seconds`、`robot_rt_cycle_deadline_misses_total`、`robot_rt_cycle_lateness_seconds` | robot, profile |
| IPC | `robot_ipc_command_age_seconds`、`robot_ipc_crc_errors_total`、`robot_ipc_dropped_total` | direction, channel |
| 总线 | `robot_bus_tx_total`、`robot_bus_rx_total`、`robot_bus_errors_total`、`robot_bus_utilization_ratio` | bus_kind, bus_name |
| 设备 | `robot_device_feedback_age_seconds`、`robot_device_state`、`robot_device_faults_total` | device_class, logical_group |
| ROS | `robot_ros_qos_deadline_missed_total`、`robot_ros_subscription_dropped_total` | node, topic_group |
| 资源 | `robot_process_cpu_ratio`、`robot_process_rss_bytes`、`robot_temperature_celsius` | process, cpu |

每个指标定义单位、采样周期、保留时间和告警阈值。RT 计数器使用无锁原子；直方图在非 RT 线程批量消费。

## 健康模型

统一健康状态：

```text
UNKNOWN -> PROBING -> READY -> ACTIVE
                         \-> DEGRADED -> RECOVERING -> READY
                         \-> FAULT -> SAFE_STOP/OFFLINE
```

`/robot/<robot_id>/health` 发布当前状态、原因、最近转换时间、数据新鲜度和版本指纹；`/diagnostics` 使用 `diagnostic_msgs/DiagnosticArray` 提供人可读摘要。状态与故障事件分开：状态允许新订阅者读取当前值，故障事件带 `event_id/sequence` 且不会被 transient-local 运动命令语义混淆。

## Trace 与飞行记录器

### 端到端 trace

一个命令贯穿：

```text
ROS ingress -> command_mux -> IPC commit -> RT cycle accept
  -> protocol encode -> bus tx -> device rx
  -> protocol decode -> state snapshot -> ROS publish
```

每个节点只传递已有 `trace_id` 和 `sequence`，不在热路径生成 UUID。ROS 边界使用 `ros2_tracing`/LTTng；RT 使用固定容量 binary flight recorder，记录最近 N 秒的周期、事件、命令摘要和帧元数据（默认不记录敏感 payload）。故障事件触发异步 dump，dump 失败不能影响停机。

### 原始帧与回放

总线 raw frame 采集在非 RT worker 完成，带 interface、方向、硬件时间、过滤规则和配置 hash。录包格式优先 MCAP/rosbag2；原始帧另存带版本的 capture 文件。`robotctl replay` 可把 capture 注入 virtual transport，复现 codec、调度和状态机，而不需要真实执行器。

## 调试工具与运行手册

建议提供一个只读优先的 `robotctl`：

```text
robotctl doctor                 # 内核、权限、RMW、网卡、时钟、配置锁
robotctl status                 # 生命周期、设备健康、总线计数、命令 age
robotctl trace dump --last 10s  # 导出 flight recorder（需授权）
robotctl bus capture --bus can0 # 受控 raw capture
robotctl device identify <uid>  # 只读探测
robotctl device recover <uid>   # 显式、审计、带超时的恢复
```

运动/arm/固件升级等危险动作必须要求 operator 身份、设备 allowlist 和当前安全状态；CLI 不绕过 `command_mux` 或硬件安全链路。

最小排障顺序：

1. `robotctl doctor/status`：确认配置指纹、生命周期、权限、时钟和总线链路。
2. 检查 command age、source lease、validity、QoS 连接和 IPC CRC/drop。
3. 检查 RT deadline/WCET、WKC/CAN error/bus-off、设备 feedback age 和 fault bits。
4. 导出 trace + raw capture，在 virtual transport 回放；再做单设备、低速 HIL。
5. 变更只能通过版本化 profile/feature flag 发布，保留一键停机和回滚。

## 告警规则示例

| 告警 | 条件（示例） | 动作 |
| --- | --- | --- |
| RTDeadlineMiss | 连续 3 次或 1 分钟内 > 0 | 撤销运动 lease，进入 DEGRADED/SAFE_STOP，保留 trace |
| FeedbackStale | 必需设备 age > 2 周期 | 停止依赖该反馈的控制器，发布原因 |
| BusOff | 任一 CAN bus-off | 停止该 bus 轴，不自动 re-arm，要求显式恢复 |
| QueueDrop | command/event drop > 0 | 提高诊断级别并记录 source/channel |
| ConfigDrift | 运行 fingerprint 与锁不符 | 禁止 ACTIVE，进入 QUARANTINED |
