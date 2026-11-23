<!-- Author: BearKing -->
# RS06 电机 CAN 协议完整参考

> 来源: RS06 User Manual (260428 版) §4-§6
> 电机: RobStride 06 准直驱关节电机 | 48VDC | 力矩常数 1.09 Nm/Arms | 减速比 9:1
> CAN 接口: CAN 2.0, 1Mbps, 29-bit 扩展帧 (私有协议) / 11-bit 标准帧 (MIT/CANopen)

---

## 1. 关键参数

| 参数 | 值 |
|------|------|
| 额定电压 | 48V DC (范围 15-60V) |
| 额定力矩 | 11 Nm |
| 峰值力矩 | 36 Nm |
| 额定电流 (相峰值) | 14.3 Apk |
| 最大电流 (相峰值) | 57.0 Apk |
| 空载转速 | 480 rpm |
| CAN 波特率 | **1 Mbps** |
| 编码器分辨率 | 14-bit 绝对值 |
| 重量 | 621g |
| 极数 | 28 |
| 相数 | 3 |

---

## 2. CAN 帧格式总览

### 私有协议（29-bit 扩展帧）

```
┌─────────────────────────────────────────────────────────────┐
│                    29-bit CAN ID                            │
├──────────────┬───────────────────────┬──────────────────────┤
│ bits [28:24] │     bits [23:8]       │    bits [7:0]        │
│ 通信类型(5b) │     数据区2 (16b)     │  目标电机 CAN_ID      │
├──────────────┼───────────────────────┼──────────────────────┤
│  0x00~0x19   │ 与通信类型相关的数据   │  1~127 (0x7F)        │
└──────────────┴───────────────────────┴──────────────────────┘
                         │
          ┌──────────────┴──────────────┐
          │     8-byte Data Field       │
          │   Byte0 ~ Byte7             │
          │   与通信类型相关的载荷数据    │
          └─────────────────────────────┘
```

### MIT 协议（11-bit 标准帧）

```
┌──────────────────────────────────────┐
│          11-bit CAN ID               │
├──────────────┬───────────────────────┤
│ bits [10:8]  │    bits [7:0]         │
│ 模式类型 (3b) │  目标电机 ID          │
├──────────────┼───────────────────────┤
│ 0=MotionCtrl │  1~127                │
│ 1=Position   │                       │
│ 2=Velocity   │                       │
│ 3=Config     │                       │
└──────────────┴───────────────────────┘
```

### CANopen（11-bit 标准帧, 符合 CiA 301/402）

```
标准 CANopen COB-ID 编码:
  NMT:      0x000
  Sync:     0x080
  Emergency: 0x080 + NodeID
  TxPDO1:   0x180 + NodeID
  RxPDO1:   0x200 + NodeID
  SDO(tx):  0x580 + NodeID
  SDO(rx):  0x600 + NodeID
  Heartbeat: 0x700 + NodeID
```

---

## 3. 私有协议 — 通信类型详解

### 3.1 通信类型速查表

| 类型 | 方向 | 功能 | 是否自动执行 |
|------|------|------|:---:|
| 0x00 | 查询 | 获取设备 ID + MCU UID | ✅ |
| 0x01 | 控制 | ⭐ **运控模式 (MIT 参数)** | ✅ |
| 0x02 | 反馈 | 电机状态反馈 (位置/速度/力矩/温度) | ✅ (接收) |
| 0x03 | 命令 | 电机使能运行 | ✅ |
| 0x04 | 命令 | 电机停止/清错 | ✅ |
| 0x06 | 命令 | 设置机械零点 | ⚠️ 预留 |
| 0x07 | 命令 | 修改 CAN ID (立即生效) | ⚠️ 预留 |
| 0x11 (17) | 查询 | 读单个参数 | ✅ |
| 0x12 (18) | 命令 | 写单个参数 (掉电丢失) | ✅ |
| 0x15 (21) | 反馈 | 故障反馈帧 | ✅ (自动) |
| 0x16 (22) | 命令 | 参数保存到 Flash | ⚠️ 手动触发 |
| 0x17 (23) | 命令 | 修改波特率 (重新上电) | 🚫 禁止 |
| 0x18 (24) | 命令 | 主动上报开关 | ⚙️ 可选 |
| 0x19 (25) | 命令 | **切换协议** (0=私有/1=CANopen/2=MIT) | 🚫 禁止 |

---

### 3.2 通信类型 0: 获取设备 ID

```
请求帧:
  29-bit ID:
    [28:24] = 0x00
    [23:8]  = bit15-8: 主机 CAN_ID
    [7:0]   = 目标电机 CAN_ID
  Data[0:7] = 全 0

响应帧:
  29-bit ID:
    [28:24] = 0x00
    [23:8]  = 目标电机 CAN_ID
    [7:0]   = 0xFE (广播响应)
  Data[0:7] = 64-bit MCU 唯一标识符
```

### 3.3 ⭐ 通信类型 1: 运控模式 (MIT 核心)

**这是最重要的控制帧。** 控制公式:

```
t_ref = Kd × (v_set - v_actual) + Kp × (p_set - p_actual) + t_ff
```

```
请求帧:
  29-bit ID:
    [28:24] = 0x01
    [23:8]  = Byte2: 前馈力矩 t_ff
    [7:0]   = 目标电机 CAN_ID
  Data[0:7]:
    Byte0-1: 目标角度 p_des   [0~65535] ↔ (-4π ~ +4π) rad       uint16
    Byte2-3: 目标角速度 v_des  [0~65535] ↔ (-50 ~ +50) rad/s     uint16
    Byte4-5: Kp              [0~65535] ↔ (0.0 ~ 5000.0)          uint16
    Byte6-7: Kd              [0~65535] ↔ (0.0 ~ 100.0)           uint16

    注: 29-bit ID 的 bits[23:8] = 前馈力矩 t_ff 的 uint16 值
        力矩范围: [0~65535] ↔ (-36 ~ +36) Nm

  编码规则: 高字节在前 (Big-Endian)

响应: 通信类型 2 反馈帧
```

### 3.4 ⭐ 通信类型 2: 电机反馈数据

```
响应帧 (电机主动发送或应答):
  29-bit ID:
    [28:24] = 0x02
    [23:8]:
      bit8-15: 当前电机 CAN_ID
      bit16-21: 故障信息 (0=无故障, 非0=有)
        bit16: 欠压故障 (<12V)
        bit17: 三相过流故障
        bit18: 过温故障 (>145°C)
        bit19: 磁编码器故障
        bit20: 堵转过载故障
        bit21: 编码器未校准
      bit22-23: 模式状态
        0 = Reset mode (复位)
        1 = Cali mode (校准中)
        2 = Motor mode (运行)
    [7:0] = 主机 CAN_ID

  Data[0:7]:
    Byte0-1: 当前角度 [0~65535] ↔ (-4π ~ +4π) rad     uint16
    Byte2-3: 当前角速度 [0~65535] ↔ (-50 ~ +50) rad/s  uint16
    Byte4-5: 当前力矩 [0~65535] ↔ (-36 ~ +36) Nm       uint16
    Byte6-7: 绕组温度 Temp(°C) × 10                      uint16

    编码: 高字节在前 (Big-Endian)
```

### 3.5 通信类型 3: 电机使能

```
请求帧:
  29-bit ID:
    [28:24] = 0x03
    [23:8]  = bit15-8: 主机 CAN_ID
    [7:0]   = 目标电机 CAN_ID
  Data[0:7] = 全 0

响应: 通信类型 2 反馈
```

### 3.6 通信类型 4: 停止 / 清错 / 读版本号

```
停止运行:
  Data[0] = 0x00  (正常停止)
  
清错:
  Data[0] = 0x01  (清除故障)

读版本号:
  Data[0] = 0x00
  Data[1] = 0xC4
  → 响应: Type2反馈, Data[0]=0x00, Data[1]=0xC4, Data[2]=0x56
          Data[3-6] = 电机版本号 (高→低)
```

### 3.7 通信类型 17: 读单个参数

```
请求:
  29-bit ID: [28:24]=0x11, bits[23:8]: 主机 CAN_ID
  Data[0-1] = 参数 Index (低字节在前) ← 见 §3.11 参数表
  Data[2-3] = 0x00
  Data[4-7] = 0x00

响应:
  29-bit ID: [28:24]=0x11
    bits[23:16]=0x00 (成功) / 0x01 (失败)
    bits[15:8]=主机 CAN_ID
  Data[0-1] = Index
  Data[2-3] = 0x00
  Data[4-7] = 参数值 (float32, 低字节在前)
```

### 3.8 通信类型 18: 写单个参数 (掉电丢失)

```
请求:
  29-bit ID: [28:24]=0x12, bits[15:8]: 主机 CAN_ID
  Data[0-1] = 参数 Index (低字节在前)
  Data[2-3] = 0x00
  Data[4-7] = 参数值 (float32, 低字节在前)

响应: 通信类型 2 反馈
```

**注意:** 此方式写入的参数掉电丢失。用**通信类型 22** 保存到 Flash。

### 3.9 通信类型 21: 故障反馈帧 (主动上报)

```
主动或查询的故障帧:
  Data[0-3] = 故障值 (0=正常)
    bit0:  电机过温 (>145°C)
    bit1:  驱动芯片故障
    bit2:  欠压 (<12V)
    bit3:  过压 (>60V)
    bit4:  B相电流过流
    bit5:  C相电流过流
    bit7:  编码器未校准
    bit8:  硬件识别故障
    bit9:  位置初始化故障
    bit14: 堵转过载保护
    bit16: A相电流过流
  
  Data[4-7] = 警告值
    bit0: 过温警告 (默认 135°C)
```

### 3.10 通信类型 25: 协议切换 (重新上电生效)

```
请求:
  Data[6] = F_CMD:
    0 = 私有协议 (默认)
    1 = CANopen
    2 = MIT 协议

  此命令执行后需重新上电才能生效。
```

### 3.11 写单个参数列表 (通信类型 18 可用)

| Index | 参数名 | 类型 | 范围 | 默认值 | r/w |
|-------|--------|------|------|--------|-----|
| `0x7005` | run_mode | uint8 | 0=运控 1=PP 2=速度 3=电流 5=CSP | — | W/R |
| `0x7006` | iq_ref | float32 | -57~57 A | — | W/R |
| `0x700A` | spd_ref | float32 | -50~50 rad/s | — | W/R |
| `0x700B` | limit_torque | float32 | 0~36 Nm | — | W/R |
| `0x7010` | cur_kp | float32 | — | 0.17 | W/R |
| `0x7011` | cur_ki | float32 | — | 0.012 | W/R |
| `0x7014` | cur_filt_gain | float32 | 0~1.0 | 0.1 | W/R |
| `0x7016` | loc_ref | float32 | rad | — | W/R |
| `0x7017` | limit_spd | float32 | 0~50 rad/s | — | W/R |
| `0x7018` | limit_cur | float32 | 0~57 A | — | W/R |
| `0x7019` | mechPos | float32 | rad | — | R |
| `0x701A` | iqf | float32 | -57~57 A | — | R |
| `0x701B` | mechVel | float32 | -50~50 rad/s | — | R |
| `0x701C` | VBUS | float32 | V | — | R |
| `0x701E` | loc_kp | float32 | — | 40 | W/R |
| `0x701F` | spd_kp | float32 | — | 6 | W/R |
| `0x7020` | spd_ki | float32 | — | 0.02 | W/R |
| `0x7021` | spd_filt_gain | float32 | — | 0.1 | W/R |
| `0x7022` | acc_rad | float32 | rad/s² | 20 | W/R |
| `0x7024` | vel_max | float32 | rad/s | 10 | W/R |
| `0x7025` | acc_set | float32 | rad/s² | 10 | W/R |
| `0x7026` | EPScan_time | uint16 | 1=10ms | 1 | W/R |
| `0x7028` | canTimeout | uint32 | 20000=1s | 0 | W/R |
| `0x7029` | zero_sta | uint8 | 0=±2π, 1=±π | 0 | W/R |
| `0x702A` | damper | uint8 | 阻尼开关 | 0 | W/R |
| `0x702B` | add_offset | float32 | 零点偏移 | 0 | W/R |
| `0x702C` | alveolous_open | uint8 | 齿槽补偿 | 0 | W/R |
| `0x702D` | iq_test | uint8 | 校准开关 | 0 | W/R |
| `0x702E` | dcc_set | float32 | PP减速 | 10 | W/R |

---

## 4. MIT 协议详解 (RS06)

### 4.1 CAN 帧格式 (11-bit 标准帧)

```
┌──────────────────────────────────────────────────────────────┐
│ 11-bit CAN ID                                                │
├─────────────┬────────────────────────────────────────────────┤
│ bits [10:8] │               bits [7:0]                       │
│ 命令/模式型  │             目标电机 CAN_ID                    │
├─────────────┼────────────────────────────────────────────────┤
│ 0=MotorCtrl │                                                │
│ 1=Position  │           1 ~ 127 (= 0x01 ~ 0x7F)             │
│ 2=Velocity  │                                                │
│ 3=Config    │                                                │
└─────────────┴────────────────────────────────────────────────┘
                          │
           ┌──────────────┴──────────────┐
           │     8-byte Data Field       │
           │     Byte0 ~ Byte7           │
           └─────────────────────────────┘
```

### 4.2 控制命令表

| 命令 | 方向 | Data[0:7] (8 bytes) | 说明 |
|------|------|---------------------|------|
| **使能** | 主机→电机 | `FF FF FF FF FF FF FF FC` | 电机进入运控模式 |
| **停止** | 主机→电机 | `FF FF FF FF FF FF FF FD` | 停止运控 |
| **MIT 运控** ⭐ | 主机→电机 | 见 §4.3 | 发送 KP/KD/pos/vel/trq |
| **设零点** | 主机→电机 | `FF FF FF FF FF FF FF FE` | 非位置模式下 |
| **清错** | 主机→电机 | Byte7 = `0xFF` | 清除当前故障 |
| **读故障** | 主机→电机 | Byte7 = 非`0xFF` | 返回故障值 |
| **设模式** | 主机→电机 | Byte7 = `0`(MIT)/`1`(位置)/`2`(速度) | 运控模式切换 |
| **改 CAN ID** | 主机→电机 | Byte7 = 新 CAN_ID | 立即生效 |
| **切协议** | 主机→电机 | Byte7 = `0`(私有)/`1`(CANopen)/`2`(MIT) | 重新上电生效 |
| **改主机 ID** | 主机→电机 | Byte7 = 新 主机 CAN_ID | |

### 4.3 ⭐ MIT 运控指令 (命令 3, ID bits[10:8]=0)

**这是 RS06 MIT 协议的核心运控帧:**

```
CAN ID: bits[10:8] = 0 (运控模式)
        bits[7:0]  = 目标电机 ID

Data[0:7]:
  Byte0-1:  目标角度 p_des  [0~65535] ↔ (-12.57 ~ +12.57) rad   uint16
  Byte2 + Byte3[7:4]: 目标速度 v_des  [0~4095] ↔ (-50 ~ +50) rad/s   uint12
  Byte3[3:0] + Byte4: Kp            [0~4095] ↔ (0 ~ 5000)           uint12
  Byte5 + Byte6[7:4]: Kd            [0~4095] ↔ (0 ~ 100)            uint12
  Byte6[3:0] + Byte7: 前馈力矩 t_ff  [0~4095] ↔ (-36 ~ +36) Nm       uint12
```

**编码特点对比私有协议:**
- 角度范围: ±12.57 rad (vs 私有 ±4π ≈ ±12.57 — 相同!)
- 速度: uint12 (0~4095) vs 私有 uint16 (0~65535)
- KP: uint12 (0~4095) vs 私有 uint16 (0~65535)
- KD: uint12 (0~4095) vs 私有 uint16 (0~65535)
- 力矩: uint12 (0~4095) vs 私有 uint16 (0~65535)

### 4.4 反馈帧 (响应命令 1)

```
CAN ID: bits[10:0] = 主机 CAN_ID
  (电机回复时使用主机 ID 作为目标地址)

Data[0:7]:
  Byte0:    电机 CAN ID
  Byte1-2:  当前角度 [0~65535] ↔ (-12.57 ~ +12.57) rad     uint16
  Byte3 + Byte4[7:4]: 当前速度 [0~4095] ↔ (-50 ~ +50) rad/s uint12
  Byte4[3:0] + Byte5: 当前力矩 [0~4095] ↔ (-36 ~ +36) Nm    uint12
  Byte6-7:  绕组温度 (°C × 10)                               uint16
```

### 4.5 RS06 MIT 位置模式 (ID bits[10:8]=1)

```
CAN ID: bits[10:8] = 1, bits[7:0] = 目标 ID
Data[0:3] = 目标位置 (rad, float32)
Data[4:7] = 目标速度 (rad/s, float32)
```

### 4.6 RS06 MIT 速度模式 (ID bits[10:8]=2)

```
CAN ID: bits[10:8] = 2, bits[7:0] = 目标 ID
Data[0:3] = 目标速度 (rad/s, float32)
Data[4:7] = 限流值 (A, float32)
```

### 4.7 RS06 MIT 参数配置模式 (ID bits[10:8]=3)

```
命令 14: 读参数
  CAN ID: bits[10:8]=3, bits[7:0]=0x7F
  Data[0:1] = Index (同私有协议参数表)
  Data[2:3] = 0
  Data[4:7] = 0
  → 响应: 参数值 (float32/uint16)

命令 15: 写参数
  CAN ID: bits[10:8]=4, bits[7:0]=0x7F
  Data[0:1] = Index
  Data[2:3] = 0
  Data[4:7] = 参数值 (float32/uint16)
```

---

## 5. CANopen 协议 (RS06)

RS06 支持标准 CANopen DS402 驱动协议。切换到 CANopen 模式后，所有通信使用标准 11-bit COB-ID。

### 5.1 CANopen 对象速查

| Index | Name | Type | Access | Unit |
|-------|------|------|--------|------|
| `0x6040` | Controlword | UINT16 | RW | — |
| `0x6041` | Statusword | UINT16 | RO | — |
| `0x6060` | Modes of operation | INT8 | RW | — |
| `0x6061` | Modes of operation display | INT8 | RO | — |
| `0x6064` | Position actual value | INT32 | RO | pulses (16384/rev) |
| `0x6071` | Target torque | INT16 | RW | 0.1% (1000 = 11 Nm) |
| `0x6077` | Torque actual value | INT16 | RO | 0.1% |
| `0x607A` | Target position | INT32 | RW | pulses |
| `0x607D` | Software position limit | DINT[2] | RW | pulses |
| `0x6081` | Profile velocity | UINT32 | RW | 0.1 rpm |
| `0x6083` | Profile acceleration | UINT32 | RW | 0.1 rpm/s |
| `0x60FF` | Target velocity | INT32 | RW | 0.1 rpm |

### 5.2 状态机 (CiA 402)

```
上电: SWITCH_ON_DISABLED
  → Controlword = 6 → READY_TO_SWITCH_ON
  → Controlword = 7 → SWITCHED_ON
  → Controlword = 15 → OPERATION_ENABLE
  
正常停止: Controlword = 1 → SWITCH_ON_DISABLED
急停 (有电压尖峰风险!): Controlword = 11
故障清除: Controlword 写故障复位过渡
```

### 5.3 运行模式

| Mode | Modes_of_operation | 说明 |
|------|-------------------|------|
| Homing | 6 | 零位设定 (当前位置=0) |
| PP (Profile Position) | 1 | 插补位置模式 |
| CSP (Cyclic Sync Position) | 5 | 周期同步位置模式 |
| Velocity | 3 | 速度模式 |
| Torque | 4 | 力矩模式 |

### 5.4 模式切换约束

**必须在 SWITCH_ON_DISABLED 状态下切换模式。** 不能在 OPERATION_ENABLE 中切换。

### 5.5 CANopen 看门狗

- 对象: `0x6099:01`
- 值 = 20000 = 1s (计时单位: 50µs)
- 默认 = 0 (禁用)
- 超时 → 电机自动 Disable

---

## 6. 数值转换公式

### 6.1 私有协议 (uint16, 16-bit 精度)

```c
// 位置: uint16 ↔ rad
#define P_MIN   -12.57f    // -4π
#define P_MAX    12.57f    // +4π
uint16_t float_to_uint_pos(float x) {
    return (uint16_t)((x - P_MIN) * 65535.0f / (P_MAX - P_MIN));
}
float uint_to_float_pos(uint16_t x) {
    return P_MIN + (float)x * (P_MAX - P_MIN) / 65535.0f;
}

// 速度: uint16 ↔ rad/s
#define V_MIN  -50.0f
#define V_MAX   50.0f
uint16_t float_to_uint_vel(float x) {
    return (uint16_t)((x - V_MIN) * 65535.0f / (V_MAX - V_MIN));
}

// 力矩: uint16 ↔ Nm
#define T_MIN  -36.0f
#define T_MAX   36.0f

// KP: uint16 ↔ float
#define KP_MIN  0.0f
#define KP_MAX  5000.0f

// KD: uint16 ↔ float
#define KD_MIN  0.0f
#define KD_MAX  100.0f
```

### 6.2 MIT 协议 (uint12, 12-bit 精度)

```c
// 位置: MIT 使用 uint16 完整范围
#define MIT_P_MIN  -12.57f
#define MIT_P_MAX   12.57f

// 速度: uint12 (0~4095) → float
float mit_vel = V_MIN + (uint12_val) * (V_MAX - V_MIN) / 4095.0f;

// 力矩: uint12 (0~4095) → float
float mit_trq = T_MIN + (uint12_val) * (T_MAX - T_MIN) / 4095.0f;
```

---

## 7. 完整运控流程

### 私有协议最小控制序列

```
1. 通信类型 3: 使能电机
   → 等待通信类型 2 反馈 (确认 mode=2: Motor mode)

2. 循环发送:
   通信类型 1: 运控模式 (pos, vel, kp, kd, t_ff)
   → 接收通信类型 2 反馈 (pos_act, vel_act, trq_act, temp)

3. 停止: 通信类型 4 (Data[0]=0)
   → 或通信类型 4 (Data[0]=1): 清错
```

### MIT 协议最小控制序列

```
1. 命令 1: 使能 (Data=FF FF FF FF FF FF FF FC)
   → 接收响应命令 1 反馈

2. 循环发送:
   命令 3: MIT 运控 (pos, vel, kp, kd, t_ff)
   → 接收响应命令 1 反馈

3. 停止: 命令 2 (Data=FF FF FF FF FF FF FF FD)
```

---

## 8. 安全约束

| 约束 | 值 | 说明 |
|------|-----|------|
| 最大电压 | 60V | 超压 → 过压保护 |
| 最小电压 | 12V | 欠压 → 停止运行 |
| 过温保护 | 145°C (绕组) | → 故障反馈 bit0 |
| 过温警告 | 135°C | → 警告 |
| CAN 超时 | 可配置 (默认 0=禁用) | 20000 = 1s |
| 模式切换 | 必须在静止时 | 运行中切换 = 危险 |
| 协议切换 | 重新上电生效 | 发送命令后需断电重启 |
| Flash 写入 | 手动触发 (类型22) | 不要每个周期都写 |
