<!-- Author: BearKing -->
# FreeRTOS 架构与迁移状态

## 当前实现

固件已经从裸机超级循环迁移到 HPM SDK 1.11.0 提供的 FreeRTOS 端口：

- FreeRTOS tick 为 1 kHz，使用 full-port 板定义的 GPTMR6；
- 现有 EtherCAT 硬件层继续使用 GPTMR0 维护 SSC 毫秒计时；
- 原 `board_timer_create()` 使用的 GPTMR3 不再初始化；
- `gateway_ethercat_task` 每 1 ms 单独执行 `MainLoop()`；
- `gateway_motor_task` 每 1 ms 轮询 CAN，当前每 1 ms 发送一次控制帧；
- `gateway_imu_task` 每 1 ms 从 UART12 的 HDMA 环形缓冲区有界取数，解析完整
  YESENSE 帧后原子发布一份带接收 tick 的快照；
- `gateway_status_task` 每 1 秒负责 LED、IMU 和电机状态输出；
- 电机命令发送周期配置为 1 ms；
- CAN 中断使用 FreeRTOS syscall-safe 的优先级 2，EtherCAT PDI 中断保持独立的
  高优先级路径；
- RTOS heap 为 32 KiB，任务创建失败、heap 分配失败和栈溢出均进入明确的
  停机断点路径。

ESC PDI 中断可能直接调用 `APPL_OutputMapping()` 和
`APPL_InputMapping()`。因此命令读取不再返回共享数组指针，而是在短全局中断
临界区内复制完整的单轴命令；反馈写入也在短临界区内完成。命令更新使用单调
序号，避免 ISR 在“检查标志”和“清除标志”之间到达时丢失更新。

## 定时器职责

| 资源 | 职责 |
| --- | --- |
| GPTMR0 | EtherCAT SSC 超时与内部毫秒计时 |
| GPTMR6 | FreeRTOS 1 kHz 调度 tick |
| GPTMR3 | 未使用，原裸机应用定时器已移除 |
| UART12 | YESENSE IMU，PF16/TX、PF17/RX，460800 8N1 |
| HDMA 通道 30 | UART12 RX，2 KiB 非缓存循环缓冲区 |

这一阶段保留两个独立时间源是为了避免同时更换调度模型和 EtherCAT 底层驱动。
GPTMR0 不用于控制任务调度，GPTMR6 不替代 ESC Sync0/PDI 事件。

HDMA 中断当前由 IMU 驱动注册，优先级为 2；ISR 只清理通道 30 状态并记录回卷或
错误，不调用 FreeRTOS API。以后若其他模块也使用 HDMA 中断，必须合并共享 IRQ
分发，不能再注册第二个 `IRQn_HDMA` 处理函数。通道 30 已保留给 IMU RX。

## IMU 数据边界

当前驱动按资料包中的 YESENSE v2.3 串口格式接收：帧头为 `0x59 0x53`，载荷使用
ID/长度/TLV，校验覆盖 TID、载荷长度和载荷。已解析温度、加速度、角速度、归一化
磁场、原始磁场、欧拉角、四元数、采样时间戳和数据就绪时间戳；每个快照携带
`valid_fields`，没有在本帧中出现的数据不会伪装为有效值。

PF16 是板端 TX，应连接 IMU RX；PF17 是板端 RX，应连接 IMU TX，并共地。460800
来自厂商 STM32F103/F401 解码例程。若实物已被上位机改成其他波特率，必须同步
修改 `IMU_UART_BAUD_RATE`。UART 初始化后、HDMA 启动前，固件会通过 YESENSE 交互
协议只读查询产品/软件版本、波特率、输出频率、输出内容和算法模式，并打印到调试
串口。查询失败不会阻止实时接收；固件不写 RAM 或 Flash 配置，仍使用传感器已有的
输出内容与频率。

## IMU EtherCAT TxPDO 桥接

SSC 当前生成的 SM3 输入过程数据为 252 字节。`APPL_InputMapping()` 必须严格按
生成的 `0x1A00..0x1A0F` 顺序输出，不能继续使用原来的 192 字节纯电机布局：

| 字节偏移 | 长度 | 对象 | 内容 |
| ---: | ---: | --- | --- |
| 0 | 4 | `0x3000` | IMU 状态位 |
| 4 | 4 | `0x3001` | `valid_fields` |
| 8 | 4 | `0x3002` | FreeRTOS 接收 tick |
| 12 | 4 | `0x3003` | IMU 采样时间，微秒 |
| 16 | 12 | `0x3004..0x3006` | 加速度 XYZ，m/s^2 |
| 28 | 12 | `0x3007..0x3009` | 角速度 XYZ，deg/s |
| 40 | 16 | `0x3010..0x3013` | 四元数 q0..q3 |
| 56 | 4 | `0x3014` | 温度，摄氏度 |
| 60 | 192 | `0x6000..0x6F02` | 16 轴位置、速度、扭矩反馈 |

`ImuStatus` 位定义为：bit0 已收到有效样本，bit1 曾发生校验错误，bit2 曾发生格式
错误，bit3 曾发生 DMA 环形缓冲区溢出，bit4 曾发生 DMA 错误。没有 IMU 样本时
EtherCAT 仍可进入 OP，状态 bit0 为 0，数据字段为 0。

本次 PDO 布局相对旧版 192 字节布局不兼容，但当前 ESI 的 `RevisionNo` 仍为 1。
正式发布前应在 SSC Project 中递增 Revision Number 后重新生成全部产物。当前联调若
暂不递增，主站必须删除旧 ESI/设备缓存、导入新 `ECAT_IO.xml` 并重新扫描从站，
否则主站可能继续使用旧的 192 字节过程映像。

## 尚未完成的多轴实时路径

当前仍不能把“固件已编译”解释为“16 轴端到端实时能力已验证”：

- `CAN_IF_ENABLED_BUS_MASK` 当前仅启用 CAN0；
- `motor_config.c` 当前在 CAN0 启用 axis0..3，共四个电机；
- CAN 发送仍使用 `mcan_transmit_blocking()`；
- 其余 3 路 CAN、每路 4 个电机的最终 ID 和物理连接尚未验证；
- RDK 主站已支持 252 字节输入解析和 320 字节全零安全输出，但尚未在 RDK/SOEM
  实机完成链接、OP 联调和 1 ms 抖动测量；
- IMU 信息查询已经实现，但具体型号、实物波特率和输出频率仍需以上板打印为准；

在启用全部 16 轴前，应先为每路 CAN 建立发送与反馈负载预算。当前 CAN0 四电机采用
1 kHz 控制发送；扩展到四路总线或进一步提高频率前，必须基于实际 standard/extended 帧、
反馈频率、仲裁延迟和最坏发送耗时重新验证，不能只按理论带宽判断。

## 验证

使用独立构建目录执行：

```powershell
cmake -S firmware/ecat_tsn_gateway `
  -B firmware/ecat_tsn_gateway/build_freertos `
  -G Ninja `
  -DBOARD=hpm6e00_full_port `
  -DHPM_BUILD_TYPE=flash_xip `
  -DCMAKE_BUILD_TYPE=debug `
  -DBOARD_SEARCH_PATH=D:\Tools\hpm\sdk_env-main\hpm_app\boards `
  -Dpython_exec=D:\Tools\hpm\sdk_env-main\tools\python3\python.exe

ninja -C firmware/ecat_tsn_gateway/build_freertos
```

YESENSE 流式解析器可在主机独立验证：

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -Ifirmware/ecat_tsn_gateway/protocols/yesense `
  firmware/ecat_tsn_gateway/protocols/yesense/yesense_parser.c `
  firmware/ecat_tsn_gateway/tests/yesense_parser_test.c `
  -o yesense_parser_test.exe
```

YESENSE 启动期查询协议可独立验证：

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -Ifirmware/ecat_tsn_gateway/protocols/yesense `
  firmware/ecat_tsn_gateway/protocols/yesense/yesense_query.c `
  firmware/ecat_tsn_gateway/tests/yesense_query_test.c `
  -o yesense_query_test.exe
```

2026-07-29 TSN 三端口硬件交换固定启用时的交叉编译结果：

| 内存区域 | 使用量 | 占比 |
| --- | ---: | ---: |
| FLASH | 184568 B | 1.10% |
| ILM | 9504 B | 3.63% |
| DLM | 32 KiB | 12.50% |
| AXI SRAM | 97488 B | 18.59% |
| AXI SRAM 非缓存区 | 2 KiB | 0.78% |
| AHB SRAM | 20 KiB | 62.50% |

目标板仍需验证 FreeRTOS 启动、EtherCAT OP 状态、1 ms 周期抖动、任务栈水位、
CAN 超时停机、IMU 连续帧校验/丢帧统计、IMU 新鲜度和长时间运行稳定性。
