# 实时运行时与性能预算

## 设计目标

“高性能”在本框架中指可重复、可解释的尾延迟和资源上限，而不是平均吞吐最高。硬实时控制只在本机 `robot_rt` 内闭环；跨主机或 ROS 2 网络只传输目标、轨迹和遥测。

## 周期模型

默认控制周期为 1 ms，设备 profile 可申请 250 us、500 us、2 ms 或更慢的 slot，但必须通过总线和 WCET admission。每个周期使用绝对单调截止时间：

```text
deadline[n] = release_0 + n * period
sleep_until(deadline[n])
read -> decode -> control/safety -> encode -> write
```

禁止用相对 `sleep(period)` 累积漂移。EtherCAT 设备优先以 Distributed Clocks/Sync0 作为相位参考；Linux `CLOCK_MONOTONIC` 作为主机观测时钟；跨设备时间转换必须记录 offset 和不确定度。

### 1 ms 初始预算

| 阶段 | 预算 | 说明 |
| --- | ---: | --- |
| 总线 RX 完成/有限消费 | 150 us | 不等待未到达的帧；每 bus 有 RX budget |
| framing + codec | 100 us | 固定缓冲区、O(1) endpoint lookup |
| 控制器与安全检查 | 150 us | 只允许已验证的 RT controller |
| TX 编码/提交 | 200 us | 非阻塞 FIFO/预分配队列 |
| 观测写入与余量 | 200 us | 只写原子计数和 flight recorder |
| **周期上限** | **950 us** | 超限触发 deadline miss，不重试补偿 |

预算是首版起点；每个 profile 需要提交实际 WCET（p50、p99、p99.9、最大）和总线占用证据。若无法满足，降低频率、拆分总线或把控制器移到本地驱动器，不能只提高线程优先级。

## 总线容量 admission

对每条 CAN 总线分别计算：

```text
U_bus = sum(frame_bits * frames_per_second) / nominal_bitrate
```

`frame_bits` 必须包含仲裁、控制、CRC、帧间隔、最坏位填充和预留错误重发。持续目标 `U_bus <= 0.70`，0.70--0.80 进入评审，超过 0.80 拒绝配置。CAN-FD 的仲裁段和数据段分开计算。EtherCAT 则按周期 payload、帧数、链路速率和从站处理时间计算，并检查 WKC 和 DC 同步。

配置编译器在启动前拒绝：重复 endpoint、未知 bus、超出帧数、RX/TX 队列不足、周期相位冲突或总线占用超限。管理/探测/固件升级流必须从 cyclic budget 单独扣除。

## Linux 调度与内存

生产 RT 主机建议：

- Ubuntu 24.04 LTS + 与目标内核匹配的 PREEMPT_RT；确认 `CONFIG_PREEMPT_RT`、高精度定时器和 IRQ threading。
- `robot_rt` 使用 `SCHED_FIFO`、固定 CPU、`mlockall(MCL_CURRENT|MCL_FUTURE)`，启动阶段触碰所有页并完成插件/配置加载后再进入 ACTIVE。
- RT CPU 与网络 IRQ、图形、录包和感知任务隔离；设置 IRQ affinity、CPU governor 为 performance，并记录实际频率。不要未经测量盲目添加 `isolcpus/nohz_full`。
- systemd/cgroup 固定 CPU、内存和实时优先级；容器模式必须显式配置 cpuset、rt runtime、`CAP_SYS_NICE` 和 `CAP_IPC_LOCK`。
- RT 热路径禁止 `malloc/free`、异常、互斥锁、文件/网络系统调用、ROS/DDS、printf、动态插件查找和无界循环。使用固定对象池、SPSC ring、seqlock 或双缓冲。

### 队列策略

| 数据 | 队列 | 满载策略 | 依据 |
| --- | --- | --- | --- |
| 运动命令 | 单槽 latest-value + sequence/deadline | 丢旧值并计数 | 新命令优先，旧命令不可排队执行 |
| 高频状态/IMU | latest snapshot 或有界 ring | 丢旧样本/降采样 | 防止遥测阻塞控制 |
| 故障/安全事件 | 独立有界事件 ring | 优先保留故障；溢出升级计数 | 不能与普通遥测争用 |
| 配置/服务 | 非 RT 队列 | 有限重试、超时、幂等 | 不进入周期关键路径 |

IPC 帧固定大小并包含 `magic/schema_version/frame_type/sequence/monotonic_ns/deadline_ns/valid_mask/crc32`。读者看到版本或 CRC 不匹配时丢弃并报告，不猜测字段含义。

## 实时与非实时边界

```text
robot_rt（RT）                         robot_control/robot_ops（非 RT）
------------------                    ------------------------------
总线收发、codec、状态机、安全       ROS executor、参数、服务、录包
固定周期、固定内存、原子计数         plugin load、设备发现、日志、升级
       <---- SHM SPSC / UDS 控制面 ---->
```

共享内存优先使用单写单读的 cache-line 对齐结构；控制命令采用 seqlock 或双槽提交，提交顺序是“写数据 -> release store sequence”。跨进程无法使用共享内存时退化为 UDS，但必须把通信延迟纳入 budget，不能在周期内同步等待。

## 性能测试方法

每次报告固定以下元数据：主机型号、内核/RT patch、CPU governor、隔离核、ROS/RMW、编译器、设备 profile hash、帧捕获、负载、温度和命令。至少运行：

1. 空载基线：测 timer release-to-start、cycle WCET 和 page fault。
2. 额定负载：全设备、全反馈、诊断开启，记录 p50/p95/p99/p99.9/max、deadline miss streak、CPU/内存。
3. 干扰负载：录包、感知、网络突发、CPU stress，验证 RT 尾延迟。
4. 故障负载：丢帧、CRC 错、bus-off、设备重启、ROS/DDS 断开，验证 fail-action 和恢复时间。
5. 长稳：至少 1 小时首版门禁（量产按风险延长），内存、队列高水位、错误率不得单调增长。

工具组合：`cyclictest`/`rtla timerlat`（内核基线）、`perf stat`/`perf sched`（资源）、`ros2_tracing` + LTTng（ROS 边界）、自有 cycle trace ring（RT 内部）、总线分析仪和 `ros2 bag` MCAP（端到端证据）。平均延迟不能替代尾延迟和故障证据。

## 发布门槛与重评触发器

配置或代码必须在以下任一条件发生时重新测量：设备数/帧数变化、周期或相位变化、协议 codec 变化、内核/RMW/编译器变化、启用录包/感知、CPU 型号变化。

首版发布门槛：预算内无未解释的 deadline miss；CAN/EtherCAT admission 通过；RT 堆稳定；所有队列有高水位和 drop 计数；故障注入达到预期安全状态；原始 trace、配置锁和版本信息可重放。
