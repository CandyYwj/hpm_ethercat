<!-- Author: BearKing -->
# EtherCAT 完整协议参考手册 — 从会用到精通

> 以 HPM6E80 ESC 硬件 + Beckhoff SSC v5.13 源码为依据。
> 目标: 读完本文，你可以独立设计新的 EtherCAT 从站、修改任意层面协议、定位问题。

## 目录

**第一部分: 物理与硬件层**
1. [ESC 芯片内部架构](#1-esc-芯片内部架构)
2. [ESC 寄存器完整手册](#2-esc-寄存器完整手册)
3. [ESC 配置数据 (SII/EEPROM)](#3-esc-配置数据-siieeprom)
4. [SyncManager 深度解析](#4-syncmanager-深度解析)
5. [FMMU 深度解析](#5-fmmu-深度解析)

**第二部分: 线缆协议**
6. [EtherCAT 帧格式](#6-ethercat-帧格式)
7. [EtherCAT 寻址模式](#7-ethercat-寻址模式)
8. [ESC 帧处理流程](#8-esc-帧处理流程)

**第三部分: 从站协议栈**
9. [ESM 状态机完整分析](#9-esm-状态机完整分析)
10. [PDO: 过程数据对象](#10-pdo-过程数据对象)
11. [SDO 与 CoE 协议](#11-sdo-与-coe-协议)
12. [邮箱协议](#12-邮箱协议)

**第四部分: 分布式时钟**
13. [DC 理论基础](#13-dc-理论基础)
14. [DC 实现细节](#14-dc-实现细节)

**第五部分: 实战修改**
15. [修改 PDO 完整指南](#15-修改-pdo-完整指南)
16. [修改 SDO 完整指南](#16-修改-sdo-完整指南)
17. [完整修改方案](#17-完整修改方案)

**第六部分: 专家级深入**
18. [对象字典底层实现](#18-对象字典底层实现)
19. [SDO 状态机](#19-sdo-状态机)
20. [错误处理与诊断](#20-错误处理与诊断)
21. [时序分析与性能优化](#21-时序分析与性能优化)
22. [调试技术](#22-调试技术)

---

# 第一部分: 物理与硬件层

## 1. ESC 芯片内部架构

### 1.1 HPM6E80 中的 ESC IP

```
┌─────────────────────────────────────────────────────┐
│                    HPM6E80 SoC                       │
│                                                     │
│  ┌──────────┐    系统总线 (AHB/APB)                 │
│  │ CPU RV32 │                                       │
│  │ 648MHz   │◄────┬──────────┬──────────────────┐   │
│  └──────────┘     │          │                  │   │
│                   ▼          ▼                  ▼   │
│              ┌──────┐  ┌──────────┐  ┌──────────┐  │
│              │ MCAN │  │   TSW    │  │   ESC    │  │
│              │x8 FD │  │ (TSN SW) │  │(EtherCAT)│  │
│              └──────┘  └──────────┘  └─────┬────┘  │
│                                            │PHY I/F│
└────────────────────────────────────────────┼───────┘
                                             │
                                      MII/RMII/RGMII
                                             │
                                      ┌──────┴──────┐
                                      │ PHY (Port0) │── RJ45 ── RDK X5
                                      │ PHY (Port1) │── RJ45 ── TSN/Jetson
                                      │ PHY (Port2) │── RJ45 ── (预留)
                                      └─────────────┘
```

### 1.2 DPRAM 存储布局

ESC 内部有 60KB DPRAM (双端口 RAM)，物理地址 0x0000-0xEFFF:

```
0x0000 ┌──────────────────┐
       │ ESC 寄存器        │ 0x0000-0x0FFF (4KB)
       │ (0x0000-0x0FFF)  │
0x1000 ├──────────────────┤ ← MEMORY_START_ADDRESS (用户区起点)
       │ SM0: MBoxOut      │ 典型: 0x1000-0x107F
       │ (主站→从站)       │
0x1080 ├──────────────────┤
       │ SM1: MBoxIn       │ 典型: 0x1080-0x10FF
       │ (从站→主站)       │
0x1100 ├──────────────────┤
       │ SM2: PDO Output   │ 典型: 0x1100-0x123F (320 bytes)
       │ (主站→从站)       │
0x1240 ├──────────────────┤
       │ SM3: PDO Input    │ 典型: 0x1240-0x12FF (192 bytes)
       │ (从站→主站)       │
0x1300 ├──────────────────┤
       │ 未使用/保留       │
       │                   │
0xEFFF └──────────────────┘
```

**关键约束:** SM 区域不能重叠。`CheckSmSettings()` 中的地址重叠检测代码保证这一点。

### 1.3 PDI (Process Data Interface) 类型

HPM6E80 ESC 使用 **内部总线 PDI** (也叫 Digital I/O PDI 或 On-Chip Bus):

```
                 CPU
                  │
           AHB Bus Interface
                  │
        ┌─────────┴─────────┐
        │     ESC IP        │
        │  ┌─────────────┐  │
        │  │ PDI Bridge   │  │ ← 寄存器 0x0140 控制
        │  │ (AHB↔DPRAM)  │  │
        │  └─────────────┘  │
        │  ┌─────────────┐  │
        │  │ 60KB DPRAM   │  │
        │  └─────────────┘  │
        └───────────────────┘

ESC 配置数据中 PDI 类型 = 0x04 (On-Chip Bus)
```

对比其他 PDI 类型:
| PDI 类型 | ESC Config | 典型场景 |
|---------|-----------|---------|
| SPI Slave | 0x01 | 外部 MCU 通过 SPI 连接 ESC |
| 8/16-bit µC | 0x02/0x03 | 并行总线 MCU |
| **On-Chip Bus** | **0x04** | **HPM6E80 — MCU 和 ESC 集成在同一芯片** |
| 32-bit I/O | 0x05 | 高速并行 IO |

### 1.4 ESC 中断到 CPU 的路径

```
ESC 硬件事件
  │
  ▼
AL Event Register (0x0220) 位被置1
  │
  ├── 如果 AL Event Mask (0x0204) 对应位=1
  │     └── ESC IRQ 引脚拉高 → CPU NVIC 中断
  │           └── CPU 进入 PDI_Isr()
  │
  └── 如果对应位=0 (屏蔽)
        └── 只有轮询 MainLoop() 才能检测到
```

你的项目中:
```c
// DC/SM-Sync: 中断使能, ISR 中处理
StartInputHandler() → SetALEventMask(PROCESS_OUTPUT_EVENT)
                    → bEscIntEnabled = TRUE

// FreeRun: 中断屏蔽, MainLoop 轮询
bEscIntEnabled = FALSE
```

---

## 2. ESC 寄存器完整手册

### 2.1 寄存器分类

| 区域 | 地址范围 | 功能 |
|------|---------|------|
| ESC 信息 | 0x0000-0x000F | 芯片类型、版本、DPRAM 大小 |
| 从站控制 | 0x0010-0x001F | 从站地址、站点别名 |
| DL 状态 | 0x0100-0x012F | 数据链路层状态 |
| **AL 控制/状态** | **0x0120-0x013F** | **应用层状态机** |
| PDI 控制 | 0x0140-0x015F | 过程数据接口配置 |
| 中断/事件 | 0x0200-0x022F | AL Event + 屏蔽 |
| 看门狗 | 0x0400-0x044F | PDI 看门狗 |
| **EEPROM** | **0x0500-0x050F** | EEPROM 访问接口 |
| **SyncManager** | **0x0800-0x087F** | SM 0-7 配置/状态 |
| **DC** | **0x0900-0x09AF** | 分布式时钟 |
| 用户内存 | 0x0F00-0x0FFF | ESC 特性特定寄存器 |

### 2.2 ESC 信息寄存器 (0x0000-0x000F)

```c
0x0000: ESC 类型
  [7:0]   ESC 类型 (ET1100=0x06, ET1200=0x01, HPM IP Core=?)
0x0001: 版本
  [7:0]   ESC 版本号
0x0002: 端口数
0x0004: 通信信息  ★
  [11:8]  支持的 SM 通道数 (HPM6E80: 8)
  [15:12] 支持的 FMMU 通道数 (HPM6E80: 8)
  [31:16] DPRAM 大小 (KB) (HPM6E80: 60)
0x0008: ESC 特性
  [3]     1=支持 64-bit DC (否则只有 32-bit)
  [6]     1=支持 ECAT LED 覆盖
  [7]     1=支持 EEPROM 仿真
```

### 2.3 AL 控制/状态寄存器 (0x0120-0x013F)

```c
0x0120: AL Control Request (16-bit)  ★★★
  ┌───────┬───────┬─────────────────────────────────┐
  │ Bits  │ 名称   │ 含义                            │
  ├───────┼───────┼─────────────────────────────────┤
  │ [3:0] │ STATE  │ 请求状态 (1=INIT,2=PREOP,4=SAFEOP,8=OP)│
  │ [4]   │ Ack    │ 错误确认                         │
  │ [5]   │ ID     │ 请求 Explicit Device ID          │
  │ [15:6]│ 保留   │                                  │
  └───────┴───────┴─────────────────────────────────┘
  物理特性: 主站写入 → ESC 置 AL_CONTROL_EVENT
           从站读后 → ESC 自动清除该事件
           从站写入 → ESC 清除 STATE_CHANGE 位

0x0130: AL Status Response (8-bit) ★★★
  [3:0]  当前状态 (1=INIT,2=PREOP,4=SAFEOP,8=OP)
  [4]    STATE_CHANGE (错误指示/确认)
  [5]    ID 应答
  [6]    保留
  [7]    保留

0x0134: AL Status Code (16-bit)
  0x0000 = 无错误
  0x001A = 同步错误
  0x001B = SM 看门狗
  0x0032 = PLL 同步错误
  ... (完整列表见 ecatslv.h)
```

### 2.4 中断/事件寄存器 (0x0200-0x022F)

```c
0x0200: IRQ 信号寄存器 (由 ESC 硬件自动控制)
0x0204: AL Event Mask ★
  bit=1: 该事件产生 IRQ
  bit=0: 该事件只轮询

0x0220: AL Event Request (16-bit) ★★★
  读: 获取所有待处理事件 (读后不清除)
  写: 清除特定位 (写1清除)
  
  位:
  [0]   AL Control Event   — 主站写了 0x0120
  [2]   Sync0 Event        — DC Sync0 脉冲
  [3]   Sync1 Event        — DC Sync1 脉冲
  [4]   SM Change Event    — SM 配置被改
  [5]   EEPROM Cmd Pending — EEPROM 命令待处理
  [8]   MBox Write Event   — SM0 被写入
  [9]   MBox Read Event    — SM1 被读出
  [10]  Output Event       — SM2 被写入 ★ 你的核心事件
  [11]  Input Event        — SM3 被读出
```

### 2.5 EEPROM 接口 (0x0500-0x050F)

```c
0x0500: EEPROM Configuration
  [0]     1=PDI 控制 EEPROM
  [8]     1=PDI 锁定 EEPROM
  [15:0]  配置 (来自 EEPROM 的16-byte ConfigData)

0x0502: EEPROM Control/Status
  [6]     读宽度: 0=4 byte, 1=8 byte
  [8:10]  命令:
          0x1=读 (ESC_EEPROM_CMD_READ)
          0x2=写 (ESC_EEPROM_CMD_WRITE)
          0x4=重载 (ESC_EEPROM_CMD_RELOAD)
  [11]    CRC 错误
  [12]    EEPROM 未加载
  [13]    命令错误
  [14]    写错误
  [15]    EEPROM 忙

0x0504: EEPROM Address
  [15:0]  当前操作的字地址

0x0508-0x050F: EEPROM Data
  4 或 8 字节的数据缓冲区 (取决于 0x0502.6)
```

---

## 3. ESC 配置数据 (SII/EEPROM)

### 3.1 EEPROM 内容结构

EEPROM (Slave Information Interface) 是 EtherCAT 从站的"身份证"，包含:

```
Word 0x00-0x07:  ESC 寄存器配置区 (16 bytes → 写入 ESC 0x0100-0x010F)
Word 0x08-0x0F:  从站信息区
  [0]    Vendor ID           32-bit
  [2]    Product Code        32-bit
  [4]    Revision Number     32-bit
  [6]    Serial Number       32-bit
Word 0x10-0x13:  硬件延迟
Word 0x14+:      字符串区 (设备名、硬件/软件版本)
Word ...:         邮箱/PDO/DC 配置区
Word ...:         Category 区 (分类信息)
```

### 3.2 ESC 寄存器配置区 (Word 0x00-0x07)

这 16 bytes 在 ESC 上电后自动加载到寄存器 0x0100-0x010F:

```
Byte 0:    PDI 类型 (0x04 = On-Chip Bus)
Byte 1:    PDI 配置
Byte 2:    SYNC/LATCH 配置
Byte 3:    PDI 扩展配置
Byte 4:    邮箱 SM 配置
Byte 5:    邮箱 SM 方向 + 模式
Byte 6:    邮箱 SM 大小
Byte 7:    邮箱 SM 使能
Byte 8:    PDO 输出 SM 配置
... (总共 16 bytes)
```

你的项目中的值: `800C8166000000001234`

```
80 = PDI 类型 0x04 | bit7=1 (On-Chip Bus + 设备仿真使能)
0C = 0x000C (保留)
81 = SM0 控制字节
66 = SM1 控制字节
...
```

### 3.3 EEPROM 仿真 (Flash → EEPROM)

HPM6E80 不使用物理 EEPROM 芯片，而是将 EEPROM 数据存储在 MCU 内部 Flash 中:

```c
// ecat_config.h
#define FLASH_EEPROM_ADDR   (board_flash_base + offset)  // Flash 中的偏移
#define FLASH_EEPROM_SIZE   0x10000  // 64KB

// eeprom.h (SSC Tool 生成)
unsigned char aEepromData[2048] = {
    // 完整的 2048 字节 EEPROM 镜像
    0x80, 0x0C, 0x81, 0x66, ... // ESC 配置区
    0x4D, 0x50, 0x48, 0x00, ... // Vendor ID = 0x0048504D
    ...
};

// main.c 中的同步检查
if (!gateway_eeprom_image_matches())
    gateway_refresh_eeprom_image();
// 逐字比对 aEepromData[] 与 Flash 中的镜像, 不一致则刷写
```

---

## 4. SyncManager 深度解析

### 4.1 SM 寄存器完整定义

每个 SM 占用 8 字节 (从 0x0800+8N 开始):

```c
// 0x0800+8*N: Address & Length (32-bit)
//   [15:0]  物理起始地址 (必须在 DPRAM 0x1000-0xEFFF 范围内)
//   [31:16] 缓冲区长度 (bytes)

// 0x0804+8*N: Control Byte (8-bit)
//   [0]     0=主站读, 1=主站写 ★
//   [1]     0=3-buffer 模式, 1=1-buffer 模式
//   [2]     保留
//   [3]     0=正常, 1=ECAT 帧类型 (仅 SM0)
//   [4]     保留
//   [5]     保留
//   [6]     0=无看门狗, 1=看门狗触发 ★

// 0x0805+8*N: Status Byte (8-bit)
//   [0]     缓冲区状态
//   [1]     1=邮箱满 (SM0 被完整写入/MB1被完整读取)
//   [3]     1=读取完成确认

// 0x0806+8*N: Activate Byte (8-bit)
//   [0]     1=SM 使能 ★
//   [1]     1=重复请求 (Repeat Request)
//   [7:2]   保留

// 0x0807+8*N: PDI Control Byte (8-bit)
//   [0]     0=PDI 使能SM, 1=PDI 禁用SM ★ (SSC用此来使能/禁用)
//   [1]     1=重复请求确认 (Repeat Ack)
```

### 4.2 3-Buffer 模式工作原理

```
主站每周期写入 PDO 输出, 从站也需要读。

3-Buffer 模式的硬件自动管理:

  Buffer 0 ──── 当前被主站写入
  Buffer 1 ──── 上一个完成的帧
  Buffer 2 ──── 从站正在读取

  ESC 硬件保证:
  - 从站总是读到完整帧 (不会半帧)
  - 主站总是可以开始写下一帧 (不会因为从站正在读而阻塞)
  - 自动切换: 主站写完成 → 该 buffer 变为"最新" → 从站读时获得最新

对比 1-Buffer:
  Buffer 0 ──── 主站写和从站读共享
  风险: 主站写到一半时从站读 → 半新半旧数据
  用于: 邮箱 (非实时数据, 不需高吞吐)
```

### 4.3 SM 操作的关键时序

```
使能 SM:
  DisableSyncManChannel: 写 0x01000000 到 SM PDI Control
  EnableSyncManChannel:  写 0x00000000 到 SM PDI Control
  → 必须轮询确认 PDI Control bit0 变成预期的值

读 SM 配置:
  GetSyncMan(channel):
    HW_EscRead(&SyncManInfo, 0x0800+8*channel, 8)
    → 同步读取整个 SM 描述符

开始/停止:
  StartInputHandler():
    EnableSyncManChannel(SM2)  PDO 输出
    EnableSyncManChannel(SM3)  PDO 输入

  StopInputHandler():
    DisableSyncManChannel(SM2)
    DisableSyncManChannel(SM3)
```

---

## 5. FMMU 深度解析

### 5.1 为什么需要 FMMU: 逻辑寻址

EtherCAT 的性能核心是**逻辑寻址**: 主站一个 Datagram 可以读写多个从站的数据，而不需要逐站寻址。

```
节点寻址: 主站 → 从站 A → 从站 B → 从站 C
  每个 Datagram 只能访问一个从站 → 慢

逻辑寻址:
  主站 → [逻辑地址 0x00001000, 长度 64 bytes]
    ESC A FMMU: 逻辑 0x00001000 → 物理 0x1100 (SM2) ← 匹配 16 bytes
    ESC B FMMU: 逻辑 0x00001010 → 物理 0x1100 (SM2) ← 匹配 20 bytes
    ESC C FMMU: 逻辑 0x00001024 → 物理 0x1100 (SM2) ← 匹配 28 bytes
  → 一个 Datagram 同时写 3 个从站的 PDO!
```

### 5.2 FMMU 配置 (16 bytes per channel)

```c
// ESC 寄存器 0x0600+16*N
[0-3]  逻辑地址起始   (32-bit)
[4-5]  长度           (16-bit)
[6]    逻辑起始位偏移 (8-bit)  — 位级精度!
[7]    逻辑终止位偏移 (8-bit)
[8-9]  物理地址起始   (16-bit)
[10]   物理起始位偏移 (8-bit)
[11]   方向:
         1 = 只读 (从站→主站)
         2 = 只写 (主站→从站)  ← PDO Output
         3 = 读写
[12]   激活 (1=使能)
[13-15] 保留
```

### 5.3 FMMU 映射实例

16 轴 MIT 从站:

```
主站配置 2 个 FMMU:

FMMU0 (Output): 逻辑 0x00000000 → ESC 物理 0x1100, 长度 320
FMMU1 (Input):  逻辑 0x00001000 → ESC 物理 0x1240, 长度 192

主站 EtherCAT 帧 → 逻辑写 0x00000000, 320B → FMMU0 → 写到 DPRAM 0x1100 (SM2)
主站 EtherCAT 帧 → 逻辑读 0x00001000, 192B → FMMU1 → 从 DPRAM 0x1240 (SM3) 读取
```

---

# 第二部分: 线缆协议

## 6. EtherCAT 帧格式

### 6.1 以太网帧结构

```
┌─────────┬─────────┬──────────┬──────────────────────┬──────┐
│Dst MAC  │Src MAC  │EtherType │   EtherCAT 数据       │ FCS  │
│6 bytes  │6 bytes  │0x88A4    │  (变长)              │4 bytes│
└─────────┴─────────┴──────────┴──────────────────────┴──────┘
```

EtherType `0x88A4` 标识这是一个 EtherCAT 帧。

### 6.2 EtherCAT 帧头 (2 bytes)

```
Bit [0-10]:  长度 (EtherCAT 数据区长度, 不含头部)
Bit [11]:    保留
Bit [12-14]: 类型
               1 = 接续帧 (有前导 EtherCAT 数据的中间帧)
               其他 = 起始帧
Bit [15]:    保留
```

一个完整 EtherCAT 数据可由多个以太网帧拼接而成。

### 6.3 EtherCAT Datagram (数据报)

每个 Datagram = 10 byte Header + 数据 + 2 byte Working Counter:

```
[0]     Cmd    (1 byte)  — 命令码
[1]     Idx    (1 byte)  — 帧内索引
[2-3]   Addr   (2 bytes) — 地址 (节点寻址) 或 物理地址低16位
[4]     Len    (1 byte)  — 数据长度低8位 (0-255)
[5]     LenHi  (1 byte)  — 长度 [11:8] + 中断 (位15)
[6]     Cir    (2 bytes) — 逻辑地址高16位 (逻辑寻址)
[8-9]   IRQ    (2 bytes) — 保留/中断请求
[10-...] Data  (0-N bytes)
[...+2] WKC    (2 bytes) — Working Counter
```

### 6.4 所有 EtherCAT 命令一览

```
Cmd  名称   寻址方式      方向        示例用途
──────────────────────────────────────────────────
0    NOP    无操作        无
1    APRD   自动递增      读          扫描从站
2    APWR   自动递增      写
3    APRW   自动递增      读写
4    FPRD   固定地址      读          DC 时间读取
5    FPWR   固定地址      写          DC 时间写入
6    FPRW   固定地址      读写
7    BRD    广播          读          全局系统时间
8    BRW    广播          写
9    LRD    逻辑地址      读          PDO 输入 ★
10   LWR    逻辑地址      写          PDO 输出 ★
11   LRW    逻辑地址      读写
12   ARMW   自动递增      读写(读-修改-写)
```

LRD/LWR 是最常用的 PDO 命令。L 开头 = 逻辑寻址(走 FMMU)，F 开头 = 固定地址(直接 DPRAM 地址)，B 开头 = 广播(所有从站)。

### 6.5 Working Counter (WKC) 深度解析

WKC 是 EtherCAT 的流控机制:

```
WKC 由一个从站处理后 +N, N 取决于命令:

读命令 (FPRD/LRD/BRD): 成功返回数据 → +1
写命令 (FPWR/LWR/BRW): 成功写入数据 → +1
读写命令 (FPRW/LRW): 读取成功 → +1, 写入成功 → +1 (总共 +2)
ARMW: 读取成功 → +1, 写入成功 → +2 (总共 +3)

特殊情况:
  地址不匹配: WKC += 0 (数据被忽略)
  广播命令: 每个从站都处理, WKC 累加所有匹配从站的计数
```

主站通过检查 WKC 来判断从站是否正常工作。

---

## 7. EtherCAT 寻址模式

### 7.1 四种寻址模式

| 模式 | 命令前缀 | 地址字段含义 |
|------|---------|------------|
| **位置寻址** | F (Fixed) | 物理地址 [15:0] + 环形偏移 |
| **节点寻址** | A (Auto-Increment) | 16-bit 节点地址 |
| **逻辑寻址** | L (Logical) | 32-bit 逻辑地址 (FMMU 映射) |
| **广播** | B (Broadcast) | 无地址 |

### 7.2 位置寻址 (Fixed Addressing)

```
主站 → 从站在环网中的位置固定:

环形顺序:
  ┌───┐    ┌───┐    ┌───┐
  │ A │───→│ B │───→│ C │
  └───┘    └───┘    └───┘
  位置0     位置1     位置2

  位置0 = 第一个经过的从站
  位置-1 = 最后一个经过的从站 (可用负数位置)

FPWR, 从站地址寄存器 = 0x1100 → 只有位置匹配的从站处理
```

### 7.3 节点寻址 (Auto-Increment Addressing)

```
主站 → 地址字段 = 0x0000

从站处理流程:
  if (地址 == 0x0000)
      处理此 Datagram, WKC++
  else if (地址 < 0xFFFF)
      地址-- (转给下一从站)

APRD, Addr=0 → 第一个从站处理
APRD, Addr=1 → 第二个从站处理
APRD, Addr=0xFFFF → 所有从站都忽略
```

主站上电时用 APRD 扫描: Addr=0 → 读第一个从站的 Vendor ID，Addr=-1 → 继续读，直到 Addr=-1 且无响应 → 知道有多少从站。

### 7.4 逻辑寻址 (最常用)

```
主站 → 32-bit 逻辑地址 (来自 Datagram Header Addr [15:0] + Cir [15:0])

ESC 硬件:
  for (每个使能的 FMMU) {
      if (逻辑地址在 FMMU 范围内)
          → 映射到物理 DPRAM 地址
          → 执行读/写
          → WKC += 1
  }
```

你的 PDO 通信就靠这个。主站配置好 FMMU 后，LRD/LWR 命令自动路由。

---

## 8. ESC 帧处理流程

### 8.1 一帧通过 ESC 的完整过程

```
① 帧从 Port0 进入 ESC
   ↓
② ESC 检查 EtherType
   if (EtherType == 0x88A4) → 这是 EtherCAT 帧
   else if (EtherType == 其他) → 根据 MAC 转发表处理 (如 TSN 帧)
   ↓
③ ESC 解析每个 Datagram
   对于每个 Datagram:
     a) 判断寻址方式 (看 Cmd)
     b) 判断地址是否匹配 (自身/下一个/忽略)
     c) 如果匹配:
        - 读命令: 从 DPRAM 读数据 → 插入到输出帧
        - 写命令: 从输入帧提取数据 → 写到 DPRAM
     d) 更新 WKC
   ↓
④ 帧从 Port1 发出 (如果存在) 或返回主站
   ↓
⑤ 有写操作时, ESC 检查 SM 事件条件
   如果满足触发条件 → 置对应的 AL Event 位 → 可选 IRQ
```

### 8.2 SM2 事件的触发时序

```
主站发送 LWR → ESC 写入 SM2 buffer
  ↓
ESC 硬件判断:
  if (写入的数据量 >= SM2 的触发长度)
      置 PROCESS_OUTPUT_EVENT (0x0220 bit10)

从站检测:
  中断模式: PDI_Isr() 立即被调用
  轮询模式: MainLoop() 下次检查时发现事件

从站操作:
  PDO_OutputMapping() → 读 DPRAM → APPL_OutputMapping()
```

---

# 第三部分: 从站协议栈

## 9. ESM 状态机完整分析

(此章基于 [§5-6 of the earlier doc] 的深化)

### 9.1 状态转换的种族条件处理

SSC 处理了一个关键的种族条件: SAFEOP→OP 转换。

```
问题: 主站发送 SAFEOP→OP 命令时, 可能还没有开始周期发送 PDO 数据

SSC 的解决方案:
  SAFEOP→OP 触发:
    1. AL_ControlInd() → StartOutputHandler()
    2. StartOutputHandler() 返回 NOERROR_INWORK (0xFF)  ← 不立即完成!
    3. 设置 bEcatWaitForAlControlRes = TRUE
    4. 设置 EsmTimeoutCounter = SAFEOP2OPTIMEOUT (9000ms)
    5. 每周期 AL_ControlRes() 检查:
       - 是否收到第一个 SM2 事件? (bEcatFirstOutputsReceived)
       - 是否 DC 同步就绪? (i16WaitForPllRunningCnt)
    6. 条件满足 → 调用 APPL_StartOutputHandler()
       → bEcatOutputUpdateRunning = TRUE → 完成转换
    7. 超时 → 拒绝转换, 错误码 ALSTATUSCODE_SMWATCHDOG
```

### 9.2 错误处理机制

```c
// ecatslv.c — AL_ControlInd 的错误处理

// 两种情况:
// ① 主站驱动的状态转换失败:
nAlStatus |= STATE_CHANGE;     // bit4=1 (错误标记)
SetALStatus(nAlStatus, code);  // 写错误码到 0x0134
// 主站必须发送 STATE_CHANGE|currentState 来确认错误

// ② 从站内部错误 (看门狗/DC 同步):
CheckIfEcatError() → AL_ControlInd(STATE_SAFEOP, error_code)
// 自动从 OP 回退到 SAFEOP

// ③ 错误恢复:
主站写 0x0120 = 目标状态 | STATE_CHANGE → bErrAck = 1
→ AL_ControlInd 清除 STATE_CHANGE → 重新使能 SM → 尝试进入目标状态
```

---

## 10. PDO: 过程数据对象

### 10.1 PDO 同步模式选择指南

| 场景 | 推荐模式 | 优点 | 缺点 |
|------|---------|------|------|
| 原型/调试 | FreeRun | 简单, 无需配置 | 无同步, 抖动大 |
| 低延迟 | SM-Sync | 无需 DC 硬件支持 | 只和主站帧同步 |
| 多从站同步 | DC-Sync | 1µs 精度 | 需要 DC 配置 |
| 你的场景 | DC-Sync | RDK 主站支持, 全系统同步 | — |

### 10.2 PDO 数据一致性保证

**问题:** PDO 有多个对象 (5×REAL32)，主站一次性写入，但 ESC 可能在中途被从站读取。

**ESC 硬件保证:**
```
3-Buffer 模式:
  主站写 → Buffer X (硬件锁定, 从站不可读)
  写完成 → 硬件原子切换 → Buffer X 成为"最新"
  从站读 → 永远读到完整的最新 buffer

1-Buffer 模式:
  主站写 → 直接写 buffer
  从站读 → 可能读到不完整数据
  → 使用 SM 锁定寄存器 (0x0805) 来协调
```

### 10.3 PDO Mapping 的位级精度

```c
// 支持非字节对齐的 PDO 映射!

// 例子: 映射 16-bit + 8-bit + 1-bit
0x60400010  →  Index=0x6040, 16 bits
0x60410008  →  Index=0x6041, 8 bits
0x60420001  →  Index=0x6042, 1 bit
// 总计 25 bits = 3.125 bytes → 映射后占 4 bytes

// FMMU 配置中, bit offset 可以精确到位:
// 逻辑起始位偏移 = 6 → PDO 数据对齐到第 6 位
```

---

## 11. SDO 与 CoE 协议

### 11.1 CoE (CAN application layer over EtherCAT)

CoE 借用了 CANopen 的对象字典模型:

```
CANopen                  EtherCAT (CoE)
────────                 ──────────────
SDO (Service Data)       SDO (CoE Mailbox)
PDO (Process Data)       PDO (EtherCAT SyncManager)
Object Dictionary        Object Dictionary
EDS (Electronic Sheet)   ESI (EtherCAT Slave Info)
NMT (Network Mgmt)       ESM (State Machine)
```

### 11.2 SDO 协议细节

**Expedited Upload (≤4 bytes):**
```
请求 (主站→从站):
  [cmd=0x40][Index Lo/Hi][SubIdx][Reserved]
  CoE Number = 1 (SDO Request)
  Mailbox Channel = 3 (CoE)

响应 (从站→主站):
  [cmd=0x43][Index Lo/Hi][SubIdx][4-byte data]
  Mailbox Channel = 3 (CoE)
```

**Normal Upload (>4 bytes):**
```
请求:
  [cmd=0x40][Index][SubIdx][Reserved]
  不含 Size (让从站决定)

响应:
  [cmd=0x42][Index][SubIdx][Size 4B][N-byte data]
  含完整大小信息
```

**Download (任意大小):**
```
请求 (带大小):
  [cmd=0x22][Index][SubIdx][Size 4B][N-byte data]
  → 从站验证大小, 如果匹配 → 写入

响应:
  [cmd=0x60][Index][SubIdx][Reserved]
  → 成功确认
```

**Segmented Transfer (>120 bytes, 超过邮箱 buffer):**
```
请求段 0:
  [cmd=0x00/0x01(toggle)][N bytes data]
  → 从站: toggle=0

响应段 0:
  [cmd=0x20(toggle flipped)][M bytes data]

请求段 1:
  [cmd=0x01(toggle)][N bytes data]
  → 继续到传输完成
```

### 11.3 CoE SDO Info 服务

SDO Info 不是读写数据，而是**询问对象字典的元数据**:

```
请求: Index=0x1000, SubIdx=0, type=GetEntryDescription
响应: 返回该对象的类型、访问权限、名称

请求: Index=0x0000 (获取下一个对象的索引)
响应: 返回下一个对象的 Index → 主站可以遍历整个对象字典
```

这是 TwinCAT 扫描设备时做的第一件事: 遍历所有对象 → 建立对象字典缓存。

---

## 12. 邮箱协议

### 12.1 邮箱缓冲区管理 (来自 mailbox.c 的注释)

SSC 维护 4 个缓冲区指针:

```
psWriteMbx:   正在写的缓冲区
psReadMbx:    正在 SM1 发送区等待主站读取的缓冲区
psRepeatMbx:  上一次的发送缓冲区 (供重复请求)
psStoreMbx:   暂存的发送缓冲区 (用于重复请求期间的暂存)
```

### 12.2 重复请求 (Repeat Request)

主站可以请求从站重发上一次的邮箱响应 (例如 CRC 错误):

```
1. 主站切换 SM1 PDI Control 的 Repeat Request 位
2. ECAT_Main() 检测到 repeat bit toggle
3. MBX_MailboxRepeatReq() 重新从 psRepeatMbx 发送
4. 从站写 Repeat Ack 位 → 确认重复请求
```

### 12.3 邮箱大小考虑

```c
// 邮箱 buffer 大小 = SIZEOF(TMBX):
//   TMBX = Mailbox Header(6) + 最大协议 header(约20) + SDO data(最多 mailbox size)
//
// 你的配置:
//   SM0/SM1 size = 0x80 = 128 bytes
//   → 实际可用 SDO 数据 = 128 - 6(mbx header) - ~8(coe header) ≈ 114 bytes
//   → > 114 bytes 的 SDO 访问需要使用 Segmented Transfer
```

---

# 第四部分: 分布式时钟

## 13. DC 理论基础

### 13.1 DC 的目标

让所有从站拥有**完全同步的时间基准**，从而:

- 多个从站同时采样输入 (Sync0 触发)
- 多个从站同时更新输出 (Sync0 触发)
- 同步精度 < 100 ns (理论上), 实际 ~1 µs

### 13.2 DC 核心参数

```c
System Time (64-bit, ns):
  从 2000-01-01 00:00:00 开始的纳秒计数
  由首个支持 DC 的从站 (DC Reference Clock) 提供

Sync0 Cycle Time (32-bit, ns):
  Sync0 信号的间隔
  1ms = 1,000,000 ns

Sync0 Shift Time (32-bit, ns):
  Sync0 相对于 Sync 帧到达的偏移
  用于补偿传播延迟

Sync1 Cycle Time (32-bit, ns):
  Sync1 信号的间隔 (通常 > Sync0)
  可用于子周期: Sync1 = 4ms, Sync0 = 1ms
```

### 13.3 传播延迟补偿

```
主站发送帧的传播延迟:

  主站 ── 100ns ──► 从站 A ── 150ns ──► 从站 B ── 120ns ──► 返回

  A 接收到帧的时间: T + 100ns
  B 接收到帧的时间: T + 250ns
  两者不同步!

DC 补偿:
  A 的 Sync0 = SystemTime + 500ns (预定义的偏移)
  B 的 Sync0 = SystemTime + 500ns ← 基于相同的 SystemTime
  → A 和 B 在同一时刻产生 Sync0 脉冲
```

---

## 14. DC 实现细节

### 14.1 System Time 的建立

```
1. 主站选择 DC Reference Clock 从站 (通常是第一个支持 DC 的)
2. 主站 FPWR: 写入 DC Reference Clock 从站的 0x0910 (System Time)
3. ESC 硬件: System Time 开始自增 (每个 10ns tick +10)
4. 主站 ARMW: 读出 0x0910 → 写入下一个从站的 0x0910
5. 所有从站拥有同步的 System Time
```

### 14.2 Sync0 生成

```c
// ESC 硬件自动运行:
while (1) {
    if (SystemTime >= nextSync0Time) {
        产生 Sync0 脉冲
        nextSync0Time += Sync0CycleTime
        置 SYNC0_EVENT (0x0220 bit2)
    }
}
```

### 14.3 SSC 中的 DC 处理

```c
// ① StartInputHandler: 检查 DC 配置有效性
if (Sync0CycleTime < MIN_PD_CYCLE_TIME)  // 31.2 µs
    return ALSTATUSCODE_DCSYNC0CYCLETIME;

// ② Sync0_Isr: 每次 Sync0 脉冲
Sync0WdCounter = 0;  // 喂狗
ECAT_Application();  // 同步触发用户应用

// ③ DC_CheckWatchdog: 每 1ms 检查 DC 健康
if (Sync0WdCounter >= Sync0WdValue)  // Sync0 看门狗过期
    bDcRunning = FALSE;  // → CheckIfEcatError → SAFEOP

// ④ SAFEOP→OP 时确认 DC 就绪
if (bDcSyncActive)
    i16WaitForPllRunningTimeout = 200;  // 等待 200ms PLL 稳定
```

---

# 第五部分: 实战修改

## 15. 修改 PDO 完整指南

### 15.1 你的 PDO 修改检查清单

```
□ ecat_def.h         — MAX_PD_*_SIZE
□ digital_ioObjects.h  — 对象变量 + PDO mapping struct
□ digital_ioObjects.h  — SM 分配数组 (sRxPDOassign/sTxPDOassign)
□ digital_ioObjects.h  — ApplicationObjDic[] 注册
□ gateway_app.h       — mit_command_t / mit_feedback_t struct
□ gateway_app.h       — #define 常量 (NUM_AXES, *_FLOATS)
□ gateway_app.c       — s_command_objects / s_feedback_objects 指针表
□ gateway_app.c       — sync_*_cache() 函数体
□ gateway_app.c       — APPL_*Mapping() 偏移计算
□ main.c              — PDO size check
□ ESI XML             — PDO 映射条目同步
□ eeprom.h            — EEPROM 镜像同步
```

### 15.2 改 PDO 的踩坑指南

| 坑 | 症状 | 解决 |
|----|------|------|
| MAX_PD_INPUT/OUTPUT 写反 | 从站进不了 OP | INPUT=反馈, OUTPUT=命令 |
| PDO mapping count 和 struct 不符 | 编译通过但数据错位 | 确保 SubIndex count = struct 元素数 |
| 漏改 ESI | 主站按旧 PDO 长度读 → 变量错位 | TwinCAT 显示奇怪的变量值 |
| 漏改 eeprom.h | 启动时 EEPROM 同步刷新, 无线循环 | 和 ESI/ObjectDic 保持一致 |

---

## 16. 修改 SDO 完整指南

(见 §10-11 的实战例子: 添加自定义 SDO 对象、Record 对象、参数保存、模式切换)

### 16.1 添加 SDO 的完整代码框架

```c
// ① 在 digital_ioObjects.h 声明对象变量
PROTO UINT16 MyNewObject0x5000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {100}  // 默认值
#endif
;

// ② 在 gateway_app.c 实现 Read/Write (如需要)
UINT8 ObjRead0x5000(UINT16 Index, UINT8 SubIdx, UINT32 Size,
                    UINT16 *pData, UINT8 bCA) { ... }
UINT8 ObjWrite0x5000(UINT16 Index, UINT8 SubIdx, UINT32 Size,
                     UINT16 *pData, UINT8 bCA) { ... }

// ③ 在 digital_ioObjects.h 注册
#ifdef _OBJD_
{NULL, NULL, 0x5000, {DEFTYPE_UNSIGNED16, 0 | (OBJCODE_VAR<<8)},
 &sEntryDesc0x5000, aName0x5000, &MyNewObject0x5000,
 ObjRead0x5000, ObjWrite0x5000, 0x0000},
#endif
```

---

## 17. 完整修改方案

### 17.1 典型场景: 添加一个新的 PDO 组 + SDO 配置

假设你需要:
- 2 个额外的电机轴 (Motor 16, 17)
- 1 个新的 SDO 对象 "Motor Count" (0x5000) 让主站可以查询当前有多少电机
- 1 个新的 SDO 对象 "CAN Bus Enable" (0x5001) 让主站控制 CAN 总线

改动量: ~15 个文件, ~150 行代码。

实施流程:
```
1. 改 gateway_app.h: NUM_MOTORS 16→18, 调整常量
2. 改 gateway_app.c: 扩展 s_*_objects[][] 指针表 (2 个新轴)
3. 改 gateway_app.c: 添加 ObjRead0x5000, ObjWrite0x5001
4. 改 digital_ioObjects.h: 添加 Motor16/17 的 8 个对象
5. 改 digital_ioObjects.h: 扩展 PDO mapping struct
6. 改 digital_ioObjects.h: 注册 2 个新的 PDO mapping 到 SM 分配
7. 改 digital_ioObjects.h: 注册所有新对象到 ApplicationObjDic
8. 改 ecat_def.h: 增大 MAX_PD_*_SIZE
9. 改 main.c: 更新 PDO size check
10. 改 ESI XML
11. 重新生成 eeprom.h
```

---

# 第六部分: 专家级深入

## 18. 对象字典底层实现

### 18.1 双向链表结构

```c
// objdef.c — 对象字典是双向链表
TOBJECT *pApplicationObjDicFirst = &ApplicationObjDic[0];
TOBJECT *pApplicationObjDicLast  = &ApplicationObjDic[N-1];

// OBJ_GetObjectHandle(Index):
//   遍历链表 → 找到 pObj->Index == Index → return pObj
//
// COE_ObjInit():
//   遍历链表 → 调用 OBJ_InitObj(pObj) 为每个对象分配/初始化
```

### 18.2 对象大小计算

```c
// objdef.c
UINT32 OBJ_GetObjectSize(UINT16 Index, UINT8 bCompleteAccess)
{
    TOBJECT *pObj = OBJ_GetObjectHandle(Index);
    
    UINT16 Code = pObj->Type >> 8;  // OBJCODE_VAR/ARR/REC
    UINT16 BitLen = pObj->Type & 0xFF;  // 位长度

    switch (Code) {
        case OBJCODE_VAR:  // 简单变量
            return BitLen / 8;
        case OBJCODE_ARR:  // 数组
            UINT16 count = *(UINT16 *)pObj->pVarPtr;  // SubIdx0
            return count * (BitLen / 8);
        case OBJCODE_REC:  // 记录
            // 遍历每个 SubIdx, 累加其位长度
            UINT32 size = 0;
            for (int i = 1; i <= count; i++)
                size += pObj->pEntryDesc[i].BitLen;
            return size / 8;
    }
}
```

### 18.3 OBJ_Write 的访问控制

```c
UINT8 OBJ_Write(UINT16 Index, UINT8 SubIdx, UINT32 Size,
                UINT16 *pData, UINT8 bCompleteAccess)
{
    // ① 对象存在
    if (!pObj) return ABORTIDX_OBJECT_NOT_EXISTING;

    // ② 写权限
    if (!(pObj->Access & ACCESS_WRITE))
        return ABORTIDX_UNSUPPORTED_ACCESS;

    // ③ PreOP 下只能写允许 PreOP 写的对象
    if ((nAlStatus & STATE_MASK) == STATE_PREOP
        && !(pObj->Access & ACCESS_WRITE_PREOP))
        return ABORTIDX_UNSUPPORTED_ACCESS;

    // ④ Complete Access 检查
    if (bCompleteAccess) {
        if (!COMPLETE_ACCESS_SUPPORTED)
            return ABORTIDX_COMPLETE_ACCESS_NOT_SUPPORTED;
        if (SubIdx != 0)
            return ABORTIDX_ENTRY_CANT_BE_WRITTEN_SI0_NOT_0;
    }

    // ⑤ 大小验证
    UINT32 objSize = OBJ_GetObjectSize(Index, bCompleteAccess);
    if (Size != objSize)
        return ABORTIDX_DATA_CANNOT_BE_READ_OR_STORED;

    // ⑥ 自定义 Write 函数
    if (pObj->Write != NULL)
        return pObj->Write(Index, SubIdx, Size, pData, bCompleteAccess);

    // ⑦ 默认: 直接拷贝
    memcpy(pObj->pVarPtr, pData, objSize);
    return 0;
}
```

---

## 19. SDO 状态机

### 19.1 SDO Server 状态

SSC 中 SDO 服务端每个连接维护:

```c
// sdoserv.c — 核心变量
static UINT16 nSdoCommandOffset;       // 当前 SDO 命令在 mailbox 中的偏移
static UINT16 nSdoReqDataLength;       // 请求数据长度
static UINT16 nSdoResDataLength;       // 响应数据长度
static UINT8  u8SdoSegToggle;         // 分段传输的 toggle bit
static UINT32 u32SdoCompleteAccessSize;// Complete Access 的完整大小
static UINT32 u32SdoPendingSegSize;    // 分段传输中还剩多少
```

### 19.2 SDO 下载状态机

```
Initial State
  ↓ SDO Download Request 到达
  ↓
Parse Request (SDOS_SdoDownloadReq)
  ↓
  ├── Expedited? (data ≤ 4 bytes in cmd)
  │     → Validate size
  │     → OBJ_Write()
  │     → Send Download Response (确认)
  │
  ├── Normal? (data fits in mailbox)
  │     → Validate size
  │     → OBJ_Write()
  │     → Send Download Response
  │
  └── Segmented? (data > mailbox size)
        → Start segmented transfer
        → Store first segment
        → Send Segmented Response (ack first segment)
        ↓
        Wait for next segment
        ↓
        Receive next segment (toggle bit flipped)
        → Copy data to aggregate buffer
        → Send ack
        ↓
        Repeat until all segments received
        → OBJ_Write() on complete data
        → Send final ack
```

### 19.3 SDO Timeout 和错误恢复

```
SDO 没有显式的超时机制 — 邮箱协议本身不处理超时。
主站如果在超时内未收到 SDO 响应 → 可以:
  1. 发送 SDO Abort → 从站清除 SDO 状态
  2. 重新发送整个 SDO 请求
```

---

## 20. 错误处理与诊断

### 20.1 SSC 错误处理架构

```
层级 1 — 硬件错误:         ESC 自动检测 → AL Event
层级 2 — SM/Watchdog 错误:  CheckSmSettings / AL_ControlInd → AL Status Code
层级 3 — 应用错误:          ECAT_StateChange → 驱动状态转换
层级 4 — 诊断消息:          DIAG_* → 0x10F3 对象 → Emergency 邮箱消息
```

### 20.2 错误码体系

```c
// ecatslv.h — 完整 AL Status Code 定义 (部分)
ALSTATUSCODE_NOERROR             0x0000  // 正常
ALSTATUSCODE_INVALIDSMINCFG      0x001E  // SM3 配置无效
ALSTATUSCODE_INVALIDSMOUTCFG     0x001D  // SM2 配置无效
ALSTATUSCODE_INVALIDINPUTMAPPING 0x0024  // PDO 输入映射无效
ALSTATUSCODE_INVALIDOUTPUTMAPPING 0x0025 // PDO 输出映射无效
ALSTATUSCODE_SMWATCHDOG          0x001B  // SM 看门狗超时
ALSTATUSCODE_SYNCERROR           0x001A  // 同步错误
ALSTATUSCODE_DCPLLSYNCERROR      0x0032  // DC PLL 同步错误
ALSTATUSCODE_DCSYNC0CYCLETIME    0x0036  // Sync0 周期无效
ALSTATUSCODE_DCSYNC1CYCLETIME    0x0037  // Sync1 周期无效
ALSTATUSCODE_MBX_COE             0x0043  // CoE 邮箱错误
ALSTATUSCODE_EE_ERROR            0x0051  // EEPROM 错误
```

### 20.3 诊断对象 0x10F3

SSC 提供 0x10F3 (Sync Error Settings):
```
0x10F3:01 — 最大同步错误计数
0x10F3:02 — Sync Error 极限 (超过此值 → AL Status Code = 0x001A)
0x10F3:03 — 同步错误计数器当前值
```

---

## 21. 时序分析与性能优化

### 21.1 PDO 通信延迟分析

```
EtherCAT 周期 = 1ms:

T+0µs    主站 Send Frame
T+5µs    帧到达从站 (传播延迟 ~20ns/m)
T+5.5µs  ESC 写 SM2 → PROCESS_OUTPUT_EVENT
T+6µs    PDI_Isr() 进入 (中断延迟 ~0.5µs)
T+12µs   PDO_OutputMapping() 完成 → APPL_OutputMapping()
T+20µs   APPL_OutputMapping() 执行完毕 (16轴 解析)
         ... 固件处理 ...
T+50µs   PDO_InputMapping() 执行 → SM3 数据就绪
T+900µs  下一个 Sync0 → ECAT_Application 触发
T+1000µs 主站下一个帧读取 SM3

从站内部总延迟: ~50µs
端到端 (主站→从站处理→主站读取): ~1ms (取决于 ECAT 周期)
```

### 21.2 CAN 电机的端到端延迟

```
EtherCAT SM2 Event → APPL_OutputMapping → CAN 帧发送
  ESC→MCU:  ~2µs
  MCU处理:  ~18µs (16轴解析)
  CAN发送:  ~200µs (4电机×2帧 @ 5Mbps, 单总线)

CAN 反馈 → ECU 处理 → APPL_InputMapping
  CAN接收:  ~200µs
  MCU处理:  ~10µs

EtherCAT 到 CAN 到反馈的最大路径延迟: ~430µs
在 1ms ECAT 周期内: (1000-430)/1000 ≈ 57% — 仍有充足余量
```

### 21.3 优化建议

```
瓶颈 1: CAN 总线带宽
  → CAN FD @ 8Mbps (vs 1Mbps): 延迟降低 8x
  → 多 MCAN 总线并行: 4 条总线比 1 条快 4x

瓶颈 2: MCU 处理时间
  → 当前 16轴 × 5 float 解析 ≈ 18µs (已优化)
  → 如果扩展到几百轴: 考虑 DMA 辅助

瓶颈 3: EtherCAT 周期
  → 1ms 足够, 不需要改
  → 如果未来需 100µs 周期: 考虑减少 PDO 长度
```

---

## 22. 调试技术

### 22.1 调试接口速查

```c
// ① ESC 寄存器读取 (最底层)
UINT32 val;
HW_EscReadDWord(val, 0x0130);  // 读 AL Status
HW_EscReadDWord(val, 0x0220);  // 读 AL Event
HW_EscReadDWord(val, 0x0980);  // 读 DC Control

// ② SSC 状态变量 (中间层)
printf("SSC: int=%d dc=%d out=%d 1stOut=%d al=%02X to=%d\n",
       bEscIntEnabled, bDcSyncActive, bEcatOutputUpdateRunning,
       bEcatFirstOutputsReceived, nAlStatus, EsmTimeoutCounter);

// ③ PDO 数据采样 (应用层)
printf("CMD[0]: pos=%.3f vel=%.3f trq=%.3f kp=%.1f kd=%.1f\n",
       g_commands[0].target_position, g_commands[0].target_velocity,
       g_commands[0].ff_torque, g_commands[0].kp, g_commands[0].kd);
```

### 22.2 常见故障诊断决策树

```
从站进不了 OP:
  ① 检查 nAlStatus = ?
     0x04 (SAFEOP) → SM2/SM3 配置或主站没发 PDO
     0x02 (PREOP)  → SM2/SM3 配置错误, 检查 PDO 大小和映射
     0x01 (INIT)   → EEPROM 加载失败或邮箱配置错误
  
  ② 检查 EsmTimeoutCounter
     -1 → 没有挂起的转换 (正常)
     0  → 转换超时失败!
     >0 → 转换还在等待

  ③ 检查 AL Status Code (0x0134)
     0x001D → SM2 输出配置错误 (PDO 大小不匹配)
     0x001E → SM3 输入配置错误
     0x0024 → PDO 输入映射无效
     0x0025 → PDO 输出映射无效

主站看到奇怪的 PDO 值:
  → PDO 映射和实际布局不一致
  → 检查 ESI IS 里的 PDO 描述
  → 检查 digital_ioObjects.h 里的 struct 定义
  
CAN 电机无反馈:
  → CAN 总线未连接/终端电阻缺失
  → CAN filter 未配置, 帧被丢弃
  → CAN 电机 ID 不匹配
```

### 22.3 诊断 LED

```c
// ESC LED 自动反映从站状态
Run LED:
  OFF     = INIT
  BLINK   = PREOP     (2.5 Hz)
  SINGLE  = SAFEOP    (200ms ON, 1000ms OFF)
  ON      = OP

Error LED:
  OFF       = 无错误
  BLINK     = 一般错误 (如 EEPROM 错误)
  SINGLE    = 同步/Watchdog 错误
  DOUBLE    = SM Watchdog 错误
```

---

## 附录 A: SSC 源码模块依赖图

```
main.c
  ├── ecatappl.c (MainInit, MainLoop, PDO_InputMapping, PDO_OutputMapping)
  │     ├── ecatslv.c (ECAT_Init, ECAT_Main, AL_ControlInd, StartInputHandler)
  │     │     ├── esc.h (寄存器宏)
  │     │     ├── mailbox.c (MBX_Init, MBX_Main, ...)
  │     │     │     └── mailbox.h (TMBX, MBXERR_*)
  │     │     └── ecatcoe.c (COE_Init, COE_ServiceInd)
  │     │           └── sdoserv.c (SDO 服务状态机)
  │     │                 └── objdef.c (OBJ_Read, OBJ_Write, OBJ_GetObjectHandle)
  │     │                       └── objdef.h (DEFTYPE_*, OBJCODE_*)
  │     └── coeappl.c (PDO 分配, Sync 参数)
  ├── gateway_app.c (APPL_* 回调 + 自定义 ObjRead/Write)
  │     └── gateway_app.h (mit_command_t, mit_feedback_t)
  └── gateway_bridge.c (TSN 桥接, 已禁用)
```

## 附录 B: 修改时的 ESC 寄存器追踪法

```c
// 调试时, 用这个方法实时观察寄存器变化:

// 1. 看状态转换
UINT32 al_status, al_event, al_control;
HW_EscReadDWord(al_status,  0x0130);  // AL Status
HW_EscReadDWord(al_control, 0x0120);  // AL Control
HW_EscReadDWord(al_event,   0x0220);  // AL Event

// 2. 看 SM 配置
UINT32 sm2_addr_len, sm2_ctrl;
HW_EscReadDWord(sm2_addr_len, 0x0800+8*2);  // SM2 Address+Length
HW_EscReadDWord(sm2_ctrl,     0x0800+8*2+4);// SM2 Control+Status

// 3. 看 Sync0 配置
UINT32 dc_ctrl, sync0_cycle;
HW_EscReadDWord(dc_ctrl,     0x0980);  // DC Control
HW_EscReadDWord(sync0_cycle, 0x09A0);  // Sync0 Cycle Time

// 4. 检查 EEPROM 状态
UINT32 eeprom_config, eeprom_ctrl;
HW_EscReadDWord(eeprom_config, 0x0500);  // EEPROM Config
HW_EscReadDWord(eeprom_ctrl,   0x0502);  // EEPROM Ctrl/Status
```

## 附录 C: 对象字典中的 CoE 标准对象

这些是 SSC/CoE 预定义的标准对象，你不需要改，但要知道它们的存在:

| Index | 名称 | 含义 |
|-------|------|------|
| 0x1000 | Device Type | 设备类型 (你的: 0x00000000 = 自定义) |
| 0x1008 | Device Name | 设备名 (你的: ecat_tsn_gateway) |
| 0x1009 | HW Version | 硬件版本 |
| 0x100A | SW Version | 固件版本 |
| 0x1018 | Identity | Vendor/Product/Revision/Serial |
| 0x10F1 | Error Settings | 错误处理配置 |
| 0x10F3 | Sync Error | 同步错误计数器 |
| 0x10F8 | Timestamp | DC 时间戳 |
| 0x1C00 | SM Type | SM 类型 |
| **0x1C12** | **RxPDO Assign** | **SM2 上的 PDO 列表** |
| **0x1C13** | **TxPDO Assign** | **SM3 上的 PDO 列表** |
| 0x1C32 | SM2 Sync Params | SM2 同步参数 |
| 0x1C33 | SM3 Sync Params | SM3 同步参数 |
| 0xF000 | Modular Profile | 模块化设备配置 |
| 0xF030 | Module List | 当前已配置的模块列表 |
