# 测试与验证策略

## 验证原则

构建成功或 `ros2 topic echo` 能看到数据，不足以证明实时、安全或协议正确。每个需求都要有实现位置、可观察验收、测试命令/环境和原始证据；目标硬件、固件、配置、内核、RMW 和协议 hash 写入报告。

## 测试分层

| 层级 | 目标 | 典型内容 | 运行位置 |
| --- | --- | --- | --- |
| 静态/schema | 提前拒绝错误配置 | JSON Schema、单位/范围、endpoint 冲突、预算 | CI/开发机 |
| codec 单测 | 固定线缆契约 | golden encode/decode、端序、CRC、边界和错误码 | host CI、交叉编译 |
| property/fuzz | 找解析器异常 | 短帧、随机 DLC、保留位、NaN/Inf、乱序和重复 | ASan/UBSan/fuzz runner |
| virtual transport | 验证状态机 | 延迟、丢帧、bus-off、设备重启、时钟漂移 | SIL |
| ROS 契约 | 固定接口/QoS | IDL、QoS 匹配、lifecycle、resource claim、action cancel | ROS 2 CI |
| 组件集成 | 验证 IPC/driver | `robot_rt` + virtual/SocketCAN vcan + SystemInterface | Ubuntu target |
| HIL | 验证真实时序/电气 | 单轴、逐总线、全配置、EMI/掉电/重连 | 目标板/驱动器 |
| 系统/E2E | 验证用户路径 | 轨迹、遥控、传感器融合、录包回放、升级 | 整机 |
| 性能/长稳 | 证明尾延迟和资源 | WCET、jitter、总线占用、1h soak、干扰负载 | PREEMPT_RT |
| 安全/恢复 | 证明 fail-action | 断网、进程杀死、权限、重放、非法命令、E-stop | HIL/整机 |

## 协议/设备验收清单

- 协议 spec、wire major、schema hash、固件兼容矩阵和抓包 provenance 已审核。
- encode/decode 双向 golden vectors 与旧版本兼容/拒绝行为通过。
- CRC/长度/ID/DLC/范围/finite/保留位和 consumed length 的失败路径有断言。
- virtual transport 和 vcan/loopback 覆盖丢帧、重复、乱序、延迟、bus-off、复位、掉电。
- 单设备 read-only probe、低速 canary、逐总线和全设备 HIL 都验证方向、单位、校准、限位和安全停机。
- 新设备的 `device_uid`、endpoint、能力、周期、资源 claim、诊断和 ROS mapping 无隐式数组下标。

## 实时与总线验收

固定环境后运行空载、额定、干扰、故障和长稳五组测试，记录：

```text
cycle WCET: p50/p95/p99/p99.9/max
release jitter、deadline miss/streak、RT CPU/stack/page fault
bus utilization、WKC/CAN error/bus-off、TX/RX/drop/retry
command age/expiry、feedback age/sequence gap、IPC queue watermark
```

初始门槛见 [00-project-brief.md](00-project-brief.md)。任何新的协议/设备/内核/RMW/编译器或录包配置改变都必须重新跑 admission 和性能测试。

## 故障注入矩阵

| 注入 | 期望结果 | 必须保留的证据 |
| --- | --- | --- |
| ROS/DDS 断开 | 本地 command timeout，进入 SAFE/DEGRADED | command age、RT event、硬件状态 |
| `robot_rt` kill/卡死 | MCU/hardware watchdog 切断输出 | watchdog reason、boot reason、trace |
| IPC CRC/schema 错 | 丢包，不执行未知数据 | CRC/drop counter、错误码 |
| CAN bus-off/EtherCAT WKC 错 | 受影响轴停止，不自动 re-arm | 总线错误帧、状态转换 |
| 反馈 stale/NaN/越界 | 控制器拒绝/停用，发布诊断 | quality、age、fault event |
| 重复/乱序/重放命令 | sequence/lease 拒绝 | source、sequence、审计 |
| 设备断电/重启 | SAFE -> probe -> ready，不能自动 ACTIVE | 生命周期和版本指纹 |
| 磁盘满/录包失败 | 只影响 ops，控制继续 | recorder error、RT 无 deadline miss |
| 时钟跳变/PTP 丢失 | watchdog 使用单调时钟，告警并按策略降级 | clock offset、event |

## CI/CD 与证据归档

CI 至少包含格式、编译（Humble/Jazzy profile）、静态分析、单测、fuzz smoke、ASan/UBSan、ROS interface/QoS 契约、依赖/许可证和密钥扫描。目标板流水线再执行 HIL、实时和长稳测试；不把主机模拟结果冒充硬件通过。

每次发布归档：源码/容器 digest、编译器、内核、ROS/RMW、配置和 protocol lock hash、设备固件 ID、测试命令、原始 metrics/trace/capture、失败与偏差说明、批准人和回滚 slot。
