<!-- Author: BearKing -->
# CAN 扩展帧和 CAN FD 的区别

> 这份文档用于 RS06 电机接入前的基础概念说明。
> 重点结论：**扩展帧不是 CAN FD，CAN FD 也不等于扩展帧。**

## 1. 一句话区别

| 名称 | 解决什么问题 | 关键变化 |
|---|---|---|
| 标准帧 | 普通 CAN 2.0 帧格式 | 11-bit CAN ID，最多 8 字节数据 |
| 扩展帧 | CAN ID 不够用 | 29-bit CAN ID，最多仍然 8 字节数据 |
| CAN FD | 数据量和速度不够用 | 最多 64 字节数据，数据段可以更高速 |

所以：

```text
扩展帧：主要改 CAN ID 长度
CAN FD：主要改数据长度和数据段速度
```

RS06 默认使用的是：

```text
CAN 2.0
1 Mbps
29-bit 扩展帧
DLC = 8 bytes
```

它不是 CAN FD。

## 2. 标准帧是什么

标准帧就是最常见的 CAN 2.0A 帧。

特点：

```text
CAN ID: 11 bit
数据长度: 0~8 byte
仲裁速率: 固定一个 CAN 波特率，例如 1 Mbps
```

标准帧 ID 范围：

```text
0x000 ~ 0x7FF
```

也就是最多 2048 个 ID。

例子：

```text
ID  = 0x001
DLC = 8
Data = FF FF FF FF FF FF FF FC
```

很多 MIT 电机协议会用标准帧，比如：

```text
CAN ID = motor_id
Data   = position / velocity / kp / kd / torque
```

这里的 `motor_id` 通常就是 1、2、3 这种 11-bit 标准 ID。

## 3. 扩展帧是什么

扩展帧是 CAN 2.0B 引入的帧格式。

特点：

```text
CAN ID: 29 bit
数据长度: 0~8 byte
仲裁速率: 固定一个 CAN 波特率，例如 1 Mbps
```

扩展帧 ID 范围：

```text
0x00000000 ~ 0x1FFFFFFF
```

扩展帧和标准帧相比，主要区别是 **ID 从 11 bit 变成 29 bit**。

但是注意：

```text
扩展帧的数据区仍然最多只有 8 字节。
扩展帧不代表更快。
扩展帧不代表 CAN FD。
```

RS06 私有协议就是典型的扩展帧用法。

它把 29-bit CAN ID 拆成几个字段：

```text
bits [28:24] = 通信类型
bits [23:8]  = 数据区
bits [7:0]   = 目标电机 CAN ID
```

例如 RS06 运控帧：

```text
CAN ID:
  bits [28:24] = 0x01，表示运控命令
  bits [23:8]  = torque_ff 的 uint16 编码值
  bits [7:0]   = motor_id

Data[0:7]:
  Byte0-1 = target_position
  Byte2-3 = target_velocity
  Byte4-5 = kp
  Byte6-7 = kd
```

这个设计的意思是：RS06 不只用 Data[0:7] 传数据，还把一部分信息塞进了 29-bit CAN ID 里。

这就是它必须用扩展帧的原因。

## 4. CAN FD 是什么

CAN FD 全称是 CAN Flexible Data-rate。

它解决的是两个问题：

1. 传统 CAN 一帧最多 8 字节，数据太少。
2. 传统 CAN 整帧都只能用同一个波特率，速度不够。

CAN FD 的特点：

```text
CAN ID: 可以是 11 bit，也可以是 29 bit
数据长度: 最多 64 byte
仲裁段速率: 例如 1 Mbps
数据段速率: 可以更高，例如 2 Mbps / 5 Mbps / 8 Mbps
```

CAN FD 有两个关键位：

| 字段 | 含义 |
|---|---|
| FDF | 表示这是一帧 CAN FD 帧 |
| BRS | Bit Rate Switch，表示数据段是否切到更高波特率 |

CAN FD 可以这样用：

```text
11-bit 标准 ID + CAN FD 数据帧
29-bit 扩展 ID + CAN FD 数据帧
```

所以 CAN FD 和扩展帧不是同一层概念：

```text
标准帧 / 扩展帧：说的是 ID 长度
Classic CAN / CAN FD：说的是帧类型、数据长度、是否支持数据段变速
```

## 5. 四种组合关系

CAN 里可以有这四种组合：

| 组合 | ID 长度 | 数据长度 | 是否 CAN FD | 例子 |
|---|---:|---:|---|---|
| CAN 2.0 标准帧 | 11 bit | 0~8 byte | 否 | 普通 CANopen PDO、部分 MIT 电机 |
| CAN 2.0 扩展帧 | 29 bit | 0~8 byte | 否 | RS06 私有协议 |
| CAN FD 标准帧 | 11 bit | 0~64 byte | 是 | 某些高速电机或传感器 |
| CAN FD 扩展帧 | 29 bit | 0~64 byte | 是 | 高速大数据量设备 |

RS06 默认属于第二种：

```text
CAN 2.0 扩展帧
29-bit ID
8 byte Data
1 Mbps
不是 CAN FD
```

## 6. 为什么扩展帧不一定更快

扩展帧比标准帧的 ID 更长，所以同样 8 字节数据时，扩展帧在线上占用的 bit 更多。

粗略理解：

```text
标准帧 8 字节：约 110~130 bit
扩展帧 8 字节：约 130~160 bit
```

实际还会受位填充、ACK、帧间隔影响。

所以扩展帧不是为了提速，而是为了让 ID 能承载更多信息。

RS06 使用扩展帧，是因为它要在 CAN ID 里面放：

```text
通信类型
主机 ID
目标电机 ID
力矩前馈
故障信息
电机模式
```

这些字段用 11-bit 标准 ID 放不下。

## 7. 为什么 CAN FD 更快

CAN FD 可以更快，是因为它允许数据段切换到更高波特率。

例如：

```text
仲裁段: 1 Mbps
数据段: 5 Mbps
```

仲裁段包括 CAN ID，因为多个节点要靠它抢占总线，所以通常不能随便提太高。

数据段只有赢得仲裁的节点在发，因此可以用更高速度。

不过 CAN FD 更快有前提：

1. MCU 控制器支持 CAN FD。
2. CAN 收发器支持 CAN FD。
3. 总线上所有相关节点支持 CAN FD。
4. 电机协议本身支持 CAN FD。

HPM6E80 的 MCAN 控制器可以支持 CAN FD，但 RS06 默认协议是 CAN 2.0。因此接 RS06 时不能因为 MCU 支持 CAN FD，就把总线直接配置成 CAN FD。

## 8. 对 RS06 接入的影响

### 8.1 HPM 侧应该怎么配

RS06 默认接入建议：

```text
CAN 模式: Classic CAN / CAN 2.0
ID 类型: Extended / 29-bit
波特率: 1 Mbps
DLC: 8
CAN FD: disabled
BRS: disabled
```

也就是说：

```text
enable_canfd = false
id_type = extended
baudrate = 1000000
dlc = 8
```

### 8.2 USB-CAN 工具应该怎么看

USB-CAN 工具里通常要注意这些选项：

```text
Baudrate: 1 Mbps
Frame format: Extended frame / 扩展帧
Frame type: Data frame / 数据帧
CAN FD: Off
DLC: 8
```

如果 USB-CAN 工具只打开了标准帧显示，你可能看不到 RS06 的帧，或者 ID 显示异常。

### 8.3 主站 EtherCAT 和 CAN 的关系

EtherCAT 主站不知道 CAN 是标准帧、扩展帧还是 CAN FD。

主站只看到 EtherCAT PDO：

```text
主站 -> HPM:
  TargetPosition
  TargetVelocity
  FFTorque
  Kp
  Kd

HPM -> 主站:
  ActualPosition
  ActualVelocity
  ActualTorque
```

CAN 帧格式是 HPM 固件内部的事：

```text
EtherCAT PDO float
  -> HPM 固件转换
  -> RS06 CAN 2.0 扩展帧
  -> RS06 电机
```

## 9. 对 16 轴性能的影响

因为 RS06 默认不是 CAN FD，而是 1Mbps Classic CAN 扩展帧，所以 16 轴性能要按 Classic CAN 估算。

每个电机每个周期至少：

```text
1 帧控制
1 帧反馈
```

也就是：

```text
16 轴 = 32 帧 / 周期
```

如果全放在一路 1Mbps CAN 上：

```text
32 帧 * 约 150 bit = 4800 bit
4800 bit / 1 Mbps = 4.8 ms
```

这还没有算软件调度、错误重发、总线空隙。

所以：

```text
单路 CAN 无法稳定跑 16 轴 1kHz。
```

推荐：

```text
8 路 CAN，每路 2 个电机：适合 1ms 控制周期
4 路 CAN，每路 4 个电机：更适合 2ms 控制周期
1 路 CAN，16 个电机：只适合低频功能验证
```

如果电机真的支持 CAN FD，并且协议也改成 CAN FD，才可以重新按 CAN FD 速度评估。

但 RS06 默认私有协议不要这样假设。

## 10. HPM MCAN 配置概念

HPM 的外设叫 MCAN，这个名字容易让人误会。

MCAN 是控制器模块名字，它可以支持：

```text
Classic CAN 标准帧
Classic CAN 扩展帧
CAN FD 标准帧
CAN FD 扩展帧
```

所以看到 `MCAN` 不代表当前一定在用 CAN FD。

我们接 RS06 时，应该使用 MCAN 的 Classic CAN 能力：

```text
MCAN controller: 使用
CAN FD frame: 不使用
Extended ID: 使用
```

更直白地说：

```text
硬件外设叫 MCAN
但发出去的是 CAN 2.0 扩展帧
不是 CAN FD 帧
```

## 11. 调试时怎么判断当前帧类型

可以从下面几个点判断：

### 11.1 看 ID 长度

如果 ID 大于 `0x7FF`，一定不是标准帧，只能是扩展帧：

```text
0x001       -> 可能是标准帧
0x18FF0101  -> 扩展帧
```

### 11.2 看 DLC

如果 DLC 大于 8，一定是 CAN FD：

```text
DLC = 8   -> 可能是 Classic CAN，也可能是 CAN FD
DLC = 12  -> 一定是 CAN FD
DLC = 64  -> 一定是 CAN FD
```

注意：DLC 等于 8 不能证明它不是 CAN FD，只能说它没有超过 Classic CAN 的数据长度。

### 11.3 看工具显示的 FD/FDF/BRS

USB-CAN 或示波器工具通常会显示：

```text
Extended / Standard
CAN FD / Classic CAN
BRS on / off
```

RS06 默认应该看到：

```text
Extended = Yes
CAN FD = No
BRS = No
DLC = 8
Baudrate = 1 Mbps
```

## 12. 常见误区

### 误区 1：扩展帧就是 CAN FD

错。

扩展帧只是 29-bit ID，数据仍然最多 8 字节。

### 误区 2：CAN FD 一定是扩展帧

错。

CAN FD 可以用 11-bit 标准 ID，也可以用 29-bit 扩展 ID。

### 误区 3：HPM MCAN 就必须开 CAN FD

错。

MCAN 是外设名字。它可以发 Classic CAN，也可以发 CAN FD。

### 误区 4：RS06 用扩展帧，所以速度比标准帧快

错。

扩展帧 ID 更长，同样 8 字节数据时反而比标准帧占用更多总线时间。

### 误区 5：MCU 支持 CAN FD，电机就能用 CAN FD

错。

CAN FD 必须 MCU、收发器、电机和协议都支持。

## 13. 给当前工程的建议

当前 RS06 接入工程建议这样定：

```text
第一版:
  RS06 私有协议
  CAN 2.0
  29-bit 扩展帧
  1 Mbps
  DLC = 8
  不开 CAN FD

后续可选:
  如果确认电机支持 CAN FD 协议，再新增 CAN FD 后端
  如果切到 RS06 MIT 11-bit 协议，再新增标准帧后端
```

代码层面建议命名清楚：

```c
typedef enum {
    CAN_IF_FRAME_CLASSIC = 0,
    CAN_IF_FRAME_FD = 1,
} can_if_frame_format_t;

typedef enum {
    CAN_IF_ID_STANDARD = 0,
    CAN_IF_ID_EXTENDED = 1,
} can_if_id_format_t;
```

不要用一个 `is_canfd` 去表达扩展帧，否则后面一定会混。

正确配置应该是两个维度：

```c
frame_format = CAN_IF_FRAME_CLASSIC;
id_format = CAN_IF_ID_EXTENDED;
```

这才准确表示：

```text
Classic CAN + 29-bit Extended ID
```

