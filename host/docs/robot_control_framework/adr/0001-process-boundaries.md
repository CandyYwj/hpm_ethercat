# ADR-0001：实时设备平面与 ROS 2 控制平面分进程

- 状态：提议
- 日期：2026-08-31
- 范围：Ubuntu 主机机器人运行时

## 背景

EtherCAT/CAN 周期、设备协议、安全 watchdog 需要可测量的上界；ROS 2 executor、DDS discovery、参数、第三方控制器和录包则需要灵活性，可能分配、阻塞或产生突发负载。把两者混在一个无约束进程会让单个回调或插件破坏整个控制环；完全绕过 ROS 2 又失去标准控制器和工具生态。

## 候选

1. 所有设备和控制器放 ROS 2 单进程。
2. `robot_rt` 与 `robot_control` 分进程，以固定大小 SHM/UDS 交换。
3. 全部自研、只保留 ROS 2 作为远程工具。

## 决策

选择 2。`robot_rt` 拥有总线、codec、设备状态和最后安全检查；`robot_control` 拥有 ros2_control、command mux、lifecycle 和 ROS 接口；应用/ops 可独立运行。默认 SHM 双缓冲/SPSC，UDS 仅作非周期控制面备用；RT 不依赖 DDS、文件、动态插件或日志。

## 后果

收益：故障隔离、CPU/权限隔离、尾延迟可测、保留 ROS 2 生态和渐进迁移路径。代价：需要 IPC schema、版本兼容和少量复制；控制器/总线周期对齐需测量；进程退出后的安全动作仍必须由硬件/MCU watchdog保证。

## 验证与复审

以当前 `GatewayEcat` 为基线，比较分离和共进程 profile 的 WCET、jitter、command age、WKC/CAN 错误和故障注入结果。若测得 IPC/分离边界使预算无法满足，才在停机安全边界内评估共进程；若仍不满足，迁移内环至 MCU/专用 RT 平台。
