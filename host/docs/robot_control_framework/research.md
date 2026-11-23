# 技术调研与方案评估

## 调研范围

调研日期：2026-08-31。目标是为 Ubuntu 主机上的高性能、可调试、可观察机器人控制框架选择可替换的成熟组件，而不是证明某个组件在所有硬件上都“最优”。性能结论必须在目标 RDK/网卡/设备数量上复测。

约束：硬实时总线和安全动作不能依赖 DDS；当前仓库已有 SOEM、HPM FreeRTOS、ROS 2 Humble/TROS、CAN/IMU/PowerLink 实现；需要渐进迁移、离线能力、协议/设备扩展和可追溯运维。

## 官方资料与事实

| 组件/事实 | 结论 | 来源（访问 2026-08-31） |
| --- | --- | --- |
| ROS 2 Jazzy | 官方 deb 目标为 Ubuntu 24.04 Noble；发行版支持页列出 Jazzy 至 May 2029 | [Jazzy release](https://docs.ros.org/en/jazzy/Releases/Release-Jazzy-Jalisco.html)、[Ubuntu install](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)、[distributions](https://docs.ros.org/en/jazzy/Releases.html) |
| ros2_control | 提供 ROS 2 的实时控制框架、hardware component、controller manager 和标准 controllers；硬件插件的 `read()/write()` 是明确扩展边界 | [Jazzy docs](https://control.ros.org/jazzy/index.html)、[writing hardware](https://control.ros.org/jazzy/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html) |
| ROS 2 QoS | reliability、history/depth、durability、deadline、lifespan、liveliness 会影响连接和时序；sensor profile 使用 best effort/较小队列 | [QoS concepts](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html) |
| ROS 2 lifecycle | 节点有 Unconfigured/Inactive/Active/Finalized 及显式转换；节点 lifecycle 不等于硬件安全 arm 状态 | [managed nodes design](https://design.ros2.org/articles/node_lifecycle.html) |
| ROS 2 tracing | 官方 `ros2_tracing` 提供 ROS 2 tracepoints/工具链，可与 LTTng 配合；仍需自有 RT flight recorder | [ros2_tracing](https://github.com/ros2/ros2_tracing) |
| Linux CAN | SocketCAN 以 Berkeley socket API 暴露 CAN，支持过滤和错误帧通知；适合作为 transport，不包含设备语义 | [kernel SocketCAN](https://docs.kernel.org/networking/can.html) |
| Cyclone DDS | 提供 DDS 配置、安全和 shared-memory 相关文档；具体 RMW 选择需在目标板基准验证 | [Cyclone DDS docs](https://cyclonedds.io/docs/cyclonedds/latest/) |
| SOEM | 当前仓库在 2026-06-12 的提交为 `2f73eaa`；LICENSE.md 明确 GPLv3/商业双授权 | [SOEM repo](https://github.com/OpenEtherCATsociety/SOEM)、本地 `host/tools/SOEM/LICENSE.md` |

## 候选路线

### A：成熟组件组合（推荐）

`robot_rt` 自有实时核心 + EtherCAT/CAN transport + 纯 codec/device driver + ROS 2 Jazzy + ros2_control facade + Cyclone DDS/Fast DDS profile + PREEMPT_RT + systemd/ros2_tracing。

优点：复用 ROS 控制器/消息/工具和 Linux 总线生态；实时边界可独立测量和替换；与当前 `GatewayEcat`/协议 codec 迁移路径短。代价：需要维护 IPC schema、两类运行时和设备插件契约；SOEM 许可证需要产品决策。

### B：ROS 2 单进程控制

把 controller manager、硬件插件和总线主站放在一个进程，以减少复制和延迟。

优点：结构简单、ros2_control 资源链路直接。风险：ROS executor、pluginlib、日志或某个控制器的分配/锁可能污染 RT；单进程崩溃拖垮总线和安全监督；难以隔离第三方控制器。只作为经过性能和故障测试的可选 profile。

### C：完全自研消息/控制平台

自定义 RPC、控制器生命周期、设备模型和工具，不依赖 ROS 2。

优点：边界完全可控，适合极端硬实时或封闭产品。代价：重复实现录包、资源 claim、轨迹 action、诊断、多机和生态；团队维护面显著增大。当前 Ubuntu/ROS 需求下不采用，可在 <100 us 或认证约束触发时重新评估。

### D：以 Zenoh/自定义数据面取代 DDS

在 ROS 2 外增加 Zenoh 或其他数据路由，解决 WAN/多机器人流。

优点：可探索跨网段、边缘和大规模路由。风险：增加 RMW/网关组合、运维和安全面，不能替代本机 RT；当前规模缺少必须条件。保留为应用/多机网关的实验选项，不进入首版基线。

## 决策矩阵（讨论工具）

权重：需求适配 25%、实时隔离 20%、生态/交付 15%、扩展性 15%、可观察/测试 10%、运维 10%、迁移/退出 5%。1--5 分为团队初评，低置信度项需 PoC。

| 方案 | 需求 | RT隔离 | 生态 | 扩展 | 观测/测试 | 运维 | 迁移 | 加权印象 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A 组合成熟组件 | 5 | 5 | 5 | 5 | 4 | 4 | 4 | **4.75** |
| B ROS 单进程 | 4 | 2 | 5 | 4 | 3 | 4 | 4 | 3.65 |
| C 全自研 | 3 | 5 | 1 | 5 | 2 | 2 | 1 | 3.10 |
| D Zenoh 扩展数据面 | 3 | 4 | 3 | 4 | 3 | 2 | 2 | 3.20 |

分数不是基准结果。A 的主要硬风险是 SOEM GPLv3/商业授权和目标 RMW/RT kernel 的实测；若许可证或实时预算不成立，保留相同 `robot_rt`/codec/IPC 契约，只替换 EtherCAT backend 或部署 profile。

## 性能模型与待验证 PoC

首版按 [00-project-brief.md](00-project-brief.md) 的 1 kHz、p99.9 和总线 70% admission 预算。必须做的最小 PoC：

1. 当前 `GatewayEcat` 与提取后的 `robot_rt` 周期对比：相同设备、PDO、SOEM、CPU affinity，测 WCET/jitter/WKC。
2. SHM 双缓冲与 UDS 备用路径：测 command ingress-to-cycle、反馈 age、CRC/drop，在 CPU/网络干扰下测尾延迟。
3. Humble/TROS 与 Jazzy 上的 Cyclone DDS/Fast DDS QoS/发现/退出：测连接时间、deadline、丢包、CPU/内存和安全治理。
4. `ros2_control` 分离与共进程 profile：测 controller update 对 RT 的影响，验证第三方控制器动态分配和阻塞行为。
5. 真实 CAN 帧（含位填充/错误重发）和 EtherCAT payload：核算 admission，不能使用只包含 payload 的乐观公式。

PoC 报告固定版本、硬件、内核、编译器、负载、命令、原始数据和限制；只有端到端结果能改变架构选择时才保留复杂优化。

## 许可证与供应链结论

ROS 2/ros2_control 常用包和当前仓库自有代码按各自 `package.xml`/LICENSE 审查；Cyclone DDS 为独立开源项目，具体链接和分发义务以其版本许可证为准。当前 SOEM LICENSE 明确 GPLv3 或商业授权，闭源/商业产品不得默认把仓库内 SOEM 当作无条件可分发依赖。发布前由负责人完成 SPDX 清单、SBOM、CVE 扫描和法务确认，或选用合规 EtherCAT 后端；该结论不构成法律意见。
