<!-- Author: BearKing -->
# MIT 型 16 轴电机控制器 EtherCAT PDO 设计

## 1. 架构概览

```
RDK X5 (EtherCAT Master)
  │
  │  EtherCAT frame (每周期)
  │  ┌─────────────────────────────────────────────────────┐
  │  │ RxPDO (主站→从站): Motor0 cmd | Motor1 cmd | ... | Motor15 cmd │  320 bytes
  │  │ TxPDO (从站→主站): Motor0 fb  | Motor1 fb  | ... | Motor15 fb  │  192 bytes
  │  └─────────────────────────────────────────────────────┘
  ▼
HPM6E80 (EtherCAT Slave + CAN Master)
  │
  │  固件: APPL_OutputMapping 解析 16 轴命令
  │         APPL_InputMapping  聚合 16 轴反馈
  │
  ▼
CAN FD 总线 × N → MIT 协议电机驱动板 × 16
```

**设计原则:**
- 所有数值使用 **REAL32 (float32)**，保持精度一致
- 使用 SSC v5.13 Slot 机制实现 16 轴自动偏移
- PDO 字段与 MIT CAN 协议帧一对一对应，固件实现零拷贝传递
- 不使用 CiA 402 状态机，主站直接下发 MIT 型指令

---

## 2. 单轴 PDO 布局

### 2.1 命令通道 (RxPDO: 主站 → 从站)

| 对象索引 (Axis N) | 名称 | 类型 | 位宽 | 字节 | 含义 |
|-------------------|------|------|------|------|------|
| `0x2000 + N×0x100` | Target Position | REAL32 | 32 | 4 | 期望位置 (rad / m) |
| `0x2001 + N×0x100` | Target Velocity | REAL32 | 32 | 4 | 期望速度 (rad/s / m/s) |
| `0x2002 + N×0x100` | FF Torque | REAL32 | 32 | 4 | 前馈力矩 (Nm) |
| `0x2003 + N×0x100` | KP | REAL32 | 32 | 4 | 位置刚度 (Nm/rad or Nm/m) |
| `0x2004 + N×0x100` | KD | REAL32 | 32 | 4 | 速度阻尼 (Nm/(rad/s) or Nm/(m/s)) |
| **单轴合计** | | | **160** | **20** | |

### 2.2 反馈通道 (TxPDO: 从站 → 主站)

| 对象索引 (Axis N) | 名称 | 类型 | 位宽 | 字节 | 含义 |
|-------------------|------|------|------|------|------|
| `0x3000 + N×0x100` | Actual Position | REAL32 | 32 | 4 | 实际位置 (rad / m) |
| `0x3001 + N×0x100` | Actual Velocity | REAL32 | 32 | 4 | 实际速度 (rad/s / m/s) |
| `0x3002 + N×0x100` | Actual Torque | REAL32 | 32 | 4 | 实际力矩 (Nm) |
| **单轴合计** | | | **96** | **12** | |

### 2.3 每轴总计

```
命令: 5 × float32 = 20 bytes
反馈: 3 × float32 = 12 bytes
───────────────────────────
每轴: 32 bytes (PDO 双向)
```

---

## 3. 16 轴对象字典完整分配

### 3.1 命令对象 (0x2000 ~ 0x2FFF)

```
Motor 0:  0x2000  0x2001  0x2002  0x2003  0x2004
Motor 1:  0x2100  0x2101  0x2102  0x2103  0x2104
Motor 2:  0x2200  0x2201  0x2202  0x2203  0x2204
Motor 3:  0x2300  0x2301  0x2302  0x2303  0x2304
Motor 4:  0x2400  0x2401  0x2402  0x2403  0x2404
Motor 5:  0x2500  0x2501  0x2502  0x2503  0x2504
Motor 6:  0x2600  0x2601  0x2602  0x2603  0x2604
Motor 7:  0x2700  0x2701  0x2702  0x2703  0x2704
Motor 8:  0x2800  0x2801  0x2802  0x2803  0x2804
Motor 9:  0x2900  0x2901  0x2902  0x2903  0x2904
Motor 10: 0x2A00  0x2A01  0x2A02  0x2A03  0x2A04
Motor 11: 0x2B00  0x2B01  0x2B02  0x2B03  0x2B04
Motor 12: 0x2C00  0x2C01  0x2C02  0x2C03  0x2C04
Motor 13: 0x2D00  0x2D01  0x2D02  0x2D03  0x2D04
Motor 14: 0x2E00  0x2E01  0x2E02  0x2E03  0x2E04
Motor 15: 0x2F00  0x2F01  0x2F02  0x2F03  0x2F04
```

### 3.2 反馈对象 (0x3000 ~ 0x3FFF)

```
Motor 0:  0x3000  0x3001  0x3002
Motor 1:  0x3100  0x3101  0x3102
Motor 2:  0x3200  0x3201  0x3202
Motor 3:  0x3300  0x3301  0x3302
Motor 4:  0x3400  0x3401  0x3402
Motor 5:  0x3500  0x3501  0x3502
Motor 6:  0x3600  0x3601  0x3602
Motor 7:  0x3700  0x3701  0x3702
Motor 8:  0x3800  0x3801  0x3802
Motor 9:  0x3900  0x3901  0x3902
Motor 10: 0x3A00  0x3A01  0x3A02
Motor 11: 0x3B00  0x3B01  0x3B02
Motor 12: 0x3C00  0x3C01  0x3C02
Motor 13: 0x3D00  0x3D01  0x3D02
Motor 14: 0x3E00  0x3E01  0x3E02
Motor 15: 0x3F00  0x3F01  0x3F02
```

### 3.3 PDO Mapping 对象

```
RxPDO Mapping:
  Motor 0:  0x1600  (5 entries × 32bit = 160 bits)
  Motor 1:  0x1610
  ...
  Motor 15: 0x16F0

TxPDO Mapping:
  Motor 0:  0x1A00  (3 entries × 32bit = 96 bits)
  Motor 1:  0x1A10
  ...
  Motor 15: 0x1AF0
```

---

## 4. 总 PDO 尺寸

```
RxPDO (SM2, 输出): 16 motors × 20 bytes = 320 bytes = 0x140
TxPDO (SM3, 输入): 16 motors × 12 bytes = 192 bytes = 0xC0
────────────────────────────────────────────────────────
总计 PDO 数据:      512 bytes

EtherCAT 帧 payload 限制: ~1486 bytes (1500 - Ethernet/IP/UDP headers)
  → 512 bytes << 1486 bytes ✓ 绰绰有余
```

---

## 5. 配置参数

### 5.1 ecat_def.h

```c
#define MAX_PD_INPUT_SIZE   0xC0   // 192 bytes (TxPDO, slave -> master)
#define MAX_PD_OUTPUT_SIZE  0x140  // 320 bytes (RxPDO, master -> slave)
#define CiA402_SAMPLE_APPLICATION  0  // 不使用 CiA 402
#define DEVICE_NAME          "mit_motor_16axis"
#define PRODUCT_CODE         0x00000002
#define DEVICE_PROFILE_TYPE  0x00000000  // 无标准 profile
```

### 5.2 SSC Tool Slot 配置

```xml
<Slots SlotPdoIncrement="16" SlotIndexIncrement="#x100">
    <Slot MinInstances="1" MaxInstances="1"><Name>Motor 0</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 1</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 2</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 3</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 4</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 5</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 6</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 7</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 8</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 9</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 10</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 11</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 12</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 13</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 14</Name></Slot>
    <Slot MinInstances="0" MaxInstances="1"><Name>Motor 15</Name></Slot>
</Slots>
```

### 5.3 SyncManager (ESC DPRAM) 布局

```
SM0 (Mailbox Out):  0x1000, size 0x80
SM1 (Mailbox In):   0x1080, size 0x80
SM2 (PDO Output):   0x1100, size 0x140  (RxPDO = 从站接收 = 320 bytes)
SM3 (PDO Input):    0x1240, size 0xC0   (TxPDO = 从站发送 = 192 bytes)
```

---

## 6. 固件数据结构 (HPM6E80)

### 6.1 强类型结构体

```c
// gateway_app.c

/** MIT 单轴命令 (20 bytes, packed) */
typedef struct __attribute__((packed)) {
    float target_position;   // 期望位置  (rad 或 m)
    float target_velocity;   // 期望速度  (rad/s 或 m/s)
    float ff_torque;         // 前馈力矩  (Nm)
    float kp;                // 位置刚度  (Nm/rad 或 Nm/m)
    float kd;                // 速度阻尼  (Nm/(rad/s) 或 Nm/(m/s))
} mit_command_t;

/** MIT 单轴反馈 (12 bytes, packed) */
typedef struct __attribute__((packed)) {
    float actual_position;   // 实际位置  (rad 或 m)
    float actual_velocity;   // 实际速度  (rad/s 或 m/s)
    float actual_torque;     // 实际力矩  (Nm)
} mit_feedback_t;

/* 16 轴指令 + 反馈缓存 */
static mit_command_t  g_commands[NUM_MOTORS];   // 16 × 20 = 320 bytes
static mit_feedback_t g_feedback[NUM_MOTORS];   // 16 × 12 = 192 bytes
```

### 6.2 PDO 回调

```c
#define NUM_MOTORS 16
#define MIT_CMD_BYTES  20
#define MIT_FB_BYTES   12

void APPL_OutputMapping(UINT16 *pData)
{
    for (int i = 0; i < NUM_MOTORS; i++) {
        mit_command_t *cmd = (mit_command_t *)((uint8_t *)pData + i * MIT_CMD_BYTES);
        g_commands[i] = *cmd;
        // 后续: 通过 CAN/CAN FD 发送 cmd 到电机 i
    }
}

void APPL_InputMapping(UINT16 *pData)
{
    for (int i = 0; i < NUM_MOTORS; i++) {
        mit_feedback_t *fb = (mit_feedback_t *)((uint8_t *)pData + i * MIT_FB_BYTES);
        *fb = g_feedback[i];
    }
}
```

### 6.3 PDO 数据在内存中的布局

```
RxPDO buffer (320 bytes, ESC DPRAM → pData):
┌──────────┬──────────┬──────────┬─────┬───────────┐
│ Motor 0  │ Motor 1  │ Motor 2  │ ... │ Motor 15  │
│ 20 bytes │ 20 bytes │ 20 bytes │     │ 20 bytes  │
└──────────┴──────────┴──────────┴─────┴───────────┘
  offset=0   offset=20  offset=40        offset=300

TxPDO buffer (192 bytes, pData → ESC DPRAM):
┌──────────┬──────────┬──────────┬─────┬───────────┐
│ Motor 0  │ Motor 1  │ Motor 2  │ ... │ Motor 15  │
│ 12 bytes │ 12 bytes │ 12 bytes │     │ 12 bytes  │
└──────────┴──────────┴──────────┴─────┴───────────┘
  offset=0   offset=12  offset=24        offset=180
```

---

## 7. TwinCAT / CODESYS 集成

### 7.1 主站扫描结果

```
Device: mit_motor_16axis
  ├─ Box 1: Motor 0  (MIT Motor, 20/12 bytes)
  ├─ Box 2: Motor 1  (MIT Motor, 20/12 bytes)
  ├─ Box 3: Motor 2  (MIT Motor, 20/12 bytes)
  ├─ ...
  └─ Box 16: Motor 15 (MIT Motor, 20/12 bytes)
```

### 7.2 主站侧 Process Data 变量名

```
Motor 0:
  TxPDO (Output to slave):   Target Position 0  (REAL32)
                             Target Velocity 0  (REAL32)
                             FF Torque 0        (REAL32)
                             KP 0               (REAL32)
                             KD 0               (REAL32)
  RxPDO (Input from slave):  Actual Position 0  (REAL32)
                             Actual Velocity 0  (REAL32)
                             Actual Torque 0    (REAL32)
...
Motor 15:
  ... 同上
```

### 7.3 控制流程 (TwinCAT PLC 示例)

```
每个周期:
  FOR i := 0 TO 15 DO
    // MIT 控制律
    motor[i].ff_torque := desired_torque[i];
    motor[i].target_position := traj_pos[i];
    motor[i].target_velocity := traj_vel[i];
    motor[i].kp := stiffness[i];
    motor[i].kd := damping[i];

    // 读取反馈
    actual_pos[i]  := motor[i].actual_position;
    actual_vel[i]  := motor[i].actual_velocity;
    actual_trq[i]  := motor[i].actual_torque;
  END_FOR
```

---

## 8. SSC Tool 操作步骤摘要

1. **打开现有配置** `HPM_ECAT_IO_Config.xml`
2. **修改参数**: DEVICE_NAME, PRODUCT_CODE, MAX_PD_*_SIZE
3. **禁用 CiA 402**: `CiA402_SAMPLE_APPLICATION = 0`
4. **配置 Modular Device Profile**: Index distance = 0x100, 16 slots
5. **创建 MIT Motor Module**: 5 command entries + 3 feedback entries
6. **分配 16 Slots**: Motor 0 mandatory, Motor 1~15 optional
7. **生成代码**: `mit_motor.c` / `mit_motor.h` / `mit_motorObjects.h`
8. **Patch + 集成**: 参考 SDK `ssc_cia402.patch` 的硬件适配

---

## 9. 与现有 gateway 项目的共存

TSN 通道 (`gateway_bridge.c`) 和 MIT 电机控制互不影响：

```
ECAT PDO (SM2/SM3) ──── MIT 电机命令/反馈
ECAT Mailbox (SM0/SM1) ──── CoE SDO 配置 (如需要)
TSN DMA / Ethernet    ──── Jetson 独立数据通道
```

两者在 `main.c` 的主循环中共存:

```c
while (bRunApplication == TRUE) {
    MainLoop();              // ECAT: PDO + 状态机 (SSC)
    gateway_bridge_poll();   // TSN: ECAT↔Ethernet 转发
    mit_can_poll();          // CAN: 电机反馈收集 (未来添加)
}
```

---

## 10. 配置速查表

| 参数 | 值 |
|------|-----|
| 电机数量 | 16 |
| 控制协议 | MIT 型 (位置+速度+力矩前馈+KP+KD) |
| 数据类型 | REAL32 (float) |
| 每轴命令字节 | 20 |
| 每轴反馈字节 | 12 |
| RxPDO 总大小 | 320 bytes (0x140) |
| TxPDO 总大小 | 192 bytes (0xC0) |
| 命令对象索引范围 | 0x2000 ~ 0x2FFF |
| 反馈对象索引范围 | 0x3000 ~ 0x3FFF |
| 轴索引用距 (SlotIndexIncrement) | 0x100 |
| PDO 索引间距 (SlotPdoIncrement) | 16 |
| SSC 版本 | 5.13 |
| 设备 profile | 0x0000 (自定义) |
| CiA 402 | 禁用 |
| ESC DPRAM | 60 KB (绰绰有余) |
| DC 同步 | 支持 |
