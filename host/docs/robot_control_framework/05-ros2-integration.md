# ROS 2 集成与实时边界设计

| 项目 | 内容 |
| --- | --- |
| 状态 | **提议** |
| 适用平台 | Ubuntu 主机、RDK/TROS 边缘控制机 |
| 当前兼容基线 | ROS 2 Humble（含 TROS Humble） |
| 目标基线 | Ubuntu 24.04 + ROS 2 Jazzy LTS |
| 关联决策 | [`adr/0002-ros2-control.md`](adr/0002-ros2-control.md) |

本文规定 ROS 2 在机器人控制框架中的位置、实时边界、进程和包边界，以及新增
执行器/传感器/协议时的接入方式。本文是架构约束，不等同于功能安全认证；任何
急停、扭矩切断和失控保护都必须在独立的硬件、固件或实时核心中实现。

## 1. 结论摘要

1. ROS 2 是系统的**集成总线和控制平面**：承载任务、规划、状态、参数、工具和
   多机协作。ROS 2/DDS 不作为唯一的安全硬实时闭环，也不直接承担物理急停。
2. 1 kHz EtherCAT/CAN 周期、协议编解码、命令看门狗和降级动作属于
   `robot_rt` 实时核心。实时核心不调用 DDS、参数服务、日志格式化或动态内存。
3. ROS 2 控制器通过有界的实时缓冲区或共享内存与实时核心交换“最新有效快照”。
   队列满时丢弃旧的非安全设定值是明确语义，不能让控制周期阻塞等待 ROS。
4. 采用 `ros2_control` 作为标准硬件抽象和控制器管理层，但约束其
   `read()/write()`、控制器更新函数必须满足实时规则；现有 SOEM 线程先保留，
   通过 `SystemInterface` 逐步接入。
5. 当前 RDK/TROS 以 Humble 兼容为第一阶段，新的 Ubuntu 主机以 Jazzy 为目标。
   两者通过版本化接口和兼容桥接共存，不在同一部署中混用未经验证的 RMW、消息
   ABI 或控制器插件。

## 2. 运行时分层

### 2.1 推荐进程拓扑

```text
                         任务/规划/感知/用户工具
                    ┌─────────────────────────────┐
                    │ ROS 2 应用域                  │
                    │ planner  perception  UI      │
                    │ 多线程 executor，允许非实时   │
                    └──────────────┬──────────────┘
                                   │ DDS（QoS/权限/域隔离）
                    ┌──────────────▼──────────────┐
                    │ ROS 2 控制域                  │
                    │ command_mux / lifecycle       │
                    │ controller_manager            │
                    │ ros2_control hardware plugin  │
                    │ 有界 callback + 控制更新线程  │
                    └──────────────┬──────────────┘
                                   │ lock-free IPC
                  memfd 共享内存/Unix socket（本机）
                                   │
                    ┌──────────────▼──────────────┐
                    │ robot_rt 实时核心             │
                    │ EtherCAT/CAN/串口 transport   │
                    │ 总线调度、协议 codec          │
                    │ 命令看门狗、限幅、降级        │
                    │ SCHED_FIFO + CPU 隔离         │
                    │ 不运行 ROS/DDS                │
                    └──────────────┬──────────────┘
                                   │ EtherCAT/CAN/USB/UART
                    ┌──────────────▼──────────────┐
                    │ MCU/驱动器/传感器             │
                    │ 固件看门狗、硬件急停、伺服环   │
                    └─────────────────────────────┘

       独立 ops 域：diagnostics、rosbag2/MCAP、tracing、metrics、审计日志
```

默认把 `robot_rt` 和 ROS 2 控制域放在 RDK 同一台机器上；规划、可视化和录包可以
在 Ubuntu PC 上运行。若硬件供应商只允许在 ROS 进程内加载主站，可使用“共进程
配置”：将 `controller_manager` 的更新线程和硬件插件放在实时 CPU 上，但仍把
DDS 回调、参数和安全状态变更通过实时缓冲区隔离。共进程并不取消固件/硬件看门狗。

### 2.2 边界责任表

| 域/线程 | 主要职责 | 时序目标 | 允许 | 禁止/失败行为 |
| --- | --- | --- | --- | --- |
| MCU/驱动器 | PWM/CAN 伺服、硬件保护、断线停机 | 由设备保证 | 固定内存、硬件 watchdog | 不能依赖 ROS 才能进入安全态 |
| `robot_rt` 周期线程 | PDO、总线调度、codec、采样快照 | 500 us--1 ms（按总线预算） | 预分配、原子快照、绝对时钟、`SCHED_FIFO` | 不分配、不阻塞、不打印；WKC/超时异常立即清零或降级 |
| 控制更新线程 | 控制器链、限幅、资源声明、写入 RT 缓冲 | 100--1000 Hz | 固定大小数据、实时安全控制器 | 不执行 DDS 服务、磁盘 I/O、动态加载 |
| ROS bridge executor | 接收命令、发布反馈、生命周期和诊断 | 反馈通常 50--500 Hz | 有界回调、QoS deadline/lifespan | 不能把回调延迟传给总线周期；拒绝过期命令 |
| 应用 executor | 规划、感知、UI、脚本 | 由功能决定 | Python、动态内存、可靠服务 | 不直接打开 EtherCAT/CAN 原始接口 |
| ops/录包 | 追踪、指标、录制、审计 | 低优先级 | 磁盘/网络 I/O | 不得反压控制发布者；磁盘满只影响录包 |

### 2.3 数据所有权与交换契约

实时核心是硬件事实的唯一来源；ROS 节点维护的是带时间戳的观察值和请求值，不能
通过“回写状态话题”伪造硬件状态。两域之间使用固定大小的双缓冲或 SPSC
`latest-value` 缓冲，每个槽包含：

- `schema_version`、`robot_id`、`source_id`、`sequence`；
- 单调时钟采样时间、有效截止时间（`valid_until`）；
- 固定长度的轴/传感器数组和 `valid_mask`；
- 状态位、错误码、CRC/完整性标记。

发布者先写 payload，再以 release 顺序递增序号；读取者以 acquire 顺序验证序号，
遇到半写入、版本不认识或超时数据直接丢弃。共享内存协议必须有 magic、字节序、
结构体大小和兼容版本，不能直接把 C++ 私有布局当作跨版本 ABI。

## 3. ROS 2 的职责与非职责

### 3.1 适合放入 ROS 2 的内容

| 能力 | 推荐 ROS 2 机制 | 备注 |
| --- | --- | --- |
| 动态反馈 | Topic | 使用标准 `sensor_msgs`、`control_msgs`，高频数据采用有界消息 |
| 静态参数/能力 | 参数 + 启动时服务 | 参数在 `configure` 前校验；运行中修改需经过策略检查 |
| 状态切换/功能开关 | Lifecycle + Service/Action | `configure/activate/deactivate/cleanup` 与独立安全状态同时存在 |
| 轨迹/任务 | Action | 结果、取消、超时和抢占有明确语义，不把长任务塞进服务 |
| 诊断/健康 | `diagnostic_msgs`、指标 Topic、服务 | 发布错误码、年龄、计数器和恢复建议 |
| 多机协作 | DDS/ROS graph、显式路由 | 远端发送轨迹或低频目标；本机插值和看门狗 |
| 记录与回放 | `rosbag2`（MCAP）+ `/clock` | 录包进程隔离，不能阻塞实时发布 |

### 3.2 不应依赖 ROS 2 的内容

- 物理急停、扭矩切断、制动电阻和过流/过温保护；
- 1 kHz 周期内的网络发现、DDS 序列化、参数查找、服务调用和日志输出；
- 断网后仍必须执行的安全动作；
- 需要确定性上界但尚未完成分配、锁和调度测量的控制器。

ROS 2 的 `liveliness`、`deadline` 和命令超时只提供软件层观测与降级触发，不能替代
安全继电器或固件 watchdog。任何“停止”请求都必须有独立的最终停机路径。

## 4. `ros2_control` 采用决策

### 4.1 决策

采用 `ros2_control` 作为**标准硬件资源和控制器编排层**，但将实时主站后端视为
独立组件。一个物理 EtherCAT 网关/总线通常对应一个 `SystemInterface`，总线内部
再按设备型号选择 codec；不要为每个电机创建一个 ROS 进程或一个独立周期线程。

硬件插件建议暴露：

- 标准状态接口：`position`、`velocity`、`effort`、`temperature`、`fault`；
- 标准命令接口：按控制模式选择 `position`、`velocity`、`effort`；
- MIT/厂商扩展：`kp`、`kd`、`feedforward_effort`、`enable` 等有明确单位的接口，
  只在确有控制器消费者时增加；
- 由 URDF/Xacro、`joint_limits` 和校准配置描述轴名、方向、软/硬限位和传动比。

`read()`/`write()` 只做固定大小的复制、序号检查和后端快照交换，不打开设备、
不等待 DDS、不分配内存、不调用可能阻塞的日志。设备发现、PDO 映射、模式切换和
参数解析在 `on_init`/`on_configure` 完成；激活前先确认轴状态、限位和安全许可。

### 4.2 两种运行配置

| 配置 | 适用 | 结构 | 约束 |
| --- | --- | --- | --- |
| 分离配置（默认） | 当前 `GatewayEcat` 已有独立 SOEM 线程；需最大隔离 | `robot_rt` 自有周期，`SystemInterface` 通过共享快照读写 | 控制器周期与总线周期要测量对齐；IPC 只传最新值 |
| 共进程配置 | 需要控制器与总线共享内存、延迟极低 | `controller_manager` 更新线程调用后端，DDS 回调另线程 | 所有控制器必须通过 RT 审查；动态切换只能在非运行窗口 |

分离配置的控制器输出如果超过 `command_timeout`，实时核心必须清零并报告
`COMMAND_STALE`；不能因为 ROS 反馈频率低而保持最后一个无限期有效的扭矩。

### 4.3 与当前代码的映射

| 当前实现 | 目标位置 | 迁移说明 |
| --- | --- | --- |
| `hpm_gateway_control::GatewayEcat`、`gateway_ecat_api.cpp` | `robot_rt` 后端库/守护进程 | 保留 1 kHz 绝对周期、`LatestBuffer`、WKC/超时统计；去掉 ROS 类型依赖 |
| `HpmGatewayControlNode` ROS 回调/反馈定时器 | ROS bridge + `SystemInterface` | 先保留兼容节点，再把字段映射到标准接口 |
| `hpm_gateway_msgs/Motor*` | `robot_interfaces` + 兼容适配器 | 固定数组可继续读；新接口带版本、单位、来源和有效期 |
| `hpm_gateway_power` Python 串口节点 | 非实时 `power` 生命周期节点 | 串口阻塞、重试和状态发布不得进入 RT 域；协议 codec 可复用 |
| gamepad 节点 | 命令源 → `command_mux` | 死区、重复发送和 dead-man 只产生请求，不直接写 PDO |

先运行现有节点与新 `SystemInterface` 的**影子模式**：新路径计算并记录输出但不发
总线，比较轴值、序号、时间戳和超时行为；通过后再切换唯一写入者。

## 5. Executor 与线程模型

### 5.1 Executor 拓扑

1. `robot_rt`：普通 `pthread`/专用守护进程，使用 `clock_nanosleep(...,
   TIMER_ABSTIME)`、`SCHED_FIFO`、CPU 亲和性和 `mlockall`。它不创建 `rclcpp::Node`。
2. 控制更新线程：固定顺序为 `read -> safety/limit -> controller chain -> write`。
   使用预分配消息和 `realtime_tools::RealtimeBuffer` 或等价快照。更新线程不处理
   参数、服务和日志。
3. ROS bridge：优先使用 `StaticSingleThreadedExecutor` 或显式 `rclcpp::WaitSet`，
   将命令接收、状态发布、诊断和生命周期放入不同 callback group；必要时为每组
   配置独立线程，但不把这些线程绑到 RT 核心。
4. 应用域：可使用 `MultiThreadedExecutor`/组件容器；CPU、内存和回调数量受资源
   限制，避免应用回调抢占控制线程。

### 5.2 实时规则

- 激活后控制热路径禁止 `new/delete`、字符串拼接、未界定容器增长、文件/网络 I/O、
  mutex/条件变量等待和 ROS 日志；
- 所有数组在配置阶段按最大轴数/传感器数分配，消息采用有界序列；
- 订阅回调只验证来源、单位、有限值、序号和有效期，然后原子替换快照；
- 状态发布失败只递增计数器，不反向阻塞 `read/write`；
- 回调组不能假设“多线程 executor 会自动解决数据竞争”，每份共享状态要有唯一
  所有者和明确的 acquire/release 或互斥边界；
- 动态 controller/plugin 加载、模式切换和参数重载发生在 `inactive` 状态；
- 生产机采用 PREEMPT_RT 内核、IRQ/网卡绑核、`CAP_SYS_NICE`/`CAP_IPC_LOCK` 最小
  权限，并用 systemd 管理重启和资源限制。Docker 仅用于开发和非实时工具，实时
  进程容器化前必须验证 cgroup、网络 raw socket、锁页和调度权限。

### 5.3 控制预算

每个硬件配置都要记录以下测量，而不是只记录平均值：

| 指标 | 最低证据 |
| --- | --- |
| 总线周期 | p50/p99/p999/最大执行时间、周期抖动、deadline miss |
| 命令链路 | 接收时间到 PDO 写入的年龄、过期/拒绝/覆盖计数 |
| 反馈链路 | 采样到发布的年龄、丢帧和序号跳变 |
| 总线健康 | WKC 错误、重连、设备状态、各 codec 错误 |
| 资源 | RT 线程 CPU、栈高水位、锁页、分配次数、IRQ 延迟 |
| ROS/DDS | 发布延迟、匹配状态、QoS 不兼容、发现流量 |

阈值按机构、总线和控制器需求在配置中声明；“没有观测到失败”不能作为满足实时
需求的证明。发布门禁至少应要求代表性负载下无未解释的 deadline miss，并保留原始
trace/metrics 证据。

## 6. DDS、RMW 与 QoS

### 6.1 版本与 RMW 策略

- 现阶段 RDK/TROS 使用 Humble；接口包和 launch 必须能在 Humble 构建，避免使用
  仅 Jazzy 提供的 API。供应商 TROS 的默认 RMW 先保持不变，新增 RMW 需在目标机
  做发现、延迟、丢包和退出测试。
- 新 Ubuntu 24.04 部署目标为 Jazzy LTS。生产环境固定 ROS 发行版、RMW 实现、DDS
  XML 和消息包版本，写入构建制品清单。
- 一套机器人部署优先统一使用一个 RMW。可选 `rmw_cyclonedds_cpp`（显式网卡、静态
  peer、跨网段行为可控）或供应商验证过的 Fast DDS；不能假定不同 RMW 在所有 QoS、
  共享内存和安全扩展上完全等价。

### 6.2 推荐 QoS 矩阵

下表是起始 profile；设备实测后可调深度和 deadline，但不得由单个节点随意定义。

| 通道 | History/Depth | Reliability | Durability | 其它约束 |
| --- | --- | --- | --- | --- |
| 本机高频命令（位置/速度/扭矩设定） | KeepLast/1 | BestEffort | Volatile | `deadline <= 2T`、`lifespan <= command_timeout`；序号、来源、有效期必填 |
| 远端高频命令 | 不推荐持续 DDS | -- | -- | 发送轨迹/低频目标，由本机插值；Wi-Fi 断开必须自动进入安全态 |
| 单次 arm/stop/mode 请求 | KeepLast/10 | Reliable | Volatile | Service/Action；幂等、权限、超时和结果码；物理停机不依赖它 |
| 电机/IMU 高频反馈 | KeepLast/5 | BestEffort | Volatile | `deadline` 触发诊断；订阅者读取最新样本，不堆积历史 |
| 低频设备状态/电源状态 | KeepLast/10 | Reliable | Volatile | 允许有限重试；明确 `comm_lost` 和最后有效时间 |
| 机器人描述/校准/能力 | KeepLast/1 | Reliable | TransientLocal | 启动后晚加入者可获得；版本和校验和必填 |
| diagnostics/事件审计 | KeepLast/20--100 | Reliable | Volatile | 不允许反压 RT；必要时单独 recorder |
| 图像/点云 | KeepLast/2--5 | BestEffort | Volatile | 设置 `lifespan`；大消息避免跨 WAN，优先共享内存/压缩 |

QoS 的可靠性只解决传输策略，不解决控制语义：BestEffort 命令必须配合序号、
有效期和 RT watchdog；Reliable 命令也不能无限排队发送过期扭矩。为防止发布者和
订阅者因策略不匹配而静默断开，`robot_interfaces` 提供集中 QoS profile，并在 CI
中运行匹配性测试（包括 deadline、liveliness、TransientLocal late joiner）。

### 6.3 发现、域和网络

- `ROS_DOMAIN_ID` 不硬编码为单一常数；生产配置按机器人/实验室分配，并把域 ID、
  机器人 ID 和 namespace 同时写入启动清单，防止跨机器人误控。
- 单机/同 VLAN 可使用受限 multicast；跨 VLAN 或多机生产环境采用静态 peer、DDS
  discovery server 或 DDS Router/等价网关，禁止把发现流量直接暴露到公网。
- 控制、遥测和开发工具使用网络分区/防火墙/Topic allowlist。启用 SROS2 时为控制
  发布者、订阅者、服务和 action 分发最小权限治理文件；证书轮换和失效行为要演练。
- RDK 与 PC 的系统时钟使用 chrony/PTP；消息同时保留硬件采样单调时间和 ROS 时间，
  不用接收时刻伪造传感器采样时间。

## 7. 消息、命名与生命周期

### 7.1 包边界

建议的 ROS 2 工作区包边界如下：

```text
robot_interfaces/       # 版本化 msg/srv/action，固定边界和单位
robot_description/      # URDF/Xacro、transmission、limits、校准引用
robot_transport/        # transport/codec 的 C++ 库，不依赖 rclcpp
robot_rt_backend/       # robot_rt 守护进程、共享内存协议和指标
robot_hardware/         # ros2_control System/Actuator/SensorInterface 插件
robot_control/          # controller_manager、command_mux、安全策略
robot_sensors/           # 非实时设备驱动、TF、时间戳和诊断
robot_bringup/           # launch、参数、生命周期编排和 systemd 模板
robot_observability/     # tracing、metrics、诊断和录包配置
robot_sim/               # ros2_control mock、仿真和回放适配器
robot_tools/             # CLI、协议探针、HIL 和故障注入工具
```

`robot_transport` 不应反向依赖 ROS 消息；协议测试可以在主机独立运行。只有
`robot_hardware`/`robot_control` 依赖 `rclcpp`、`hardware_interface` 和接口包。
Python 适用于配置、工具和低速设备，不进入 `robot_rt` 或 1 kHz 控制器。

### 7.2 规范化消息

优先使用 `sensor_msgs`、`geometry_msgs`、`trajectory_msgs`、`control_msgs` 和
`diagnostic_msgs`。厂商特有信息放入版本化 `robot_interfaces`，消息至少包含：

- `header.stamp`（采样/命令生成时间）和 `frame_id`；
- `sequence`、`source_id`、`priority`/租约标识；
- `valid_mask`、状态/故障位、单位明确的字段；
- `schema_version` 和必要的 `valid_until`；
- 固定长度或有明确上限的数组，避免无界字符串和序列。

命名空间采用 `/robot/<robot_id>/...`，例如：

```text
/robot/r01/joint_states
/robot/r01/joint_command
/robot/r01/imu/data
/robot/r01/diagnostics
/robot/r01/rt_metrics
/robot/r01/control/arm
```

现有 `/hpm_gateway/motor_command(s)`、`/hpm_gateway/motor_feedback` 在过渡期由
兼容节点双向映射，文档明确弃用版本和删除条件；禁止新控制器继续绑定旧私有名称。

### 7.3 命令仲裁与生命周期

每个可写资源只有一个事实上的 owner。`command_mux` 依据资源声明、优先级、租约、
dead-man 和有效期选择命令；失去租约或发现序号回退时拒绝命令并发布原因。建议的
安全状态机为：

```text
BOOT -> DISCOVERING -> INACTIVE -> READY -> ARMED -> RUNNING
                                      │         │
                                      └─────────┴──> FAULT -> RECOVERING
```

ROS lifecycle（unconfigured/inactive/active/finalized）只管理节点资源，不等同于
`ARMED/RUNNING`。进入 `ARMED` 需要硬件状态、限位、通信 watchdog 和安全许可全部
满足；任何域检测到超时、WKC 错误、过温或急停都可撤销许可。恢复必须显式确认，
不能由 DDS 重连自动重新上扭矩。

## 8. 扩展协议、执行器和传感器

### 8.1 新协议接入流程

1. 在 `robot_transport` 中记录协议版本、帧格式、字节序、节点寻址、周期预算、
   重试/超时和故障语义，建立 golden frame 测试向量。
2. 实现独立的 `codec`（`encode/decode`）和对应的 transport 后端（CAN-FD、
   EtherCAT、串口、USB 或 UDP）；codec 不创建 ROS 节点、不分配内存。
3. 定义 capability/版本清单和配置 schema；启动时发现并校验，不在运行周期动态
   加载未知插件。pluginlib 可用于配置阶段加载，生产制品需锁定插件版本和校验和。
4. 在 `robot_hardware` 增加/扩展 `SystemInterface`，把协议字段映射到标准状态/命令
   接口；保留原始错误码、计数器和序号，不能只映射成一个布尔 `ok`。
5. 增加主机单测、仿真、断线/畸形帧/重复帧测试、HIL 和实时预算；在 bringup 中加入
   capability 检查、QoS profile 和诊断项。

协议的公共消息只描述机器人语义，不把 CAN ID、PDO 偏移等线格式泄漏给上层控制器。
需要传递原始帧时使用低速诊断 topic，并设置权限，避免业务代码依赖私有字节布局。

### 8.2 新执行器接入流程

1. 在 `robot_description` 增加唯一 `joint_name`、轴方向、单位、传动比、校准、软/硬
   限位和允许的 command/state interfaces。
2. 在总线级硬件插件中增加设备描述（bus、node_id、protocol、watchdog、周期），
   复用已有总线调度线程；不要为单个设备复制一份 executor。
3. 在 `ros2_control` 导出接口，配套 `joint_state_broadcaster`、控制器 YAML、资源
   claim 和模式切换；MIT 增益等扩展接口必须有单位、范围和控制器消费者。
4. 用 mock/sim 先验证限位、反向、断线和模式切换，再做低能量 HIL；最终由安全评审
   确认默认 disarm、超时清零和急停路径。

### 8.3 新传感器接入流程

1. 驱动在采集点记录硬件时间戳和原始状态；通过 PTP/单调时钟映射到 ROS 时间，不在
   publisher 回调中用当前时间替代采样时间。
2. 优先发布标准 `sensor_msgs`，设置 `frame_id`、协方差、单位、`valid_mask` 和
   `diagnostic_updater` 状态；厂商扩展另发有版本的消息。
3. 高频数据写入有界 ring/latest buffer，由非实时节点按需求发布；定义溢出、丢帧、
   重连和无效样本语义。传感器异常不能悄悄复用上一帧当作新鲜数据。
4. 添加录包/回放和仿真适配，使算法在无硬件时可复现相同时间戳、状态位和故障。

## 9. 可观察性与调试

### 9.1 必备观测面

- `/robot/<id>/diagnostics`：按 hardware、transport、control、safety 分组，带
  `OK/WARN/ERROR/STALE`、错误码、首次/最近发生时间和恢复建议；
- `/robot/<id>/rt_metrics`：周期执行/抖动、deadline、WKC、命令年龄、丢弃、重连、
  CPU/栈/锁页和队列水位；计数器使用单调累加，不用日志行数统计；
- `ros2_tracing`/LTTng：至少覆盖命令接收、仲裁、写入快照、RT read/write、反馈
  采样和发布，统一 `sequence`/trace id 以计算端到端延迟；
- `rosbag2` MCAP：控制命令、反馈、诊断、参数快照和 `/tf` 分层录制；录包进程低
  优先级，磁盘或网络背压只丢录制数据并告警；
- journald/systemd：结构化启动、生命周期、插件版本、配置 hash、退出原因和重启
  次数，禁止在高频循环中无界打印。

### 9.2 调试工具约束

`ros2 topic echo/hz`, GUI 和协议探针默认运行在开发/诊断域，不直接连接生产控制
域；需要在线观察时使用只读权限、采样/限速和专用 recorder。每次故障记录机器人
ID、软件/固件/ESI 版本、RMW 配置、时钟状态、QoS、trace 和 metrics，保证能从命令
序号追到总线帧和反馈。

## 10. 迁移计划、兼容与回滚

### 阶段 A：契约冻结（Humble）

- 固定当前 PDO 契约（320 字节电机命令 payload、252 字节反馈；若生成 ESI 暴露 332 字节诊断填充则以 ESI/主站实际长度为准）、ESI revision、轴 ID 和 `hpm_gateway_msgs` 字段；
- 新建 `robot_interfaces`、QoS profile 和 namespace 映射，旧 topic 保留只读/兼容
  发布；
- 为 `GatewayEcat` 的 timeout、WKC、序号和 `LatestBuffer` 增加契约测试。

### 阶段 B：实时后端抽取

- 将 SOEM 和协议代码抽成不依赖 ROS 的 `robot_rt_backend`；保留现有节点作为 bridge，
  通过 feature flag 选择旧/新后端；
- 在 RDK 上测量 1 kHz 周期、IPC 年龄和 CPU/IRQ 预算，未达标时不切换写入者。

### 阶段 C：`ros2_control` 影子和切换

- 实现一个总线级 `SystemInterface`，先只读反馈并影子计算命令；
- 通过 `controller_manager`、资源 claim 和 `joint_state_broadcaster` 验证标准接口；
- 停机窗口内切换到唯一写入者，保留旧 launch 一键回滚；观察至少一个完整回归周期。

### 阶段 D：Jazzy 目标与生产化

- 在 Ubuntu 24.04/Jazzy 构建同一接口版本，运行序列化、QoS、HIL 和实时基准；
- 用 systemd 先启用 RT 后端，再启用 ROS 控制域和应用域；PC 仅部署规划/可视化/录包；
- 启用 SROS2、静态发现/路由和分域，记录制品 SBOM、插件 hash 和配置 hash。

回滚条件包括：任何未解释的 RT deadline miss、命令年龄超限、WKC/序号错误增加、
QoS 不匹配导致控制丢失、状态机在无许可时上扭矩，或新旧路径输出不一致。回滚只需
停止新控制域、清零实时后端并恢复旧 launch；恢复后仍需人工重新 arm，不能自动续转。

## 11. 风险与控制措施

| 风险 | 后果 | 控制措施/剩余风险 |
| --- | --- | --- |
| 把 DDS 当作 1 kHz 总线 | 网络拥塞或发现抖动导致过期扭矩 | 本机 RT backend + latest snapshot；远端只发轨迹；仍需现场测量 |
| QoS 不匹配/队列堆积 | 订阅静默断开或执行旧命令 | 集中 profile、CI 匹配测试、depth=1、deadline/lifespan；跨 RMW 仍有实现差异 |
| `ros2_control` 插件非实时 | 控制周期阻塞、优先级反转 | `read/write` RT 审查、预分配、影子模式和 HIL；第三方控制器需单独评估 |
| 参数/插件运行中变更 | 资源竞争、轴模式错误 | 仅 inactive 配置/加载，配置 hash 和权限审计；现场恢复仍需人工确认 |
| Python 串口/脚本进入热路径 | GIL、阻塞 I/O、不可预测延迟 | 保持在非实时域；必要时以独立 C++ 服务替换；链路 watchdog 独立 |
| 多机器人域/namespace 冲突 | 跨机器人误控 | 域 ID、robot namespace、SROS2 allowlist 三重隔离；上线前做误发测试 |
| 时间基准漂移 | 反馈年龄和 TF 错误 | PTP/chrony、硬件采样时间、时钟健康诊断；失步时标记数据 stale |
| 录包和诊断反压 | CPU/磁盘争用导致控制抖动 | 独立进程/CPU、best-effort 观测、限速和磁盘配额；数据可能丢失 |
| PDO/ESI 或消息版本漂移 | 字段错位、危险输出 | schema/ESI revision、启动校验、兼容桥、影子比较；旧设备缓存需清理 |

## 12. 验收清单

在宣布某机器人配置“可生产”前，必须有可追溯证据：

1. ROS 2、RMW、DDS 配置、消息和插件版本已锁定；Humble 兼容和 Jazzy 目标构建均有
   结果，或明确列出未支持项。
2. RT 周期在代表性最大轴数/最大传感器负载下完成 p99/p999/最大值测量；deadline、
   WKC、命令超时、反馈丢弃和安全降级均有故障注入结果。
3. 命令跨域、重复、乱序、过期、无租约、DDS 断网和节点重启不会重新上扭矩；物理
   急停在 ROS/DDS 完全离线时仍有效。
4. QoS 匹配、late joiner、TransientLocal、liveliness、跨机路由和 SROS2 权限测试
   已记录；跨机高频命令被配置拒绝或转换为本机轨迹。
5. 新增执行器/传感器/协议具备 codec golden vectors、mock/sim、HIL、诊断、回放和
   回滚路径；旧 topic 的弃用版本和删除条件写入发布说明。
6. trace、metrics、bag、journald 能用同一 `sequence`/时间戳关联一次命令到硬件反馈，
   并能在不影响控制的情况下停止录包和可视化工具。
