# ADR-0003：以版本化 codec、设备描述和静态 RT 注册表扩展

- 状态：提议
- 日期：2026-08-31
- 范围：总线协议、执行器、传感器和设备配置

## 背景

当前 RS06/ENCOS/YESENSE 已有纯解析代码，但轴映射、协议分支、PDO 和编译期配置仍分散。新增设备若直接修改 manager switch，容易把 wire 细节泄漏到 ROS、引入线性试探和不可测的周期成本。另一方面，运行时随意加载插件会引入 ABI、签名和实时安全风险。

## 候选

1. 在现有 manager 中继续增加协议/设备 `switch`。
2. 五层契约（transport/framing/codec/profile/driver）+ 版本化 descriptor + 启动期插件/静态 registry。
3. 全部使用通用动态脚本/反射描述，运行时解释。

## 决策

选择 2。codec 纯函数化；descriptor 经 schema/lint 后编译成不可变 runtime table；host 插件只在 configure 阶段加载并校验 hash/签名/ABI，固件静态链接 registry，ACTIVE 后禁止热替换。协议以 reverse-DNS ID + wire major 注册，设备以 `device_uid/semantic_name` 标识；未知或不兼容设备进入 QUARANTINED。

## 后果

新增协议需要 RFC/spec、golden vectors、codec、profile、driver、仿真/HIL 和观测证据，前期门槛较高，但核心 scheduler 不再随设备数量线性复杂化。需要维护多个版本（wire、plugin ABI、firmware、descriptor schema、ROS API）和 A/B 回滚包；持久设备参数仍需单独备份和人工确认。

## 验证与复审

至少一个新设备走完“描述 -> 注册 -> virtual transport -> HIL -> ROS mapping -> 回滚”全流程；通过重复 endpoint、预算超限、非法帧、未知 major 和恶意插件测试。若设备数量/动态拓扑需要运行中热插拔，重新评估静态表与安全授权边界，而不直接放开热加载。
