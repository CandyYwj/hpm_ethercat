# 未来目标大架构图

文档状态：**提案（目标架构）**。图中的 `robot_rt`、固定大小 IPC、注册表、仿真/HIL 和签名发布链是未来建设目标；现有 `hpm_gateway_control`、SOEM worker、PowerLink 和 gamepad 节点通过迁移适配器逐步接入。

这张图回答四个问题：

1. ROS 2 位于哪里，以及为什么不进入最终安全闭环；
2. 命令、状态、诊断和安全信号分别走哪条路径；
3. 新协议、执行器和传感器从哪里接入；
4. 配置、代码、仿真、HIL、签名发布和回滚怎样形成闭环。

## 总体架构

主图使用原始画布尺寸渲染。文档站点若仍按页面宽度缩放，可在图上右键选择“在新标签页打开图片”，或使用浏览器 `Ctrl` + `+` / `Ctrl` + 鼠标滚轮放大；在支持横向滚动的 Markdown 查看器中可直接拖动查看全图。

```mermaid
%%{init: {"flowchart": {"useMaxWidth": false}}}%%
flowchart TB
  %% 离线交付链：所有运行时可变输入先被编译、校验、签名
  subgraph Delivery[配置、协议与交付流水线（离线）]
    direction LR
    robot_yaml[robot.yaml<br/>设备与机器人配置]
    protocol_spec[protocol spec.yaml<br/>线缆格式与能力]
    schemas[JSON Schema<br/>接口与版本约束]
    compiler[profile compiler<br/>语义校验与代码生成]
    registry_lock[protocol lock + device table<br/>ID、版本、过滤器、预算]
    quality_gate[CI 质量门禁<br/>golden / fuzz / 静态分析 / WCET]
    signed_bundle[签名发布包<br/>profile、插件、固件兼容矩阵]
    ab_deploy[A/B 部署与回滚<br/>inactive -> canary -> active]

    robot_yaml --> compiler
    protocol_spec --> compiler
    schemas --> compiler
    compiler --> registry_lock
    registry_lock --> quality_gate
    quality_gate --> signed_bundle
    signed_bundle --> ab_deploy
  end

  subgraph Host[Ubuntu 机器人主机运行时]
    direction TB

    subgraph AppPlane[应用与运维平面（非实时）]
      direction LR
      mission[任务与规划]
      perception[感知与融合]
      teleop[遥控与手柄]
      ops[robot_ops<br/>标定、诊断、OTA、robotctl]
      recorder[rosbag2 / MCAP<br/>录包与回放]
    end

    subgraph RosPlane[ROS 2 控制平面（可重启、可观测）]
      direction LR
      dds[DDS / RMW<br/>发现、QoS、SROS2]
      controllers[controller_manager<br/>ros2_control controllers]
      mux[command_mux<br/>来源、租约、优先级、deadman]
      lifecycle[lifecycle / bringup<br/>配置、资源 claim、模式切换]
      facade[robot_hardware<br/>SystemInterface facade]
      ros_obs[ROS diagnostics / tracing<br/>标准消息与健康状态]
    end

    subgraph Boundary[受控进程边界（固定大小、版本化）]
      direction LR
      cmd_ipc[Command snapshot<br/>latest-value + deadline + lease + CRC]
      state_ipc[State snapshot<br/>sequence + timestamp + valid mask + CRC]
      event_ipc[Safety/event ring<br/>有界、不可被遥测挤占]
      rt_watchdog[IPC heartbeat<br/>超时触发本地 fail-action]
    end

    subgraph RT[robot_rt 硬实时平面（不依赖 DDS/rclcpp）]
      direction TB
      scheduler[RT scheduler<br/>固定周期、相位、预算、锁页]
      safety[Safety supervisor<br/>arm、限幅、斜率、deadline、故障锁存]
      snapshots[RT snapshots<br/>命令/状态所有权与时间戳]
      registry[静态 registry<br/>启动加载后冻结]

      subgraph DeviceStack[协议与设备层（按实例编译，周期内无动态加载）]
        direction LR
        drivers[Device drivers<br/>生命周期、能力、诊断、fail-action]
        profiles[Device profiles<br/>身份、映射、单位、限制、时序]
        codecs[Protocol codecs<br/>纯函数、边界检查、golden vectors]
        framing[Framing<br/>长度、分片、CRC、重同步]
        transports[Transport backends<br/>EtherCAT、SocketCAN、UART、UDP]
      end

      flight[Flight recorder<br/>cycle、WCET、jitter、队列水位、raw event]

      scheduler --> snapshots
      snapshots --> safety
      safety --> drivers
      registry --> drivers
      drivers --> codecs
      drivers -->|feedback snapshot| snapshots
      drivers <--> profiles
      profiles <--> codecs
      codecs <--> framing
      framing <--> transports
      scheduler --> flight
      safety --> flight
      transports --> flight
    end
  end

  subgraph Physical[总线、设备与独立安全链路]
    direction TB
    subgraph Buses[物理通信]
      direction LR
      ethercat[EtherCAT<br/>PDO / WKC / DC]
      can[CAN / CAN-FD<br/>SocketCAN / MCAN]
      serial[UART / RS-485]
      udp[受控 UDP / 专用链路]
    end
    subgraph Devices[设备实例]
      direction LR
      actuators[执行器<br/>电机、驱动器、制动器]
      sensors[传感器<br/>IMU、编码器、力/温度]
      mcu[MCU / 电源板<br/>本地状态机与 watchdog]
    end
    safety_hw[独立安全链路<br/>急停、STO、限位、电源 watchdog]
    sto[驱动器安全输入 / STO]

    ethercat <--> actuators
    can <--> actuators
    serial <--> sensors
    udp <--> mcu
    mcu <--> actuators
    mcu <--> sensors
    safety_hw --> sto
    safety_hw --> mcu
  end

  subgraph Validation[仿真、HIL 与证据]
    direction LR
    virtual[virtual transport<br/>确定性回放与 SIL]
    hil[HIL / 单轴台架<br/>断线、bus-off、stale、急停]
    evidence[证据库<br/>trace、capture、报告、profile hash]
    virtual --> evidence
    hil --> evidence
  end

  %% 应用和 ROS 2 数据流
  mission --> dds
  perception --> dds
  teleop --> dds
  dds --> controllers
  dds --> mux
  ops <--> dds
  controllers --> mux
  lifecycle --> facade
  mux --> facade
  facade --> cmd_ipc
  cmd_ipc --> rt_watchdog
  cmd_ipc --> snapshots

  %% RT 状态、事件和观测向上汇聚
  snapshots -->|feedback snapshot| state_ipc
  safety --> event_ipc
  flight --> event_ipc
  state_ipc -->|feedback snapshot| facade
  facade -->|state projection| ros_obs
  event_ipc --> ros_obs
  ros_obs --> dds
  ros_obs --> recorder
  recorder --> evidence
  ops --> recorder

  %% 物理 I/O 与运行时的绑定
  transports <--> ethercat
  transports <--> can
  transports <--> serial
  transports <--> udp
  safety -.请求 / 监测.-> safety_hw
  rt_watchdog -.超时 / 失联.-> safety
  safety_hw -.旁路停机 / 状态.-> safety

  %% 迁移入口：现有路径保留为适配器，逐步收敛到新边界
  legacy[现有 hpm_gateway_control<br/>GatewayEcat + LatestBuffer<br/>迁移期兼容适配器]
  power[现有 hpm_gateway_power<br/>普通 ROS 2 节点]
  gamepad[现有 gamepad 节点<br/>普通 ROS 2 节点]
  legacy -.shadow / fallback.-> facade
  power -.-> dds
  gamepad -.-> dds

  %% 交付链注入运行时和验证环境
  ab_deploy -.签名 profile / plugin.-> lifecycle
  ab_deploy -.锁文件 / 静态表.-> registry
  signed_bundle -.测试输入.-> virtual
  signed_bundle -.测试输入.-> hil
  virtual -.替代 transport 后端.-> transports
  hil -.真实链路验证.-> transports

  classDef plane fill:#e8f0f8,stroke:#315a7d,color:#102a43
  classDef rt fill:#e8f5ec,stroke:#28734d,color:#123b26
  classDef boundary fill:#fff4dd,stroke:#9a6700,color:#4d3200
  classDef safety fill:#fde8e7,stroke:#b42318,color:#5f1712
  classDef artifact fill:#f2ecfb,stroke:#6941c6,color:#32176b
  classDef legacy fill:#f3f4f6,stroke:#6b7280,color:#374151

  class mission,perception,teleop,ops,recorder,dds,controllers,mux,lifecycle,facade,ros_obs plane
  class scheduler,safety,snapshots,registry,drivers,profiles,codecs,framing,transports,flight rt
  class cmd_ipc,state_ipc,event_ipc,rt_watchdog boundary
  class safety_hw,sto,mcu safety
  class robot_yaml,protocol_spec,schemas,compiler,registry_lock,quality_gate,signed_bundle,ab_deploy,virtual,hil,evidence artifact
  class legacy,power,gamepad legacy
```

### 图例与不变量

| 图中关系 | 含义 | 必须保持的约束 |
| --- | --- | --- |
| 实线向下 | 命令/配置/控制依赖 | 命令只能经 `command_mux`、IPC 和 `safety` 到达设备 |
| 实线向上 | 状态/诊断/观测 | 状态唯一事实来源是 `robot_rt` snapshot；ROS 只是投影 |
| 虚线 | 旁路、迁移或安全触发 | 不把虚线通路当作普通控制闭环；超时必须有明确 fail-action |
| 黄色边界 | 进程间 IPC | 固定大小、schema/version、sequence、deadline、CRC、有限队列 |
| 绿色区域 | 硬实时所有权 | 禁止 DDS、阻塞 I/O、动态分配、日志和运行时插件加载 |
| 红色区域 | 独立安全机制 | 急停、STO、限位、硬件/MCU watchdog 不依赖 ROS 运行 |
| 紫色区域 | 离线交付与验证 | 只有经过校验、签名和预算检查的 artifact 才能激活 |

## 新增协议、执行器或传感器的落点

```mermaid
flowchart LR
  request[需求 / 厂商协议] --> rfc[协议 RFC<br/>ID、版本、时序、安全语义]
  rfc --> spec[spec.yaml + golden vectors]
  spec --> codec[纯 codec<br/>frame <-> typed data]
  codec --> reg[registry<br/>过滤器、能力、WCET、许可证]
  reg --> profile[device profile<br/>身份、单位、限幅、时序]
  profile --> driver[driver<br/>状态机、fail-action、诊断]
  driver --> virtual[virtual transport / SIL]
  virtual --> hil[单设备 HIL / 故障注入]
  hil --> canary[单总线 canary<br/>shadow -> active]
  canary --> release[签名发布 + A/B 回滚]

  profile --> rosmap[ROS mapping<br/>标准接口、frame_id、QoS]
  driver --> rtobs[RT metrics / trace]
  rtobs --> evidence[验证证据与运行手册]
```

新增内容必须沿着这条链路进入。协议 codec 不直接接 ROS，ROS 只看到 canonical command/state；执行器和传感器的设备差异由 profile/driver 消化。任何新增实例都要重新计算总线占用、内存、队列和周期预算。

## 分阶段落地

```mermaid
flowchart LR
  m0[M0 契约冻结<br/>基线测量、PDO/ID、许可证] --> m1[M1 RT + IPC<br/>scheduler、watchdog、snapshot]
  m1 --> m2[M2 Registry + Profile<br/>协议/设备可扩展]
  m2 --> m3[M3 ROS 2 facade<br/>ros2_control、mux、lifecycle]
  m3 --> m4[M4 安全与观测<br/>SROS2、trace、MCAP、A/B]
  m4 --> m5[M5 生产门禁<br/>全轴 HIL、长稳、现场回滚]

  m0 -.现有 GatewayEcat.-> m1
  m2 -.RS06 / ENCOS / YESENSE.-> m3
  m3 -.Humble 兼容 profile.-> m4
```

每个里程碑都必须有可回放的原始 trace、配置/协议 hash 和故障证据；未通过门禁的模块保持在 shadow、QUARANTINED 或 inactive slot，不直接成为新的运动写入者。

## 相关设计

- [01-architecture.md](01-architecture.md)：模块职责、状态机、数据所有权和迁移边界
- [03-device-model-and-extension.md](03-device-model-and-extension.md)：设备描述与执行器/传感器接入
- [04-protocol-extension.md](04-protocol-extension.md)：协议分层、注册表、版本和回滚
- [05-ros2-integration.md](05-ros2-integration.md)：ROS 2、`ros2_control`、QoS 与 IPC
- [06-observability-and-debugging.md](06-observability-and-debugging.md)：指标、trace、录包和故障定位
- [09-testing-and-validation.md](09-testing-and-validation.md)：SIL、HIL、故障注入和发布证据
- [10-roadmap.md](10-roadmap.md)：M0--M5 实施计划
