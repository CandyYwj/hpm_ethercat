<!-- Author: BearKing -->
# SSC v5.13 协议栈源码逐行分析

> 覆盖 `ecatappl.c`、`ecatslv.c`、`objdef.c`、`sdoserv.c`、`ecatcoe.c`、`mailbox.c`、`coeappl.c`、`applInterface.h` 的完整源码级分析。

## 目录

1. [架构总览](#1-架构总览)
2. [文件清单与职责](#2-文件清单与职责)
3. [启动流程: MainInit](#3-启动流程-maininit)
4. [主循环: MainLoop](#4-主循环-mainloop)
5. [ECAT_Main: 事件轮询与状态机](#5-ecat_main-事件轮询与状态机)
6. [AL_ControlInd: 状态机核心](#6-al_controlind-状态机核心)
7. [PDO 数据搬运: PDI_Isr → APPL_Input/OutputMapping](#7-pdo-数据搬运)
8. [SyncManager 与同步模式](#8-syncmanager-与同步模式)
9. [对象字典: objdef + sdoserv](#9-对象字典-objdef--sdoserv)
10. [邮箱协议: mailbox.c + ecatcoe.c](#10-邮箱协议-mailboxc--ecatcoec)
11. [完整调用链图示](#11-完整调用链图示)
12. [用户应用层接口](#12-用户应用层接口)
13. [关键全局变量速查表](#13-关键全局变量速查表)

---

## 1. 架构总览

```
main.c (用户入口)
  │
  ├── MainInit()           ← SSC 初始化
  │     └── ECAT_Init()    ← ESC 硬件初始化
  │
  └── while(1) {
        MainLoop()         ← 主循环
          ├── ECAT_Application()  ← 用户应用逻辑 (APPL_Application)
          ├── PDO_InputMapping()  ← 仅 FreeRun 模式
          └── ECAT_Main()         ← 事件处理
                ├── MBX_Main()           ← 邮箱收/发
                ├── AL_ControlInd()      ← ESM 状态机 ★
                └── AL_ControlRes()      ← 挂起转换完成
      }

中断上下文 (DC/SM-Sync 模式):
  PDI_Isr()
    ├── PDO_OutputMapping() → APPL_OutputMapping(pData) ★
    ├── ECAT_Application()
    └── PDO_InputMapping()  → APPL_InputMapping(pData)  ★
```

**核心设计原则:**
- **双模式运行**: FreeRun (MainLoop 轮询) 和 Sync/DC (中断驱动)，由 `bEscIntEnabled` 切换
- **两层回调**: 栈内部函数 (SSC) + 用户回调 (`APPL_*`，由你的 `gateway_app.c` 实现)
- **ESC DPRAM**: 主站和从站 MCU 共享的双端口 RAM，通过 SyncManager 协调访问

---

## 2. 文件清单与职责

| 文件 | 行数 | 职责 | 修改者 |
|------|------|------|--------|
| `ecatslv.h` | 468 | ESC 寄存器宏、SM 常量、AL 状态码、全局变量声明 | Beckhoff |
| `ecatslv.c` | ~2800 | ESC 操作 + **ESM 状态机** + SM 管理 + DC/看门狗 | Beckhoff |
| `ecatappl.h` | 136 | 位域宏、EEPROM/运行标志变量声明 | Beckhoff |
| `ecatappl.c` | ~1210 | **PDO 映射** + **PDI ISR** + **MainLoop 调度** | Beckhoff |
| `ecatcoe.c` | ~500 | **CoE 邮箱协议**: SDO 请求/响应路由 | Beckhoff |
| `mailbox.c` | ~1000 | **邮箱收发底层**: SM0/SM1 读写、分段重组 | Beckhoff |
| `objdef.c` | ~1800 | **对象字典读写**: OBJ_Read/Write、权限检查 | Beckhoff |
| `sdoserv.c` | ~1200 | **SDO 服务端**: Expedited/Segmented/Complete Access | Beckhoff |
| `coeappl.c` | ~1000 | **CoE 应用层**: PDO 分配初始化、Sync 参数 (0x1C3x) | Beckhoff |
| `digital_io.c` | ~300 | **用户应用骨架**: 状态转换回调 + PDO 遍历 | **用户** |
| `digital_ioObjects.h` | ~270 | **用户 PDO 对象字典** + 映射结构体 | **用户手写** |
| `applInterface.h` | 188 | `APPL_*` 回调函数指针声明 + `MainInit/MainLoop` 原型 | Beckhoff |
| `ecat_def.h` | 1084 | 全局配置 (PDO 大小、同步参数、设备信息、类型定义) | Beckhoff |
| `esc.h` | — | ESC 寄存器地址宏 (0x0100, 0x0130, 0x0220...) | Beckhoff |

**你的项目中，`digital_io.c` 已被替换为 [`gateway_app.c`](..\firmware\ecat_tsn_gateway\application\gateway_app.c)，`digital_ioObjects.h` 已改为 MIT 电机 PDO。**

---

## 3. 启动流程: MainInit

[ecatappl.c:731](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L731)

```c
UINT16 MainInit(void)
{
    UINT16 Error = 0;

    // ① 重置所有应用层函数指针 = NULL
    pAPPL_EEPROM_Read  = NULL;
    pAPPL_EEPROM_Write = NULL;
    pAPPL_EEPROM_Reload = NULL;
    pAPPL_EEPROM_Store  = NULL;
    pAPPL_MainLoop = NULL;
    pAPPL_CoeReadInd = NULL;
    pAPPL_CoeWriteInd = NULL;

    // ② 核心初始化
    ECAT_Init();

    // ③ 对象字典初始化
    COE_ObjInit();

    bInitFinished = TRUE;

    // ④ 测量系统时间读取延迟 (循环 1000 次取最小值)
    // 存入 u32SystemTimeReadFailure 用于后续 cycle time 计算

    // ⑤ 读 ESC 寄存器 0x502.6 确定 EEPROM 读宽度
    HW_EscReadDWord(TmpData, ESC_EEPROM_CONFIG_OFFSET);
    EepromReadSize = ((TmpData & 0x40) > 0) ? 8 : 4;

    return Error;
}
```

### ECAT_Init() 详解

[ecatslv.c:2555](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatslv.c#L2555)

```c
void ECAT_Init(void)
{
    // ① 读 ESC 寄存器 0x0004 (通信信息)
    //    bits 11-8: SM 通道数 (HPM6E80 = 8)
    //    bits 31-16: DPRAM 大小 (KB)
    HW_EscReadDWord(TmpVar, ESC_COMM_INFO_OFFSET);
    nMaxSyncMan = (UINT8)((TmpVar & 0x0F00) >> 8);
    nMaxEscAddress = (UINT16)(((TmpVar & 0xFFFF0000) >> 16) << 10) + 0xFFF;

    // ② 读 0x0502: 检查 EEPROM 加载状态
    UpdateEEPROMLoadedState();
    // CRC Error 或 Load Error → EepromLoaded = FALSE

    // ③ 禁用全部 SM 通道
    for (i = 0; i < nMaxSyncMan; i++)
        DisableSyncManChannel(i);
    // DisableSyncManChannel: 写 0 到 SM 激活字节 → 轮询确认

    // ④ 邮箱初始化
    MBX_Init();

    // ⑤ 全局变量初始化
    bApplEsmPending = FALSE;
    bEcatOutputUpdateRunning = FALSE;
    bDcSyncActive = FALSE;
    // ...

    // ⑥ 设置 AL Status = INIT (0x01)
    nAlStatus = STATE_INIT;
    SetALStatus(nAlStatus, 0);
    // SetALStatus 写 ESC 寄存器 0x0130 (AL Status) 和 0x0134 (AL Status Code)
    // 同时设置 Run LED = OFF, Error LED = OFF

    // ⑦ CoE 初始化 (SDO 服务端、Sync 参数 0x1C32/0x1C33)
    COE_Init();

    // ⑧ 清空 AL Event Mask (0x0204) = 0
    ResetALEventMask(0);
}
```

**启动后状态:** 从站处于 INIT，所有 SM 禁用，等待主站发送 AL Control 命令。

---

## 4. 主循环: MainLoop

[ecatappl.c:840](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L840)

```c
void MainLoop(void)
{
    if (bInitFinished == FALSE) return;  // 初始化保护

    // ═══════════════════════════════════════════
    // FreeRun 模式: 轮询触发
    // 条件: 中断未使能 OR 首次输出未收到 OR DC 未激活
    // ═══════════════════════════════════════════
    if ((!bEscIntEnabled || !bEcatFirstOutputsReceived) && !bDcSyncActive)
    {
        if (!bEscIntEnabled)  // FreeRun: 主动轮询
        {
            UINT16 ALEvent = HW_GetALEventRegister();  // 读 ESC 0x0220
            ALEvent = SWAPWORD(ALEvent);

            if (ALEvent & PROCESS_OUTPUT_EVENT)  // bit9 = SM2 事件
            {
                bEcatFirstOutputsReceived = TRUE;
                if (bEcatOutputUpdateRunning)  // 从站在 OP?
                {
                    PDO_OutputMapping();  // ★ SM2 数据 → APPL_OutputMapping
                }
            }
            else if (nPdOutputSize == 0)  // 纯输入设备
            {
                if (ALEvent & PROCESS_INPUT_EVENT)
                    bEcatFirstOutputsReceived = TRUE;
            }
        }

        DISABLE_ESC_INT();
        ECAT_Application();  // → APPL_Application()

        if (bEcatInputUpdateRunning && nPdInputSize > 0)
            PDO_InputMapping();  // ★ APPL_InputMapping → SM3 数据

        ENABLE_ESC_INT();
    }

    // ═══════════════════════════════════════════
    // 1ms 定时器 → 看门狗 + DC 检查 + EEPROM 存储
    // ═══════════════════════════════════════════
    {
        UINT32 CurTimer = (UINT32)HW_GetTimer();
        if (CurTimer >= ECAT_TIMER_INC_P_MS)
        {
            ECAT_CheckTimer();
            HW_ClearTimer();
        }
    }

    // DC 时间戳溢出检查
    if (u32CheckForDcOverrunCnt >= CHECK_DC_OVERRUN_IN_MS)
        COE_SyncTimeStamp();

    // ═══════════════════════════════════════════
    // 核心事件处理 (状态机 + 邮箱)
    // ═══════════════════════════════════════════
    ECAT_Main();

    // CoE 后台 + 错误检查
    COE_Main();
    CheckIfEcatError();  // SM 看门狗 + DC 同步错误检测

    // 用户注册的 MainLoop 回调 (通常不用)
    if (pAPPL_MainLoop != NULL)
        pAPPL_MainLoop();
}
```

**三种运行模式对比:**

| 模式 | `bEscIntEnabled` | `bDcSyncActive` | PDO 触发位置 |
|------|:---:|:---:|------|
| FreeRun | FALSE | FALSE | MainLoop 轮询 `PROCESS_OUTPUT_EVENT` |
| SM-Sync | TRUE | FALSE | `PDI_Isr()` ISR 中 `PROCESS_OUTPUT_EVENT` |
| DC-Sync | TRUE | TRUE | `Sync0_Isr()` / `Sync1_Isr()` ISR |

---

## 5. ECAT_Main: 事件轮询与状态机

[ecatslv.c:2633](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatslv.c#L2633)

```c
void ECAT_Main(void)
{
    UINT16 ALEventReg;
    UINT16 EscAlControl = 0;

    // ① 邮箱后台 (被挂起的 SDO 响应重试等)
    MBX_Main();

    if (bMbxRunning)
    {
        // 读 SM1 激活状态 (0x080C-0x080F)
        HW_EscReadDWord(sm1Activate, 0x080C);
        sm1Status = sm1Activate;
    }

    // ② 读 AL Event Register (ESC 0x0220)
    ALEventReg = HW_GetALEventRegister();
    ALEventReg = SWAPWORD(ALEventReg);

    // ── EEPROM 命令挂起 (bit5) ──
    if (ALEventReg & EEPROM_CMD_PENDING)
        EEPROM_CommandHandler();  // 处理读/写/重载

    // ── AL Control 事件 (bit0): 主站写了 0x0120 ──
    if ((ALEventReg & AL_CONTROL_EVENT) && !bEcatWaitForAlControlRes)
    {
        HW_EscReadDWord(tmpVal, ESC_AL_CONTROL_OFFSET);  // 读 0x0120
        EscAlControl = (UINT16)SWAPDWORD(tmpVal);

        // Explicit Device ID (0x0120 bit5)
        if ((EscAlControl & STATE_DEVID) == STATE_DEVID)
        {
            if (!bExplicitDevIdRequested)
                u16IdValue = APPL_GetDeviceID();
            bExplicitDevIdRequested = TRUE;
        }

        ALEventReg &= ~(AL_CONTROL_EVENT | SM_CHANGE_EVENT);
        AL_ControlInd((UINT8)EscAlControl, 0);  // ★ 核心状态机入口
    }

    // ── SM Change 事件 (bit4): 主站写了 SM 配置 ──
    if ((ALEventReg & SM_CHANGE_EVENT) && !bEcatWaitForAlControlRes
        && 无错误 && 非INIT)
    {
        ALEventReg &= ~SM_CHANGE_EVENT;
        AL_ControlInd(nAlStatus & STATE_MASK, 0);
    }

    // ③ 处理挂起的状态转换
    if (bEcatWaitForAlControlRes)
        AL_ControlRes();

    // ④ 邮箱事件 (SM1 读取 + 重复请求 + SM0 写入)
    if (bMbxRunning)
    {
        // SM1 读事件: 主站读取了从站邮箱
        if (!sm1BufferFull && bSendMbxIsFull)
        {
            HW_EscWriteDWord(0, u16EscAddrSendMbx);  // 确认事件
            ALEventReg &= ~MAILBOX_READ_EVENT;
            MBX_MailboxReadInd();  // 放入下一条待发数据
        }

        // 重复请求 (Repeat toggle bit)
        if (repeat_bit_toggled)
            MBX_MailboxRepeatReq();  // 重发上一次响应

        // SM0 写事件: 主站写入了邮箱
        if (ALEventReg & MAILBOX_WRITE_EVENT)
        {
            ALEventReg &= ~MAILBOX_WRITE_EVENT;
            MBX_CheckAndCopyMailbox();  // 读取并处理邮箱数据
        }
    }
}
```

**AL Event Register (0x0220) 位定义:**

| 位 | 宏 | 含义 |
|:--|------|------|
| 0 | `AL_CONTROL_EVENT` | 主站写了 AL Control (0x0120) |
| 2 | `SYNC0_EVENT` | DC Sync0 信号产生 |
| 3 | `SYNC1_EVENT` | DC Sync1 信号产生 |
| 4 | `SM_CHANGE_EVENT` | SM 配置寄存器被写入 |
| 5 | `EEPROM_CMD_PENDING` | EEPROM 命令待处理 |
| 8 | `MAILBOX_WRITE_EVENT` | SM0 邮箱被主站写入 |
| 9 | `MAILBOX_READ_EVENT` | SM1 邮箱被主站读取 |
| 10 | `PROCESS_OUTPUT_EVENT` | SM2 PDO 输出被写入 |
| 11 | `PROCESS_INPUT_EVENT` | SM3 PDO 输入被读取 |

---

## 6. AL_ControlInd: 状态机核心

[ecatslv.c:1517-1988](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatslv.c#L1517)

这是整个 SSC 最核心的函数 (~470 行)，处理所有 EtherCAT 状态转换。

### 6.1 入口与错误确认机制

```c
void AL_ControlInd(UINT8 alControl, UINT16 alStatusCode)
{
    UINT16 result = 0;
    UINT8  bErrAck = 0;
    UINT8  stateTrans;

    // 停超时、标记待确认
    EsmTimeoutCounter = -1;
    bApplEsmPending = TRUE;

    // ── 错误确认判断 ──
    if (alControl & STATE_CHANGE)  // bit4=1: 主站确认错误
    {
        bErrAck = 1;
        nAlStatus &= ~STATE_CHANGE;  // 清除错误标记
    }
    else if ((nAlStatus & STATE_CHANGE)         // 当前有错误
          && (alControl & STATE_MASK) != STATE_INIT)
    {
        return;  // 有未确认错误 → 拒绝任何非 INIT 请求
    }
    else
    {
        nAlStatus &= STATE_MASK;
    }

    // ── 构造 stateTrans: high-nibble=旧状态, low-nibble=新状态 ──
    alControl &= STATE_MASK;
    stateTrans = nAlStatus;
    stateTrans <<= 4;
    stateTrans += alControl;
```

`stateTrans` 是一个编码值，例如 `SAFEOP(0x04) → OP(0x08)` = `0x48`。

### 6.2 SM 参数验证

```c
    // ── 根据目标状态级别检查 SM ──
    switch (stateTrans)
    {
    case INIT_2_PREOP:
    case OP_2_PREOP:
    case SAFEOP_2_PREOP:
    case PREOP_2_PREOP:
        // PREOP: 只检查邮箱 SM0/SM1
        result = CheckSmSettings(MAILBOX_READ + 1);  // = 2
        break;

    case PREOP_2_SAFEOP:
        // ★ 先调用 APPL_GenerateMapping 重新计算 PDO 大小
        result = APPL_GenerateMapping(&nPdInputSize, &nPdOutputSize);
        if (result != 0) break;
        // fall-through: 继续检查全部 SM
    case SAFEOP_2_OP:
    case OP_2_SAFEOP:
    case SAFEOP_2_SAFEOP:
    case OP_2_OP:
        // SAFEOP/OP: 检查所有 SM 通道
        result = CheckSmSettings(nMaxSyncMan);
        break;
    }
```

**CheckSmSettings()** 逐 SM 验证:
- SM0/SM1 (邮箱): 使能、方向、1-buffer 模式、size 0x24~0x80、地址 0x1000~0x2FFF
- SM2 (PDO 输出): size 匹配 `nPdOutputSize`、方向=写、看门狗触发位
- SM3 (PDO 输入): size 匹配 `nPdInputSize`、方向=读
- **地址重叠检测**: PDO 区域不能与邮箱区域重叠

### 6.3 状态转换执行

```c
    if (result == 0)
    {
        nEcatStateTrans = 0;
        switch (stateTrans)
        {
        // ═══ INIT → PREOP ═══
        case INIT_2_PREOP:
            UpdateEEPROMLoadedState();
            if (!EepromLoaded) { result = ALSTATUSCODE_EE_ERROR; break; }

            result = MBX_StartMailboxHandler();   // 初始化邮箱 SM0/SM1
            if (result == 0) {
                bApplEsmPending = FALSE;
                result = APPL_StartMailboxHandler();  // ← 用户回调
                if (result == 0) bMbxRunning = TRUE;
            }
            break;

        // ═══ PREOP → SAFEOP ═══
        case PREOP_2_SAFEOP:
            result = StartInputHandler();  // SM2/3 配置 + DC 检测 + 看门狗
            if (result == 0) {
                bApplEsmPending = FALSE;
                result = APPL_StartInputHandler(&u16ALEventMask);  // ← 用户回调
                if (result == 0) {
                    SetALEventMask(u16ALEventMask);  // 使能 ESC 中断
                    bEcatInputUpdateRunning = TRUE;
                }
            }
            break;

        // ═══ SAFEOP → OP ═══
        case SAFEOP_2_OP:
            if (bErrAck) {
                // 错误恢复: 重新使能 SM2
                if (nPdOutputSize > 0) EnableSyncManChannel(PROCESS_DATA_OUT);
            }

            result = StartOutputHandler();  // 默认返回 NOERROR_INWORK
            if (result == 0) {
                bApplEsmPending = FALSE;
                result = APPL_StartOutputHandler();  // ← 用户回调
                if (result == 0) bEcatOutputUpdateRunning = TRUE;
            }
            break;

        // ═══ OP → SAFEOP ═══
        case OP_2_SAFEOP:
            APPL_StopOutputHandler();  // ← 用户回调
            StopOutputHandler();       // 禁用 SM2 输出
            bApplEsmPending = FALSE;
            break;

        // ═══ OP→PREOP→INIT 级联回退 ═══
        case OP_2_PREOP:
            APPL_StopOutputHandler();
            StopOutputHandler();
            stateTrans = SAFEOP_2_PREOP;  // 继续回退
        case SAFEOP_2_PREOP:
            APPL_StopInputHandler();
            StopInputHandler();  // 禁用 SM2/3，清 AL Event Mask
            bApplEsmPending = FALSE;
            break;

        // ═══ 同状态 = 错误确认 ═══
        case PREOP_2_PREOP: case SAFEOP_2_SAFEOP: case OP_2_OP:
            if (bErrAck) APPL_AckErrorInd(stateTrans);  // ← 用户回调
            result = NOERROR_NOSTATECHANGE;  // 0xFE
            break;
        }
    }
```

### 6.4 状态转换结果处理

```c
    // ── NOERROR_INWORK (0xFF): 挂起等待 ──
    if (result == NOERROR_INWORK)
    {
        bEcatWaitForAlControlRes = TRUE;
        nEcatStateTrans = stateTrans;

        // 设置超时
        switch (nEcatStateTrans) {
            case INIT_2_PREOP:   EsmTimeoutCounter = PREOPTIMEOUT;    break; // 2000ms
            case PREOP_2_SAFEOP:
            case SAFEOP_2_OP:    EsmTimeoutCounter = SAFEOP2OPTIMEOUT; break; // 9000ms
            default:             EsmTimeoutCounter = 200;              break;
        }
        EsmTimeoutCounter -= EsmTimeoutCounter / 10;  // 减 10% 提前量
    }
    // ── 状态实际改变 ──
    else if (alControl != (nAlStatus & STATE_MASK))
    {
        if (result != 0) {
            // 失败 → 设错误标记
            nAlStatus |= STATE_CHANGE;
            if (bEcatOutputUpdateRunning) {  // 从 OP 退出时禁用 SM2
                APPL_StopOutputHandler();
                StopOutputHandler();
                DisableSyncManChannel(PROCESS_DATA_OUT);
            }
        } else {
            nAlStatus = alControl;  // 成功 → 更新状态
        }
        bEcatWaitForAlControlRes = FALSE;
        SetALStatus(nAlStatus, result);  // 写 ESC 0x0130 + LED
    }
    // ── 错误确认无状态变化 ──
    else {
        bEcatWaitForAlControlRes = FALSE;
        SetALStatus(nAlStatus, 0);  // 清 AL Status Code
    }
}
```

### 6.5 ESM 状态转换图

```
       ┌── AL_Control(0x01) ───► INIT (0x01) ◄── 超时/错误 ───┐
       │                                                        │
  ┌────┴────┐                                                   │
  │  INIT   │── AL_Control(0x02) ──► PREOP (0x02)               │
  └─────────┘   CheckSm + Mailbox Start                         │
                                           │                    │
                              AL_Control(0x04)                  │
                              StartInputHandler                 │
                              DC/Sync 配置                       │
                                           │                    │
                                       ┌───┴────┐              │
                                       │ SAFEOP │◄──────────────┘
                                       │ (0x04) │  看门狗/DC错误
                                       └───┬────┘
                                           │
                              AL_Control(0x08)
                              StartOutputHandler
                              (等待首次 SM2 事件)
                                           │
                                       ┌───┴────┐
                                       │   OP   │
                                       │ (0x08) │
                                       └────────┘
```

### 6.6 StartInputHandler: SM2/SM3 配置 + 同步决策

[ecatslv.c:695-1256](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatslv.c#L695)

```c
UINT16 StartInputHandler(void)
{
    // ① 读 SM2/SM3 的 DPRAM 地址
    nEscAddrOutputData = SM2 物理地址;
    nEscAddrInputData  = SM3 物理地址;

    // ② 地址重叠检查: PDO 区 vs 邮箱区 vs PDO 互斥
    if (SM3区 与 SM0/SM1 重叠) return ALSTATUSCODE_INVALIDSMINCFG;
    if (SM2区 与 SM0/SM1/SM3 重叠) return ALSTATUSCODE_INVALIDSMOUTCFG;

    // ③ DC 寄存器分析 (0x0980-0x0983)
    //    确定同步类型: FreeRun / SM-Sync / DC-Sync0 / DC-Sync1
    //    设置 bDcSyncActive, bEscIntEnabled, u16ALEventMask

    // ④ SM/Sync0 时序参数计算
    //    子周期: LatchInputSync0Value, u16SmSync0Value

    // ⑤ 看门狗配置 (ESC 0x0400/0x0420)
    //    EcatWdValue = 超时时间 (ms)

    // ⑥ 使能 SM2/SM3
    EnableSyncManChannel(PROCESS_DATA_OUT);  // SM2
    EnableSyncManChannel(PROCESS_DATA_IN);   // SM3

    // ⑦ 初始化输入 PDO (写入初始值)
    PDO_InputMapping();

    return ALSTATUSCODE_NOERROR;
}
```

---

## 7. PDO 数据搬运

### 7.1 PDI_Isr: DC/SM-Sync 中断入口

[ecatappl.c:482](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L482)

```c
void PDI_Isr(void)
{
    BOOL SyncAcknowledgePending = FALSE;
    UINT16 ALEvent = HW_GetALEventRegister_Isr();  // 读 ESC 0x0220
    ALEvent = SWAPWORD(ALEvent);

    // ── Sync1 事件 ──
    if (ALEvent & SYNC1_EVENT) {
        Sync1_Isr();
        SyncAcknowledgePending = TRUE;
    }

    if (bEscIntEnabled)
    {
        // ── SM2 输出事件: 主站写入了 PDO 数据 ──
        if (ALEvent & PROCESS_OUTPUT_EVENT)
        {
            if (bDcRunning && bDcSyncActive)
                u16SmSync0Counter = 0;  // 重置 SM/Sync0 序列计数器

            if (sSyncManOutPar.u16SmEventMissedCounter > 0)
                sSyncManOutPar.u16SmEventMissedCounter--;

            bEcatFirstOutputsReceived = TRUE;

            if (bEcatOutputUpdateRunning)  // OP 状态?
                PDO_OutputMapping();
            else
                // 不在 OP: 只读事件, 不处理数据
                HW_EscReadDWordIsr(u32dummy, nEscAddrOutputData);
        }

        // ── SM-Sync 模式: 用 SM 事件触发应用逻辑 ──
        if (sSyncManOutPar.u16SyncType == SYNCTYPE_SM_SYNCHRON)
            ECAT_Application();

        // ── PDO 输入映射 ──
        if (bEcatInputUpdateRunning && nPdInputSize > 0
            && (输入同步类型 = SM_SYNCHRON 或 SM2_SYNCHRON))
        {
            PDO_InputMapping();
        }

        // ── 检测 cycle exceed ──
        ALEvent = HW_GetALEventRegister_Isr();
        if (ALEvent & PROCESS_OUTPUT_EVENT)  // 本次处理期间新事件又来了
        {
            sSyncManOutPar.u16CycleExceededCounter++;
            HW_EscReadDWordIsr(u32dummy, nEscAddrOutputData);  // 确认
        }
    }

    // ── Sync0 事件 ──
    if (ALEvent & SYNC0_EVENT) {
        Sync0_Isr();
        SyncAcknowledgePending = TRUE;
    }

    // ── 确认 Sync 事件 (读 DC Sync Status 0x0981) ──
    if (SyncAcknowledgePending) {
        volatile UINT32 SyncState = 0;
        HW_EscReadDWord(SyncState, ESC_DC_SYNC_STATUS);
    }

    COE_UpdateSyncErrorStatus();
}
```

### 7.2 Sync0_Isr: DC 同步下的应用触发

[ecatappl.c:599](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L599)

```c
void Sync0_Isr(void)
{
    Sync0WdCounter = 0;  // 重置 Sync0 看门狗

    if (bDcSyncActive)
    {
        // ── Latch 机制: 确定当前 Sync0 是否需要锁存输入 ──
        BOOL bCallInputMapping = FALSE;
        if (bEcatInputUpdateRunning && LatchInputSync0Value > 0 && nPdInputSize > 0)
        {
            if (LatchInputSync0Value > LatchInputSync0Counter)
                LatchInputSync0Counter++;
            if (LatchInputSync0Value == LatchInputSync0Counter)
                bCallInputMapping = TRUE;
        }

        // ── SM/Sync0 序列检查 ──
        if (u16SmSync0Value > 0)
        {
            if (u16SmSync0Counter > u16SmSync0Value)
                sSyncManOutPar.u16SmEventMissedCounter += 3;

            if (nPdOutputSize == 0 && nPdInputSize > 0)
                // 输入专用: 检查主站是否读了输入
                if (!(ALEvent & PROCESS_INPUT_EVENT)) u16SmSync0Counter++;
            else
                u16SmSync0Counter++;
        }

        // ── 触发应用逻辑 ──
        ECAT_Application();  // → APPL_Application()

        // ── 指定 Sync0 上锁存输入 ──
        if (bCallInputMapping) {
            PDO_InputMapping();
            if (LatchInputSync0Value == 1)
                LatchInputSync0Counter = 0;  // 每 Sync0 都锁存
        }
    }
}
```

### 7.3 PDO_OutputMapping 和 PDO_InputMapping

[ecatappl.c:229](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L229) & [ecatappl.c:299](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\ecatappl.c#L299)

```c
// ★ PDO 输出: 主站 → 从站
void PDO_OutputMapping(void)
{
    // 从 ESC DPRAM 读 SM2 数据 → aPdOutputData[]
    HW_EscReadIsr((MEM_ADDR *)aPdOutputData, nEscAddrOutputData, nPdOutputSize);
    // 调用用户回调
    APPL_OutputMapping((UINT16 *)aPdOutputData);
    // 例: 你的代码解析 16 轴 × 5 float = 320 bytes
}

// ★ PDO 输入: 从站 → 主站
void PDO_InputMapping(void)
{
    // 用户回调填充 aPdInputData[]
    APPL_InputMapping((UINT16 *)aPdInputData);
    // 写入 ESC DPRAM SM3 区域, 主站自动读取
    HW_EscWriteIsr((MEM_ADDR *)aPdInputData, nEscAddrInputData, nPdInputSize);
}
```

### 7.4 完整数据流

```
PDO 输出 (主站→从站):
  主站 EtherCAT 帧
    → ESC 硬件写 DPRAM SM2 区
    → ESC 置 PROCESS_OUTPUT_EVENT
    → PDI_Isr() / MainLoop() 检测
    → PDO_OutputMapping()
    → HW_EscReadIsr(): DPRAM → aPdOutputData[]
    → APPL_OutputMapping(pData)  ★ 你的 MIT 命令解析

PDO 输入 (从站→主站):
  APPL_InputMapping(pData)  ★ 你的 MIT 反馈填充
    → HW_EscWriteIsr(): aPdInputData[] → DPRAM SM3 区
    → 下一个 EtherCAT 帧主站自动读取 SM3
```

---

## 8. SyncManager 与同步模式

### 8.1 SM 硬件寄存器布局

每个 SM (0-7) 占用 8 bytes:

```
Offset        Bits      内容
0x0800+8N     [15:0]    物理起始地址
              [31:16]   长度 (bytes)
0x0804+8N     [7:0]     控制字节 (方向+模式)
              [15:8]    状态字节
              [23:16]   激活字节 (bit0=使能)
              [31:24]   PDI 控制
```

### 8.2 分配表

| SM | 名称 | 方向 | 典型配置 |
|----|------|------|---------|
| 0 | MBoxOut | 主→从 | 0x1000, 128B, 1-buf |
| 1 | MBoxIn | 从→主 | 0x1080, 128B, 1-buf |
| 2 | PDO Output | 主→从 | 0x1100, 320B, 3-buf |
| 3 | PDO Input | 从→主 | 0x1240, 192B, 3-buf |
| 4-7 | — | — | 未使用 |

### 8.3 同步模式自动检测

`StartInputHandler()` 中的决策树:

```
读 DC 寄存器 0x0980-0x0983
  ├── DC 激活 (bit0/bit16)
  │   ├── Sync0 使能 → 检查周期范围 → SyncType = DCSYNC0
  │   ├── Sync0+Sync1 使能 → 子周期 → SyncType = DCSYNC1
  │   └── bDcSyncActive = TRUE
  │
  └── DC 未激活
      ├── AL_EVENT_ENABLED=1 → SM-Sync
      │   └── bEscIntEnabled = TRUE
      │
      └── 否则 → FreeRun
          └── 要求 3-buffer 模式
```

---

## 9. 对象字典: objdef + sdoserv

### 9.1 对象条目结构

[objdef.h](d:\Tools\hpm\sdk_env-main\gatewat\hpm_gateway\firmware\ecat_tsn_gateway\SSC\Src\objdef.h)

```c
typedef struct {
    TOBJECT *pParent;           // 父对象 (NULL=顶层)
    TOBJECT *pChild;            // 子对象
    UINT16  Index;              // 0x1000-0xFFFF
    TYPEDESC Type;              // 高8位=类别码, 低16位=bit长度
    TSDOINFOENTRYDESC *pEntryDesc; // 条目描述数组
    UCHAR   *pName;             // 名称字符串
    void    *pVarPtr;           // → 实际数据
    UINT16 (*Read)(...);        // 自定义读 (可NULL)
    UINT16 (*Write)(...);       // 自定义写 (可NULL)
    UINT16  Access;             // 访问标志
} TOBJECT;
```

### 9.2 OBJ_Read / OBJ_Write

```c
UINT8 OBJ_Read(UINT16 Index, UINT8 SubIdx, UINT32 Size,
               UINT16 *pData, UINT8 bCompleteAccess)
{
    TOBJECT *pObj = OBJ_GetObjectHandle(Index);  // 查找对象
    if (!pObj) return ABORTIDX_OBJECT_NOT_EXISTING;

    if (!(pObj->Access & ACCESS_READ))  // 权限检查
        return ABORTIDX_UNSUPPORTED_ACCESS;

    if (pObj->Read != NULL)  // 自定义读函数
        return pObj->Read(Index, SubIdx, Size, pData, bCA);

    // 默认: 直接从 pObj->pVarPtr 拷贝数据
    UINT32 objSize = OBJ_GetObjectSize(Index, bCA);
    MEMCPY(pData, pObj->pVarPtr, objSize);
    return 0;
}
```

### 9.3 SDO 请求流程

```
主站 SDO 请求 (CoE Mailbox)
  → ECAT_Main → MBX_CheckAndCopyMailbox
  → MBX_MailboxWriteInd (mailbox.c)
  → ECAT_CoEIndication (ecatcoe.c)
  → 解析 CoE 头部 (SDO Req / Info)
  → SDOS_SdoUploadReq / SDOS_SdoDownloadReq (sdoserv.c)
  → OBJ_Read / OBJ_Write (objdef.c)
  → 响应 → MBX_MailboxSendReq → SM1 → 主站读取
```

---

## 10. 邮箱协议: mailbox.c + ecatcoe.c

### 10.1 邮箱帧结构

```
Mailbox Header (6 bytes):
  [0-1]  数据长度 (不含头部)
  [2-3]  地址 (固定 0x0000)
  [4]    通道 (0=AoE, 2=EoE, 3=CoE, 4=FoE, 5=SoE)
  [5]    优先级 + 类型 + 错误标志

CoE Header (紧跟 Mailbox Header, 可变长度):
  [0-1]  服务编号 (1=SDO Req, 3=SDO Info, ...)
  [2-5]  标志 + 数据长度
```

### 10.2 邮箱收发流水线

```c
// 接收:
MBX_CheckAndCopyMailbox()
  1. 读 SM0 首字节确认事件
  2. 检查 SM0 缓冲区有效性
  3. 复制到本地 buffer
  4. 解析通道 → 转发到 CoE/FoE/SoE 处理器
  5. 处理完成 → 准备响应

// 发送:
MBX_MailboxSendReq()
  1. 组装响应数据到 SM1 缓冲区
  2. 设置 SM1 状态 → 告诉主站有新数据
  3. 等待主站读取 (MAILBOX_READ_EVENT)
```

---

## 11. 完整调用链图示

### 11.1 启动序列

```
main()
  ├── ecat_hardware_init()     ESC + PHY
  ├── MainInit()
  │     ├── ECAT_Init()
  │     │     ├── 读 0x0004 → nMaxSyncMan, nMaxEscAddress
  │     │     ├── UpdateEEPROMLoadedState()
  │     │     ├── DisableSyncManChannel(ALL)
  │     │     ├── MBX_Init()
  │     │     ├── SetALStatus(INIT, 0)
  │     │     └── COE_Init()
  │     ├── COE_ObjInit()
  │     └── 测量系统时间读取延迟
  ├── pAPPL_EEPROM_* = ...     EEPROM 仿真注册
  ├── APPL_GenerateMapping()   计算 PDO 大小
  └── bRunApplication = TRUE
```

### 11.2 DC-Sync 模式完整运行时序

```
──── 中断上下文 ────

硬件 Sync0 脉冲 + EtherCAT 帧到达
  │
PDI_Isr()
  ├── PROCESS_OUTPUT_EVENT → PDO_OutputMapping()
  │     ├── HW_EscReadIsr()           DPRAM SM2 → 本地
  │     └── APPL_OutputMapping()      你的 MIT 命令解析
  │
  ├── SYNC0_EVENT → Sync0_Isr()
  │     ├── Sync0WdCounter = 0
  │     ├── SM/Sync0 序列检查
  │     ├── ECAT_Application() → APPL_Application()
  │     └── PDO_InputMapping() (Latch)
  │           ├── APPL_InputMapping()  你的 MIT 反馈
  │           └── HW_EscWriteIsr()     本地 → DPRAM SM3
  │
  └── Sync 确认 (读 DC Sync Status)

──── 主循环上下文 ────

MainLoop()  ← 与中断并行运行
  ├── (DC 模式跳过 PDO 轮询块)
  ├── ECAT_CheckTimer()         1ms 看门狗
  ├── ECAT_Main()
  │     ├── MBX_Main()
  │     ├── AL_ControlRes()     挂起状态转换处理
  │     └── MBX 事件处理
  ├── COE_Main()
  └── CheckIfEcatError()        SM/DC 错误检测
```

---

## 12. 用户应用层接口

### 12.1 必须实现的回调

| 函数 | 调用时机 | 返回值含义 |
|------|---------|-----------|
| `APPL_StartMailboxHandler` | INIT→PREOP | 0=OK |
| `APPL_StopMailboxHandler` | PREOP→INIT | 0=OK |
| **`APPL_StartInputHandler`** | PREOP→SAFEOP | 0=OK; 此处可做硬件初始化 |
| `APPL_StopInputHandler` | SAFEOP→PREOP | 0=OK |
| **`APPL_StartOutputHandler`** | SAFEOP→OP | 0=OK |
| `APPL_StopOutputHandler` | OP→SAFEOP | 0=OK |
| **`APPL_OutputMapping`** | 每次 SM2 事件 | — |
| **`APPL_InputMapping`** | 每次 SM3 更新前 | — |
| **`APPL_GenerateMapping`** | PREOP→SAFEOP | 计算 InputSize/OutputSize |
| `APPL_Application` | 每周期 | — |
| `APPL_AckErrorInd` | 错误确认 | — |
| `APPL_GetDeviceID` | Explicit ID | 设备 ID |

### 12.2 16 轴 MIT 控制的 PDO 回调

```c
// 320 bytes output (master → slave)
void APPL_OutputMapping(UINT16 *pData)
{
    for (int axis = 0; axis < 16; axis++) {
        float *f = (float *)((uint8_t *)pData + axis * 20);
        // f[0]=target_pos, f[1]=target_vel, f[2]=ff_torque, f[3]=kp, f[4]=kd
        // 后续: 发送 CAN 帧
    }
}

// 192 bytes input (slave → master)
void APPL_InputMapping(UINT16 *pData)
{
    for (int axis = 0; axis < 16; axis++) {
        float *f = (float *)((uint8_t *)pData + axis * 12);
        f[0] = motor[axis].actual_position;
        f[1] = motor[axis].actual_velocity;
        f[2] = motor[axis].actual_torque;
    }
}
```

### 12.3 回调调用顺序

以 PREOP→SAFEOP 为例:

```
1. CheckSmSettings()           ← SSC (SM 参数验证)
2. StartInputHandler()         ← SSC (DC/Sync 配置)
3. APPL_StartInputHandler()    ← 用户 (额外硬件初始化)
4. SetALEventMask()            ← SSC (使能 ESC 中断)
5. bEcatInputUpdateRunning=TRUE ← SSC
```

SSC 先做自己的工作，再调用你的 `APPL_*`。返回 0 继续，返回 !0 拒绝转换。

### 12.4 SSC 全局对象字典接口

```c
TOBJECT *OBJ_GetObjectHandle(UINT16 Index);          // 按索引查找
UINT8 OBJ_Read(UINT16 Index, SubIdx, Size, pData, CA); // SDO 读
UINT8 OBJ_Write(UINT16 Index, SubIdx, Size, pData, CA);// SDO 写
UINT32 OBJ_GetObjectSize(UINT16 Index, BOOL CA);      // 获取对象大小
void COE_ObjInit(void);                                // 对象字典初始化
```

### 12.5 SSC 状态机接口

```c
UINT16 MainInit(void);           // 栈初始化 (由 main.c 调用一次)
void MainLoop(void);             // 主循环 (由 main.c 周期调用)
void ECAT_StateChange(UINT8 state, UINT16 code);  // 应用驱动状态转换
void ECAT_Init(void);            // ESC 硬件初始化
void ECAT_Main(void);            // 邮箱+状态机处理
void PDI_Isr(void);              // PDI 中断服务 (由硬件 ISR 调用)
void DC_CheckWatchdog(void);     // DC 看门狗 (由 ECAT_CheckTimer 调用)
```

---

## 13. 关键全局变量速查表

| 变量 | 含义 | 值域 |
|------|------|------|
| `nAlStatus` | 当前 AL 状态 | 1=INIT, 2=PREOP, 4=SAFEOP, 8=OP |
| `bEscIntEnabled` | 中断使能 | TRUE=Sync/DC |
| `bDcSyncActive` | DC 同步激活 | TRUE=DC-Sync |
| `bEcatOutputUpdateRunning` | OP 标志 | TRUE=处理 SM2 输出 |
| `bEcatInputUpdateRunning` | SAFEOP/OP 标志 | TRUE=更新 SM3 输入 |
| `bEcatFirstOutputsReceived` | 首次 SM2 事件 | TRUE=已收到 |
| `bEcatWaitForAlControlRes` | 挂起转换 | TRUE=等待完成 |
| `bApplEsmPending` | 应用层待确认 | TRUE=需 APPL_* 回调 |
| `nEcatStateTrans` | 当前转换编码 | hi-nibble=旧, lo-nibble=新 |
| `EsmTimeoutCounter` | ESM 超时倒计时 | -1=禁用, 0=超时 |
| `EcatWdValue` | SM 看门狗 (ms) | 0=禁用 |
| `nPdInputSize` | PDO 输入大小 | 192 (16轴×12B) |
| `nPdOutputSize` | PDO 输出大小 | 320 (16轴×20B) |
| `nEscAddrOutputData` | SM2 DPRAM 基址 | 如 0x1100 |
| `nEscAddrInputData` | SM3 DPRAM 基址 | 如 0x1240 |
| `nMaxSyncMan` | ESC SM 通道数 | 8 (HPM6E80) |
| `bRunApplication` | 主循环标志 | TRUE=运行 |
| `Sync0WdCounter` | Sync0 看门狗计数器 | 每 ms +1 |
| `Sync0WdValue` | Sync0 看门狗阈值 | 0=禁用 |
