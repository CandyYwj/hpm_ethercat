<!-- Author: BearKing -->
# 从以太网帧到 APPL 回调：完整数据流

> 从最底层的网线信号讲起，追踪一个以太网帧如何经过 HPM6E80 的 ESC 硬件、SSC 协议栈，最终到达你的 `APPL_OutputMapping()` 和 `APPL_InputMapping()`。

---

## 1. 先复习：一个标准以太网帧长什么样

任何以太网通信，不管你是上网浏览网页还是跑 EtherCAT，数据在网线上都是一个帧一个帧地传输：

```
一个标准以太网帧（IEEE 802.3）：

┌─────────┬──────────┬──────────┬──────────┬──────────────────┬──────────┐
│ 前导码   │ 目的MAC   │ 源MAC     │ EtherType │   载荷(Payload)   │  CRC32   │
│ 8 bytes │  6 bytes  │  6 bytes  │  2 bytes  │  46 ~ 1500 bytes  │ 4 bytes  │
└─────────┴──────────┴──────────┴──────────┴──────────────────┴──────────┘
                                               ↑
                                    EtherType 决定了这个帧是什么协议
```

**EtherType 是关键**——接收方通过这个 2 字节的值判断帧的类型：

| EtherType 值 | 协议 | 说明 |
|---|---|---|
| `0x0800` | IPv4 | 普通上网流量 |
| `0x0806` | ARP | 地址解析协议（查 "谁的 IP 是 xxx"） |
| `0x8100` | VLAN | 虚拟局域网标签 |
| `0x88A4` | **EtherCAT** | ← 我们关心的 |
| `0x88B5` | 自定义 | 网关项目 TSN 侧的网关协议 |

---

## 2. HPM6E80 硬件上的帧处理流水线

从网线信号到 MCU 能读到的数据，经过以下硬件路径：

```
  网线
   │
   ▼
┌──────────┐
│  RJ45 座  │  物理接口
└────┬─────┘
     │ 差分信号 (TX+/TX-/RX+/RX-)
     ▼
┌──────────┐
│  PHY 芯片 │  物理层收发器（如 RTL8211）
│           │  把差分模拟信号转成数字 MII/GMII 信号
│           │  同时：自动协商速率(100M/1000M)、链路检测(Link Up/Down)
└────┬─────┘
     │ MII / GMII / RGMII（并行数字接口）
     ▼
┌──────────────────────────────────────────────────────┐
│  HPM6E80 芯片内部                                     │
│                                                      │
│  ┌──────────────────────────────────────────────┐    │
│  │ ESC 模块                                      │    │
│  │                                              │    │
│  │  ① MAC 层                                    │    │
│  │     - 从 MII 信号解析出以太网帧                │    │
│  │     - 校验 CRC32（错误就丢弃）                 │    │
│  │     - 检查目的 MAC 地址是否匹配                │    │
│  │     ↓                                        │    │
│  │  ② EtherType 分类器                          │    │
│  │     - 读 EtherType 字段（帧的第 13-14 字节）   │    │
│  │     ↓                                        │    │
│  │     EtherType = 0x88A4 ?                     │    │
│  │       ├── 是 → 进入 ③ EtherCAT 处理单元        │    │
│  │       └── 否 → 看 ⑤ 端口转发配置               │    │
│  │     ↓                                        │    │
│  │  ③ EtherCAT 处理单元                          │    │
│  │     - 解析 EtherCAT 数据报（datagram）         │    │
│  │     - 根据命令类型和地址，决定读/写 DPRAM 哪里   │    │
│  │     - 一边接收，一边处理，一边转发（on-the-fly） │    │
│  │     ↓                                        │    │
│  │  ④ DPRAM（双端口 RAM）                        │    │
│  │     - SM2 区：主站写的 PDO 输出数据             │    │
│  │     - SM3 区：MCU 写好的 PDO 输入数据          │    │
│  │     - SM0/SM1 区：邮箱数据                    │    │
│  │     ↓                                        │    │
│  │  ⑤ 转发逻辑                                  │    │
│  │     - EtherCAT 帧：自动转发到下一个端口         │    │
│  │     - 非 EtherCAT 帧：看 TSN 交换机配置转发     │    │
│  │     ↓                                        │    │
│  │  ⑦ PDI（过程数据接口）→ AHB 片上总线           │    │
│  │     - MCU 通过 HPM_ESC_BASE (0x40000000) 读写  │    │
│  └──────────────────────────────────────────────┘    │
│                         │                            │
│                         │ AHB 总线                    │
│                         ▼                            │
│  ┌──────────────────────────────────────────────┐    │
│  │ RISC-V CPU (你的代码运行在这里)                 │    │
│  │                                              │    │
│  │  SSC 协议栈 + gateway_app.c 回调               │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 3. 帧分类：EtherCAT 帧 vs 非 EtherCAT 帧

HPM6E80 的 ESC P0/P1/P2 各有两种用途：

```
                    P0                       P1                       P2
                    ══                       ══                       ══
EtherCAT 模式:   连主站/上行              连下一个从站              连下一个从站
                 处理 EtherType=0x88A4   处理 EtherType=0x88A4    处理 EtherType=0x88A4

TSN 交换机模式:   　(不用)                 转发非 EtherCAT 帧        转发非 EtherCAT 帧
                                         ↓                        ↓
                                        Jetson 设备              其他以太网设备
```

**ESC 每收到一个帧，先看目的 MAC 和 EtherType，决定走哪条路：**

```
收到的帧
  │
  ├── EtherType == 0x88A4（EtherCAT 帧）
  │     └──→ ESC 的 EtherCAT 处理单元接管
  │          - 解析内部的 EtherCAT 数据报
  │          - 读写 DPRAM
  │          - 可能触发 SM 事件 → SSC → APPL_*
  │          - 自动转发到下一个端口
  │
  └── EtherType != 0x88A4（普通以太网帧）
        └──→ 进入 TSN 交换机查表
             - MAC 查找表匹配 → 单播到对应端口
             - 未知 MAC → 广播到所有端口
             - 也可能被 CPU 捕获（如果配置了）
```

**网关项目里这两条路都在用**：

| 路径 | 用途 | EtherType | 经过的 SM |
|---|---|---|---|
| EtherCAT | RDK X5 ↔ ESC ↔ MCU ↔ Jetson | `0x88A4` | SM2/SM3（PDO） |
| 非 EtherCAT | Jetson ↔ TSN P1 ↔ TSN P2 ↔ 其他设备 | `0x0800`, `0x88B5` 等 | 无（不经过 DPAM） |

---

## 4. 深入 EtherCAT 帧内部：数据报（Datagram）

一个 EtherCAT 帧（EtherType=0x88A4）的载荷里可以包含**多个数据报**，每个数据报是一个独立的"命令"：

```
┌─────────────────────────────────────────────────────────────────┐
│ 以太网帧（EtherType=0x88A4）                                     │
├─────────────────────────────────────────────────────────────────┤
│ MAC头: Dest=广播, Src=主站MAC, EtherType=0x88A4                  │
├─────────────────────────────────────────────────────────────────┤
│ EtherCAT 头: 长度=..., 类型=1(从站通信)                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───────────────────────────────────────────────────────┐      │
│  │ 数据报 #1 (Datagram)                                   │      │
│  │ ┌─────────┬──────────┬──────────────┬──────────────┐  │      │
│  │ │ 命令     │  地址     │   长度        │   数据        │  │      │
│  │ │ 1 byte  │  4 bytes  │  2 bytes      │  n bytes     │  │      │
│  │ │ 例如:   │ 例如:     │  例如:         │              │  │      │
│  │ │ LWR=12  │ 0x1200   │  60 bytes     │  60字节PDO   │  │      │
│  │ └─────────┴──────────┴──────────────┴──────────────┘  │      │
│  └───────────────────────────────────────────────────────┘      │
│                                                                 │
│  ┌───────────────────────────────────────────────────────┐      │
│  │ 数据报 #2 (Datagram)                                   │      │
│  │ ┌─────────┬──────────┬──────────────┬──────────────┐  │      │
│  │ │ 命令     │  地址     │   长度        │   数据        │  │      │
│  │ │ LRD=13  │ 0x1300   │  60 bytes     │  (返回时填)   │  │      │
│  │ └─────────┴──────────┴──────────────┴──────────────┘  │      │
│  └───────────────────────────────────────────────────────┘      │
│                                                                 │
│  (可能还有 #3, #4, ...)                                         │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│ 工作计数器 (WKC) — 每处理一个从站 +1                               │
└─────────────────────────────────────────────────────────────────┘
```

### 4.1 常见数据报命令

| 命令 | 缩写 | 全称 | 用途 | 对应邮箱/PDO |
|---|---|---|---|
| `0x01` | APRD | Auto-increment Physical Read | 按从站位置读 | 初始化时扫描 |
| `0x02` | APWR | Auto-increment Physical Write | 按从站位置写 | 初始化时扫描 |
| `0x03` | APRW | Auto-increment Physical ReadWrite | 按从站位置读+写 | 初始化时扫描 |
| `0x04` | FPRD | Fixed Physical Read | 按从站地址读 | SDO 读（邮箱） |
| `0x05` | FPWR | Fixed Physical Write | 按从站地址写 | SDO 写（邮箱） |
| `0x06` | FPRW | Fixed Physical ReadWrite | 按从站地址读+写 | 较少用 |
| `0x07` | BRD | Broadcast Read | 广播读 | 读所有从站的同一个地址 |
| `0x08` | BWR | Broadcast Write | 广播写 | 写所有从站的同一个地址 |
| `0x09` | BRW | Broadcast ReadWrite | 广播读+写 | 较少用 |
| `0x0A` | LRD | Logical Read | 逻辑地址读 | PDO 读（FMMU 翻译后的地址） |
| `0x0B` | LWR | Logical Write | 逻辑地址写 | PDO 写（FMMU 翻译后的地址） |
| `0x0C` | LRW | **Logical ReadWrite** | 逻辑地址读+写 | ★ PDO 最常用！一条命令同时读+写 |
| `0x0D` | ARMW | Auto-increment Read Multiple Write | 自增读+多处写 | DC 时钟同步 |

### 4.2 地址的含义区别

```
物理地址（Physical Address）:
  直接指定 DPRAM 中的偏移
  例: FPRD addr=0x0810 → 读 ESC 偏移 0x0810（SM2 配置寄存器）
  
  用于: 寄存器访问、邮箱、初始化配置

逻辑地址（Logical Address）:
  不直接指定 DPRAM 偏移，而是经过 FMMU 翻译
  例: LRW addr=0x00001000 → FMMU0 翻译 → DPRAM 0x1200 (SM2 数据区)
  
  用于: PDO 过程数据交换（主站只关心"逻辑地址"，FMMU 负责翻译）
```

---

## 5. ESC 硬件处理一个数据报的过程

以一个典型的 `LRW` 命令（逻辑读写，PDO 最常用）为例：

```
数据报到达 ESC:
  LRW, 逻辑地址=0x00001000, 长度=60, 数据=[主站的60字节输出数据]
  
  ESC 硬件处理步骤：

  Step 1: 查 FMMU
         FMMU0 配置（由主站在启动时写入）:
           逻辑地址 0x00001000 → 物理地址 0x1200（SM2 数据区）
           长度: 60 字节
           方向: 输出（主站→从站）
         
         物理地址 0x1200 就是 SM2 管理的数据区起始地址。

  Step 2: SM 硬件检查
         SM2 状态: 激活？三缓冲区模式？
         → 取一个空闲的缓冲区
         → 把数据报中的 60 字节"输出数据"写入缓冲区

  Step 3: SM 硬件翻转
         → 标记"这个缓冲区有新数据"
         → 产生 SM2 事件（可触发中断或 AL Event 寄存器置位）

  Step 4: 读取数据（同一条 LRW 命令的后半部分）
         SM3 数据区（物理地址 0x1300）中 MCU 已经写好的 60 字节
         → 读出来
         → 填入数据报的"返回数据"区域
         
  Step 5: 转发帧
         → 帧继续传递到下一个端口（如果有级联的从站）
```

**示意图**：

```
     以太网帧 ──────────────────────────────────────────▶
     
     数据报: LRW, 逻辑 0x1000, 长度 60
     ┌───────────────────────────────────────────────────┐
     │ 命令 │ 逻辑地址 │ 长度 │  主站输出数据 (60B)        │
     │ LRW  │ 0x1000  │  60  │ [0][1]...[59]              │
     └───────────────────────────────────────────────────┘
     
     FMMU0 翻译:
     逻辑 0x1000 → 物理 0x1200 (SM2)
     逻辑 0x2000 → 物理 0x1300 (SM3) ← 主站之前配好的
     
     ╔════════════════════════════════════════════════════╗
     ║  ESC 硬件做的事（CPU 完全不参与）:                 ║
     ║                                                  ║
     ║  写: 数据报中的 60B → DPRAM 0x1200 (SM2 缓冲区)    ║
     ║  读: DPRAM 0x1300 (SM3 缓冲区) → 填回数据报        ║
     ║  标: SM2 置事件位                                 ║
     ║  转: 帧发到下一个端口                              ║
     ╚════════════════════════════════════════════════════╝
     
     帧继续 ──────────────────────────────────────────▶
     
     返回时，数据报已变成:
     ┌───────────────────────────────────────────────────┐
     │ 命令 │ 逻辑地址 │ 长度 │  从站输入数据 (60B)        │
     │ LRW  │ 0x1000  │  60  │ [状态][位置][温度]...       │
     └───────────────────────────────────────────────────┘
     ↑ 这是 SM3 里 MCU 上一次写入的数据
```

---

## 6. 从 ESC 事件到 MCU 中断（硬件→CPU）

ESC 硬件处理完 PDO 数据后，通过以下方式通知 CPU：

### 6.1 FreeRun 模式（默认）

```
ESC 硬件                       MCU (你的代码)
────────                       ──────────────

PDO 数据到达 (SM2)
  │                               
  ├─→ ESC 的 AL Event 寄存器 (0x0220) 某 bit 被置1
  │   表示 "SM2 有新数据"                   
  │                               
  │                               while(1) {
  │                                 MainLoop()  ← 轮询
  │                                   │
  │                                   ├─→ HW_EscReadDWord(..., ESC_AL_EVENT_OFFSET)
  │                                   │   读到 AL Event 寄存器，发现 SM2 bit=1
  │                                   │
  │                                   ├─→ PDO_OutputMapping()
  │                                   │     └─→ APPL_OutputMapping(pData) ← 你的回调
  │                                   │
  │                                   ├─→ ECAT_Application()
  │                                   │     └─→ APPL_Application()
  │                                   │         └─→ APPL_InputMapping(pData) ← 你的回调
  │                                   │
  │                                   └─→ 继续循环
  │                                 }
```

### 6.2 中断模式（SM-Sync / DC-Sync）

```
ESC 硬件                         中断控制器 (NVIC)            MCU (你的 ISR)
────────                         ────────────────            ──────────────

PDO 数据到达 (SM2)
  │
  ├─→ SM2 事件触发 ESC 中断信号
  │
  │                                PDI IRQ (IRQn_ESC)
  │                                  │
  │                                  └─→ ecat_pdi_isr()
  │                                        │
  │                                        ├─→ PDI_Isr() [SSC]
  │                                        │     ├─→ 读 AL Event
  │                                        │     ├─→ PDO_OutputMapping()
  │                                        │     │     └─→ APPL_OutputMapping()
  │                                        │     └─→ PDO_InputMapping()
  │                                        │           └─→ APPL_InputMapping()
  │                                        │
  │                                        └─→ 中断返回
  │                                           继续执行被中断的主循环
```

---

## 7. 四种帧类型的完整路径对比

### 7.1 PDO 过程数据帧（周期最快，最重要）

```
帧特征: EtherType=0x88A4, 数据报命令=LRW, 地址=逻辑地址(FMMU翻译)

路径:
  主站 → 网线 → PHY → ESC MAC
    → EtherCAT 处理单元
    → 检查命令: LRW
    → FMMU 翻译: 逻辑地址 → 物理 DPRAM 地址 (SM2 数据区)
    → 写: 把输出数据写入 SM2 缓冲区
    → 读: 从 SM3 缓冲区读输入数据填回帧
    → SM2 事件触发
    → 转发帧到下一端口
    → (MCU 侧) MainLoop 或 ISR 检测 SM2 事件
    → SSC: PDO_OutputMapping()
    → 你的 APPL_OutputMapping(pData)  ← pData 指向 60 字节输出数据
    → SSC: ECAT_Application() → APPL_Application() → APPL_InputMapping(pData)
    → 你的 APPL_InputMapping(pData)  ← 你把输入数据写入 pData (SM3)
    → 下个帧来时 ESC 自动读 SM3 发给主站

你收到的 pData:
  [60 字节的二进制数据，按 0x1600 mapping 的顺序排列]
  可能是透明 buffer，也可能是你定义的 packed 结构体
```

### 7.2 SDO 邮箱帧（偶尔，配置用）

```
帧特征: EtherType=0x88A4, 数据报命令=FPRD/FPWR, 地址=物理DPRAM地址(SM0/SM1)

路径:
  主站 → ... → ESC
    → 检查命令: FPWR (写物理地址)
    → 地址 0x1000 (SM0 数据区，邮箱)
    → ESC 把数据写入 SM0 的邮箱缓冲区
    → SM0 事件触发
    → (MCU 侧) MainLoop 检测 SM0 事件
    → SSC: mailbox.c → 解析邮箱协议头
    → 识别邮箱类型: CoE (CANopen over EtherCAT)
    → ecatcoe.c: 解析 SDO 请求
       ├── SDO Download (写): objdef.c 写对象字典 → 更新 C 变量
       │     └─→ pAPPL_CoeWriteInd(index, subidx, ...) ← 通知你
       └── SDO Upload (读): objdef.c 读对象字典 → 从 C 变量取值
             └─→ pAPPL_CoeReadInd(index, subidx, ...) ← 通知你
    → 组装 SDO 响应 → 写入 SM1 邮箱缓冲区
    → 下个帧来时 ESC 从 SM1 读走发给主站

没有 pData 指针传递，读写的是对象字典里的 C 变量(如 CanBitrate0x8001)
```

### 7.3 寄存器访问帧（初始化时用）

```
帧特征: EtherType=0x88A4, 数据报命令=APRD/APWR/FPRD/FPWR, 地址=ESC寄存器偏移

路径:
  主站 → ... → ESC
    → 检查命令: FPWR (写物理地址)
    → 地址 0x0810 (SM2 配置寄存器)
    → ESC 直接把数据写入 SM2 配置寄存器（设置 SM2 的起始地址和长度）
    → 不产生 SM 事件（这是 ESC 内部寄存器，不经过 SM）
    → MCU 不感知（完全由 ESC 硬件处理）

典型场景:
  - 主站写 0x0810-0x0817: 配置 SM2（起始地址 + 长度 + 模式）
  - 主站写 0x09A0-0x09A7: 配置 DC SYNC0 周期
  - 主站写 0x0120: 请求状态改变 (INIT→PREOP→SAFEOP→OP)
```

### 7.4 非 EtherCAT 帧（网关项目的 TSN 侧）

```
帧特征: EtherType != 0x88A4（如 0x0800 IPv4, 0x88B5 网关协议）

路径:
  Jetson → 网线 → TSN P1 端口 PHY → ESC MAC
    → ESC 检查 EtherType: 不是 0x88A4
    → 不进入 EtherCAT 处理单元
    → 进入 TSN 交换机的 MAC 查找表
    → 查表: 目的 MAC 匹配哪个端口？
        ├── 匹配 → 单播到对应端口
        └── 未匹配 → 广播到所有非 EtherCAT 端口（P1, P2）
    → 同时: gateway_tsn_rx_isr() (DMA 中断)
        - 如果 EtherType == 0x88B5 → 推入 ring buffer
        - gateway_bridge_poll() 从 ring buffer 取数据
        - 这是你的网关应用层逻辑，不经过 SSC

MCU 侧的 TSN ISR:
  这是 DMA 中断（不是 ESC 的 PDI 中断）
  gateway_tsn_rx_isr() → ring buffer → gateway_bridge_poll()
  
  不经过 EtherCAT 处理单元，不经过 DPRAM，不触发 SM 事件
```

---

## 8. 完整时序图：一个 EtherCAT 周期

把上面的通路串起来，一个完整周期发生了什么：

```
时间轴 →

[主站侧]
  主站组建 EtherCAT 帧:
    数据报1: LWR, 逻辑0x1000, 60B (输出给从站)
    数据报2: LRD, 逻辑0x2000, 60B (从从站读输入)
    ↓
  通过网卡发出
  
  ════════════════════ 网线 ════════════════════

[HPM6E80 侧 - 硬件]
  T0: 前导码到达，PHY 锁定信号
  T1: MAC 层接收完毕，CRC 校验通过
  T2: 检查 EtherType = 0x88A4 → 进入 EtherCAT 处理单元
  T3: 数据报1 (LWR):
       - FMMU0: 逻辑 0x1000 → 物理 0x1200 (SM2 数据区)
       - ESC 硬件写: 60B 输出数据 → DPRAM 0x1200~0x123B
       - SM2 三缓冲区轮转: 标记"生产者的 buffer 有新数据"
  T4: 数据报2 (LRD):
       - FMMU1: 逻辑 0x2000 → 物理 0x1300 (SM3 数据区)
       - ESC 硬件读: DPRAM 0x1300~0x133B → 填入帧
       - SM3 三缓冲区轮转: 标记"消费者的 buffer 已发送"
  T5: 帧被转发到 P1/P2 端口（如果有下级从站）
  T6: 帧从最后一个从站返回（或环回）
  
  ════════════════════ ESC 内部 ════════════════════

[MCU 侧 - FreeRun 模式]
  T7: CPU 执行到 MainLoop()
  T8: MainLoop → 读 AL Event 寄存器 (0x0220)
        bit[1] = 1 → SM2 有事件（新数据到达）
  T9: MainLoop → PDO_OutputMapping()
        → 你的 APPL_OutputMapping(pData)
        → pData = DPRAM 0x1200 的 60 字节
        → 你 memcpy 到 g_pdo_output_buf
        → g_pdo_output_updated = true
  T10: MainLoop → ECAT_Application()
        → 你的 APPL_Application() (空，网关不做)
        → PDO_InputMapping()
        → 你的 APPL_InputMapping(pData)
        → pData = DPRAM 0x1300 的 60 字节缓冲区
        → 你 memcpy g_pdo_input_buf → pData
  T11: MainLoop 剩余部分（状态机、邮箱、看门狗...）
  T12: 你的 gateway_bridge_poll()
        → 检查 g_pdo_output_updated 或 ring buffer
        → 转发 ECAT→TSN 或 TSN→ECAT

[下一个 EtherCAT 帧]
  T13: 数据报 LRD 读 SM3 → ESC 硬件读到 MCU 刚写入的输入数据 → 发回主站
  
  ════════════════════ 网线 ════════════════════

[主站侧]
  收到返回帧:
    数据报1: LWR 返回（WKC+1）
    数据报2: LRD 返回，数据=从站上报的 60B 输入数据
  ↓
  主站应用: 读取从站状态和输入数据
```

---

## 9. 为什么 APPL_InputMapping 的数据"晚一拍"？

注意上面时序中的时间差：

```
T3: ESC 写输出数据到 SM2 → MCU 马上能读到 → 本周期生效
T10: MCU 写输入数据到 SM3 → 数据在 SM3 缓冲区里
T13: 下一个帧才能读到 SM3 里的数据 → 晚一个周期

所以:

周期 N:   主站发输出↓          从站填输入↑ (主站收到的是周期N-1的输入)
周期 N+1: 主站发输出↓          从站填输入↑ (主站收到的是周期N的输入)
```

这不是 bug——这是 EtherCAT 的"流水线"设计。`APPL_OutputMapping` 是 **"立即消费"**（数据一到就读），`APPL_InputMapping` 是 **"提前生产"**（数据先写好，等下一个帧来取）。

---

## 10. 一句话总结各帧类型

| 帧类型 | EtherType | 数据报命令 | ESC 怎么处理 | 最终触发什么回调 |
|---|---|---|---|---|
| PDO 过程数据 | `0x88A4` | LRW | FMMU 翻译→写SM2+读SM3→产生SM2事件 | `APPL_OutputMapping` + `APPL_InputMapping` |
| SDO 邮箱 | `0x88A4` | FPRD/FPWR | 直接写物理地址(SM0)→产生SM0事件 | `pAPPL_CoeWriteInd` + `pAPPL_CoeReadInd` |
| ESC 寄存器 | `0x88A4` | FPRD/FPWR | 直接写 ESC 硬件寄存器 | MCU 不感知 |
| 非 EtherCAT | `0x0800` `0x88B5` 等 | 无 EtherCAT 数据报 | TSN 交换机查表转发 | DMA ISR → ring buffer → 你的轮询 |

---

## 11. 在你的代码里怎么观察这些帧？

| 你想看什么 | 加日志的地方 |
|---|---|
| 有没有 EtherCAT 帧到达 | `APPL_OutputMapping()` 入口 `printf` |
| 输出数据是什么内容 | `APPL_OutputMapping` 中 `printf("%02X ...", ((uint8_t*)pData)[0], ...)` |
| 主站有没有发 SDO | `pAPPL_CoeWriteInd` 中 `printf("SDO write: 0x%04X:%d\n", index, subindex)` |
| 非 EtherCAT 帧有没有来 | `gateway_tsn_rx_isr()` 中计数并打印 |
| 从站进 OP 了吗 | `APPL_StartOutputHandler()` 中 `printf("OP!\n")` |
| PDO 长度是否正确 | `APPL_GenerateMapping` 中 `printf("PDO: %d in, %d out\n", ...)` |
