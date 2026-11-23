# ADR-0005：Ubuntu 24.04/Jazzy 为新基线，Humble/TROS 为兼容 profile

- 状态：提议
- 日期：2026-08-31
- 范围：host 构建、部署和 ROS 发行版

## 背景

当前 RDK/TROS 工作区基于 ROS 2 Humble，既有部署和 SOEM 工具不能一次性切换；新框架需要长期维护的 Ubuntu/ROS 组合、明确的 RMW 和可重复制品。

## 决策

新产品 profile 采用 Ubuntu 24.04 LTS + ROS 2 Jazzy LTS + PREEMPT_RT；当前 RDK 使用 Ubuntu 22.04 + Humble/TROS 兼容 profile。公共 `robot_interfaces`、descriptor schema、IPC 和协议 lock 不依赖发行版私有 API。生产环境固定一个 RMW（先评估 Cyclone DDS；供应商要求时使用经过测试的 Fast DDS），并锁定 DDS XML、插件 hash、容器/包版本。

## 后果

可获得 Jazzy 的长期支持和 Noble deb 包，但需维护双发行版 CI、处理 ROS/ABI 差异和目标板升级。Humble profile 不得成为新功能的永久分叉；当供应商兼容期结束或新功能无法跨版本实现时，按路线图迁移并保留 N-1 回滚。

## 验证与复审

在两 profile 上构建接口、hardware facade、QoS/lifecycle 和 virtual transport；在目标 RMW/内核上测 discovery、deadline、资源、退出和实时 jitter。若目标硬件不能运行 24.04/Jazzy，继续使用兼容 profile，但每个新设备仍必须通过相同 canonical 契约。
