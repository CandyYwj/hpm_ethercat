# Ubuntu 机器人控制框架

这是面向 Ubuntu 主机的机器人控制框架设计基线。它把硬实时设备控制、ROS 2 集成、应用算法和运维观测拆成明确的边界，目标是让系统在扩展总线、协议、执行器和传感器时不需要反复修改核心控制循环。

文档状态：**提案（可评审）**。本目录只描述目标架构和迁移路径，不替换现有 `host/hpm_gateway_ros2_ws` 或固件实现。所有性能数字是首版预算，必须在目标硬件上测量后才能升级为发布门槛。

## 先看什么

| 读者 | 入口 |
| --- | --- |
| 未来总体架构图 | [13-future-architecture-diagram.md](13-future-architecture-diagram.md) |
| 架构评审 | [01-architecture.md](01-architecture.md)、[adr/](adr/) |
| 新增协议/执行器/传感器 | [03-device-model-and-extension.md](03-device-model-and-extension.md)、[04-protocol-extension.md](04-protocol-extension.md) |
| ROS 2 开发 | [05-ros2-integration.md](05-ros2-integration.md) |
| 接口、消息与错误码 | [12-interface-contracts.md](12-interface-contracts.md) |
| 实时性能 | [02-runtime-and-performance.md](02-runtime-and-performance.md) |
| 调试与线上运维 | [06-observability-and-debugging.md](06-observability-and-debugging.md)、[08-deployment-and-operations.md](08-deployment-and-operations.md) |
| 发布验收 | [07-safety-reliability-security.md](07-safety-reliability-security.md)、[09-testing-and-validation.md](09-testing-and-validation.md) |
| 项目计划 | [10-roadmap.md](10-roadmap.md) |
| 当前代码事实与迁移边界 | [11-current-state-and-migration.md](11-current-state-and-migration.md) |
| 组件调研与选型依据 | [research.md](research.md) |
| 新协议/设备提交模板 | [templates/](templates/) |

## 一页决策

1. `robot_rt` 是不依赖 DDS 的硬实时进程（或等价的 MCU/RTOS 运行时）。它拥有总线、设备驱动、周期调度和最后一道安全检查。
2. `robot_control` 是 ROS 2 控制平面。它使用 `ros2_control` 的标准硬件/控制器接口，但通过固定大小共享内存快照访问 `robot_rt`，不在 `read()`/`write()` 中直接操作 EtherCAT、CAN 或串口。
3. 规划、感知、遥控、诊断、录包和升级属于普通 Linux/ROS 2 应用平面。ROS 2 断开时，硬件仍由本地看门狗进入安全状态。
4. 协议分为 Transport、Framing、Codec、Device Profile、Driver 五层；新增协议先写规范和纯编解码器，再接入设备驱动，不把私有帧暴露给业务节点。
5. 设备由稳定的 `device_uid` 和语义名称标识，不能继续依赖“数组下标 = CAN ID”这类隐式契约。
6. 可观察性是接口的一部分：每个周期、队列、总线、设备和协议都输出计数器、时间戳、质量位和故障原因；高频路径不打印、不分配、不阻塞。

## 推荐基线

- 生产主机：Ubuntu 24.04 LTS、ROS 2 Jazzy LTS、PREEMPT_RT 内核；C++20 用于新的 host 核心，固件/兼容边界保留 C ABI。
- 兼容主机：Ubuntu 22.04 + Humble 仅作为现有 RDK/TROS 的适配 profile，公共 IDL 和设备描述不得依赖某个发行版特性。
- RMW：部署时固定一种实现并做目标板基准；默认评估 Cyclone DDS，若供应商栈要求 Fast DDS 则使用 Fast DDS profile。RMW 选择不改变实时平面的接口。
- EtherCAT：短期可通过现有 SOEM 适配器接入；SOEM 当前仓库为 GPLv3/商业双授权，商业产品在发布前必须完成许可证审查或替换为合规后端。

## 文档原则

- 事实、决定、假设和待验证项分开写。
- 所有“高性能”描述都绑定工作负载、p99/p99.9、资源和测量方法。
- 安全停机、急停、限位和看门狗不依赖 ROS 话题、服务或 DDS discovery。
- 配置在启动前解析和校验，实时线程只消费生成的静态表。
- 采用成熟标准（`ros2_control`、标准 ROS 消息、SocketCAN、systemd、ros2_tracing），自研内容集中在设备模型、实时 IPC 和项目特有协议适配。

## 目录约定（目标仓库布局）

```text
host/
  robot_framework/
    rt/                 # robot_rt：周期调度、安全、设备运行时
    ipc/                # 固定大小共享内存/UDS 契约
    transports/         # EtherCAT、SocketCAN、串口、UDP 等
    protocols/          # 规范、codec、设备 profile、注册锁
    devices/             # 执行器/传感器 driver
    ros2/
      robot_interfaces/  # 版本化消息/服务/action
      robot_hardware/    # ros2_control SystemInterface facade
      robot_bringup/     # launch、参数、URDF、生命周期编排
      robot_diagnostics/  # 诊断、指标、trace 导出
    sim/                  # virtual transport、SIL/HIL 夹具
    tools/                # robotctl、回放、协议分析工具
  docs/robot_control_framework/
```

目标目录是演进方向，不要求本次立即创建全部代码；迁移按 [10-roadmap.md](10-roadmap.md) 的纵向切片进行。
