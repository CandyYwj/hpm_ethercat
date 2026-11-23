<!-- Author: BearKing -->
# HPM 工程中接入 CAN/MCAN 指南

> 当前目标：在 `firmware/ecat_tsn_gateway` 工程里加入 HPM6E80 MCAN，用于后续连接 RS06 电机。
> RS06 第一版建议使用：**Classic CAN + 29-bit 扩展帧 + 1Mbps + DLC=8**。

## 1. 先分清 HPM 的 MCAN 和 CAN FD

HPM 外设名字叫 **MCAN**，但这不代表一定要开 CAN FD。

MCAN 控制器可以发：

```text
Classic CAN 标准帧
Classic CAN 扩展帧
CAN FD 标准帧
CAN FD 扩展帧
```

接 RS06 默认私有协议时，应该配置成：

```text
MCAN controller : 使用
CAN FD frame    : 不使用
CAN ID          : 29-bit 扩展帧
Bitrate         : 1 Mbps
DLC             : 8
```

也就是：

```c
can_config.enable_canfd = false;
tx_frame.use_ext_id = 1;
tx_frame.ext_id = rs06_id;
tx_frame.canfd_frame = 0;
tx_frame.bitrate_switch = 0;
tx_frame.dlc = 8;
```

## 2. HPM6E80 有哪些 MCAN

`hpm_soc_ip.h` 里已经定义了：

```text
HPM_MCAN0
HPM_MCAN1
HPM_MCAN2
HPM_MCAN3
HPM_MCAN4
HPM_MCAN5
HPM_MCAN6
HPM_MCAN7
```

这说明 SoC 侧最多支持 8 路 MCAN。

但是能不能用，还要看板子：

1. 这个 MCAN 的 TX/RX 引脚有没有 mux 到外部。
2. 外部有没有 CAN 收发器。
3. 收发器 EN/STB 引脚是否需要 GPIO 拉高/拉低。
4. 终端电阻是否接好。
5. 这路 MCAN 有没有和 EtherCAT/TSN/其他外设引脚冲突。

所以软件第一步建议只启用 **一路 MCAN**，例如 `HPM_MCAN0`，验证能收发后再扩多路。

## 3. 当前工程应该加哪些文件

建议先只加 CAN 抽象层，不要一开始就写 RS06 电机控制：

```text
firmware/ecat_tsn_gateway/can_if/
  can_if.h
  can_if_hpm_mcan.c
```

后续 RS06 再放到：

```text
firmware/ecat_tsn_gateway/rs06/
  rs06_proto.h
  rs06_proto.c
  rs06_motor.h
  rs06_motor.c
```

第一步只做：

```text
HPM MCAN <-> USB-CAN 工具
```

不要直接接电机。先确认 CAN 硬件链路是通的。

## 4. CMake 怎么加

修改：

```text
firmware/ecat_tsn_gateway/CMakeLists.txt
```

增加：

```cmake
sdk_inc(can_if)
sdk_src(can_if/can_if_hpm_mcan.c)
```

如果后面加 RS06：

```cmake
sdk_inc(rs06)
sdk_src(rs06/rs06_proto.c)
sdk_src(rs06/rs06_motor.c)
```

HPM SDK 的 MCAN 驱动头文件来自 SDK，通常不用手动 `sdk_src(hpm_mcan_drv.c)`，SDK 会根据引用和组件处理。如果链接时报 `mcan_xxx undefined`，再检查 SDK 构建组件是否包含 driver 源。

## 5. `can_if.h` 建议长什么样

先定义一个和硬件无关的 CAN 帧：

```c
#ifndef CAN_IF_H
#define CAN_IF_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CAN_IF_ID_STANDARD = 0,
    CAN_IF_ID_EXTENDED = 1,
} can_if_id_format_t;

typedef enum {
    CAN_IF_FRAME_CLASSIC = 0,
    CAN_IF_FRAME_FD = 1,
} can_if_frame_format_t;

typedef struct {
    uint8_t bus;
    can_if_id_format_t id_format;
    can_if_frame_format_t frame_format;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t timestamp_us;
} can_if_frame_t;

bool can_if_init(void);
bool can_if_send(const can_if_frame_t *frame);
bool can_if_receive(can_if_frame_t *frame);
uint32_t can_if_get_tx_count(uint8_t bus);
uint32_t can_if_get_rx_count(uint8_t bus);
uint32_t can_if_get_error_count(uint8_t bus);

#endif
```

为什么要这样拆：

- `id_format` 表示标准帧还是扩展帧。
- `frame_format` 表示 Classic CAN 还是 CAN FD。
- 这两个不是一回事，必须分开。

RS06 第一版就是：

```c
id_format = CAN_IF_ID_EXTENDED;
frame_format = CAN_IF_FRAME_CLASSIC;
```

## 6. `can_if_hpm_mcan.c` 初始化流程

HPM SDK 示例里核心流程是：

```c
mcan_get_default_config(base, &can_config);
can_config.baudrate = 1000000;
can_config.enable_canfd = false;
can_config.mode = mcan_mode_normal;

board_init_can(base);
can_src_clk_freq = board_init_can_clock(base);

mcan_init(base, &can_config, can_src_clk_freq);
```

接 RS06 时关键配置：

```c
can_config.baudrate = 1000000;
can_config.enable_canfd = false;
can_config.mode = mcan_mode_normal;
```

如果先做内部自测，可以临时用：

```c
can_config.mode = mcan_mode_loopback_internal;
```

但是接 USB-CAN 或真实电机时，必须用 normal：

```c
can_config.mode = mcan_mode_normal;
```

## 7. 扩展帧发送怎么写

HPM 的发送结构体是 `mcan_tx_frame_t`。

扩展帧重点是：

```c
tx.use_ext_id = 1;
tx.ext_id = id & 0x1FFFFFFF;
```

Classic CAN 重点是：

```c
tx.canfd_frame = 0;
tx.bitrate_switch = 0;
tx.dlc = 8;
```

示例：

```c
static bool can_if_send_ext_classic(MCAN_Type *base, uint32_t id, const uint8_t data[8])
{
    mcan_tx_frame_t tx;

    memset(&tx, 0, sizeof(tx));
    tx.use_ext_id = 1;
    tx.ext_id = id & 0x1FFFFFFFUL;
    tx.dlc = 8;
    tx.canfd_frame = 0;
    tx.bitrate_switch = 0;
    memcpy(tx.data_8, data, 8);

    return mcan_transmit_blocking(base, &tx) == status_success;
}
```

第一版可以先用 `mcan_transmit_blocking()`，容易调通。

后面性能优化再换：

```c
mcan_transmit_via_txfifo_nonblocking()
```

## 8. 接收怎么做

最简单的调试方式是轮询 RX FIFO：

```c
mcan_rx_message_t rx;

if (mcan_receive_from_fifo_blocking(base, 0, &rx) == status_success) {
    if (rx.use_ext_id) {
        id = rx.ext_id;
    } else {
        id = rx.std_id;
    }
}
```

但是正式接电机时，不建议在主循环里 blocking 等 CAN。

正式方式应该是：

1. MCAN RX 中断收到帧。
2. ISR 里只把帧搬到 ring buffer。
3. `motor_manager_poll_1ms()` 再解析 ring buffer。

原因：不能让 CAN 接收阻塞 EtherCAT `MainLoop()`。

## 9. 中断方式怎么接

HPM 示例使用：

```c
SDK_DECLARE_EXT_ISR_M(BOARD_APP_CAN_IRQn, board_can_isr)
```

如果我们不用 `BOARD_APP_CAN_BASE`，而是自己选 `HPM_MCAN0`，建议写成：

```c
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN0, can0_isr)
```

ISR 基本流程：

```c
static void can0_isr(void)
{
    uint32_t flags = mcan_get_interrupt_flags(HPM_MCAN0);

    if ((flags & MCAN_EVENT_RECEIVE) != 0) {
        mcan_rx_message_t rx;

        while (mcan_receive_from_fifo_blocking(HPM_MCAN0, 0, &rx) == status_success) {
            /* 放入 ring buffer，不在这里解析 RS06 */
        }
    }

    mcan_clear_interrupt_flags(HPM_MCAN0, flags);
}
```

初始化时打开中断：

```c
can_config.interrupt_mask = MCAN_EVENT_RECEIVE | MCAN_EVENT_TRANSMIT;
mcan_init(HPM_MCAN0, &can_config, can_src_clk_freq);
mcan_enable_interrupts(HPM_MCAN0, can_config.interrupt_mask);
intc_m_enable_irq_with_priority(IRQn_MCAN0, 1);
```

注意：具体函数名和中断宏以 SDK 头文件为准，先照 `samples/drivers/mcan/src/mcan_demo.c` 改最稳。

## 10. 放到 `main.c` 的哪里

当前 `main.c` 大概流程是：

```c
board_init();
...
ecat_hardware_init();
MainInit();
APPL_GenerateMapping();
while (bRunApplication == TRUE) {
    MainLoop();
}
```

建议 CAN 初始化放在 EtherCAT 初始化成功之后、进入主循环之前：

```c
printf("ESC hardware OK\r\n");

MainInit();

...

APPL_GenerateMapping(&nPdInputSize, &nPdOutputSize);

if (!can_if_init()) {
    printf("CAN init FAILED\r\n");
    while (1) {
    }
}
printf("CAN init OK: Classic CAN extended frame 1Mbps\r\n");

bRunApplication = TRUE;
printf("Gateway running\r\n");
```

主循环里第一版可以只打印计数：

```c
while (bRunApplication == TRUE) {
    MainLoop();

    can_if_poll_test(); /* 第一版测试用，后面换 motor_manager_poll_1ms() */

    if (s_ms1000 >= 1000) {
        s_ms1000 = 0;
        printf("CAN: tx=%lu rx=%lu err=%lu\r\n",
               can_if_get_tx_count(0),
               can_if_get_rx_count(0),
               can_if_get_error_count(0));
    }
}
```

后面接 RS06 后换成：

```c
motor_manager_poll_1ms();
```

## 11. 第一阶段推荐测试

不要一上来接 RS06。先这样测：

```text
HPM MCAN0_TX/RX -> CAN 收发器 -> USB-CAN
```

USB-CAN 设置：

```text
Baudrate: 1Mbps
Frame: Extended
CAN FD: Off
DLC: 8
```

HPM 每秒发一帧：

```text
ID   = 0x0100FD01
DLC  = 8
Data = 01 02 03 04 05 06 07 08
```

USB-CAN 应该能看到：

```text
扩展帧
ID = 0x0100FD01
DLC = 8
Data = 01 02 03 04 05 06 07 08
```

然后 USB-CAN 发回一帧：

```text
ID   = 0x020001FD
DLC  = 8
Data = 00 00 00 00 00 00 00 00
```

HPM 串口应该看到：

```text
CAN rx_count 增加
收到的是扩展帧
ID 正确
```

## 12. 第二阶段再接 RS06

CAN 链路确认后，再做 RS06：

1. 用 `rs06_proto.c` 生成使能帧。
2. 发给电机。
3. 看是否收到通信类型 `0x02` 的反馈帧。
4. 解析位置、速度、力矩。
5. 写入 `gateway_app_set_motor_feedback(axis, pos, vel, torque)`。

不要第一步就把 EtherCAT 下发命令直接发给真实电机。

推荐先固定命令：

```text
position = 当前附近
velocity = 0
torque_ff = 0
kp = 很小
kd = 适中
```

确认反馈正常后，再接主站 PDO。

## 13. 常见问题

### 13.1 编译找不到 `board_init_can`

说明板级支持文件可能没有暴露这个函数，或者当前 board 不支持该 CAN 初始化。

处理：

1. 搜索 SDK board 目录里的 `board_init_can`。
2. 确认当前工程 board 是 `hpm6e00evk` 还是自定义新板。
3. 如果新板没有，需要自己写 pinmux 和 clock 初始化。

### 13.2 USB-CAN 看不到帧

优先检查：

1. USB-CAN 是否设置 1Mbps。
2. 是否打开扩展帧显示。
3. CANH/CANL 是否接反。
4. 两端是否共地。
5. 是否有 120 欧终端电阻。
6. CAN 收发器 EN/STB 是否处于正常工作状态。
7. HPM 是否真的用 normal mode，不是 internal loopback。

### 13.3 一直 ACK error

常见原因：

1. 总线上没有另一个节点 ACK。
2. USB-CAN 没打开。
3. 波特率不一致。
4. 收发器没工作。
5. 线没接好。

### 13.4 能发不能收

检查：

1. RX 引脚 pinmux。
2. RX FIFO filter 是否拒收了扩展帧。
3. global filter 是否设置成 reject。
4. USB-CAN 发的是标准帧还是扩展帧。

## 14. 当前建议的最小代码路线

第一步只做这 4 件事：

1. 新增 `can_if/can_if.h`。
2. 新增 `can_if/can_if_hpm_mcan.c`。
3. CMake 加入 `sdk_inc(can_if)` 和 `sdk_src(can_if/can_if_hpm_mcan.c)`。
4. `main.c` 调用 `can_if_init()`，每秒发一帧扩展帧测试。

成功标准：

```text
USB-CAN 能看到 HPM 发出的 29-bit 扩展帧。
HPM 能统计 USB-CAN 发回来的扩展帧。
EtherCAT 原来的启动和 OP 不被影响。
```

## 15. 当前工程已落地的第一阶段代码

现在工程里已经新增：

```text
firmware/ecat_tsn_gateway/can_if/can_if.h
firmware/ecat_tsn_gateway/can_if/can_if_hpm_mcan.c
```

并且 `CMakeLists.txt` 已经加入：

```cmake
sdk_inc(can_if)
sdk_src(can_if/can_if_hpm_mcan.c)
```

`main.c` 已经在 EtherCAT PDO mapping 之后调用：

```c
can_if_init();
```

启动后会打印：

```text
CAN init OK: initMask=0x??, Classic CAN extended frame 1Mbps
```

每秒状态打印会增加：

```text
CAN: initMask=0x?? | C0 tx=... rx=... err=... drop=... | ... | C7 ...
```

### 15.1 当前代码初始化的是 8 路 MCAN 控制器

当前 `can_if_hpm_mcan.c` 按 8 路写好：

```text
bus 0 -> HPM_MCAN0
bus 1 -> HPM_MCAN1
bus 2 -> HPM_MCAN2
bus 3 -> HPM_MCAN3
bus 4 -> HPM_MCAN4
bus 5 -> HPM_MCAN5
bus 6 -> HPM_MCAN6
bus 7 -> HPM_MCAN7
```

每路配置：

```text
Classic CAN
1 Mbps
CAN FD disabled
RX FIFO interrupt enabled
TX/error interrupt enabled
```

### 15.2 `initMask=0xFF` 不等于 8 路物理 CAN 已经可用

`initMask` 只说明 MCAN 控制器初始化成功。

真正能不能连外部电机，还取决于：

1. pinmux 是否把 MCAN_TX/RX 配到了正确引脚。
2. CAN 收发器是否使能。
3. CANH/CANL 是否接线正确。
4. 是否有终端电阻。
5. 是否和 EtherCAT/TSN/LCD 等引脚冲突。

当前 `hpm6e00_full_port` 自定义板级文件还没有 8 路 CAN pinmux，所以代码里提供了两个弱函数：

```c
void can_if_board_init_pins(uint8_t bus, MCAN_Type *base);
void can_if_board_set_transceiver(uint8_t bus, bool enable);
```

默认实现是空的。

新板要 8 路 CAN 全部可用时，需要在板级文件里强定义这两个函数。

### 15.3 新板 pinmux 应该在哪里补

建议在新板自己的板级源码里新增，例如：

```text
hpm_app/boards/hpm6e00_full_port/board_can.c
```

或者直接放到已有 `pinmux.c` / `board.c`。

示例结构：

```c
void can_if_board_init_pins(uint8_t bus, MCAN_Type *base)
{
    (void)base;

    switch (bus) {
    case 0:
        /* 配 MCAN0 TX/RX pinmux */
        break;
    case 1:
        /* 配 MCAN1 TX/RX pinmux */
        break;
    ...
    case 7:
        /* 配 MCAN7 TX/RX pinmux */
        break;
    default:
        break;
    }
}

void can_if_board_set_transceiver(uint8_t bus, bool enable)
{
    switch (bus) {
    case 0:
        /* 控制 CAN0 收发器 EN/STB */
        break;
    ...
    default:
        break;
    }
}
```

如果收发器没有 EN/STB，或者硬件默认常开，那么 `can_if_board_set_transceiver()` 可以保持空实现。

### 15.4 当前构建状态

当前已验证：

```powershell
cmake --build firmware/ecat_tsn_gateway/build
```

编译通过，并生成：

```text
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.elf
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.bin
```

