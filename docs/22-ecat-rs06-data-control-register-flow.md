<!-- Author: BearKing -->
# EtherCAT 到 RS06 的数据流、控制流与寄存器级路径

本文档解释当前工程从 EtherCAT 主站命令到 RS06 电机控制，再从 RS06 反馈回 EtherCAT 主站的完整链路。

当前代码状态：

- EtherCAT PDO：16 轴 MIT 风格命令和反馈。
- 当前启用电机：`axis14 -> CAN7 -> RS06 motor_id 127`。
- 当前 CAN：只初始化 CAN7，`CAN_IF_ENABLED_BUS_MASK = 0x80`。
- 当前控制模式：直接使用 EtherCAT 主站下发的控制量发送 RS06 运控帧。
- 当前反馈模式：RS06 type2 反馈解析后写回 EtherCAT TxPDO。

---

## 1. 总览

整体链路可以分成两条：

1. 控制下发链路：

```text
EtherCAT master
  -> ESC process output RAM
  -> SSC MainLoop()
  -> APPL_OutputMapping()
  -> gateway_app command cache
  -> motor_manager_poll_1ms()
  -> rs06_private_encode_motion_control()
  -> can_if_send()
  -> HPM MCAN7 TX FIFO / TXBAR
  -> CAN7 TXD/RXD pins
  -> RS06 motor
```

2. 反馈上报链路：

```text
RS06 motor
  -> CAN feedback frame, type 0x02
  -> HPM MCAN7 RX FIFO
  -> MCAN interrupt IR.RF0N / IR.RF1N
  -> can_if_isr()
  -> can_if RX ring
  -> motor_manager_poll_1ms()
  -> rs06_private_decode_feedback()
  -> gateway_app_set_motor_feedback()
  -> APPL_InputMapping()
  -> ESC process input RAM
  -> EtherCAT master
```

---

## 2. EtherCAT PDO 数据结构

文件：

```text
firmware/ecat_tsn_gateway/application/gateway_app.h
firmware/ecat_tsn_gateway/application/gateway_app.c
```

### 2.1 主站下发命令

每个轴 5 个 `REAL32`：

```c
typedef struct {
    REAL32 target_position;
    REAL32 target_velocity;
    REAL32 feedforward_torque;
    REAL32 kp;
    REAL32 kd;
} mit_motor_command_t;
```

16 轴总大小：

```text
16 axes * 5 floats * 4 bytes = 320 bytes
```

启动时应看到：

```text
PDO: 192 bytes in, 320 bytes out
```

这里的 `out` 是 EtherCAT 主站到从站，也就是主站下发控制命令。

### 2.2 从站上报反馈

每个轴 3 个 `REAL32`：

```c
typedef struct {
    REAL32 actual_position;
    REAL32 actual_velocity;
    REAL32 actual_torque;
} mit_motor_feedback_t;
```

16 轴总大小：

```text
16 axes * 3 floats * 4 bytes = 192 bytes
```

这里的 `in` 是从站到 EtherCAT 主站，也就是电机实际反馈。

---

## 3. EtherCAT 控制下发路径

### 3.1 SSC 触发 APPL_OutputMapping

SSC 协议栈在 `MainLoop()` 中处理 EtherCAT 状态机和过程数据。当主站进入 OP 并发送 RxPDO 后，SSC 会调用：

```c
void APPL_OutputMapping(UINT16 *pData)
```

`pData` 指向 SSC 从 ESC process output RAM 读出的 PDO 数据。

### 3.2 APPL_OutputMapping 拆 PDO

代码逻辑：

```c
void APPL_OutputMapping(UINT16 *pData)
{
    const uint8_t *src = (const uint8_t *)pData;

    for (axis = 0; axis < MIT_MOTOR_AXIS_COUNT; axis++) {
        for (item = 0; item < MIT_MOTOR_COMMAND_FLOATS; item++) {
            memcpy(s_command_objects[axis][item], src, sizeof(REAL32));
            src += sizeof(REAL32);
        }
        sync_command_cache(axis);
    }

    s_motor_command_updated = TRUE;
}
```

作用：

1. 按 axis 顺序读取 320 字节 RxPDO。
2. 每 4 字节拷贝成一个 `REAL32`。
3. 同步到 `s_motor_commands[axis]`。
4. 设置 `s_motor_command_updated = TRUE`。

### 3.3 motor_manager 读取命令

`main.c` 每 1ms 调用：

```c
motor_manager_poll_1ms();
```

里面检查：

```c
if (gateway_app_motor_command_updated()) {
    s_last_command_ms = s_time_ms;
    s_command_seen = true;
    s_timeout_stop_sent = false;
    gateway_app_clear_motor_command_updated();
}
```

含义：

- 收到过主站 Output PDO 后，才允许发 RS06 控制。
- 如果一直没有主站命令，不会自动使能，不会发全 0 运控帧。
- 如果超过 `MOTOR_MANAGER_COMMAND_TIMEOUT_MS = 100ms` 没有新命令，会 `motor_manager_stop_all()`。

### 3.4 send_motion_controls 生成 RS06 命令

当前配置：

```c
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE      1
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL   1
```

所以 `send_motion_controls()` 会执行。

核心逻辑：

```c
snapshot_command(axis, &cmd);

cmd.position_rad   = TargetPosition;
cmd.velocity_rad_s = TargetVelocity;
cmd.torque_nm      = FFTorque;
cmd.kp             = Kp;
cmd.kd             = Kd;
```

如果该轴还没有使能：

```c
rs06_private_encode_enable(cfg->bus, cfg->motor_id, cfg->host_id, &frame);
can_if_send(&frame);
s_axis_state[axis].enable_sent = true;
```

然后发送运控帧：

```c
rs06_private_encode_motion_control(cfg->bus, cfg->motor_id, &cmd, &frame);
can_if_send(&frame);
```

---

## 4. RS06 私有协议编码

文件：

```text
firmware/ecat_tsn_gateway/rs06/rs06_proto.c
```

### 4.1 29-bit 扩展 ID

RS06 私有协议使用 29-bit 扩展帧：

```text
bits[28:24] = communication type
bits[23:8]  = data field
bits[7:0]   = target motor id
```

代码：

```c
uint32_t rs06_private_make_id(uint8_t type, uint16_t data, uint8_t target_id)
{
    return ((((uint32_t)type) & 0x1FUL) << 24)
         | (((uint32_t)data) << 8)
         | target_id;
}
```

### 4.2 使能帧

```c
rs06_private_encode_enable(bus, motor_id, host_id, &frame)
```

生成：

```text
type      = 0x03
data      = host_id
target_id = motor_id
dlc       = 8
data[0:7] = 0
```

### 4.3 运控帧

```c
rs06_private_encode_motion_control(bus, motor_id, &cmd, &frame)
```

生成：

```text
type      = 0x01
data      = torque_ff 的 uint16 编码
target_id = motor_id
```

payload：

```text
Byte0-1: position uint16, -12.57..12.57 rad
Byte2-3: velocity uint16, -50..50 rad/s
Byte4-5: kp       uint16, 0..5000
Byte6-7: kd       uint16, 0..100
```

字节序：

```text
Big-endian，高字节在前
```

所以从主站到 RS06 的 5 个控制量映射是：

| EtherCAT RxPDO | RS06 私有协议字段 |
|---|---|
| `TargetPosition` | payload Byte0-1 |
| `TargetVelocity` | payload Byte2-3 |
| `FFTorque` | CAN ID bits[23:8] |
| `Kp` | payload Byte4-5 |
| `Kd` | payload Byte6-7 |

---

## 5. CAN 抽象层到 MCAN 发送寄存器

文件：

```text
firmware/ecat_tsn_gateway/can_if/can_if_hpm_mcan.c
```

### 5.1 can_if_frame_t

RS06 协议层生成的是硬件无关结构：

```c
typedef struct {
    uint8_t bus;
    can_if_id_format_t id_format;
    can_if_frame_format_t frame_format;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_if_frame_t;
```

当前 RS06 私有协议：

```text
bus          = 7
id_format    = CAN_IF_ID_EXTENDED
frame_format = CAN_IF_FRAME_CLASSIC
id           = 29-bit RS06 ID
dlc          = 8
```

### 5.2 can_if_send 转成 MCAN TX frame

```c
bool can_if_send(const can_if_frame_t *frame)
```

核心逻辑：

```c
tx.dlc = frame->dlc;
tx.canfd_frame = 0;
tx.bitrate_switch = 0;

if (frame->id_format == CAN_IF_ID_EXTENDED) {
    tx.use_ext_id = 1;
    tx.ext_id = frame->id & 0x1FFFFFFF;
}

memcpy(tx.data_8, frame->data, frame->dlc);

mcan_transmit_blocking(HPM_MCAN7, &tx);
```

这里明确关闭 CAN FD：

```text
canfd_frame = 0
bitrate_switch = 0
```

也就是 Classic CAN 2.0，8 字节。

### 5.3 mcan_write_txfifo 写 Message RAM

HPM SDK 驱动里：

```c
hpm_stat_t mcan_write_txfifo(MCAN_Type *ptr, mcan_tx_frame_t *tx_frame)
```

关键步骤：

1. 读 TX FIFO 状态寄存器：

   ```c
   mcan_is_txfifo_full(ptr)
   ```

   内部检查：

   ```c
   ptr->TXFQS & MCAN_TXFQS_TFQF_MASK
   ```

   寄存器：

   ```text
   TXFQS, offset 0xC4, TX FIFO/Queue Status
   ```

2. 获取 TX FIFO put index：

   ```c
   MCAN_TXFQS_TFQPI_GET(ptr->TXFQS)
   ```

3. 根据 put index 计算 TX buffer element 地址：

   ```c
   mcan_get_txbuf_elem_addr(ptr, put_index)
   ```

   这个地址来自 MCAN Message RAM，基址和布局由 `TXBC` 配置。

4. 写两个 32-bit header：

   ```c
   msg_hdr[0] = tx_frame_u32[0];
   msg_hdr[1] = tx_frame_u32[1];
   ```

   header 中包含：

   - 29-bit extended id
   - XTD/use_ext_id
   - RTR
   - DLC
   - CAN FD flag
   - BRS flag

5. 写 payload：

   ```c
   msg_data[i] = tx_frame->data_32[i];
   ```

### 5.4 TXBAR 触发发送

写完 Message RAM 后，驱动调用：

```c
mcan_send_add_request(ptr, put_index);
```

内部就是：

```c
ptr->TXBAR = (1UL << put_index);
```

寄存器：

```text
TXBAR, offset 0xD0, Tx Buffer Add Request
```

含义：

- 软件把某个 TX buffer 准备好了。
- 写 `TXBAR` 对应 bit。
- MCAN 硬件开始仲裁并发送该帧。

### 5.5 TXBTO 判断发送完成

`mcan_transmit_blocking()` 会等待：

```c
while (!mcan_is_transmit_occurred(ptr, put_index)) { ... }
```

内部检查：

```c
ptr->TXBTO & (1UL << put_index)
```

寄存器：

```text
TXBTO, offset 0xD8, Tx Buffer Transmission Occurred
```

如果 TX 成功，对应 bit 置位。

---

## 6. MCAN 初始化和寄存器配置

### 6.1 当前只初始化 CAN7

文件：

```text
firmware/ecat_tsn_gateway/can_if/can_if_hpm_mcan.c
```

当前：

```c
#define CAN_IF_ENABLED_BUS_MASK 0x80UL
```

所以 `can_if_init()` 只会初始化 bus7：

```text
bit7 = 1 -> CAN7 enabled
bit0..6 = 0 -> CAN0..CAN6 disabled
```

启动打印：

```text
CAN init OK: initMask=0x80
```

### 6.2 板级引脚寄存器

板级文件：

```text
D:\Tools\hpm\sdk_env-main\hpm_app\boards\hpm6e00_full_port\pinmux.c
```

CAN7 引脚：

```c
HPM_IOC->PAD[IOC_PAD_PD30].FUNC_CTL = IOC_PD30_FUNC_CTL_MCAN7_RXD;
HPM_IOC->PAD[IOC_PAD_PD31].FUNC_CTL = IOC_PD31_FUNC_CTL_MCAN7_TXD;

HPM_IOC->PAD[IOC_PAD_PD29].PAD_CTL =
    IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
HPM_IOC->PAD[IOC_PAD_PD29].FUNC_CTL = IOC_PD29_FUNC_CTL_GPIO_D_29;
```

寄存器含义：

| 寄存器 | 作用 |
|---|---|
| `HPM_IOC->PAD[PD30].FUNC_CTL` | 把 PD30 复用为 MCAN7 RXD |
| `HPM_IOC->PAD[PD31].FUNC_CTL` | 把 PD31 复用为 MCAN7 TXD |
| `HPM_IOC->PAD[PD29].PAD_CTL` | STBY 引脚打开上拉 |
| `HPM_IOC->PAD[PD29].FUNC_CTL` | 把 PD29 配成 GPIO |

### 6.3 收发器 STBY 控制

板级文件：

```text
D:\Tools\hpm\sdk_env-main\hpm_app\boards\hpm6e00_full_port\board.c
```

CAN7：

```c
index = GPIO_DO_GPIOD;
pin = 29;
gpio_set_pin_output_with_initial(HPM_GPIO0, index, pin, enable ? 0 : 1);
```

含义：

```text
enable = true  -> PD29 输出 0 -> transceiver 退出 standby -> CAN 可收发
enable = false -> PD29 输出 1 -> transceiver standby
```

### 6.4 MCAN 时钟

`can_if_init_clock(7)`：

```c
clock_add_to_group(clock_can7, BOARD_RUNNING_CORE & 0x1);
clock_set_source_divider(clock_can7, clk_src_pll1_clk0, 10);
src_clk_freq = clock_get_frequency(clock_can7);
```

然后 `mcan_init()` 用 `src_clk_freq` 和：

```c
config.baudrate = 1000000;
```

计算 1Mbps CAN bit timing。

### 6.5 mcan_init 关键寄存器

HPM SDK 里：

```c
hpm_stat_t mcan_init(MCAN_Type *ptr, mcan_config_t *config, uint32_t src_clk_freq)
```

关键寄存器路径：

1. 进入初始化模式：

   ```c
   ptr->CCCR |= MCAN_CCCR_INIT_MASK;
   ptr->CCCR |= MCAN_CCCR_CCE_MASK;
   ```

   寄存器：

   ```text
   CCCR, offset 0x18
   INIT bit0: 进入初始化模式
   CCE  bit1: 允许写保护配置寄存器
   ```

2. 设置 nominal bit timing：

   ```c
   ptr->NBTP =
       MCAN_NBTP_NBRP_SET(prescaler - 1) |
       MCAN_NBTP_NTSEG1_SET(num_seg1 - 1) |
       MCAN_NBTP_NTSEG2_SET(num_seg2 - 1) |
       MCAN_NBTP_NSJW_SET(num_sjw - 1);
   ```

   寄存器：

   ```text
   NBTP, offset 0x1C
   ```

   作用：

   - 设置 CAN 1Mbps 的时间量子、采样点、SJW。

3. 设置工作模式：

   当前：

   ```c
   config.mode = mcan_mode_normal;
   ```

   所以：

   ```c
   ptr->CCCR &= ~(MCAN_CCCR_MON_MASK | MCAN_CCCR_TEST_MASK);
   ```

   含义：

   - 不是 listen-only。
   - 不是 loopback。
   - 是正常收发模式。

4. 关闭 CAN FD：

   ```c
   ptr->CCCR &= ~(MCAN_CCCR_FDOE_MASK | MCAN_CCCR_BRSE_MASK);
   ```

   含义：

   - `FDOE=0`：CAN FD disabled。
   - `BRSE=0`：不做 CAN FD bitrate switch。

5. 配置 Message RAM：

   ```c
   mcan_config_ram(ptr, &config->ram_config);
   ```

   主要写：

   | 寄存器 | 作用 |
   |---|---|
   | `SIDFC` | 标准 ID filter list 地址和数量 |
   | `XIDFC` | 扩展 ID filter list 地址和数量 |
   | `RXF0C` | RX FIFO0 起始地址、大小、模式 |
   | `RXF1C` | RX FIFO1 起始地址、大小、模式 |
   | `RXESC` | RX element data size |
   | `TXBC` | TX buffer/FIFO 起始地址、数量、FIFO/queue 模式 |
   | `TXESC` | TX element data size |
   | `TXEFC` | TX event FIFO 配置 |

6. 配置过滤器：

   ```c
   mcan_config_all_filters(ptr, &config->all_filters_config);
   ```

   当前我们设置：

   ```c
   accept_non_matching_std_frame_option = RXFIFO0;
   accept_non_matching_ext_frame_option = RXFIFO0;
   reject_remote_std_frame = true;
   reject_remote_ext_frame = true;
   ```

   最终写：

   ```c
   ptr->GFC = ...
   ```

   寄存器：

   ```text
   GFC, offset 0x80, Global Filter Configuration
   ```

   含义：

   - 非匹配标准帧也进 RXFIFO0。
   - 非匹配扩展帧也进 RXFIFO0。
   - 拒收 remote frame。

7. 配置中断：

   我们设置：

   ```c
   interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT | MCAN_EVENT_ERROR;
   ```

   驱动写：

   ```c
   ptr->IE |= mask;
   ptr->ILE = 1U;
   ```

   寄存器：

   | 寄存器 | 作用 |
   |---|---|
   | `IR`, offset 0x50 | interrupt flag |
   | `IE`, offset 0x54 | interrupt enable |
   | `ILS`, offset 0x58 | interrupt line select |
   | `ILE`, offset 0x5C | interrupt line enable |

8. 清中断标志：

   ```c
   ptr->IR = ~0UL;
   ```

9. 退出初始化模式：

   ```c
   ptr->CCCR &= ~MCAN_CCCR_INIT_MASK;
   ```

   MCAN 开始正常收发。

---

## 7. MCAN 接收寄存器路径

### 7.1 硬件收到 CAN 帧

RS06 返回 type2 反馈帧后，MCAN 硬件根据过滤器配置把帧放入 RX FIFO0。

由于当前 `GFC` 配置接受非匹配扩展帧到 RXFIFO0，所以 RS06 29-bit 扩展帧会进入 RXFIFO0。

硬件置位中断标志：

```text
IR.RF0N = 1
```

寄存器：

```text
IR, offset 0x50
RF0N bit0: RX FIFO0 new message
```

### 7.2 can_if_isr 读取 IR

中断入口：

```c
void can_if_mcan7_isr(void)
{
    can_if_isr(7U);
}
```

核心：

```c
flags = mcan_get_interrupt_flags(base);
```

等价于：

```c
flags = base->IR;
```

如果：

```c
flags & MCAN_INT_RXFIFO0_NEW_MSG
```

就调用：

```c
can_if_rx_from_fifo(bus, 0U);
```

### 7.3 mcan_read_rxfifo 读取 RX FIFO0

SDK 里：

```c
mcan_read_rxfifo(ptr, 0, &rx)
```

关键寄存器：

1. 读 RX FIFO0 状态：

   ```c
   uint32_t rxf0s = ptr->RXF0S;
   ```

   寄存器：

   ```text
   RXF0S, offset 0xA4
   ```

   重要字段：

   | 字段 | 含义 |
   |---|---|
   | `F0FL` | FIFO0 fill level，有多少帧 |
   | `F0GI` | FIFO0 get index，当前该读哪个 element |
   | `F0PI` | FIFO0 put index，硬件下次写哪个 element |

2. 如果：

   ```c
   F0FL == 0
   ```

   说明 FIFO 为空。

3. 取 get index：

   ```c
   elem_index = MCAN_RXF0S_F0GI_GET(rxf0s);
   ```

4. 根据 `RXF0C` 和 `RXESC` 计算 Message RAM 地址。

5. 从 Message RAM 读取：

   ```c
   rx_frame_u32[0] = msg_hdr[0];
   rx_frame_u32[1] = msg_hdr[1];
   rx_frame->data_32[i] = msg_data[i];
   ```

6. 读完后 acknowledge：

   ```c
   ptr->RXF0A = elem_index;
   ```

   寄存器：

   ```text
   RXF0A, offset 0xA8
   ```

   含义：

   - 告诉 MCAN：这个 FIFO element 软件读完了。
   - 硬件更新 FIFO get index 和 fill level。

### 7.4 can_if_rx_from_fifo 转换为 can_if_frame_t

```c
frame.bus = bus;
frame.id_format = rx.use_ext_id ? CAN_IF_ID_EXTENDED : CAN_IF_ID_STANDARD;
frame.id = rx.use_ext_id ? rx.ext_id : rx.std_id;
frame.dlc = rx.dlc;
memcpy(frame.data, rx.data_8, frame.dlc);
```

然后：

```c
s_buses[bus].rx_count++;
ring_push(bus, &frame);
```

注意：

- ISR 不解析 RS06。
- ISR 不写 EtherCAT。
- ISR 只把 CAN 帧塞到软件 ring buffer。

这样中断时间短，协议解析放到主循环里做。

### 7.5 清中断标志

ISR 结束前：

```c
mcan_clear_interrupt_flags(base, flags);
```

等价于：

```c
base->IR = flags;
```

MCAN 的 IR 是写 1 清除。

---

## 8. RS06 反馈到 EtherCAT TxPDO

### 8.1 motor_manager 取 ring buffer

`motor_manager_poll_1ms()` 中：

```c
handle_can_rx();
```

内部：

```c
while (can_if_receive(&frame)) {
    if (rs06_private_decode_feedback(&frame, &feedback)) {
        handle_feedback(&feedback);
    }
}
```

### 8.2 解析 RS06 type2 反馈

`rs06_private_decode_feedback()` 做：

1. 解析 29-bit ID：

   ```text
   type = bits[28:24]
   data = bits[23:8]
   target host id = bits[7:0]
   ```

2. 判断 `type == 0x02`。
3. 从 `data` 里取：

   ```text
   motor_id
   fault_bits
   mode
   ```

4. 从 payload 里取：

   ```text
   position
   velocity
   torque
   temperature
   ```

5. 把 uint16 反算成 float。

### 8.3 写入 gateway_app 反馈缓存

`handle_feedback()` 中：

```c
gateway_app_set_motor_feedback((UINT16)axis,
                               (REAL32)pos,
                               (REAL32)vel,
                               (REAL32)tq);
```

这里就是反馈上报的关键点。

`gateway_app_set_motor_feedback()` 写：

```c
*s_feedback_objects[axis][0] = actual_position;
*s_feedback_objects[axis][1] = actual_velocity;
*s_feedback_objects[axis][2] = actual_torque;
```

这些对象对应 EtherCAT TxPDO：

```text
ActualPosition
ActualVelocity
ActualTorque
```

### 8.4 APPL_InputMapping 上报给主站

当 SSC 要准备输入过程数据时，会调用：

```c
APPL_InputMapping(UINT16 *pData)
```

它会按 16 轴顺序，把反馈对象拷贝到 ESC input PDO buffer：

```c
for (axis = 0; axis < MIT_MOTOR_AXIS_COUNT; axis++) {
    for (item = 0; item < MIT_MOTOR_FEEDBACK_FLOATS; item++) {
        memcpy(dst, s_feedback_objects[axis][item], sizeof(REAL32));
        dst += sizeof(REAL32);
    }
}
```

所以主站读 TxPDO 时看到的是：

```text
axis0 feedback
axis1 feedback
...
axis14 feedback <- RS06 id127 实际反馈
axis15 feedback
```

---

## 9. 当前控制流时序

### 9.1 main loop 时序

`main.c`：

```c
while (bRunApplication == TRUE) {
    MainLoop();

    while (s_ms10 > 0U) {
        s_ms10--;
        motor_manager_poll_1ms();
    }

    if (s_ms1000 >= 1000) {
        print_can_status();
        motor_manager_print_status();
    }
}
```

控制流特点：

1. `MainLoop()` 先处理 EtherCAT。
2. EtherCAT 如果收到 RxPDO，会更新 command cache。
3. `motor_manager_poll_1ms()` 读取 command cache 并发 CAN。
4. CAN 接收中断只入队。
5. `motor_manager_poll_1ms()` 再从队列解析反馈。
6. 解析后的反馈写回 gateway_app。
7. 下一次 `APPL_InputMapping()` 把反馈打包给主站。

### 9.2 主站命令门槛

当前为了安全：

```c
if (gateway_app_motor_command_updated()) {
    s_command_seen = true;
    s_last_command_ms = s_time_ms;
}
```

只有 `s_command_seen == true` 且命令没有超时，才发 RS06 运控帧。

如果超过 100ms 没有新命令：

```c
motor_manager_stop_all();
```

这会发 RS06 type4 stop 帧。

---

## 10. 寄存器级收发总结

### 10.1 初始化阶段写的关键寄存器

| 模块 | 寄存器 | 作用 |
|---|---|---|
| IOC | `PAD[PD30].FUNC_CTL` | PD30 复用为 MCAN7_RXD |
| IOC | `PAD[PD31].FUNC_CTL` | PD31 复用为 MCAN7_TXD |
| IOC | `PAD[PD29].FUNC_CTL` | PD29 复用为 GPIO，用作 STBY |
| GPIO | GPIO output register | PD29 输出 0，使能 CAN transceiver |
| MCAN | `CCCR.INIT` | 进入/退出初始化模式 |
| MCAN | `CCCR.CCE` | 允许写配置寄存器 |
| MCAN | `NBTP` | 设置 1Mbps nominal bit timing |
| MCAN | `CCCR.FDOE/BRSE` | 清 0，关闭 CAN FD |
| MCAN | `GFC` | 全局接收过滤，扩展帧进入 RXFIFO0 |
| MCAN | `RXF0C/RXF1C` | 配置 RX FIFO 起始地址和深度 |
| MCAN | `RXESC` | 配置 RX element payload size |
| MCAN | `TXBC` | 配置 TX FIFO/buffer 起始地址和深度 |
| MCAN | `TXESC` | 配置 TX element payload size |
| MCAN | `IR` | 清中断标志 |
| MCAN | `IE` | 使能 RX/TX/error 中断 |
| MCAN | `ILE` | 打开 MCAN interrupt line |

### 10.2 发送阶段用的关键寄存器

| 步骤 | 寄存器/内存 | 说明 |
|---|---|---|
| 查 TX FIFO 是否满 | `TXFQS.TFQF` | 满则不能写 |
| 获取 TX FIFO put index | `TXFQS.TFQPI` | 决定写哪个 element |
| 写帧头/数据 | Message RAM TX element | 写 29-bit ID、DLC、data |
| 请求发送 | `TXBAR = 1 << index` | 硬件开始发送 |
| 等待发送完成 | `TXBTO & (1 << index)` | 对应 bit 置位说明发送成功 |
| TX 中断标志 | `IR.TC` | transmit completed |

### 10.3 接收阶段用的关键寄存器

| 步骤 | 寄存器/内存 | 说明 |
|---|---|---|
| 新帧中断 | `IR.RF0N` | RX FIFO0 有新帧 |
| 查看 FIFO 状态 | `RXF0S` | fill level / get index / put index |
| 读 RX element | Message RAM RX FIFO0 element | 读帧头和 payload |
| 释放 FIFO element | `RXF0A = elem_index` | 告诉硬件该帧读完 |
| 清中断标志 | `IR = flags` | 写 1 清除 |

---

## 11. 当前日志如何对应到内部状态

示例：

```text
CAN: initMask=0x80 | C7 tx=0 rx=6319 err=0 drop=0
RS06: online=1/16 tx=6319 rx=6318 fb=6305 direct=1 cmd=1
RS06 axis14 bus=7 id=127 online=1 dev=0 fb=6305 pos=... target=... err=... vel=... tq=... temp=... fault=0x00 mode=2 age=1ms
```

解释：

| 字段 | 来源 |
|---|---|
| `initMask=0x80` | `can_if_get_initialized_mask()`，只有 CAN7 initialized |
| `C7 rx` | MCAN ISR 收到帧后 `s_buses[7].rx_count++` |
| `C7 err` | MCAN error event 后 `error_count++` |
| `drop` | 软件 RX ring 满时增加 |
| `RS06 tx` | `send_frame_for_axis()` 成功后增加 |
| `RS06 rx` | device id 或 feedback 被 manager 识别后增加 |
| `fb` | type2 feedback 数量 |
| `direct=1` | 当前使用 EtherCAT 主站命令直控 |
| `cmd=1` | 已收到过 EtherCAT Output PDO |
| `pos/vel/tq` | RS06 type2 反馈解码结果 |
| `target` | 当前从 EtherCAT 命令读取到的位置目标 |
| `fault` | RS06 反馈故障位 |
| `mode` | RS06 电机模式 |
| `age` | 距离上次反馈的时间 |

---

## 12. 现在调试时最重要的判断

1. EtherCAT 主站是否开始发命令：

   ```text
   cmd=1
   ```

2. CAN7 是否干净：

   ```text
   C7 err=0
   drop=0
   ```

3. RS06 是否在回反馈：

   ```text
   fb 持续增长
   age 很小
   ```

4. 电机是否在运行模式：

   ```text
   mode=2
   fault=0x00
   ```

5. 主站目标是否合理：

   ```text
   target 不要突然离 pos 很远
   ```

如果主站第一次目标是 0，而电机当前 `pos=2.44 rad`，那电机会直接向 0 rad 运动。所以主站启动时最好先把 `TargetPosition` 初始化为当前 `ActualPosition` 附近，再逐步改变目标。
