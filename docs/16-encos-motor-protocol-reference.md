<!-- Author: BearKing -->
# ENCOS 电机 CAN 协议完整参考

> 来源: ENCOS 调试技术手册 V1.19EAP
> CAN 接口: CAN 2.0, 1Mbps, 11-bit 标准帧 | 支持 CAN FD
> 电机类型: FOC 无刷电机 (EC-4310/8112/10020/13715/13720)

---

## 1. 关键参数

| 参数 | 值 |
|------|------|
| CAN 波特率 | **1 Mbps** (默认), 可切 CAN FD |
| 帧格式 | **11-bit 标准帧** |
| 最大控制频率 | **2000 Hz** (0.4ms) |
| CAN 超时保护 | 500ms (默认, 可配置) |
| 供电电压 | 20~57V DC (额定 24/48V) |
| 过温保护 | MOS 105°C, 电机 120°C (可配置) |

---

## 2. 控制公式

ENCOS 使用 MIT 型控制律:

```
motor_current = KP × (pos_desired - pos_current) + KD × (spd_desired - spd_current) + Torque / KT
```

默认参数:
- KP = 15
- KD = 0.5

---

## 3. CAN 帧格式总览

### 3.1 标准帧 (11-bit ID)

```
┌──────────────────────────────────────────────────────────────────┐
│                      11-bit CAN ID                               │
├───────────────┬──────────────────────────────────────────────────┤
│ bits [10:5]   │              bits [4:0]                          │
│ 取决于帧类型   │         电机 ID (Motor ID)                       │
├───────────────┴──────────────────────────────────────────────────┤
│ 控制帧: 命令类型 + 电机 ID                                       │
│ 查询帧: 0x04/0x06/0x07 等                                       │
│ 广播:   Motor ID = 0x1F (31)                                     │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 特殊 CAN ID

| CAN ID | 用途 | 帧格式 |
|--------|------|--------|
| `0x7FF` | 查询/设置 CAN ID / 重置 CAN ID / 刹车控制 | 广播/点对点 |
| Motor ID + 命令前缀 | 控制 + 反馈 | 点对点 |
| `Motor ID & 0x07` 相关 | 模式控制 | 点对点 |

---

## 4. 运控模式 (MIT 帧)

### 4.1 ⭐ 力位混控命令 (运控模式, MIT 核心帧)

```
CAN ID:
  bits[10:5] = 命令类型 (0x00 = 运控)
  bits[4:0]  = 电机 ID

Data[0:7] (8 bytes):
  ┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
  │Byte0 │Byte1 │Byte2 │Byte3 │Byte4 │Byte5 │Byte6 │Byte7 │
  └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
  ├─KP (12b)─┤├─KD (9b)─┤├──Pos (16b)──┤├──Vel (12b)──┤├─Trq(12b)─┤
  Byte0[7:5]: KP[11:8]     (高3位)
  Byte0[4:0] + Byte1[7:4]: KP[7:0]   (中8位)
  ────────────
  Byte1[3:0] + Byte2[7:3]: KD[8:0]   (9位)
  Byte2[2:0] + Byte3:      位置高11位
  ─ (实际编码更紧凑, 见下方详表)

  详细编码:
  Byte0[7:5] + Byte0[4:0] + Byte1[7:4]: KP (12 bits, 0~4095)
    ↕ KP = float_val / 500.0 * 4095
  Byte1[3:0] + Byte2[7:3]:              KD (9 bits, 0~511)
    ↕ KD = float_val / 5.0 * 511
  Byte2[2:0] + Byte3:                   位置 (11 bits + 继续)
  Byte3 + Byte4:                        位置 (16 bits, 0~65535)
    ↕ 位置 ↦ -12.5 ~ +12.5 rad
  Byte5[7:4] + Byte6:                   速度 (12 bits, 0~4095)
    ↕ 速度 ↦ -18.0 ~ +18.0 rad/s
  Byte7 + Byte5[3:0]:                   力矩 (12 bits, 0~4095)
    ↕ 力矩 ↦ -30.0 ~ +30.0 Nm
```

**编码范围:**

| 参数 | CAN 范围 | 物理范围 | 转换公式 |
|------|---------|---------|---------|
| KP | 0 ~ 4095 (uint12) | 0.0 ~ 500.0 | `kp_can = kp_float / 500.0 * 4095` |
| KD | 0 ~ 511 (uint9) | 0.0 ~ 5.0 | `kd_can = kd_float / 5.0 * 511` |
| 位置 | 0 ~ 65535 (uint16) | -12.5 ~ +12.5 rad | `p_can = (p_rad + 12.5) / 25.0 * 65535` |
| 速度 | 0 ~ 4095 (uint12) | -18.0 ~ +18.0 rad/s | `v_can = (v_rads + 18.0) / 36.0 * 4095` |
| 力矩 | 0 ~ 4095 (uint12) | -30.0 ~ +30.0 Nm | `t_can = (t_nm + 30.0) / 60.0 * 4095` |

### 4.2 反馈帧 (运控模式响应)

```
CAN ID:
  bits[10:5] = 0x01
  bits[4:0]  = 电机 ID

Data[0:7]:
  Byte0-3: 实际位置 (float32, rad)
  Byte4-5: 实际速度 (uint16, 0~32767 ↦ 0~3276.7 rpm, ÷10)
  Byte6[7:4]:  故障码 (uint2, 0~3)
      0 = 正常
      1 = 过流
      2 = 过温
      3 = 过压/欠压
  Byte6[3:0] + Byte7: 实际电流 (uint12, 0~4095 ↦ 0~409.5A, ÷10)
```

---

## 5. 伺服位置模式

### 5.1 位置命令帧

```
CAN ID:
  bits[10:5] = 0x02
  bits[4:0]  = 电机 ID

Data[0:7]:
  Byte0[7:5]: 命令类型 (0x02)
  Byte0[4:0]: 电机 ID
  Byte1-3:    保留 (0x00)
  Byte4-7:    目标位置 (float32, rad)
```

### 5.2 伺服速度模式

```
CAN ID:
  bits[10:5] = 0x03
  bits[4:0]  = 电机 ID

Data[0:7]:
  Byte0[7:5]: 命令类型 (0x03)
  Byte1[4:0]: 电机 ID
  Byte2-3:    保留 (0x00)
  Byte4-7:    目标速度 (float32, RPM)
  或:  Byte4-5: RPM (uint16, 0~65535 ↦ 0~6553.5 RPM)
       Byte6-7: 限流 (uint16, 0~65535 ↦ 0~6553.5A ÷10)
```

### 5.3 电流/力矩模式

```
CAN ID:
  bits[10:5] = 0x04
  bits[4:0]  = 电机 ID

Data:
  Byte0[7:5]: 命令类型 (0x04)
  模式选择:
    0 = 力矩模式
    1 = 电流 Iq
    2 = 电流 Id
    3 = 混合模式
  int16 目标值 (范围 -32768~32767):
    力矩模式: -327.68 ~ +327.67 Nm
    电流模式: -327.68 ~ +327.67 A
```

---

## 6. 配置命令 (CAN ID 0x06 前缀)

### 6.1 配置帧统一格式

```
请求帧:
  Byte0[7:5] = 0x06 (配置命令前缀)
  Byte0[4:0] = Motor ID
  Byte1      = 0xFF (广播标志, 或具体 ID)
  Byte2[4:0] = Motor ID
  Byte3      = 配置子命令 (0x01~0x11)
  Byte4~7    = 配置参数 (取决于子命令)

响应帧:
  Byte0[7:5] = 0x06
  Byte1      = 0xFE (响应标识)
  Byte3      = 配置子命令 (回显)
  Byte4~7    = 返回值
```

### 6.2 配置子命令表

| 子命令 | 功能 | 数据 | 范围 |
|--------|------|------|------|
| `0x01` | 加速度 | uint16 × 100 | 0~2000 = 0~20 rad/s² |
| `0x02` | 通信协议选择 | uint8 | 0=CAN / 1=CANFD / 2=CANopen (暂未启用) |
| `0x04` | 运控 PID | uint16 × 100 | — |
| `0x05` | MIT KP 范围 | uint16 | — |
| `0x06` | MIT KD 范围 | uint16 | — |
| `0x07` | POS MIN/MAX | int16 × 100 | 位置范围 |
| `0x08` | SPD MIN/MAX | int16 × 100 | -1800~1800 = -18~18 rad/s |
| `0x09` | TOR MIN/MAX | int16 × 10 | -900~900 = -90~90 Nm |
| `0x0A` | CUR MIN/MAX | int16 × 10 | -900~900 = -90~90 A |
| `0x0B` | CAN 超时 | uint16 | ms (默认 500ms, 0=禁用) |
| `0x0C` | 电流 PI: KP + KI | KP×10000, KI×10 | — |
| `0x0D` | 速度 PI: KP + KI | KP×100000, KI×100000 | — |
| `0x0E` | 位置 PD: KP + KD | KP×100000, KD×100000 | — |
| `0x0F` | Kt 模式 (力矩系数) | uint8 mode | 0/1/2 |
| `0x10` | Kt 值 (力矩系数) | uint12 × 4 | Kt = raw / 100 |
| `0x11` | 零位校准 | uint8 | — |

### 6.3 配置示例

```c
// 设置 CAN 超时 = 500ms
// Request: C0 0B 01 F4 xx xx xx xx
//   Byte0[7:5]=6, Byte0[4:0]=MotorID
//   Byte3=0x0B (子命令)
//   Byte4-5=0x01F4=500 (uint16)

// 设置加速度 = 20 rad/s²
// Request: C0 01 07 D0 xx xx xx xx
//   Byte4-5=0x07D0=2000 → 2000/100 = 20 rad/s²

// 设置 SPD MIN=-18 rad/s, MAX=18 rad/s  
// Request: C0 08 F8 F8 07 08 xx xx
//   MIN=-1800 = 0xF8F8 (int16)
//   MAX=1800  = 0x0708 (int16)

// 设置电流 PI: KP=0.01, KI=50
// Request: C0 0C 00 64 01 F4 xx xx
//   KP=100=0x0064 → 100/10000=0.01
//   KI=500=0x01F4 → 500/10=50
```

---

## 7. 0x7FF 查询/设置命令

### 7.1 统一格式

```
CAN ID = 0x7FF (广播, 所有电机都监听)

请求帧 (Data[0:7]):
  Byte0 = 命令码
  Byte1-3 = 参数
  Byte4-7 = 数据

  命令码:
    0x01: 查询 CAN ID     → 响应包含电机 ID
    0x02: 设置 CAN ID     → Byte4 = 新 ID
    0x04: 重置 CAN ID     → 恢复默认 ID
    0x80: 刹车控制        → Byte4 = 0(释放)/1(刹车)
```

### 7.2 查询 CAN ID

```
请求: ID=0x7FF, Data: FF 00 00 82 xx xx xx xx  (Broadcast)
  或:   ID=0x7FF, Data: FF FF 00 82 xx xx xx xx  (指定电机)

响应: ID=0x7FF (从电机)
  Data: Motor ID (Byte x)
```

**注意:** 此命令会触发所有在线的电机同时响应，适合扫描总线。

### 7.3 设置 CAN ID

```
请求: ID=0x7FF, Data: FF 00 82 01 xx xx xx xx
  Byte4 = 新的 CAN ID (1~0x7E)
  
此命令立即生效，可能造成通信中断 (ID 变更)。
```

---

## 8. ENCOS ↔ MIT 协议适配

### 8.1 MIT float32 → ENCOS 运控帧

```c
// ENCOS 运控帧使用紧凑位域编码, 与非 MIT 电机不同
// 需要将你的 mit_command_t 转换成 ENCOS 的紧凑 CAN 帧

typedef struct {
    uint8_t  motor_id;      // [4:0]
    uint16_t kp_can;        // 0~4095 (float → uint12)
    uint16_t kd_can;        // 0~511  (float → uint9)
    uint16_t pos_can;       // 0~65535 (float → uint16)
    uint16_t vel_can;       // 0~4095 (float → uint12)
    uint16_t trq_can;       // 0~4095 (float → uint12)
} encos_mit_frame_t;

void mit_to_encos(const mit_command_t *mit, encos_mit_frame_t *enc) {
    // KP: 你的范围 0~N → ENCOS 0~4095 → 0~500
    enc->kp_can = (uint16_t)(mit->kp / 500.0f * 4095.0f);
    
    // KD: 你的范围 0~N → ENCOS 0~511 → 0~5
    enc->kd_can = (uint16_t)(mit->kd / 5.0f * 511.0f);
    
    // 位置: ENCOS 范围 -12.5~12.5 rad
    float pos_clamped = fminf(fmaxf(mit->target_position, -12.5f), 12.5f);
    enc->pos_can = (uint16_t)((pos_clamped + 12.5f) / 25.0f * 65535.0f);
    
    // 速度: ENCOS 范围 -18~18 rad/s
    float vel_clamped = fminf(fmaxf(mit->target_velocity, -18.0f), 18.0f);
    enc->vel_can = (uint16_t)((vel_clamped + 18.0f) / 36.0f * 4095.0f);
    
    // 力矩: ENCOS 范围 -30~30 Nm
    float trq_clamped = fminf(fmaxf(mit->ff_torque, -30.0f), 30.0f);
    enc->trq_can = (uint16_t)((trq_clamped + 30.0f) / 60.0f * 4095.0f);
}
```

### 8.2 ENCOS 反馈 → MIT float

```c
void encos_feedback_to_mit(const uint8_t data[8], mit_feedback_t *fb) {
    // 位置 (float32, Byte0-3)
    memcpy(&fb->actual_position, &data[0], 4);
    
    // 速度 (uint16 RPM → rad/s)
    uint16_t rpm_raw = (data[4] << 8) | data[5];
    float rpm = rpm_raw / 10.0f;  // 0~3276.7 RPM
    fb->actual_velocity = rpm * (2.0f * 3.14159f) / 60.0f;
    
    // 力矩 (uint12 电流 A) → 需要 Kt 常数
    uint16_t current_raw = ((data[6] & 0x0F) << 8) | data[7];
    float current = current_raw / 10.0f;  // 0~409.5A
    fb->actual_torque = current * Kt_constant;
}
```

---

## 9. 控制流程

### 9.1 最小启动序列

```
1. 查询 CAN ID (0x7FF)
   确认电机在线

2. 配置 CAN 超时 (0x0B)
   Request: C0 0B 01 F4 xx xx xx xx
   设 500ms = 防止异常时电机一直运行

3. (可选) 配置控制参数
   - 运控 KP/KD 范围 (0x05/0x06)
   - SPD/TRQ/CUR 限幅 (0x08~0x0A)
   - PI/PD 参数 (0x0C~0x0E)

4. 周期发送运控命令 (0x00)
   或 伺服位置命令 (0x02)
   或 伺服速度命令 (0x03)

5. 周期接收反馈
   监控故障码 (Byte6[7:4])

6. 异常或超时 → 发送停止/零输出
```

### 9.2 控制频率建议

| 阶段 | 频率 | 说明 |
|------|------|------|
| 初始调试 | 100~200 Hz | 低速验证协议 |
| 正常运行 | 500~1000 Hz | 1ms-2ms 周期 |
| 高速控制 | 1000~2000 Hz | 需 CAN FD 或确认总线负载 |

**不要一开始就跑 2000Hz。** 先确认总线负载、反馈稳定性和主循环时序。

---

## 10. 安全约束

| 约束 | 值 | 说明 |
|------|-----|------|
| 最大电压 | 57V | 超压停机 |
| 最小电压 | 16V | 欠压停机 |
| CAN 超时 | 500ms (默认) | 超时 → 电机停止 |
| 控制窗口 | 0.4ms (2000Hz) | 超过此频率无效 |
| 心跳 | 500ms | 超时后需重新使能 |
| ID 不匹配 | 5s 无心跳 | 电机进入安全状态 |
| 模式切换 | 需先停止 | 禁止运行中切换 |
