# ADR-0002：ROS 2 与 ros2_control 的角色及实时边界

| 项目 | 内容 |
| --- | --- |
| 状态 | **提议** |
| 日期 | 2026-08-31 |
| 决策人 | 机器人平台团队（待评审） |
| 范围 | Ubuntu 主机、RDK/TROS 边缘控制机上的控制与集成运行时 |
| 关联设计 | [`../05-ros2-integration.md`](../05-ros2-integration.md) |

## 背景

系统同时需要高频电机/总线控制、可调试性、可观察性、跨机协作和持续扩展。当前
工程已经有独立的 SOEM 周期线程（`GatewayEcat`）和 ROS 2 节点，但命令、反馈、
串口电源和 gamepad 仍由不同私有接口连接。若把所有设备和控制周期直接放入 DDS，
网络发现、序列化、executor 调度和应用回调会进入安全关键路径；若完全绕过 ROS 2，
又会失去标准控制器、工具链、录包和多机集成能力。

## 决策

### 1. ROS 2 是控制平面，不是唯一的安全硬实时数据面

ROS 2 负责任务、规划、感知、UI、参数、生命周期、诊断、记录、多机协作和规范化
控制接口。新增节点可使用标准消息和工具，但不能让 DDS 断网、发现抖动或应用回调
决定物理急停。

`robot_rt` 实时核心负责 EtherCAT/CAN/串口周期、协议 codec、限幅、命令 watchdog、
降级和与 MCU/驱动器的安全交互。它不依赖 ROS 2/DDS，使用固定内存和锁自由快照
与 ROS 控制域交换：

```text
应用/规划/工具 -- DDS --> ROS 2 控制域 -- 有界 IPC --> robot_rt -- 总线 --> 设备
       （可观察、可重启）                       （安全边界）  （独立 watchdog）
```

1 kHz（或更快）周期内禁止 DDS、参数/服务、磁盘 I/O、动态内存和无界日志。硬件急停、
扭矩切断和断线后的停机必须由硬件、固件或 `robot_rt` 独立完成。

### 2. 采用 ros2_control，但约束插件的实时行为

以总线/网关为粒度实现 `hardware_interface::SystemInterface`，向控制器提供标准
`position`、`velocity`、`effort` 状态/命令接口，并按需增加带单位和范围的 MIT
扩展（如 `kp`、`kd`、`feedforward_effort`）。设备发现、PDO 映射、pluginlib 加载、
模式切换和参数解析在 `on_init`/`on_configure` 完成；激活后 `read()`/`write()` 只做
固定大小复制、序号检查和快照交换，不等待 DDS、不分配、不阻塞、不格式化日志。

现有 SOEM 实现先作为后端保留，经过影子运行、实时测量和 HIL 后再让
`controller_manager` 成为唯一写入者。这样可复用标准控制器、资源 claim、轨迹
action 和状态广播，同时保留当前 1 kHz 周期、WKC 统计和命令超时清零语义。

### 3. 发行版兼容策略

- **当前兼容基线：ROS 2 Humble/TROS Humble。** 过渡包、消息和 launch 先在 RDK
  环境构建，不要求立即升级设备系统。
- **目标基线：Ubuntu 24.04 + ROS 2 Jazzy LTS。** 新功能先保持接口层兼容，再在
  Jazzy 目标机验证 RMW、QoS、插件 ABI、实时和安全配置。
- 生产制品固定 ROS 发行版、RMW 实现、DDS XML、接口版本和插件 hash；“能发现 topic”
  不视为跨版本兼容证明。一个部署优先统一一个 RMW，供应商默认实现须先测量。

## 被否决的替代方案

### A. 所有控制都通过 ROS 2 topic

DDS 的发现、网络拥塞、序列化、executor 调度和可靠队列没有足够的端到端最坏时延
保证；远端网络断开时也无法独立执行安全动作。ROS 2 仍可承载低频目标和轨迹。

### B. 完全绕过 ROS 2 的自研框架

需要重复实现资源声明、控制器生命周期、轨迹 action、标准消息、录包和工具生态，
长期维护成本高，也阻碍第三方算法接入。将自研部分收敛到实时后端更易维护。

### C. 立即把 SOEM 线程改写成 controller manager 插件

会同时改变调度、ABI、PDO/ESI、timeout、WKC 和故障语义，难以区分回归。应先抽取
后端、影子比较、单一写入者切换，再删除旧路径。

## 关键约束

1. ROS lifecycle 仅表示节点资源状态；`ARMED/RUNNING/FAULT` 由安全状态机和实时
   后端共同确认。DDS 重连不能自动重新上扭矩。
2. 本机高频命令默认 `KeepLast(1) + BestEffort + Volatile`，带 `sequence`、来源、
   有效期、deadline/lifespan；arm/stop 使用有权限的 service/action，但物理停机不
   依赖它。
3. 高频反馈使用 best-effort 最新样本；静态描述/能力使用 reliable + transient local；
   QoS profile 集中管理并进行 publisher/subscriber 匹配测试。
4. 远端 PC 不发送持续高频扭矩流，发送轨迹或低频目标，由 RDK 本地插值、限幅和
   watchdog；每台机器人使用独立 domain、namespace 和权限。
5. `robot_rt` 与 controller manager 之间使用有界共享内存/Unix socket 快照，槽位
   带 `schema_version`、`robot_id`、序号、单调时间、`valid_until` 和状态/CRC；槽位
   不可读或过期时进入安全降级。

## 后果

### 正面

- 保留现有 SOEM 1 kHz 路径和固件安全语义，同时获得 ROS 2/ros2_control 生态；
- 实时故障与 DDS/应用故障隔离，诊断和录包可独立重启；
- 新总线、执行器和传感器可通过 codec、硬件插件和标准消息扩展，不必修改规划器；
- Humble/TROS 与 Jazzy 主机可分阶段迁移，并用影子模式比较新旧输出。

### 代价与剩余风险

- 需要维护 IPC schema、版本兼容和两个运行时，跨进程复制会增加少量延迟；
- ros2_control 控制器是否实时取决于每个控制器实现，框架名称本身不构成保证；
- RMW/DDS、Docker 权限、PTP 时钟和网卡/IRQ 配置仍需在目标硬件测量；
- 迁移窗口内新旧 topic/PDO/消息并存，必须防止多个写入者同时 arm。

## 迁移与回滚

1. 冻结当前 PDO/ESI 和 `hpm_gateway_msgs` 契约，建立 `robot_interfaces`、QoS profile、
   trace/metrics 和兼容 topic。
2. 抽取不依赖 ROS 的 `robot_rt_backend`，保留 `GatewayEcat` 的 latest snapshot、
   command timeout、WKC 和统计行为；在 RDK 上测量周期预算。
3. 实现总线级 `SystemInterface`，先只读/影子运行；通过 HIL 和故障注入后，在停机
   窗口切换唯一写入者，旧 launch 保留为回滚入口。
4. 在 Ubuntu 24.04/Jazzy 完成构建、QoS、跨机、安全和实时验证后再移除旧私有 topic。
   任一未解释 deadline miss、过期命令执行、状态机越权或新旧输出不一致都触发回滚；
   回滚后必须人工重新 arm。

## 复审触发条件

- 控制周期需要在 MCU 内闭合，或独立 `robot_rt` 已无法满足硬实时预算；
- 供应商 TROS/Jazzy 对 RMW、ros2_control 或实时内核给出新的受支持约束；
- 引入安全认证、冗余监控或安全 PLC，导致系统安全边界改变；
- 多机器人规模使 DDS discovery/路由超出预算，需要专用数据分发层；
- 测量表明 IPC 或 controller manager 成为控制预算的主要瓶颈。
