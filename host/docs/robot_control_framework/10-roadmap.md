# 实施路线图与迁移计划

路线图按可验收纵向切片推进，不先批量创建空接口。日期是相对里程碑，具体日历由团队和硬件到位时间确定。

## M0：契约冻结与基线（2--3 周）

**结果**：可以重复构建和测量当前系统。

- 冻结现有 EtherCAT PDO/ESI、CAN/IMU/PowerLink 事实和 `hpm_gateway_msgs` 兼容范围。
- 建立设备/协议 ID、单位、时间戳、错误码、QoS 和 namespace 规范。
- 在目标 RDK/Ubuntu 上测量当前 `GatewayEcat` 周期、CAN TX FIFO 满载/丢帧语义、WKC、命令 age 和资源基线。
- 产出 schema 初版、性能报告、SOEM 许可证决定和风险登记表。

**退出条件**：有额定负载和故障基线；每个未决硬件/安全问题有 owner 和截止条件。

## M1：实时核心与 IPC（3--5 周）

**结果**：`robot_rt` 能在 virtual transport 和现有 EtherCAT 后端上运行。

- 从 `GatewayEcat` 提取不依赖 ROS 的 backend、固定快照、命令 watchdog、WKC/周期统计。
- 实现 SHM 双缓冲/SPSC、schema/sequence/deadline/CRC 和有界队列策略；UDS 只作控制面备用。
- 固化 CAN TX 非阻塞 FIFO 的容量、满载策略和重试语义；必要时补充固定容量软件队列并完成 admission。
- 实现 SAFE/READY/ACTIVE/FAULT 最小状态机和 flight recorder。

**退出条件**：virtual + HIL 单轴通过；RT 不分配/不打印；目标周期门槛有原始 trace。

## M2：设备描述与协议注册（3--5 周）

**结果**：新增一个协议或设备不修改 scheduler 核心。

- 落地 `spec.yaml`、JSON Schema、compiled profile、protocol lock 和注册器。
- 把 RS06、ENCOS、YESENSE 迁移为 codec/profile；补 golden/fuzz/property 测试。
- 生成 canonical device table、ROS mapping、诊断枚举和预算报告。
- 建立 virtual device、回放和 read-only probe 工具。

**退出条件**：至少一种新设备完成“描述 -> codec -> driver -> virtual/HIL -> diagnostic”的全流程；未知/不兼容设备进入 QUARANTINED。

## M3：ROS 2/ros2_control facade（3--5 周）

**结果**：控制器使用标准接口，旧路径可一键回退。

- 新建 `robot_interfaces`、`robot_hardware`、`command_mux` 和 lifecycle bringup。
- `SystemInterface` 只交换 IPC 快照；接入 `joint_state_broadcaster`、forward/trajectory controller。
- 旧 `hpm_gateway_control` 作为兼容 adapter；影子运行比较命令、反馈、序号和超时。
- 集中 QoS/RMW 配置，完成 Humble/Jazzy 构建和跨机测试。

**退出条件**：单一写入者、资源 claim/模式切换和命令租约通过；新旧路径可安全回滚。

## M4：安全、观测与运维（2--4 周）

**结果**：出现故障时可安全停机并定位原因。

- 接入硬件/MCU watchdog、SROS2、systemd capabilities、签名配置和 A/B 发布。
- `/diagnostics`、health、metrics、ros2_tracing、flight recorder、MCAP 录包和 `robotctl`。
- 故障注入、恢复演练、权限/重放测试和告警阈值固化。

**退出条件**：断网、进程退出、bus-off、反馈 stale、非法命令和升级失败都达到预期 fail-action，证据可归档。

## M5：生产门禁与扩展（持续）

**结果**：按机器人 profile 发布和升级。

- 全轴/全传感器 HIL、1 小时以上长稳、温度/EMI/掉电和现场回滚。
- 删除已迁移旧路径前保留至少一个版本的兼容桥和 N-1 profile。
- 将新协议、执行器、传感器以 RFC/codec/profile/测试/观测清单纳入评审。

**发布门槛**：所有质量预算有目标硬件证据；无未解释 deadline miss、过期命令执行、配置 drift 或安全状态越权。

## 责任边界

| 角色 | 负责 | 不负责 |
| --- | --- | --- |
| RT/驱动 | 周期、codec、总线、watchdog、HIL | ROS 应用业务和 UI |
| ROS/控制 | canonical 接口、controller、mux、lifecycle、QoS | 硬件急停和总线 raw 访问 |
| 应用/算法 | 规划、感知、任务、遥控 | 绕过 mux 直接发帧 |
| 运维/发布 | 签名、部署、指标、回滚、审计 | 修改运行中的 RT 表 |
| 安全/硬件 | STO、急停、限位、电源保护、认证 | 仅凭软件日志判定安全合规 |

## 重新评估触发器

- 需要 >2 kHz、亚 100 us 或多核同步而 Linux RT 无法满足；
- 设备/总线规模使 admission >70% 或 IPC/controller manager 成为主要瓶颈；
- 供应商强制新的 ROS/RMW/内核，或 Humble 兼容期结束；
- 引入安全 PLC、冗余执行器、无线闭环或跨机同步控制；
- SOEM 许可证、维护或 EtherCAT 功能不再满足产品要求。
