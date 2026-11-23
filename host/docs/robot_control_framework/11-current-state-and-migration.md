# 当前仓库基线与迁移边界

本文把目标架构与仓库事实对齐，避免设计文档把“计划”误写成“已实现”。路径以当前仓库为准，具体代码变更仍按 [10-roadmap.md](10-roadmap.md) 分阶段进行。

## 已有可复用能力

| 位置 | 当前事实 | 迁移用途 |
| --- | --- | --- |
| `host/hpm_gateway_ros2_ws/src/hpm_gateway_control` | ROS 回调与 `GatewayEcat` SOEM 周期线程分开；固定大小 PDO 快照；反馈约 200 Hz | 作为 `robot_control` 兼容 adapter 和 `robot_rt` 初始 backend |
| `host/.../gateway_ecat_api.cpp` | 1 kHz 绝对周期、WKC/timeout/deadline/drop 统计、可选 CPU/RT 优先级 | 提取为不依赖 ROS 的 RT 后端，保留统计语义 |
| `host/.../latest_buffer.hpp` | 固定大小、trivially-copyable snapshot 的 latest buffer | 作为 SHM/进程内快照原型；跨进程需补 schema、CRC 和进程间同步 |
| `firmware/ecat_tsn_gateway/protocols/rs06`、`encos` | 协议 codec 基本不访问硬件/ROS | 迁移到统一 codec registry，补版本/黄金向量/fuzz |
| `firmware/ecat_tsn_gateway/protocols/yesense` + `drivers/imu` | DMA/ring/parser/latest sample/统计；样本有 valid fields/timestamp | 作为传感器 driver 与 sample-age 契约模板 |
| `firmware/.../drivers/can` | MCAN 抽象、每总线 RX ring 和错误统计 | 适配 transport HAL；核对 Classic/FD 帧 ABI |
| `host/hpm_gateway_ros2_ws/src/hpm_gateway_power` | Python PowerLink 串口、心跳、有限重试、状态发布 | 保持非 RT lifecycle driver；通过服务/诊断接入 power domain |
| `host/hpm_gateway_ros2_ws/src/hpm_gateway_gamepad` | epoll/事件驱动手柄节点 | 作为 `command_mux` 的非 RT source，增加 deadman/lease |

## 需要冻结或修正的事实

1. 16 轴 PDO、对象字典、ESI/XML、EEPROM 镜像和主站布局是一个版本化契约；任何字段/长度变化都要递增 revision/hash 并跑布局测试。
2. `motor_config.c` 的 bus、CAN ID、协议和 enabled 状态目前是静态产品事实，不能继续让不同文档/宏各自描述；迁移后由 manifest 编译生成唯一表。
3. 当前 `gateway_motor_pdo.h` 的帧结构是固定大小、16 轴导向的兼容 ABI；新 canonical 模型不能直接把这个数组布局当作跨版本接口。
4. `can_if_frame_t` 及相关 buffer 是否支持 CAN-FD 必须以源码和目标驱动为准；如果仍只有 8 字节，升级必须新 ABI、DLC 映射和容量测试，不能仅打开枚举。
5. 文档曾出现 CAN7/单轴和 CAN0/CAN1 多轴等不同实验配置；发布时只允许由配置 fingerprint、固件 build profile 和启动报告确定当前事实。
6. EtherCAT 主站库 SOEM 当前许可证为 GPLv3/商业双授权；商业分发路径未决前，不把它写成无条件的生产依赖。

## 已确认的实现缺口（不是目标架构能力）

以下行为在迁移前必须通过测试固定，不能从目标设计倒推为当前已具备：

- `LatestBuffer::publish()` 在槽位满时返回 `false`，并不自动覆盖旧值；命令覆盖/丢弃语义需要在 IPC v1 中明确并计数。
- 当前 ROS 电机反馈映射将部分状态字段置为默认值（例如 `status=0`）；新 canonical state 必须保留真实 status/fault/quality 和来源时间。
- `CAN_IF_FRAME_FD` 枚举存在，当前发送路径仍固定 Classic CAN、最大 8 字节；CAN-FD 不能仅通过打开枚举启用。
- WKC、总线和队列错误已有计数，但完整的 `RECOVERING`/锁存故障/人工 re-arm 状态机仍属于目标能力。
- 当前 EtherCAT 产物中存在 320 字节输出与 332 字节诊断填充的不同生成物；发布前必须选定唯一 ESI/PDO artifact，并让主站长度/hash 校验与其一致。

## 迁移原则

- **单一写入者**：影子阶段新路径只读/计算，不发运动帧；切换窗口先撤销旧 lease，再启用新 writer。
- **兼容优先**：保留旧 topic/launch/PDO adapter 至至少一个发布周期；新 canonical 接口通过 remap/bridge 过渡。
- **静态实时表**：YAML/JSON、pluginlib、设备发现和 PDO 解析在非 RT configure 阶段完成，RT 只消费 compiled profile。
- **可回滚**：每一步保留旧 binary/profile、旧消息映射和安全停机入口；设备持久参数单独备份，主机回滚不能假设设备自动回滚。
- **证据先行**：每次切换绑定版本、配置 hash、HIL/性能/故障测试和 raw trace；未测量的“更快”不进入发布说明。

## 建议的迁移映射

```text
现有 GatewayEcat + LatestBuffer
        -> robot_rt_backend + robot_ipc
现有 motor_config / protocol switch
        -> compiled device table + protocol registry
现有 hpm_gateway_control ROS node
        -> robot_hardware SystemInterface + compatibility bridge
现有 hpm_gateway_msgs
        -> robot_interfaces/v1（旧包保留 adapter）
现有 power/gamepad Python/C++ 节点
        -> robot_app/ops sources -> command_mux/diagnostics
```

## 迁移前检查清单

- [ ] 固件 PDO 输入/输出长度、ESI RevisionNo、主站期望长度和 layout hash 一致。
- [ ] 每个设备有唯一 `device_uid`、semantic name、endpoint 和协议/固件兼容范围。
- [ ] 每条 CAN 总线的真实帧数、位填充、反馈率、管理流量和 70% admission 结果已记录。
- [ ] RT 周期无阻塞发送、动态分配、日志、DDS 和无界重试；page fault/IRQ/jitter 已测。
- [ ] 新旧 writer、ROS topic、QoS、namespace 和 arm 权限不存在重叠。
- [ ] 断网、RT 进程退出、bus-off、反馈 stale、非法命令和升级失败的安全结果已在 HIL 复现。
- [ ] SOEM/ROS/RMW/内核/容器许可证、SBOM、CVE 和签名流程已由负责人批准。
