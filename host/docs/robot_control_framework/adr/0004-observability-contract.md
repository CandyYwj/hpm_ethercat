# ADR-0004：把 trace、metrics、diagnostics 作为设备接口的一部分

- 状态：提议
- 日期：2026-08-31
- 范围：RT、ROS 2、总线和设备运维

## 背景

“电机不动”可能来自命令过期、QoS 不匹配、IPC 丢包、周期超时、WKC/CAN 错误、设备 fault、单位/限位或电源问题。只看 ROS 日志无法建立端到端因果，且在 RT 周期打印会破坏时序。

## 决策

定义统一事件/指标字段（robot/device/component/event/time/sequence/trace/config/firmware/reason），RT 只写原子计数和固定容量 flight recorder；非 RT 用 `diagnostic_msgs`、结构化 journald、Prometheus/OpenTelemetry 适配、`ros2_tracing`/LTTng 和 MCAP/raw capture 导出。命令从 ROS ingress 到 wire frame/feedback 贯穿 `trace_id + sequence`，不在热路径生成字符串或 UUID。

## 后果

故障可回放、指标可告警、ops 可独立重启；需要控制 label cardinality、录包磁盘配额、敏感数据脱敏和长期 schema 兼容。观测失败不得反压 RT；flight recorder dump 失败仍必须完成停机。

## 验证

注入 QoS 断连、IPC CRC、bus-off、deadline miss、反馈 stale、磁盘满和进程退出，检查安全动作与事件/trace 是否都存在；用 capture 回放复现同一状态机结果。
