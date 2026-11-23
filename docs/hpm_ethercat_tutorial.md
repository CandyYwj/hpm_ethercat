<!-- Author: BearKing -->
# HPMicro MCU EtherCAT 从零入门教程

> **目标读者**：零基础，不了解 HPM 单片机，不了解 EtherCAT 协议。
> **学习目标**：理解 EtherCAT 从站工作原理，掌握在 HPM MCU 上开发 EtherCAT 从站的方法，能够独立修改 PDO 和 SDO。
> **配套源码**：[hpm_app/software/apps/ecat_io/](../../hpm_app/software/apps/ecat_io/)（官方示例）、[gatewat/hpm_gateway](../../gatewat/hpm_gateway/)（网关项目）
> **最后更新**：2026-07-08

---

## 目录

1. [基础知识篇](#1-基础知识篇)
   - 1.1 什么是 EtherCAT
   - 1.2 EtherCAT 通信模型
   - 1.3 关键概念速查表
   - 1.4 用对讲机比喻理解整个系统
   - 1.5 HPM 单片机家族
2. [硬件篇：HPM6E80 的 ESC 外设](#2-硬件篇hpm6e80-的-esc-外设)
   - 2.1 ESC 在芯片中的位置
   - 2.2 DPRAM（双端口 RAM）— 核心数据通道
   - 2.3 SyncManager（同步管理器）
   - 2.4 FMMU（现场总线内存管理单元）
   - 2.5 三个 EtherCAT 端口
   - 2.6 EEPROM 与 ESI 文件
3. [驱动篇：ESC 驱动 API](#3-驱动篇esc-驱动-api)
   - 3.1 寄存器访问宏（HW 层）
   - 3.2 SDK ESC 驱动 API
   - 3.3 中断系统
   - 3.4 定时器
   - 3.5 PHY 管理（MDIO）
4. [协议栈篇：Beckhoff SSC](#4-协议栈篇beckhoff-ssc)
   - 4.1 SSC 是什么
   - 4.2 SSC 的文件结构
   - 4.3 核心流程：MainInit → MainLoop
   - 4.4 EtherCAT 状态机（ESM）
   - 4.5 PDO 映射：从对象字典到物理 IO
   - 4.6 CoE（CANopen over EtherCAT）
   - 4.7 分布式时钟（DC）
   - 4.8 三种同步模式
5. [实战篇：从零搭建 EtherCAT 从站](#5-实战篇从零搭建-ethercat-从站)
   - 5.1 环境搭建
   - 5.2 理解 ecat_io 示例
   - 5.3 编译固件
   - 5.4 烧录与调试
   - 5.5 用 Linux 主机测试
6. [进阶篇：修改 PDO 和 SDO](#6-进阶篇修改-pdo-和-sdo)
   - 6.1 理解 PDO mapping 条目编码格式
   - 6.2 分步修改 RxPDO（主站→从站）
   - 6.3 分步修改 TxPDO（从站→主站）
   - 6.4 修改 APPL_InputMapping 和 APPL_OutputMapping
   - 6.5 添加 SDO（只读、读写、写后触发）
   - 6.6 MAX_PD 长度配置
   - 6.7 ESI XML 同步——最容易踩的坑
   - 6.8 修改后验证步骤
   - 6.9 常见问题排查
   - 6.10 EEPROM 模拟原理
   - 6.11 多端口配置
   - 6.12 推荐电机控制对象规划
   - 6.13 修改顺序建议
7. [参考索引](#7-参考索引)

---

## 1. 基础知识篇

### 1.1 什么是 EtherCAT

**EtherCAT**（Ethernet for Control Automation Technology）是一种实时工业以太网协议，由德国 Beckhoff 公司发明，现由 ETG（EtherCAT Technology Group）维护。

一句话概括：**一个主站（Master）每隔几十微秒发一个以太网帧，这个帧像火车一样穿过所有从站（Slave），每个从站在帧经过时 "摘取" 属于自己的数据、"放入" 要上报的数据，然后帧继续前进，最后回到主站。**

与普通 TCP/IP 网络的关键区别：

| | 普通以太网 | EtherCAT |
|---|---|---|
| **通信模式** | 点对点，每个设备独立收发 | 一帧穿越所有设备（集束帧） |
| **实时性** | 毫秒级，不确定 | 微秒级，确定性 |
| **数据提取** | CPU 解析协议栈（TCP/IP） | 硬件（ESC）自动提取/插入，CPU 零拷贝 |
| **拓扑** | 星形（交换机） | 总线/树形/环形/星形 |
| **典型周期** | 1~100ms | 50μs ~ 10ms |

```
EtherCAT 数据帧的旅程：

  主站发帧
    │
    ▼
  ┌─────────┐     ┌─────────┐     ┌─────────┐
  │ 从站 1   │────▶│ 从站 2   │────▶│ 从站 3   │
  │ 读: 输出  │     │ 读: 输出  │     │ 读: 输出  │
  │ 写: 输入  │     │ 写: 输入  │     │ 写: 输入  │
  └─────────┘     └─────────┘     └─────────┘
                                        │
   帧原路返回（或从最后一个从站直接返回）  │
    ◄────────────────────────────────────┘
```

**为什么 EtherCAT 这么快？**

- **硬件处理**：每个从站有一片 ESC（EtherCAT Slave Controller）芯片/外设，帧到达时 ESC 硬件自动在 DPRAM 中读写数据，CPU 不参与帧的转发
- **飞读飞写（on-the-fly）**：帧经过 ESC 时，ESC 一边接收一边处理一边转发，延迟仅约 1μs（一个端口）
- **集束帧**：一帧携带所有从站的数据，省去每个从站独立收发的时间

### 1.2 EtherCAT 通信模型

```
┌──────────────────────────────────────────────────────────┐
│                    EtherCAT 主站                          │
│  (PC 运行 TwinCAT / Linux 运行 IGH EtherCAT Master)       │
│  负责：调度周期、发送帧、状态机管理、SDO 配置              │
└──────────────────────┬───────────────────────────────────┘
                       │ 以太网帧（EtherType=0x88A4）
                       ▼
┌──────────────────────────────────────────────────────────┐
│                    EtherCAT 从站                          │
│  ┌──────────────────────────────────────────────────┐    │
│  │  ESC（EtherCAT Slave Controller）                 │    │
│  │  硬件自动：帧解析、DPRAM 读写、SM 管理、FMMU 映射  │    │
│  └──────────────────────┬───────────────────────────┘    │
│                         │ SPI / 并行总线 / 片上总线       │
│  ┌──────────────────────▼───────────────────────────┐    │
│  │  从站 MCU（HPM6E80 RISC-V 核）                    │    │
│  │  运行 SSC 协议栈：状态机、CoE、PDO 数据应用处理     │    │
│  └──────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────┘
```

在你的 HPM6E80 上，ESC 和 MCU 是**同一颗芯片**——ESC 是芯片内部的一个硬件 IP 模块，MCU 通过片上总线直接读写 ESC 的寄存器（称为 **PDI = Process Data Interface**）。

### 1.3 关键概念速查表

学习 EtherCAT 时会遇到大量缩写，这里集中解释：

| 缩写 | 全称 | 含义 | 初学者理解 |
|---|---|---|---|
| **ESC** | EtherCAT Slave Controller | EtherCAT 从站控制器（硬件 IP） | 负责收发以太网帧的硬件，帧来了它自动读写数据 |
| **PDI** | Process Data Interface | 过程数据接口 | MCU 和 ESC 之间通信的接口（HPM 上是片上总线） |
| **DPRAM** | Dual-Port RAM | 双端口 RAM | ESC 内部的内存，主站和 MCU 都能读写，是数据交换的中转站 |
| **SM** | SyncManager | 同步管理器 | DPRAM 的"保安"，确保主站和 MCU 不会同时写同一块内存 |
| **FMMU** | Fieldbus Memory Management Unit | 现场总线内存管理单元 | 把主站的逻辑地址映射到从站的物理 DPRAM 地址 |
| **PDO** | Process Data Object | 过程数据对象 | 每个周期都要交换的数据（如 IO 状态、速度指令） |
| **SDO** | Service Data Object | 服务数据对象 | 偶尔读写的数据（如配置参数、设备名称），走邮箱协议 |
| **CoE** | CANopen over EtherCAT | 基于 CANopen 的对象字典 | EtherCAT 用 CANopen 的方式组织参数（对象字典） |
| **ESI** | EtherCAT Slave Information | 从站信息文件 | XML 文件，描述从站的 PDO/SDO/SM 配置，供主站识别 |
| **ENI** | EtherCAT Network Information | 网络信息文件 | XML 文件，描述整个 EtherCAT 网络的拓扑和配置 |
| **DC** | Distributed Clocks | 分布式时钟 | 让所有从站的时钟同步到纳秒级 |
| **ESM** | EtherCAT State Machine | EtherCAT 状态机 | Init → PreOP → SafeOP → OP 四个状态，控制从站的运行阶段 |
| **SSC** | Slave Stack Code | 从站协议栈代码 | Beckhoff 提供的 C 语言 EtherCAT 从站代码实现 |

### 1.4 用对讲机比喻理解整个系统

如果你觉得上面这些缩写太抽象，可以用对讲机系统来类比：

```
一个建筑工地上的对讲机系统：

  ┌─────────────────────────────────────────────────────────┐
  │  工头（主站）                                            │
  │  每隔 1 秒喊一次话                                       │
  └────────────────────────┬────────────────────────────────┘
                           │ 无线电波（以太网帧）
                           ▼
  ┌─────────────────────────────────────────────────────────┐
  │  工人 1（从站 1）──▶ 工人 2（从站 2）──▶ 工人 3（从站 3）  │
  │  听到"向左"就执行    听到"向右"就执行    听到"停"就执行    │
  │  汇报"我在左边"      汇报"我在右边"      汇报"已停止"      │
  └─────────────────────────────────────────────────────────┘
```

对照到 EtherCAT：

| 对讲机比喻 | EtherCAT | 说明 |
|---|---|---|
| 工头喊话 | 主站发帧 | 一帧穿过所有工人 |
| 喊话内容"向左30度" | **PDO** (过程数据) | 每轮都喊，实时命令 |
| 偶尔喊"你的工号是多少" | **SDO** (服务数据) | 偶尔问一次，配置信息 |
| 工人听到指令执行 | SM2 → `APPL_OutputMapping()` | 主站→从站的数据通道 |
| 工人汇报状态 | SM3 → `APPL_InputMapping()` | 从站→主站的数据通道 |
| 每个工人的对讲机 | **ESC** (从站控制器) | 硬件自动处理"喊话"和"汇报" |
| 内部通讯录(门牌号体系) | **对象字典** (Object Dictionary) | 0x6000 是状态, 0x7010 是指令 |
| 工人档案(入职登记表) | **ESI 文件** (XML) | 告诉工头这个工人能做什么 |
| 喊话顺序不能乱 | **SyncManager** (SM) | 保安：确保不抢话筒 |
| 喊话不用工人确认 | PDO 不确认 | 丢了就等下一轮 |
| 问工号需要工人回答 | SDO 需要确认 | 必须有回应 |

**关于 SDO 的一个重要概念**——SDO 是"带确认"的通信：

```
PDO（不确认）:
  工头喊: "向左30度!" ──────▶ 工人执行
  （不期待回应，下一轮继续喊）

SDO（确认→回应）:
  工头: "你的工号是多少？" ──▶  工人: "我的工号是9527"
  工头: "把工号改成9528"  ──▶  工人: "已改成9528"
```

### 1.5 HPM 单片机家族

**HPMicro**（先楫半导体）是一家中国 RISC-V MCU 厂商。HPM 系列基于 **Andes RISC-V 32 位内核**，主频最高可达 816MHz（HPM6E80）。

**哪些 HPM 芯片带 ESC（EtherCAT 从站控制器）？**

| 系列 | 代表型号 | 内核 | 主频 | ESC | 典型应用 |
|---|---|---|---|---|---|
| HPM6E00 | HPM6E80 | RISC-V ×2 | 816MHz | ✅ 3 端口 | 伺服驱动、IO 从站、TSN 网关 |
| HPM6700 | HPM6750 | RISC-V ×2 | 816MHz | ❌ | 高性能通用控制 |
| HPM6300 | HPM6350 | RISC-V ×1 | 600MHz | ❌ | 通用控制 |
| HPM6200 | HPM6280 | RISC-V ×2 | 600MHz | ❌ | 电机控制 |
| HPM5300 | HPM5361 | RISC-V ×1 | 360MHz | ❌ | 成本敏感型 |

只有 **HPM6E00 系列**内置 ESC 外设。本文以 **HPM6E80** 为例。

**HPM SDK 目录结构**（只需要知道几个关键位置）：

```
sdk_env/
├── hpm_sdk/                  ← SDK 源码
│   ├── drivers/              ← 外设驱动（包括 ESC 驱动）
│   │   ├── inc/hpm_esc_drv.h ← ESC 驱动 API 头文件
│   │   └── src/hpm_esc_drv.c ← ESC 驱动实现
│   ├── soc/HPM6E00/HPM6E80/  ← HPM6E80 芯片特定代码（系统时钟、启动等）
│   ├── boards/               ← 官方评估板定义
│   └── cmake/                ← CMake 构建系统
├── hpm_app/                  ← 应用层项目
│   ├── boards/hpm6e00_full_port/ ← 全端口评估板（含 EtherCAT）
│   └── software/
│       ├── drivers/          ← 应用层驱动
│       │   └── src/full_ecat_port/  ← ESC 硬件抽象层
│       └── apps/ecat_io/     ← EtherCAT IO 从站示例 ★ 重点学习对象
└── toolchains/               ← RISC-V 交叉编译工具链
```

---

## 2. 硬件篇：HPM6E80 的 ESC 外设

### 2.1 ESC 在芯片中的位置

HPM6E80 内部集成的 ESC 兼容 Beckhoff ET1100。ESC 通过片上总线连接到 RISC-V 内核，MCU 直接以内存映射方式访问 ESC 的所有寄存器。

```
HPM6E80 芯片内部：
┌───────────────────────────────────────────────────────┐
│  RISC-V CPU0 (主核)            RISC-V CPU1 (辅核)      │
│  (运行 EtherCAT 协议栈)                                │
│       │                                                  │
│       │  AHB/APB 片上总线                                │
│       │                                                  │
│  ┌────▼─────────────────────────────────────────────┐  │
│  │  ESC 模块                                          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐        │  │
│  │  │  Port 0  │  │  Port 1  │  │  Port 2  │        │  │
│  │  │  MII/GMII│  │  MII/GMII│  │  MII/GMII│        │  │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘        │  │
│  │       │              │              │              │  │
│  │  ┌────▼──────────────▼──────────────▼─────┐       │  │
│  │  │  内部以太网交换机 (EtherCAT Processing) │       │  │
│  │  └────────────────────────────────────────┘       │  │
│  │  ┌──────────┐  ┌──────────┐                      │  │
│  │  │  DPRAM   │  │  FMMU    │                      │  │
│  │  │ (60KB+)  │  │  ×8      │                      │  │
│  │  └──────────┘  └──────────┘                      │  │
│  │  ┌──────────┐                                     │  │
│  │  │  SM ×8   │  MCU 通过 0x4000_0000 基址访问      │  │
│  │  └──────────┘                                     │  │
│  └──────────────────────────────────────────────────┘  │
│       │                                                  │
│       │  ┌────────────────────┐                          │
│       └──► HPM_ESC 宏         │                          │
│          │  = (ESC_Type *)    │                          │
│          │  0x4000_0000       │                          │
│          └────────────────────┘                          │
└───────────────────────────────────────────────────────┘
```

ESC 寄存器基址是 `0x40000000`，在代码中通过 `HPM_ESC` 宏访问：

```c
// hpm_sdk 中定义
#define HPM_ESC_BASE  0x40000000UL
#define HPM_ESC       ((ESC_Type *) HPM_ESC_BASE)

// 代码中直接使用
HPM_ESC->PHY_CFG0 = ...;          // 配置 PHY
uint32_t val = HPM_ESC->GPR_CFG0; // 读通用配置
```

### 2.2 DPRAM（双端口 RAM）— 核心数据通道

DPRAM 是 ESC 内部的一块共享内存，**主站和从站 MCU 都能访问**。这是 EtherCAT 过程数据交换的核心通道。

```
DPRAM 地址空间（0x0000 ~ 0xFFFF，共 64KB）：

  0x0000 ┌──────────────────────┐
         │  ESC 寄存器区         │  ← 硬件状态/控制寄存器
         │  (0x0000 ~ 0x0FFF)   │
  0x1000 ├──────────────────────┤
         │                      │
         │  用户数据区           │  ← 过程数据、邮箱数据存放于此
         │  (0x1000 ~ 0xFFFF)   │
         │                      │
  0xFFFF └──────────────────────┘

主站通过以太网帧访问这 64KB → ESC 硬件自动把数据放到对应位置
MCU 通过内存映射读写这 64KB → 和读写普通 RAM 一样简单
```

**主站和 MCU 如何交换数据？**

```
  [主站] ──以太网帧──▶  ESC 硬件  ──写入──▶  DPRAM (SM2 区域)
                                                   │
                                          MCU 通过 HW_EscRead 读取
                                          APPL_OutputMapping()
                                                   │
                                                   ▼
                                          OutputCounter0x7010 → GPIO 输出

  [主站] ◀──以太网帧──  ESC 硬件  ◀──读取──  DPRAM (SM3 区域)
                                                   ▲
                                                   │
                                          MCU 通过 HW_EscWrite 写入
                                          APPL_InputMapping()
                                                   ▲
                                                   │
                                          InputCounter0x6000 ← GPIO 输入
```

### 2.3 SyncManager（同步管理器）

SM 是 DPRAM 的"门禁系统"，确保主站和 MCU 不会同时写同一块内存（引发数据竞态）。

- ESC 有 **8 个 SyncManager（SM0 ~ SM7）**
- 每个 SM 在 DPRAM 中划定一块区域（起始地址 + 长度）
- SM 使用**三缓冲区模式**：MCU 和主站交替读写不同的缓冲区，互不阻塞

**深入理解"三缓冲区"**：想象一个可旋转的三面白板——

```
          ┌─────────────────┐
  主站写 →│  缓冲区 0        │  正被主站写入
          ├─────────────────┤
  空闲   →│  缓冲区 1        │  等待轮换
          ├─────────────────┤
  MCU读 → │  缓冲区 2        │  正被 MCU 读出
          └─────────────────┘

每轮结束时"旋转"：
  - 主站刚写完的缓冲区 → 变成 MCU 可读的
  - MCU 刚读完的缓冲区 → 变成空闲的
  - 空闲的缓冲区 → 变成主站可写的

三者永不同步，所以永远不需要锁。
```

**EtherCAT 从站的 SM 典型配置：**

| SM | 方向 | 用途 | 缓冲区大小 |
|---|---|---|---|
| **SM0** | 主站→从站 | 邮箱输出（Mailbox Out） | 128~256 字节 |
| **SM1** | 从站→主站 | 邮箱输入（Mailbox In） | 128~256 字节 |
| **SM2** | 主站→从站 | 过程数据输出（RxPDO） | 由 PDO 映射决定 |
| **SM3** | 从站→主站 | 过程数据输入（TxPDO） | 由 PDO 映射决定 |

**SM0~SM3 是最常用的 4 个**。SM4~SM7 预留给特殊功能（如 FoE 固件更新、多通道 DC 同步等），ecat_io 和网关项目都没有使用它们。

**SM 配置的"动态地址"特性**：

SM2 和 SM3 在 DPRAM 中的实际缓冲区地址**不是硬件固定死的**，而是由 EtherCAT 主站在启动时（PREOP→SAFEOP 状态转换）通过 SDO 动态配置。每个 SM 有 8 字节配置寄存器：

```
SM 配置寄存器（位于 ESC 寄存器空间偏移 0x0800 起）:

  SM0 配置: 0x0800 ~ 0x0807
  SM1 配置: 0x0808 ~ 0x080F
  SM2 配置: 0x0810 ~ 0x0817  ← 过程数据输出
  SM3 配置: 0x0818 ~ 0x081F  ← 过程数据输入
  ...

每个 SM 的 8 字节结构:
  [31:16] 缓冲区长度        [15:0] 物理起始地址
  ┌──────────────────────┬──────────────────────┐
  │      Length           │   Physical Address   │  ← 4 bytes (AddressLength)
  ├─────────────────────────────────────────────┤
  │  Control / Status / Activate / PDI_Control  │  ← 4 bytes (Settings)
  └─────────────────────────────────────────────┘
```

MCU 在运行时通过 `GetSyncMan()` 函数动态读取这些配置——主站把地址写进 SM 寄存器，MCU 从这里读出来，然后才知道去哪里读写数据。

**典型的 DPRAM 布局示例**（取决于 ESI 配置，以下是常见值）：

```
DPRAM 偏移    内容
────────────────────────────────────
0x0000~0x07FF   ESC 寄存器区（不是 SM 管理的）
0x0800~0x0FFF   SM 配置寄存器区（8个SM × 8字节）
0x1000~0x10FF   SM0 邮箱输出 (~256B)
0x1100~0x11FF   SM1 邮箱输入 (~256B)
0x1200~0x12FF   SM2 过程数据输出 (RxPDO, 60B 实际使用)
0x1300~0x13FF   SM3 过程数据输入 (TxPDO, 60B 实际使用)
```

SM2 和 SM3 就是主站和从站交换 IO 数据的高速通道。每个 EtherCAT 周期，主站通过 SM2 把输出数据写入 DPRAM，从 SM3 读出输入数据。

### 2.4 FMMU（现场总线内存管理单元）

FMMU 把主站的**逻辑地址**映射到从站的**物理 DPRAM 地址**。

```
主站眼中的地址（逻辑地址）        从站 DPRAM 中的地址（物理地址）
────────────────────────────    ────────────────────────────────
  0x0000_1000 ──── FMMU0 ────▶  SM2 起始地址 (如 0x1200)
  0x0000_2000 ──── FMMU1 ────▶  SM3 起始地址 (如 0x1300)
```

- 你不用手动配置 FMMU → 主站在启动阶段通过 SDO 写入 SM 配置，ESC 硬件自动设置 FMMU

**比喻**：FMMU 就像邮局的地址翻译——
- 主站写"上海市南京路1号邮政信箱10号"（逻辑地址 0x1000）
- FMMU 翻译成"DPRAM 偏移 0x1200"（物理地址）
- 数据准确送达

### 2.5 三个 EtherCAT 端口

HPM6E80 的 ESC 有 3 个物理端口（P0/P1/P2），每端口各挂一个 PHY 芯片，共享一组 MDIO 管理总线。

```
典型连接拓扑（以 ecat_io 应用为例）：

  主站（PC）
    │ 以太网线
    ▼
  ESC P0 ◀── 上行端口，连主站
  ESC P1 ◀── 下行端口，连下一个从站（若有）
  ESC P2 ◀── 下行端口，连下一个从站（若有）
```

**PHY 地址计算**：

```
P0 PHY addr = <基址> + 0
P1 PHY addr = <基址> + PHY_OFFSET_VAL      ← PHY_OFFSET_VAL 默认为 1
P2 PHY addr = <基址> + PHY_OFFSET_VAL × 2
```

在 `main.c` 中通过寄存器操作设置：

```c
// 设置 OFFSET = 1，地址连续
HPM_ESC->PHY_CFG0 = ((HPM_ESC->PHY_CFG0 & (~0x1F000000UL)) | (1 << 24));
// 或使用 SDK 封装
esc_set_phy_offset(HPM_ESC, 1);
```

**端口可配置为禁用**（如果硬件上只需要 1 个端口）：

```c
esc_change_port_description(HPM_ESC, false, true, true);
// P0=正常, P1=未实现, P2=未实现
```

### 2.6 EEPROM 与 ESI 文件

每个 EtherCAT 从站上电后，ESC 会读取 EEPROM 来获取自己的"身份证"和初始配置：

**EEPROM 里存什么？**

```
EEPROM (如 24C02, 2Kbit = 256 字节)：

  Word 0x00: ESC 配置数据（端口数、PDI 类型等）
  Word 0x04: 厂商 ID (0x0048504D = "HPM")
  Word 0x06: 产品 ID (0x00000001)
  Word 0x08: 版本号
  Word 0x0A: 序列号
  Word 0x10: 邮箱配置（SM0/1 大小和地址）
  Word 0x14: 过程数据配置（SM2/3 大小和地址）
  ...        ...
```

**硬件 EEPROM vs Flash 模拟**：

- **硬件方案**：外挂一颗 I2C EEPROM 芯片（如 AT24C02），ESC 上电后自动通过 I2C 读取
- **软件方案（Flash 模拟）**：没有外挂 EEPROM，MCU 在 Flash 中保留 16KB 空间存放 EEPROM 内容，ESC 配置为 EEPROM 模拟模式。当 ESC 发出"读 EEPROM"请求时触发中断，MCU 从 Flash 读取数据后写入 ESC 的 EEPROM_DATA 寄存器。

HPM6E00 全端口板使用 Flash 模拟方案。代码中的配置：

```c
// ecat_config.h
#define FLASH_EEPROM_ADDR  0x200000  // NOR Flash 偏移 2MB
#define FLASH_EEPROM_SIZE  0x10000   // 64KB 保留区

// main.c - 注册 Flash 模拟回调
pAPPL_EEPROM_Read   = ecat_eeprom_emulation_read;
pAPPL_EEPROM_Write  = ecat_eeprom_emulation_write;
pAPPL_EEPROM_Reload = ecat_eeprom_emulation_reload;
pAPPL_EEPROM_Store  = ecat_eeprom_emulation_store;
```

**ESI 文件**：XML 文件，描述从站的 PDO 映射、SM 配置、对象字典等。主站工具（TwinCAT、Linux IGH）导入 ESI 文件后就能识别从站。ecat_io 项目的 ESI 文件位于：

```
SSC/ESI/ECAT_IO.xml    ← 由 Beckhoff SSC Tool 根据对象字典自动生成
```

**关键概念**：ESI 是给主站看的"说明书"，EEPROM 是给 ESC 硬件看的"身份证"。二者描述的是同一个从站——说明书说"我有 60 字节 PDO"，身份证（和固件实现）也必须一致。

---

## 3. 驱动篇：ESC 驱动 API

HPM SDK 的 ESC 驱动有两层：

```
  ┌────────────────────────────┐
  │  SSC 协议栈（Beckhoff）      │  ← 调用 HW_EscRead/Write 宏
  ├────────────────────────────┤
  │  ESC 硬件抽象（hpm_ecat_hw） │  ← 实现 HW_* 宏、中断服务、EEPROM 模拟
  ├────────────────────────────┤
  │  SDK ESC 驱动（hpm_esc_drv） │  ← 寄存器级 API（PHY、EEPROM、时钟、IRQ）
  ├────────────────────────────┤
  │  ESC 硬件寄存器             │  ← 基址 0x4000_0000
  └────────────────────────────┘
```

### 3.1 寄存器访问宏（HW 层）

定义在 [hpm_ecat_hw.h](../../hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.h)，是 SSC 协议栈与 ESC 硬件之间的桥梁：

```c
// ESC 基址指针
extern MEM_ADDR ESCMEM * pEsc;  // 在 HW_Init() 中初始化为 HPM_ESC_BASE

// 读 ESC 寄存器
#define HW_EscReadByte(ByteValue, Address)
// 等价于: ByteValue = ((volatile UINT8 *)pEsc)[Address]

#define HW_EscReadWord(WordValue, Address)
// 等价于: WordValue = ((volatile UINT16 *)pEsc)[Address >> 1]

#define HW_EscReadDWord(DWordValue, Address)
// 等价于: DWordValue = ((volatile UINT32 *)pEsc)[Address >> 2]

// 写 ESC 寄存器
#define HW_EscWriteByte(ByteValue, Address)
#define HW_EscWriteWord(WordValue, Address)

// 块读写（连续多个字节）
#define HW_EscRead(pData, Address, Len)
#define HW_EscWrite(pData, Address, Len)
```

这些宏不是你在应用层直接调用的——SSC 协议栈内部使用它们。但理解它们有助于看懂协议栈代码。

### 3.2 SDK ESC 驱动 API

定义在 [hpm_esc_drv.h](../../hpm_sdk/drivers/inc/hpm_esc_drv.h)，提供寄存器级操作：

```c
// ============ 时钟配置 ============
esc_core_enable_clock(HPM_ESC, true);   // 使能 ESC 核心时钟（100MHz）
esc_phy_enable_clock(HPM_ESC, true);    // 使能 ESC PHY 25MHz 参考时钟

// ============ EEPROM 配置 ============
esc_eeprom_clock_config_t config = {
    .eeprom_emulation = true,        // 使用 Flash 模拟 EEPROM
    .core_clock_en = true,
    .phy_refclk_en = true,
};
esc_config_eeprom_and_clock(HPM_ESC, &config);  // 一次性配置

// ============ EEPROM 状态检查 ============
hpm_stat_t stat = esc_check_eeprom_loading(HPM_ESC);
if (stat == status_success) {
    // EEPROM 加载成功
} else if (stat == status_esc_eeprom_checksum_error) {
    // EEPROM 空白（无内容），通信仍可能，但 PDI 不可操作
}

// ============ EEPROM 模拟函数（MCU 侧实现）============
esc_get_eeprom_cmd(HPM_ESC);          // 获取 ESC 当前请求的命令（读/写/重载）
esc_get_eeprom_word_address(HPM_ESC); // 获取当前操作的 EEPROM 地址
esc_read_eeprom_data(HPM_ESC);        // 读取 ESC 的 EEPROM_DATA 寄存器（8 字节）
esc_write_eeprom_data(HPM_ESC, data); // 写入 ESC 的 EEPROM_DATA 寄存器
esc_eeprom_emulation_ack(HPM_ESC, cmd, ack_err, crc_err);  // 向 ESC 应答 EEPROM 操作完成

// ============ PHY 管理 ============
esc_mdio_read(HPM_ESC, phy_addr, reg_addr, &data);  // 读 PHY 寄存器
esc_mdio_write(HPM_ESC, phy_addr, reg_addr, data);  // 写 PHY 寄存器
esc_set_phy_offset(HPM_ESC, 1);                     // 设置 PHY 地址偏移
esc_enable_pdi_access_mii_management(HPM_ESC);      // 允许 MCU 访问 MDIO

// ============ 中断控制 ============
esc_enable_irq(HPM_ESC, esc_sync0_irq_mask);        // 使能 SYNC0 中断
esc_disable_irq(HPM_ESC, esc_sync1_irq_mask);       // 禁能 SYNC1 中断

// ============ 信号引脚功能配置 ============
esc_config_ctrl_signal_function(HPM_ESC, index, func, invert);
// 把 ESC_CTRLx 引脚配置为特定功能：
//   esc_ctrl_signal_func_alt_nmii_link0 → LINK0 状态指示
//   esc_ctrl_signal_func_alt_link_act0  → ACT0 活动指示
//   esc_ctrl_signal_func_alt_led_run    → RUN LED
//   esc_ctrl_signal_func_alt_led_err    → ERROR LED

// ============ 复位控制 ============
esc_config_reset_source(HPM_ESC, true);   // 复位信号来自 ESC 核心
esc_pdi_reset(HPM_ESC);                   // 产生 PDI 复位（写入 'R''E''S' 序列）
```

### 3.3 中断系统

HPM6E80 的 ESC 有 4 个中断向量：

| 中断 | IRQ 号 | 触发条件 | 用途 |
|---|---|---|---|
| **PDI 中断** | `IRQn_ESC` | ESC 有事件（SM 事件、AL 事件等） | 触发过程数据交换 |
| **SYNC0 中断** | `IRQn_ESC_SYNC0` | DC 分布式时钟 SYNC0 事件 | DC-Sync 模式下触发数据处理 |
| **SYNC1 中断** | `IRQn_ESC_SYNC1` | DC 分布式时钟 SYNC1 事件 | DC-Sync 模式下触发数据处理 |
| **RESET 中断** | `IRQn_ESC_RESET` | 主站请求复位从站 | 复位从站或整个 MCU |

中断注册（位于 `hpm_ecat_hw.c`）：

```c
// PDI 中断：ESC 有数据到达
void ecat_pdi_isr(void)
{
    PDI_Isr();              // 调用 SSC 处理函数
}
SDK_DECLARE_EXT_ISR_M(IRQn_ESC, ecat_pdi_isr);  // SDK 宏，注册 ISR

// SYNC0 中断：DC 同步脉冲
void ecat_sync0_isr(void)
{
    UINT8 SyncState = 0;
    HW_EscReadByte(SyncState, 0x098E);  // ACK SYNC0
    Sync0_Isr();                         // 调用 SSC 处理函数
}
SDK_DECLARE_EXT_ISR_M(IRQn_ESC_SYNC0, ecat_sync0_isr);

// ESC 复位中断
void ecat_reset_isr(void)
{
    // 方案 A: 仅复位 ESC 外设
    // 方案 B: 复位整个 MCU（调用 ppor_sw_reset）
}
```

中断使能/禁能：

```c
void ENABLE_ESC_INT(void)
{
    // 1. 打开 ESC 内部中断源
    HPM_ESC->GPR_CFG1 |= ESC_GPR_CFG1_SYNC0_IRQ_EN_MASK
                       | ESC_GPR_CFG1_SYNC1_IRQ_EN_MASK
                       | ESC_GPR_CFG1_RSTO_IRQ_EN_MASK;
    // 2. 使能 NVIC 中断
    intc_m_enable_irq_with_priority(IRQn_ESC_SYNC0, 2);
    intc_m_enable_irq_with_priority(IRQn_ESC_SYNC1, 2);
    intc_m_enable_irq_with_priority(IRQn_ESC, 1);        // PDI 中断
    intc_m_enable_irq_with_priority(IRQn_ESC_RESET, 1);
}
```

### 3.4 定时器

SSC 协议栈需要 1ms 定时基准，用于 EtherCAT 看门狗定时器。实现使用 HPM 的通用定时器 GPTMR0：

```c
// ecat_config.h
#define ECAT_TIMER_GPTMR    HPM_GPTMR0
#define ECAT_TIMER_GPTMR_CH 0
#define ECAT_TIMER_GPTMR_CLK clock_gptmr0
#define ECAT_TIMER_INC_P_MS 1  // 每 1ms 递增

// hpm_ecat_hw.c - 配置 1ms 定时器
void ecat_timer_ms_enable(void)
{
    uint32_t gptmr_freq = clock_get_frequency(ECAT_TIMER_GPTMR_CLK);
    config.reload = gptmr_freq / 1000;  // 1ms
    gptmr_channel_config(ECAT_TIMER_GPTMR, ECAT_TIMER_GPTMR_CH, &config, false);
    gptmr_start_counter(ECAT_TIMER_GPTMR, ECAT_TIMER_GPTMR_CH);
    gptmr_enable_irq(ECAT_TIMER_GPTMR, GPTMR_CH_RLD_IRQ_MASK(ECAT_TIMER_GPTMR_CH));
    intc_m_enable_irq_with_priority(ECAT_TIMER_GPTRM_IRQ, 1);
}

// ISR：每 1ms 递增 ecat_time_ms
void ecat_time_ms_isr(void)
{
    if (gptmr_check_status(HPM_GPTMR0, ...)) {
        gptmr_clear_status(HPM_GPTMR0, ...);
        ecat_time_ms += ECAT_TIMER_INC_P_MS;  // +1ms
    }
}
SDK_DECLARE_EXT_ISR_M(IRQn_GPTMR0, ecat_time_ms_isr);

// SSC 使用
UINT32 HW_GetTimer(void) { return ecat_time_ms; }
```

### 3.5 PHY 管理（MDIO）

每个 EtherCAT 端口外挂一个以太网 PHY 芯片，ESC 通过 MDIO 管理它们：

```c
// 读 PHY 寄存器（如读取 PHY ID、链路状态）
uint16_t phy_data;
esc_mdio_read(HPM_ESC, phy_addr, 0x01, &phy_data);  // 读状态寄存器

// 写 PHY 寄存器（如配置 LED 模式）
esc_mdio_write(HPM_ESC, phy_addr, 0x1F, 0x0500);    // 配置 LED 模式

// 使能 MCU 访问 MDIO（默认 ESC 独占 MDIO 总线）
esc_enable_pdi_access_mii_management(HPM_ESC);

// ... 操作 MDIO ...

// 关闭 MCU 访问 MDIO
esc_disable_pdi_access_mii_management(HPM_ESC);
```

---

## 4. 协议栈篇：Beckhoff SSC

### 4.1 SSC 是什么

**SSC**（Slave Stack Code）是 Beckhoff 提供的 C 语言 EtherCAT 从站协议栈。它实现了：

- EtherCAT 状态机（ESM）：Init → PreOP → SafeOP → OP
- CoE 协议：对象字典、SDO 上传/下载、PDO 映射
- 邮箱协议：Mailbox 的收发和分发
- 分布式时钟（DC）
- EEPROM 访问
- 看门狗

SSC 是**硬件无关**的——它通过一组 `HW_*` 宏（见第 3 章）来访问 ESC，你只需适配这些宏即可移植到任何 MCU。

**许可**：SSC 需要 Beckhoff 授权。开发完成后，如果你的产品量较大，需要购买 ET9300 许可证。免费版可用于评估和研发。

### 4.2 SSC 的文件结构

ecat_io 使用了 SSC v5.13，文件位于 `SSC/Src/`：

```
SSC/Src/
├── ecatappl.c/h        ← ★ 核心：MainInit(), MainLoop(), ECAT_Application()
├── ecatslv.c/h         ← EtherCAT 状态机（ESM）：状态转换逻辑
├── ecatcoe.c/h         ← CoE 协议：SDO 上传/下载、对象字典访问
├── coeappl.c/h         ← CoE 应用层：紧急消息、PDO 映射验证
├── mailbox.c/h         ← 邮箱分发器：识别 CoE/FoE/EoE/SoE/VoE
├── objdef.c/h          ← 对象字典引擎：查找、读写对象
├── sdoserv.c/h         ← SDO 服务：分段传输、完整访问
├── applInterface.h     ← 应用层接口：APPL_* 回调函数声明
├── ecat_def.h          ← ★ 配置核心：所有编译开关 + 厂商/产品 ID
├── digital_ioObjects.h ← ★ 设备对象字典：PDO 映射、SM 分配
├── digital_io.c/h      ← SSC 生成的模板（ecat_io 不用，改用 application/digital_io.c）
├── esc.h               ← ESC 寄存器定义
├── eeprom.h            ← EEPROM 结构体定义
└── Config/             ← SSC Tool 工程文件
    ├── HPM_ECAT_IO_Config.xml  ← SSC Tool 配置文件
    └── ESI/ECAT_IO.xml         ← ESI 设备描述文件
```

**重要**：`SSC/Src/digital_io.c` 是 SSC 工具生成的模板文件。当前 ecat_io 和 hpm_gateway 工程都没有编译它，而是编译 `application/` 目录下的自定义 `gateway_app.c`（或 `digital_io.c`）。所以改 `SSC/Src/digital_io.c` **不会影响固件行为**——要改的是 `application/` 下的文件。

### 4.3 核心流程：MainInit → MainLoop

从站固件的生命周期分为三个阶段：

```
上电 → MainInit() → 主循环 { MainLoop() }
```

**阶段一：MainInit()**（`ecatappl.c`）

```c
void MainInit(void)
{
    // ① 初始化变量
    bInitFinished = FALSE;
    bRunApplication = FALSE;

    // ② 读 ESC 硬件信息（类型、版本、DPRAM 大小）
    // ③ 初始化对象字典（加载 PDO 映射、设置默认值）
    // ④ 更新 EEPROM 中的 ESC 配置
    // ⑤ 重置 AL 状态寄存器
    // ⑥ 设置 ESC 中断使能（如果配置了 DC-Sync 模式）
    // ⑦ 初始化看门狗

    bInitFinished = TRUE;
}
```

**阶段二：MainLoop()**（`ecatappl.c`，每次主循环调用）

```
MainLoop() 单次调用执行流程：

┌─────────────────────────────────────────────┐
│ ① init guard: 如果 bInitFinished == FALSE → return │
├─────────────────────────────────────────────┤
│ ② 过程数据处理：                              │
│   FreeRun 模式（ecat_io 默认）：               │
│     → 读 AL Event 寄存器 (0x0220)            │
│     → 如果有 SM2 事件（输出数据到达）：          │
│         PDO_OutputMapping() → APPL_OutputMapping() │
│     → ECAT_Application() → APPL_Application()│
│     → PDO_InputMapping() → APPL_InputMapping()│
│   DC-Sync 模式：                              │
│     → 此步骤跳过（数据已在 ISR 中处理）         │
├─────────────────────────────────────────────┤
│ ③ 定时器维护: ECAT_CheckTimer()              │
│   → 看门狗喂狗                                │
│   → 更新 DC 系统时间                          │
│   → 处理超时                                  │
├─────────────────────────────────────────────┤
│ ④ DC 溢出检测: COE_SyncTimeStamp()           │
├─────────────────────────────────────────────┤
│ ⑤ 状态机: ECAT_Main()                       │
│   → 处理状态转换 (INIT→PREOP→SAFEOP→OP)       │
│   → 处理邮箱命令 (SDO 读写)                   │
│   → 检查 SM 配置                              │
├─────────────────────────────────────────────┤
│ ⑥ CoE 维护: COE_Main()                      │
│   → SDO 分段传输继续                          │
│   → 紧急消息处理                              │
├─────────────────────────────────────────────┤
│ ⑦ 错误检查: CheckIfEcatError()               │
├─────────────────────────────────────────────┤
│ ⑧ 应用钩子: pAPPL_MainLoop() （可选）         │
└─────────────────────────────────────────────┘
```

你的 `main.c` 主循环只需做：

```c
MainInit();         // 初始化 SSC
bRunApplication = TRUE;
while (bRunApplication == TRUE) {
    MainLoop();     // 驱动 SSC（处理状态机、PDO、邮箱）
    // 你的其他周期性任务 ...
}
```

### 4.4 EtherCAT 状态机（ESM）

从站有 5 个状态（Bootstrap 可选，ecat_io 不使用）：

```
                    ┌──────────┐
   上电/复位 ──────▶│  INIT    │  初始化
                    └────┬─────┘
                         │ 主站请求→PREOP
                    ┌────▼─────┐
                    │  PREOP   │  预运行（邮箱可用）
                    └────┬─────┘
                         │ 主站请求→SAFEOP
                    ┌────▼─────┐
                    │  SAFEOP  │  安全运行（输入可用，输出安全值）
                    └────┬─────┘
                         │ 主站请求→OP
                    ┌────▼─────┐
                    │   OP     │  正常运行（输入+输出全部可用）
                    └──────────┘

"安全"的含义：
  - SAFEOP 状态：从站可以采集输入并上报，但输出保持"安全值"（如电机停止）
  - OP 状态：输出也正常工作
  - 这是一条安全设计：主站必须先看完输入确认安全，才能让从站输出
```

**每个状态下能做什么？**

| | INIT | PREOP | SAFEOP | OP |
|---|---|---|---|---|
| Mailbox 通信 (SDO) | ❌ | ✅ | ✅ | ✅ |
| 过程数据输入 (TxPDO) | ❌ | ❌ | ✅ | ✅ |
| 过程数据输出 (RxPDO) | ❌ | ❌ | ❌ | ✅ |
| 分布式时钟 (DC) | ❌ | 可配置 | 可配置 | ✅ |

**SSC 中的状态转换回调**（你的 `application/digital_io.c` 要实现这些）：

```c
// PREOP: 邮箱可用了
UINT16 APPL_StartMailboxHandler(void)   { return ALSTATUSCODE_NOERROR; }
UINT16 APPL_StopMailboxHandler(void)    { return ALSTATUSCODE_NOERROR; }

// SAFEOP: 输入可以采集了
UINT16 APPL_StartInputHandler(UINT16 *pIntMask) { return ALSTATUSCODE_NOERROR; }
UINT16 APPL_StopInputHandler(void)      { return ALSTATUSCODE_NOERROR; }

// OP: 输出可以驱动了
UINT16 APPL_StartOutputHandler(void)    { return ALSTATUSCODE_NOERROR; }
UINT16 APPL_StopOutputHandler(void)     { return ALSTATUSCODE_NOERROR; }

// 错误确认
void APPL_AckErrorInd(UINT16 stateTrans) {}
```

返回 `ALSTATUSCODE_NOERROR` 表示同意状态转换。如果返回错误码，主站的转换请求会被拒绝。

### 4.5 PDO 映射：从对象字典到物理 IO

这是 EtherCAT 最核心的概念。以 ecat_io 为例，整个 PDO 映射链为：

```
输出方向（主站 → 从站，RxPDO）：

  主站发出的以太网帧
      │
      ▼ ① SM2 分配 (0x1C12 = sRxPDOassign)
      │   配置：{1, {0x1600}}
      │   含义：SM2 包含 1 个 PDO，索引号 0x1600
      ▼
      ▼ ② RxPDO 映射表 (0x1600 = OutputCounterProcessDataMapping)
      │   配置：{1, 0x70100020}
      │   含义：这个 PDO 包含 1 个条目
      │         条目解码：对象=0x7010, 子索引=0, 位宽=0x20(32bits)
      ▼
      ▼ ③ 物理数据对象 (0x7010 = OutputCounter0x7010)
      │   类型：UINT32（4 字节）
      │   访问：ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING
      ▼
      ▼ ④ APPL_OutputMapping() — 拷贝到本地变量
      │   MEMCPY(&OutputCounter0x7010, pData, 4);
      ▼
      ▼ ⑤ APPL_Application() 中的 APPL_SetLed()
      拆解低 8 bit → 控制 8 个 GPIO 输出引脚

输入方向（从站 → 主站，TxPDO）：

  8 个 GPIO 输入引脚
      │ ⑤ APPL_GetDipSw() 读取 → 拼成 UINT32
      ▼
      ▼ ④ APPL_InputMapping() — 拷贝到 DPRAM
      │   MEMCPY(pData, &InputCounter0x6000, 4);
      ▼
      ▼ ③ 物理数据对象 (0x6000 = InputCounter0x6000)
      │   类型：UINT32（4 字节）
      │   访问：ACCESS_READ | OBJACCESS_TXPDOMAPPING
      ▼
      ▼ ② TxPDO 映射表 (0x1A00 = InputCounterProcessDataMapping)
      │   配置：{1, 0x60000020}
      │   含义：对象=0x6000, 子索引=0, 位宽=32bits
      ▼
      ▼ ① SM3 分配 (0x1C13 = sTxPDOassign)
      │   配置：{1, {0x1A00}}
      │   含义：SM3 包含 1 个 PDO，索引号 0x1A00
      ▼
  主站收到的以太网帧（包含输入数据）
```

**PDO 映射条目编码格式**（32-bit）：

```
0x70100020  ← 这是一个 32-bit 值，编码了完整的信息
  ││    ││
  ││    └─── [7:0]   位宽（bits）：0x20 = 32 bits
  │└──────── [15:8]  子索引：0x00 = 子索引 0
  └───────── [31:16] 对象索引号：0x7010

解析结果：对象 0x7010, 子索引 0, 位宽 32 bits

更多例子：
  0x70100078 → 对象 0x7010, 子索引 0, 120 bits (15 bytes)
  0x70110020 → 对象 0x7011, 子索引 0, 32 bits
  0x60010010 → 对象 0x6001, 子索引 0, 16 bits
  0x60020008 → 对象 0x6002, 子索引 0, 8 bits
```

**为什么要用 120 bits 而不是 128？** SSC v5.13 的 PDO mapping 限制每个条目最大 255 bits。网关项目用了 4 个 `OCTET_STRING[15]`（各 120 bits），就是因为 4×120=480 小于 4×255=1020，且每个条目不超过 255 bit 限制。

**对象字典关键对象** 完整汇总：

| 对象 | 名称 | ecat_io 取值 | 含义 |
|---|---|---|---|
| **0x1000** | 设备类型 | `0x00001389` | 5001 = 模块化设备 |
| **0x1008** | 设备名称 | `"digital_io"` | 在 TwinCAT 中显示的名称 |
| **0x1009** | 硬件版本 | `"ECAT_Device"` | |
| **0x100A** | 软件版本 | `"1.0"` | |
| **0x1018** | 标识对象 | Vendor=0x0048504D, Product=1, Rev=1 | "HPM" 的 ASCII |
| **0x1600** | RxPDO 映射 | `{1, 0x70100020}` | 输出：对象=0x7010, 位宽=32 |
| **0x1A00** | TxPDO 映射 | `{1, 0x60000020}` | 输入：对象=0x6000, 位宽=32 |
| **0x1C12** | SM2 分配 | `{1, {0x1600}}` | SM2 使用 RxPDO 0x1600 |
| **0x1C13** | SM3 分配 | `{1, {0x1A00}}` | SM3 使用 TxPDO 0x1A00 |
| **0x1C32** | SM2 参数 | 同步模式、周期时间 | 输出 SM 同步参数 |
| **0x1C33** | SM3 参数 | 同步模式、周期时间 | 输入 SM 同步参数 |
| **0x6000** | 输入数据 | `UINT32` | 8 路 GPIO 输入，0x6000~0x6FFF 为输入区 |
| **0x7010** | 输出数据 | `UINT32` | 8 路 GPIO 输出，0x7000~0x7FFF 为输出区 |
| **0xF000** | 模块化设备行规 | `{2, 0x0010, 0}` | 索引间距=0x10 |

### 4.6 CoE（CANopen over EtherCAT）

CoE 把 CANopen 的对象字典"搬"到 EtherCAT 上。所有配置参数、IO 数据都通过**对象字典**（Object Dictionary）组织。

**对象字典结构**：每个对象有 4 个属性

```c
// SSC 中的对象描述
TOBJECT OBJMEM ApplicationObjDic[] = {
    {NULL, NULL, 0x6000, {DEFTYPE_UNSIGNED32, 0 | (OBJCODE_VAR << 8)},
     &entryDesc, aName, &InputCounter0x6000, NULL, NULL, 0x0000},
    //    │        │         │                     │
    //    │        │         │                     └── 实际数据变量的指针
    //    │        │         └── 数据类型 (UINT32) + 对象代码 (变量)
    //    │        └── 索引号 (0x6000)
    //    └── 链表指针
};
```

每个条目的字段含义：

| 字段 | 说明 | 示例 |
|---|---|---|
| 前两个 NULL | 链表 prev/next 指针（SSC 自动管理） | NULL, NULL |
| 索引号 | 16-bit 对象索引 | `0x6000` |
| 类型描述 | `{数据类型, 位宽 \| (对象代码 << 8)}` | `{DEFTYPE_UNSIGNED32, 0 \| (OBJCODE_VAR << 8)}` |
| 条目描述 | 子索引描述符数组 | `&entryDesc` |
| 名称 | 对象名称字符串 | `aName` |
| 数据指针 | 指向实际变量 | `&InputCounter0x6000` |
| 回调 | 读写前后回调（通常 NULL） | NULL, NULL |
| 访问标志 | 额外访问控制（通常 0） | `0x0000` |

**SDO 通信**：主站通过 SDO 读写对象字典中的任意对象。

```
主站发起 SDO 下载（写 0x7010 = 0x000000FF）：

  Mailbox 帧 (SM0)
    ├── 命令：SDO Download Request
    ├── 索引：0x7010
    ├── 子索引：0x00
    └── 数据：0x000000FF

  从站响应：
  Mailbox 帧 (SM1)
    └── 命令：SDO Download Response（确认写入）

PDO 也可以通过 SDO 重新映射——主站先通过 SDO 写 0x1C12 更改 SM 分配的 PDO，
然后重新进入 SAFEOP→OP 状态，使用新的 PDO 映射通信。
```

**SDO 写入后的"自动通知"机制**：

主站通过 SDO 写入某个对象后，SSC 可以自动通知你的应用程序。函数指针在 `applInterface.h` 中声明：

```c
extern void (*pAPPL_CoeWriteInd)(UINT16 index, UINT8 subindex, BOOL complete_access);
```

用法：在 `MainInit()` 之后注册你的回调函数，每当主站通过 SDO 写入任何对象字典条目时，SSC 都会调用这个回调，告诉你"index 0x???? 被写入了"。你可以在回调里根据 index 做对应的处理（如重新配置 CAN、切换控制模式等）。

⚠️ **注意**：`MainInit()` 会把 `pAPPL_CoeWriteInd` 清空（设为 NULL），所以你必须在 `MainInit()` **之后**再赋值。

### 4.7 分布式时钟（DC）

DC 让所有从站同步到同一个时钟。HPM6E80 支持 DC，ecat_io 通过 `ecat_def.h` 中的 `DC_SUPPORTED=1` 开启。

**DC 工作原理**：

```
  [主站] 定时发送 ARMW 帧，帧中带有主站时钟值
    │
    ▼
  所有从站收到帧时，ESC 硬件锁存本地时间，计算出与主站的时钟偏差
    │
    ▼
  从站调整本地时钟：补偿偏差 + 补偿漂移
    │
    ▼
  所有从站时钟与主站对齐（纳秒级精度）
    │
    ▼
  SYNC0 事件：按配置的周期（如 1ms）产生硬件脉冲
  → 触发 SYNC0 中断 → 从站 MCU 处理输入输出数据
  → 所有从站在同一微秒内采样和输出
```

**DC 配置**（在 SSC Tool 中设置，或直接在 `digital_ioObjects.h` 中定义）：

- **Sync0 周期**：通常等于 EtherCAT 通信周期（如 1ms）
- **Sync0 偏移**：在周期内的哪个时刻触发（如周期的 30% 处，给数据搬运留时间）

### 4.8 三种同步模式

SSC 支持三种模式，由标志位控制：

| 模式 | `bEscIntEnabled` | `bDcSyncActive` | 数据交换触发 | 时序精度 |
|---|---|---|---|---|
| **FreeRun** | FALSE | FALSE | MainLoop 轮询 AL Event 寄存器 | 差（取决于主循环速度） |
| **SM-Sync** | TRUE | FALSE | SM2 硬件中断 | 中（帧到达即触发） |
| **DC-Sync** | TRUE | TRUE | DC SYNC0 硬件中断 | 极高（纳秒级，全网络同步） |

**ecat_io 使用 FreeRun 模式**，最简单，不需要配置 DC。

模式选择由主站在配置阶段通过 SDO 完成：
- 主站写 0x1C32:01 = 0x0001 (SM2 同步模式 = SM-Sync) 或 0x0002 (DC-Sync)
- 或保持默认值 0x0000 (FreeRun)

---

## 5. 实战篇：从零搭建 EtherCAT 从站

### 5.1 环境搭建

**前提条件**：你已经把 `sdk_env` 克隆到本地（如 `D:\sdk_env`）。

**第一步：打开 SDK 命令提示符**

```cmd
cd D:\sdk_env
start_cmd.cmd
```

这会打开一个 cmd 窗口，环境变量已配置好：
- `HPM_SDK_BASE` → SDK 根目录
- `GNURISCV_TOOLCHAIN_PATH` → RISC-V 工具链
- PATH 包含 cmake、ninja、python、openocd

**第二步：验证环境**

```cmd
echo %HPM_SDK_BASE%
:: 应输出: D:\sdk_env\hpm_sdk

riscv32-unknown-elf-gcc --version
:: 应输出: riscv32-unknown-elf-gcc (Linaro GCC) 13.2.0

cmake --version
:: 应输出: cmake version 3.24.x
```

### 5.2 理解 ecat_io 示例

ecat_io 是一个**数字 IO 从站**：
- 8 路输入（拨码开关）→ 映射到 `InputCounter0x6000`
- 8 路输出（LED）→ 映射到 `OutputCounter0x7010`
- 主站每个周期写 8 个输出、读 8 个输入
- 4 个物理按键：KEYA/B/C/D（50ms 消抖）

**文件清单**（只需要关心加粗的）：

| 文件 | 需要修改吗？ | 作用 |
|---|---|---|
| **`main.c`** | ✅ 你要写 | 应用主入口，初始化 + 主循环 |
| **`ecat_config.h`** | ✅ 可能要改 | ESC 硬件配置（定时器、Flash 地址） |
| **`SSC/Src/digital_ioObjects.h`** | ✅ 要改 | PDO 映射、对象字典 |
| **`SSC/Src/ecat_def.h`** | ✅ 可能要改 | 厂商 ID、产品 ID、协议特性开关 |
| **`application/digital_io.c`** | ✅ 要改 | 你的应用逻辑（APPL_* 回调） |
| `SSC/Src/ecatappl.c` | ❌ 不需要 | SSC 核心，不改 |
| `SSC/Src/ecatslv.c` | ❌ 不需要 | SSC 状态机，不改 |
| `drivers/.../hpm_ecat_hw.c` | ❌ 不需要 | ESC 硬件抽象，不改 |
| `drivers/.../hpm_ecat_e2p_emulation.c` | ❌ 不需要 | Flash EEPROM 模拟，不改 |

### 5.3 编译固件

```cmd
:: 进入 ecat_io 项目目录
cd D:\sdk_env\hpm_app\software\apps\ecat_io

:: 方式一：使用预配置构建目录（直接编译）
ninja -C hpm6e00_full_port_flash_xip_debug

:: 方式二：从头构建
mkdir build && cd build
cmake .. -G Ninja ^
  -DBOARD=hpm6e00_full_port ^
  -DBOARD_SEARCH_PATH=D:\sdk_env\hpm_app\boards ^
  -DHPM_BUILD_TYPE=flash_xip ^
  -DCMAKE_BUILD_TYPE=debug
ninja

:: 编译产物位于 build/output/
::   demo.elf   ← ELF 文件（调试用）
::   demo.bin   ← 二进制文件（烧录用）
::   demo.map   ← 内存映射（分析用）
::   demo.asm   ← 反汇编（分析用）
```

**CMake 变量说明**：

| 变量 | 含义 | 可选值 |
|---|---|---|
| `BOARD` | 目标板名称 | `hpm6e00evk`, `hpm6e00_full_port` |
| `HPM_BUILD_TYPE` | 运行方式 | `flash_xip`(默认), `ram`, `flash_uf2` |
| `CMAKE_BUILD_TYPE` | 优化级别 | `debug`(含调试信息), `release` |
| `BOARD_SEARCH_PATH` | 额外的板级目录 | 自定义板路径 |

### 5.4 烧录与调试

**硬件准备**：
- HPM6E00 全端口评估板
- HS-Link 调试器（FT2232 芯片，CMSIS-DAP 协议）
- USB 线 ×2：一个给 HS-Link，一个给板子供电
- 网线连接 PC 和板子的 EtherCAT P0 端口

**烧录**：

VS Code 中按 `Ctrl+Shift+B` 运行 Build Task，或：

```cmd
:: 使用 SDK 提供的烧录脚本
cd D:\sdk_env
call tools\scripts\program_flash.cmd ^
  D:\sdk_env\hpm_app\software\apps\ecat_io\hpm6e00_full_port_flash_xip_debug\output\demo.bin
```

**调试**：VS Code 按 `F5`（需要预先配置 `.vscode/launch.json`，ecat_io 已包含此配置）。

### 5.5 用 Linux 主机测试

ecat_io 自带了一个简易的 Linux 测试程序：

```bash
# 进入测试目录
cd hpm_app/software/apps/ecat_io/Linux_Test

# 编译（需要安装 SOEM v2）
make

# 运行（需要 root 权限，因为要收发原始以太网帧）
sudo ./ecat_master_test eth0
```

这个测试程序使用 **SOEM**（Simple Open EtherCAT Master），会自动：
1. 扫描网络找到从站
2. 配置从站进入 OP 状态
3. 周期性发送/接收 PDO 数据
4. 打印输入和输出值

---

## 6. 进阶篇：修改 PDO 和 SDO

本章面向需要修改 PDO/SDO 的开发场景。例如：当前网关项目使用 60 字节透明 buffer（4 个 OCTET_STRING[15]），如果要改成结构化的电机控制对象，或者需要添加 SDO 配置参数，就按照本章的步骤操作。

> **改代码的 5 个层次**：修改 PDO 不只是改一个文件——你需要同时修改 **PDO mapping 定义、数据对象定义、应用层回调、最大长度限制、ESI 文件**。5 个层次任何一个不一致，从站都进不了 OP。

### 6.1 理解 PDO mapping 条目编码格式

在深入修改之前，先彻底理解 PDO mapping 条目的数值格式。当前代码中有这样的 mapping：

```c
0x70100078
```

把它拆开看：

```
0x70100078
  ││    ││
  ││    └─── [7:0]   位宽（bit length）：0x78 = 120 bits = 15 bytes
  │└──────── [15:8]  子索引（subindex）：0x00 = 子索引 0
  └───────── [31:16] 对象索引号（index）：0x7010

解析结果：对象 0x7010，子索引 0，长度 120 bits

常见 encoding 速查表：

  0x70100010 → 对象 0x7010, 子索引 0, 16 bits  (UINT16)
  0x70100020 → 对象 0x7010, 子索引 0, 32 bits  (UINT32)
  0x70110008 → 对象 0x7011, 子索引 0, 8 bits   (UINT8)
  0x60000078 → 对象 0x6000, 子索引 0, 120 bits (OCTET_STRING[15])
```

**位宽和 C 类型的对应关系**：

| 位宽 (hex) | 位宽 (decimal) | 字节数 | C 类型 | SSC 类型宏 |
|---|---|---|---|---|
| 0x08 | 8 | 1 | `UINT8` / `INT8` | `DEFTYPE_UNSIGNED8` / `DEFTYPE_INTEGER8` |
| 0x10 | 16 | 2 | `UINT16` / `INT16` | `DEFTYPE_UNSIGNED16` / `DEFTYPE_INTEGER16` |
| 0x20 | 32 | 4 | `UINT32` / `INT32` | `DEFTYPE_UNSIGNED32` / `DEFTYPE_INTEGER32` |
| 0x40 | 64 | 8 | `UINT64` / `INT64` | `DEFTYPE_UNSIGNED64` |
| 0x78 | 120 | 15 | `OCTET_STRING[15]` | `DEFTYPE_OCTETSTRING(15)` |

### 6.2 分步修改 RxPDO（主站→从站）

RxPDO 相关对象在 `digital_ioObjects.h` 中：

```
0x1C12 → SyncManager 2 assignment (SM2 包含哪些 PDO)
0x1600 → RxPDO mapping (这个 PDO 包含哪些数据字段)
0x7010~0x7013 → 实际数据对象 (GatewayOutputData)
```

**场景**：当前网关项目用 4 个 `OCTET_STRING[15]`（共 60 字节透明 buffer）。现在要改成结构化电机控制命令。

#### 第一步：设计 PDO 布局

| 对象 | 类型 | 长度 | 含义 |
|---|---|---|---|
| `0x7010:0` | `UINT16` | 16 bit | 控制字 (control word) |
| `0x7011:0` | `UINT8` | 8 bit | 电机 ID |
| `0x7012:0` | `INT32` | 32 bit | 目标位置 |
| `0x7013:0` | `INT32` | 32 bit | 目标速度 |
| `0x7014:0` | `INT16` | 16 bit | 目标力矩 |
| `0x7015:0` | `UINT16` | 16 bit | 命令序列号 |

#### 第二步：修改 RxPDO mapping 结构体 (0x1600)

在 `digital_ioObjects.h` 中：

```c
// 修改前（60 字节透明 buffer）：
typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 SI1;   // 0x70100078
    UINT32 SI2;   // 0x70110078
    UINT32 SI3;   // 0x70120078
    UINT32 SI4;   // 0x70130078
} OBJ_STRUCT_PACKED_END TOBJ1600;

PROTO TOBJ1600 OutputCounterProcessDataMapping0x1600 = {
    4,
    0x70100078,
    0x70110078,
    0x70120078,
    0x70130078
};

// 修改后（结构化电机控制）：
typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 SI1;   // 0x70100010 (UINT16)
    UINT32 SI2;   // 0x70110008 (UINT8)
    UINT32 SI3;   // 0x70120020 (INT32)
    UINT32 SI4;   // 0x70130020 (INT32)
    UINT32 SI5;   // 0x70140010 (INT16)
    UINT32 SI6;   // 0x70150010 (UINT16)
} OBJ_STRUCT_PACKED_END TOBJ1600;

PROTO TOBJ1600 OutputCounterProcessDataMapping0x1600 = {
    6,
    0x70100010,
    0x70110008,
    0x70120020,
    0x70130020,
    0x70140010,
    0x70150010
};
```

⚠️ `u16SubIndex0` 必须等于后面 SI 条目的数量（这里是 6）。

#### 第三步：定义新的数据对象

为每个 mapping 条目创建对应的对象变量。在 `digital_ioObjects.h` 中：

```c
// ===== 在 #ifdef _OBJD_ 块中定义对象描述 =====
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x7010 =
    { DEFTYPE_UNSIGNED16, 0x10, ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
OBJCONST UCHAR OBJMEM aName0x7010[] = "ControlWord\000\377";

OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x7011 =
    { DEFTYPE_UNSIGNED8, 0x08, ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
OBJCONST UCHAR OBJMEM aName0x7011[] = "TargetMotorId\000\377";
// ... 类似定义 0x7012~0x7015 ...
#endif

// ===== 在文件末尾定义实际变量 =====
PROTO UINT16 ControlWord0x7010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;

PROTO UINT8 TargetMotorId0x7011
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
// ... 类似定义 0x7012~0x7015 ...
```

#### 第四步：加入对象字典

每个对象必须注册到 `ApplicationObjDic[]` 数组中：

```c
{NULL, NULL, 0x7010,
 {DEFTYPE_UNSIGNED16, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x7010,
 aName0x7010,
 &ControlWord0x7010,
 NULL, NULL, 0x0000},

{NULL, NULL, 0x7011,
 {DEFTYPE_UNSIGNED8, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x7011,
 aName0x7011,
 &TargetMotorId0x7011,
 NULL, NULL, 0x0000},
// ... 类似添加 0x7012~0x7015 ...
```

**对象描述字段详解**：

| 字段 | 示例值 | 含义 |
|---|---|---|
| 索引 | `0x7010` | 对象在字典中的唯一编号 |
| 类型+代码 | `{DEFTYPE_UNSIGNED16, 0 \| (OBJCODE_VAR << 8)}` | 16-bit 无符号变量 |
| 对象代码 | `OBJCODE_VAR` | 普通变量（不是数组、不是记录） |
| 访问标志 | `ACCESS_READWRITE \| OBJACCESS_RXPDOMAPPING` | 可读写 + 可被 RxPDO 映射 |

### 6.3 分步修改 TxPDO（从站→主站）

TxPDO 相关对象：

```
0x1C13 → SyncManager 3 assignment (SM3 包含哪些 PDO)
0x1A00 → TxPDO mapping (这个 PDO 包含哪些数据字段)
0x6000~0x6003 → 实际数据对象 (GatewayInputData)
```

操作步骤与 RxPDO 对称。推荐把电机反馈设计成结构化对象：

| 对象 | 类型 | 长度 | 含义 |
|---|---|---|---|
| `0x6000:0` | `UINT16` | 16 bit | 状态字 |
| `0x6001:0` | `UINT8` | 8 bit | 电机 ID |
| `0x6002:0` | `INT32` | 32 bit | 实际位置 |
| `0x6003:0` | `INT32` | 32 bit | 实际速度 |
| `0x6004:0` | `INT16` | 16 bit | 实际力矩 |
| `0x6005:0` | `UINT16` | 16 bit | 错误码 |
| `0x6006:0` | `UINT16` | 16 bit | 反馈序列号 |

对应 `0x1A00` mapping：

```c
typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 SI1;   // 0x60000010
    UINT32 SI2;   // 0x60010008
    UINT32 SI3;   // 0x60020020
    UINT32 SI4;   // 0x60030020
    UINT32 SI5;   // 0x60040010
    UINT32 SI6;   // 0x60050010
    UINT32 SI7;   // 0x60060010
} OBJ_STRUCT_PACKED_END TOBJ1A00;

PROTO TOBJ1A00 InputCounterProcessDataMapping0x1A00 = {
    7,
    0x60000010,
    0x60010008,
    0x60020020,
    0x60030020,
    0x60040010,
    0x60050010,
    0x60060010
};
```

**计算 PDO 总字节数**：把所有 mapping 条目的位宽加起来除以 8。

RxPDO 例子：16+8+32+32+16+16 = 120 bits = 15 bytes
TxPDO 例子：16+8+32+32+16+16+16 = 136 bits = 17 bytes

### 6.4 修改 APPL_InputMapping 和 APPL_OutputMapping

**当前透明模式**（网关项目，60 字节整体复制）：

```c
void APPL_InputMapping(UINT16 *pData)
{
    memcpy((uint8_t *)pData, g_pdo_input_buf, 60);
}

void APPL_OutputMapping(UINT16 *pData)
{
    memcpy(g_pdo_output_buf, (uint8_t *)pData, 60);
}
```

**改为结构化 PDO 后**，建议先定义 packed 结构体，然后通过结构体指针访问：

```c
// 在 gateway_app.c 或你的应用文件中定义
typedef struct __attribute__((packed)) {
    UINT16 control_word;
    UINT8  motor_id;
    INT32  target_position;
    INT32  target_velocity;
    INT16  target_torque;
    UINT16 command_seq;
} gateway_rxpdo_t;   // 15 字节，与 PDO mapping 一致

typedef struct __attribute__((packed)) {
    UINT16 status_word;
    UINT8  motor_id;
    INT32  actual_position;
    INT32  actual_velocity;
    INT16  actual_torque;
    UINT16 error_code;
    UINT16 feedback_seq;
} gateway_txpdo_t;   // 17 字节，与 PDO mapping 一致

// RxPDO: 主站发来的数据 → 解析给电机控制
void APPL_OutputMapping(UINT16 *pData)
{
    const gateway_rxpdo_t *rx = (const gateway_rxpdo_t *)pData;

    // 把 rx->target_position、rx->target_velocity 等同步给电机控制模块
    motor_set_target(rx->motor_id, rx->target_position, rx->target_velocity);
}

// TxPDO: 电机反馈 → 写入 DPRAM 供主站读取
void APPL_InputMapping(UINT16 *pData)
{
    gateway_txpdo_t *tx = (gateway_txpdo_t *)pData;

    memset(tx, 0, sizeof(*tx));
    tx->status_word = 0x0001;          // 运行中
    tx->motor_id = 1;
    tx->actual_position = motor_get_position();
    tx->actual_velocity = motor_get_velocity();
    tx->actual_torque = motor_get_torque();
    tx->error_code = motor_get_error();
    tx->feedback_seq++;
}
```

**为什么要 `__attribute__((packed))`？**

```c
// 不用 packed：编译器可能对齐到 4 字节边界
typedef struct {
    UINT16 a;  // 占 2 字节，后面可能填充 2 字节
    UINT8  b;  // 占 1 字节，后面可能填充 3 字节
    INT32  c;  // 占 4 字节
} not_packed_t;  // 实际大小可能是 12 字节（对齐了）

// 用 packed：严格按字段顺序排列，无填充
typedef struct __attribute__((packed)) {
    UINT16 a;  // 占 2 字节
    UINT8  b;  // 占 1 字节
    INT32  c;  // 占 4 字节
} packed_t;      // 实际大小 = 7 字节（严格紧凑）
```

PDO 数据在以太网帧中是**按位紧密排列**的，不允许编译器插入填充字节。不是 packed，主站收到的数据就会错位。

**如果暂时保持透明网关协议**：建议定义统一宏避免多处硬编码不一致：

```c
// gateway_protocol.h
#define GATEWAY_PDO_TOTAL_SIZE 60
```

然后在 `gateway_app.c`、`gateway_bridge.c` 等所有引用 60 的地方改用这个宏。以后改长度时只改一处。

### 6.5 添加 SDO（只读、读写、写后触发）

SDO 是 CoE 对象字典访问。只要对象被加入 `ApplicationObjDic[]`，主站就能通过 SDO 读写它。

#### 6.5.1 添加只读 SDO

示例：添加固件版本号 `0x8000:0`（`UINT32`，只读）。

```c
// ===== 在 digital_ioObjects.h 的 #ifdef _OBJD_ 块中 =====
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x8000 =
    { DEFTYPE_UNSIGNED32, 0x20, ACCESS_READ };   // 只读，32-bit
OBJCONST UCHAR OBJMEM aName0x8000[] = "FirmwareVersion\000\377";
#endif

// ===== 变量定义 =====
PROTO UINT32 FirmwareVersion0x8000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00010000   // 默认值：v1.0.0
#endif
;

// ===== 加入 ApplicationObjDic[] =====
{NULL, NULL, 0x8000,
 {DEFTYPE_UNSIGNED32, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x8000,
 aName0x8000,
 &FirmwareVersion0x8000,
 NULL, NULL, 0x0000},
```

#### 6.5.2 添加可读写 SDO

示例：CAN 波特率 `0x8001:0`（`UINT32`，可读写）。

```c
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x8001 =
    { DEFTYPE_UNSIGNED32, 0x20, ACCESS_READWRITE };  // 可读写
OBJCONST UCHAR OBJMEM aName0x8001[] = "CanBitrate\000\377";
#endif

PROTO UINT32 CanBitrate0x8001
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 1000000    // 默认 1Mbps
#endif
;
```

主站即可通过 SDO 读写：

```
主站: read  0x8001:0  →  从站返回 1000000
主站: write 0x8001:0 = 500000  →  从站确认写入
主站: read  0x8001:0  →  从站返回 500000
```

#### 6.5.3 SDO 写入后自动触发动作

如果主站写入某个 SDO 后你要立即执行动作（如重新配置 CAN），可以用 SSC 提供的 `pAPPL_CoeWriteInd` 回调：

```c
// 定义回调函数
static void gateway_coe_write_ind(UINT16 index, UINT8 subindex, BOOL complete_access)
{
    (void)subindex;
    (void)complete_access;

    switch (index) {
    case 0x8001:   // CAN 波特率被写入
        can_reconfigure(CanBitrate0x8001);  // 立即应用新波特率
        break;
    case 0x8010:   // 控制模式被写入
        motor_switch_mode(ControlMode0x8010);
        break;
    }
}

// 在 main.c 的 MainInit() 之后注册
MainInit();
pAPPL_CoeWriteInd = gateway_coe_write_ind;  // ← 必须在 MainInit 之后！
```

⚠️ **重要**：`MainInit()` 会把 `pAPPL_CoeWriteInd` 清为 NULL，所以必须在 `MainInit()` **之后**赋值。

同样有 `pAPPL_CoeReadInd`——在 SDO 读取前触发，可用于刷新动态数据（如读取电机当前温度前先采集一次）。

#### 6.5.4 哪些参数适合放 SDO？

| 适合放 SDO | 不适合放 SDO（应放 PDO） |
|---|---|
| CAN bitrate | 高频目标位置 |
| 电机 ID | 高频目标速度 |
| 控制模式 | 高频反馈位置 |
| 电机参数（极对数等） | 高频反馈电流 |
| 超时时间 | 周期性控制字 |
| 软件版本号 | 周期性状态字 |
| 错误统计计数 | |
| 调试开关 | |

**原则**：低频的、配置类的放 SDO；高频的、周期性的放 PDO。

### 6.6 MAX_PD 长度配置

当前 `ecat_def.h`：

```c
#define MAX_PD_INPUT_SIZE  0x40   // 64 字节
#define MAX_PD_OUTPUT_SIZE 0x40   // 64 字节
```

当前 60 字节没有超。如果新的 PDO 总长度超过 64 字节，**必须改大**：

```c
#define MAX_PD_INPUT_SIZE  0x80   // 128 字节
#define MAX_PD_OUTPUT_SIZE 0x80   // 128 字节
```

如果超了没改，SSC 在检查 SM 配置时会失败（SM 配置的长度 > MAX_PD 限制），从站可能停在 PREOP 或 SAFEOP。

**如何判断是否需要改大？**

```
新 RxPDO 总位宽 / 8 = 新 output 字节数 → 如果 > 64，改 MAX_PD_OUTPUT_SIZE
新 TxPDO 总位宽 / 8 = 新 input 字节数  → 如果 > 64，改 MAX_PD_INPUT_SIZE
```

### 6.7 ESI XML 同步——最容易踩的坑

固件对象字典改完后，**必须**同步更新：

```
firmware/ecat_tsn_gateway/SSC/ESI/ECAT_IO.xml
```

ESI 是主站识别从站的唯一依据。固件和 ESI 不一致是最常见的故障原因。

**必须同步的内容**：

| 项目 | ESI 中对应位置 |
|---|---|
| RxPDO mapping (0x1600) | `<RxPdo>` 段 |
| TxPDO mapping (0x1A00) | `<TxPdo>` 段 |
| SM2 分配 (0x1C12) | `<Sm>` 段，`ControlByte` 含方向 |
| SM3 分配 (0x1C13) | `<Sm>` 段 |
| 每个数据对象的类型和位宽 | `<RxPdo>`/`<TxPdo>` 中的 `<Entry>` |
| ProductCode / RevisionNo | `<Info>` 段（如修改了设备功能应更新版本号） |

**ESI 不同步的典型故障现象**：

- 主站扫描得到从站，但进不了 OP
- PREOP → SAFEOP 失败（AL status code: invalid input/output mapping）
- SAFEOP → OP 失败
- `APPL_OutputMapping()` 始终不被调用
- PDO 数据错位（比如 control_word 读到了 target_position 的值）
- SM2 长度和 `nPdOutputSize` 不一致

**原因很简单**：主站看了 ESI（说明书），按说明书配置了 SM2 = 30 字节。但固件实际期望 60 字节。主站发 30 字节，固件想要 60——不匹配，状态机拒绝进入 OP。

**重新生成 ESI 的方法**：
1. 用 Beckhoff SSC Tool 打开 `SSC/Config/HPM_ECAT_IO_Config.xml`
2. 修改对象字典
3. 导出新的 `ESI/ECAT_IO.xml`
4. 重新导入主站软件（TwinCAT 或 Linux IGH）

### 6.8 修改后验证步骤

每次修改 PDO/SDO 后，按以下顺序验证：

#### 6.8.1 编译验证

重新 `ninja`，确认无错误。重点关注：
- 没有重复符号（duplicate symbol）
- 对象字典结构体大小正确
- 新增的对象变量都已定义
- `APPL_GenerateMapping` 能正常返回

#### 6.8.2 启动日志验证

串口应看到：

```
ESC hardware OK
PDO: xx bytes in, yy bytes out   ← xx、yy 与你期望的一致
Gateway running
```

如果 `xx` 或 `yy` 是 0 或不是你期望的值，说明 PDO mapping 配置有问题。

#### 6.8.3 主站状态验证

主站应能让从站依次进入：

```
INIT → PREOP → SAFEOP → OP
```

如果卡在某个状态：

| 卡在 | 排查重点 |
|---|---|
| PREOP → SAFEOP | SM2/SM3 配置、PDO mapping、ESI 长度 |
| SAFEOP → OP | output PDO 是否被主站周期性发送、watchdog |
| 保持在 INIT | EEPROM 加载、ESC 时钟、PHY 链路 |

#### 6.8.4 PDO 收发验证

Master 写 RxPDO 后，应能看到：

```
APPL_StartOutputHandler called
APPL_OutputMapping #1 called
```

如果 `APPL_OutputMapping()` 不触发：
- 从站没进 OP
- output PDO 长度不匹配
- SM2 配置不正确
- 主站没有周期性发送 output PDO

#### 6.8.5 SDO 验证

用主站读写新增 SDO：

```
read  0x8001:0     →  应返回默认值
write 0x8001:0 = <新值>  →  应成功
read  0x8001:0     →  应返回新值
```

如果读不到：
- 对象是否加入了 `ApplicationObjDic[]`
- index 是否冲突
- access 权限是否正确
- ESI 是否同步

如果写入后动作没有触发：
- `pAPPL_CoeWriteInd` 是否在 `MainInit()` **之后**赋值
- index 判断是否正确
- 对象是否为 `ACCESS_READWRITE`

### 6.9 常见问题排查

#### 问题 1：改了 `digital_io.c` 但没有效果

当前工程没有编译 `SSC/Src/digital_io.c`。应改的是：
- `application/gateway_app.c`（或 `application/digital_io.c`）
- `SSC/Src/digital_ioObjects.h`

#### 问题 2：代码里 PDO 是 60 字节，主站却显示别的长度

说明 ESI 或主站工程没有更新。重新导入 `ECAT_IO.xml`，清理主站缓存。

#### 问题 3：从站只能到 PREOP

Mailbox 正常但 PDO/SM 配置通不过。检查：
- `0x1C12` / `0x1C13` 是否指向正确的 PDO
- `0x1600` / `0x1A00` mapping 是否正确
- ESI 中的 PDO 长度和固件是否一致

#### 问题 4：从站到 SAFEOP 但进不了 OP

- output PDO 没有被主站周期性发送
- watchdog 超时
- `APPL_StartOutputHandler()` 返回错误
- SM2 长度和 `nPdOutputSize` 不一致

#### 问题 5：PDO 数据错位

- C 结构体没有 `packed`
- PDO mapping 顺序和结构体字段顺序不一致
- bit length 写错（16 bit 写成 8 bit）
- 主站侧变量类型和从站不一致

#### 问题 6：编译报错 "object exceeds MAX_PD size"

PDO 总长度超过了 `MAX_PD_INPUT_SIZE` 或 `MAX_PD_OUTPUT_SIZE`。改 `ecat_def.h` 中的宏。

### 6.10 EEPROM 模拟原理

**为什么需要 EEPROM 模拟？**

评估板上没有外挂 EEPROM 芯片。但 EtherCAT 规范要求从站上电后 ESC 必须能从 EEPROM 读取配置。Flash 模拟用 MCU 的 NOR Flash 代替外挂 EEPROM。

**工作流程**：

```
1. 上电 → ESC 配置为 EEPROM 模拟模式
         → ESC 不启动 I2C 读时序
         → 而是拉高 EEPROM_CTRL_STAT 的 BUSY 标志

2. MCU 检测到 BUSY=1
   → 读 ESC 的 EEPROM_CTRL_STAT 寄存器获取命令（读/写/重载）
   → 读 EEPROM_ADDR 寄存器获取地址

3. 如果是 读命令：
   → MCU 从 NOR Flash (偏移 0x200000) 读 2 字节
   → 写入 ESC 的 EEPROM_DATA 寄存器
   → 调用 esc_eeprom_emulation_ack() 通知 ESC "数据已就绪"

4. ESC 继续请求下一个地址...
   重复 2~3，直到整个 EEPROM 内容加载完毕

5. esc_check_eeprom_loading() 返回成功 → 继续正常启动
```

**代码位置**：
- 模拟核心：[hpm_ecat_e2p_emulation.c](../../hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_e2p_emulation.c)
- EEPROM 内容初始化：[hpm_ecat_hw.c](../../hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.c) 中的 `ecat_eeprom_emulation_reload()`

### 6.11 多端口配置

HPM6E80 的 ESC 有 3 个端口。配置取决于你的硬件设计：

**三端口全用（默认）**：

```c
esc_config_nmii_link_source(HPM_ESC,
    true,   // P0 链路状态来自 IO
    true,   // P1 链路状态来自 IO
    true    // P2 链路状态来自 IO
);
```

**只用一个端口（如果硬件上只用了 P0）**：

```c
// 1. 禁用 P1 和 P2
esc_change_port_description(HPM_ESC,
    false,  // P0 = 实现（正常）
    true,   // P1 = 未实现
    true    // P2 = 未实现
);

// 2. 只配置 P0 的链路来源
esc_config_nmii_link_source(HPM_ESC, true, false, false);
```

### 6.12 推荐电机控制对象规划

接 RS06 和 ENCOS 电机时，推荐从透明 buffer 逐步改成结构化对象。

#### 推荐 RxPDO（主站→从站）

| Index | Name | Type | Bit Length | Direction |
|---|---|---|---|---|
| `0x7010` | ControlWord | `UINT16` | 16 | Master → Slave |
| `0x7011` | TargetMotorId | `UINT8` | 8 | Master → Slave |
| `0x7012` | TargetPosition | `INT32` | 32 | Master → Slave |
| `0x7013` | TargetVelocity | `INT32` | 32 | Master → Slave |
| `0x7014` | TargetTorque | `INT16` | 16 | Master → Slave |
| `0x7015` | CommandSeq | `UINT16` | 16 | Master → Slave |

**总计 120 bits = 15 字节**

#### 推荐 TxPDO（从站→主站）

| Index | Name | Type | Bit Length | Direction |
|---|---|---|---|---|
| `0x6000` | StatusWord | `UINT16` | 16 | Slave → Master |
| `0x6001` | ActualMotorId | `UINT8` | 8 | Slave → Master |
| `0x6002` | ActualPosition | `INT32` | 32 | Slave → Master |
| `0x6003` | ActualVelocity | `INT32` | 32 | Slave → Master |
| `0x6004` | ActualTorque | `INT16` | 16 | Slave → Master |
| `0x6005` | ErrorCode | `UINT16` | 16 | Slave → Master |
| `0x6006` | FeedbackSeq | `UINT16` | 16 | Slave → Master |

**总计 136 bits = 17 字节**

#### 推荐 SDO（配置与诊断）

| Index | Name | Type | 用途 |
|---|---|---|---|
| `0x8000` | MotorProtocolSelect | `UINT8` | 选择 RS06 或 ENCOS |
| `0x8001` | CanBitrate | `UINT32` | CAN 波特率 |
| `0x8002` | Rs06NodeId | `UINT8` | RS06 节点 ID |
| `0x8003` | EncosNodeId | `UINT8` | ENCOS 节点 ID |
| `0x8010` | ControlMode | `UINT8` | 位置/速度/力矩模式 |
| `0x8020` | CommandTimeoutMs | `UINT16` | 控制超时 |
| `0x8100` | CanRxCount | `UINT32` | 诊断：CAN 接收计数 |
| `0x8101` | CanTxCount | `UINT32` | 诊断：CAN 发送计数 |
| `0x8102` | CanErrorCount | `UINT32` | 诊断：CAN 错误计数 |

### 6.13 修改顺序建议

**不要一次性大改！** EtherCAT 从站调试时，最难排查的问题是 ESI 和对象字典不一致。建议按这个顺序逐步进行：

```
第 1 步：先只新增 SDO，不动 PDO
  └── 加一个只读 SDO (如 0x8000 固件版本)，验证 SDO 读写

第 2 步：再新增一个很小的结构化 PDO
  └── 比如只保留 control word + status word，先跑通

第 3 步：确认从站能进 OP
  └── 确认 APPL_OutputMapping() 能触发

第 4 步：再逐步加入位置、速度、力矩、错误码等字段
  └── 每加一个字段就验证一次

第 5 步：最后再把 PDO 对接 CAN 电机控制模块
  └── 此时 PDO 已经稳定，只改业务逻辑
```

**每一步验证清单**：
- [ ] 编译通过
- [ ] 串口日志 PDO 字节数正确
- [ ] 从站能进 OP
- [ ] PDO 数据收发正确
- [ ] ESI 已同步更新

---

## 7. 参考索引

### 关键源文件索引

| 文件 | 路径 | 作用 |
|---|---|---|
| ESC 驱动头文件 | [hpm_sdk/drivers/inc/hpm_esc_drv.h](hpm_sdk/drivers/inc/hpm_esc_drv.h) | SDK ESC 驱动 API |
| ESC 寄存器定义 | hpm_sdk/soc/HPM6E00/HPM6E80/hpm_esc_regs.h | ESC 所有寄存器位域定义 |
| ESC 硬件抽象 | [hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.c](hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.c) | HW 宏实现、ISR、EEPROM 模拟 |
| ESC 硬件抽象头文件 | [hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.h](hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_hw.h) | HW 宏定义、EEPROM 模拟声明 |
| PHY 初始化 | [hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_phy.c](hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_phy.c) | MDIO 访问、PHY 配置 |
| EEPROM 模拟 | [hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_e2p_emulation.c](hpm_app/software/drivers/src/full_ecat_port/hpm_ecat_e2p_emulation.c) | Flash 模拟 EEPROM 的实现 |
| SSC 核心 | [ecat_io/SSC/Src/ecatappl.c](hpm_app/software/apps/ecat_io/SSC/Src/ecatappl.c) | MainInit/MainLoop/ECAT_Application |
| SSC 状态机 | [ecat_io/SSC/Src/ecatslv.c](hpm_app/software/apps/ecat_io/SSC/Src/ecatslv.c) | ESM 状态转换 |
| 对象字典 | [ecat_io/SSC/Src/digital_ioObjects.h](hpm_app/software/apps/ecat_io/SSC/Src/digital_ioObjects.h) | PDO 映射、SM 分配 |
| SSC 配置 | [ecat_io/SSC/Src/ecat_def.h](hpm_app/software/apps/ecat_io/SSC/Src/ecat_def.h) | 编译开关、厂商 ID |
| 应用回调 | [ecat_io/application/digital_io.c](hpm_app/software/apps/ecat_io/application/digital_io.c) | APPL_* 回调实现 |
| 主入口 | [ecat_io/main.c](hpm_app/software/apps/ecat_io/main.c) | 初始化 + 主循环 |
| 硬件配置 | [ecat_io/ecat_config.h](hpm_app/software/apps/ecat_io/ecat_config.h) | Flash 基址、定时器 |
| 板级定义 | [boards/hpm6e00_full_port/board.h](hpm_app/boards/hpm6e00_full_port/board.h) | GPIO 引脚宏 |
| 板级 ESC 初始化 | [boards/hpm6e00_full_port/board.c](hpm_app/boards/hpm6e00_full_port/board.c) | `board_init_ethercat()` |
| 网关项目协议 | [gateway_protocol.h](../../gatewat/hpm_gateway/gateway_protocol.h) | 共享协议定义 |
| 网关桥接逻辑 | [gateway_bridge.c](../../gatewat/hpm_gateway/firmware/ecat_tsn_gateway/gateway_bridge.c) | ECAT↔TSN 转发 |
| 网关 PDO/SDO 修改指南 | [09-ethercat-pdo-sdo-modification-guide.md](09-ethercat-pdo-sdo-modification-guide.md) | 面向网关项目的 PDO/SDO 修改方法 |

### 外部资源

- **EtherCAT 技术协会 (ETG)**：https://www.ethercat.org/
  - 技术规范下载（需注册会员）
  - 厂商 ID 申请（免费）
- **Beckhoff SSC 下载**：https://www.beckhoff.com/en-en/products/i-o/ethercat/ethercat-slave-stack-code/
  - SSC Tool（图形化对象字典配置工具）
  - SSC 源码（含文档）
- **SOEM (Simple Open EtherCAT Master)**：https://github.com/OpenEtherCATsociety/SOEM
  - Linux/Windows 下的开源 EtherCAT 主站
  - ecat_io 的 Linux 测试程序使用的就是这个
- **IGH EtherCAT Master**：https://etherlab.org/en/ethercat/
  - Linux 下的高性能开源主站
  - 适合生产环境
- **HPM SDK 文档**：`hpm_sdk/docs/index.html`（本地）
- **HPM 官方文档**：https://www.hpmicro.com/

---

> **下一步学习建议**：
> 1. 先把 ecat_io 编译、烧录、跑通
> 2. 读一遍 `main.c` 和 `application/digital_io.c`，理解每个函数的作用
> 3. 先只加一个只读 SDO（如固件版本号），不改 PDO，验证 SDO 读写
> 4. 再改一个很小的 PDO（如 2 字节 control word + 2 字节 status word）
> 5. 确认从站能进 OP、`APPL_OutputMapping()` 能触发
> 6. 学习 EtherCAT 状态机：看懂 `ecatslv.c` 中的状态转换逻辑
> 7. 逐步加入更多 PDO 字段直至完整电机控制
> 8. 尝试 DC-Sync 模式：在 SSC Tool 中重新配置
> 9. 学习主站编程：用 SOEM 写一个简单的主站来控制你的从站
