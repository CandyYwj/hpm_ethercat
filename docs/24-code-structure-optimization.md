<!-- Author: BearKing -->
# 当前代码结构与后续优化指南

本文档记录 `firmware/ecat_tsn_gateway` 当前代码结构、关键数据流、模块职责、边界约束和后续优化路线。目标不是重新设计整个项目，而是让后续对实时性、CAN 调度、PDO 映射和模块扩展的深度优化有一个清晰的工程地图。

## 1. 当前系统定位

当前固件已经不再只是最初的 EtherCAT 到 TSN 原始以太网桥。当前主路径是：

```text
EtherCAT Master
  -> ESC/SSC
  -> EtherCAT RxPDO command objects
  -> gateway_app command cache
  -> motor_manager 1kHz control scheduler
  -> ENCOS/RS06 protocol dispatch
  -> can_if HPM MCAN backend
  -> CAN motor bus

CAN motor feedback
  -> can_if RX ring
  -> motor_manager decode/update
  -> gateway_app feedback objects
  -> EtherCAT TxPDO
  -> EtherCAT Master

YESENSE IMU
  -> UART12 + HDMA circular buffer
  -> yesense_parser
  -> imu latest sample/stats
  -> gateway_app IMU PDO prefix
  -> EtherCAT TxPDO
```

TSW 已作为独立三端口二层交换机启用。交换流量由硬件在 Port1/2/3 之间转发，不进入
CPU，也不与 EtherCAT PDO 互相转换。当前工作模式应按“EtherCAT 电机控制 + IMU
TxPDO 输入 + 独立 TSN 交换机”理解。

## 2. 顶层目录结构

```text
hpm_gateway/
  firmware/ecat_tsn_gateway/
    main.c                            板级入口和 FreeRTOS scheduler 启动
    CMakeLists.txt                    HPM SDK 构建入口
    ecat_config.h                     EtherCAT EEPROM/timer/flash 参数
    FreeRTOSConfig.h                  RTOS 配置
    app/                              系统初始化、EtherCAT SSC 回调和 PDO 缓存
    tasks/                            EtherCAT、电机、IMU、状态任务和任务配置
    drivers/can/                      CAN 抽象接口与 HPM MCAN 后端
    drivers/imu/                      YESENSE UART/HDMA 驱动
    drivers/tsn/                      三端口 TSN 硬件交换初始化
    protocols/encos/                  ENCOS CAN 协议纯编解码
    protocols/rs06/                   RS06 CAN 协议纯编解码
    protocols/yesense/                YESENSE 数据和查询协议解析
    motor/                            电机轴配置与 RS06 调度
    SSC/                              Beckhoff SSC 和对象字典边界
    tests/                            主机侧纯逻辑测试
  host/
    rdk_ecat_motor_rt/                当前 SOEM 周期主站和 PDO 布局测试
  docs/                               设计、协议、调试和结构文档
```

## 3. 构建结构

固件构建入口是 `firmware/ecat_tsn_gateway/CMakeLists.txt`。

关键构建点：

- `CONFIG_FREERTOS=1`：当前工程使用 FreeRTOS。
- `CONFIG_EEPROM_EMULATION=1`：启用 EEPROM flash mirror。
- TSN switch 固件始终编译并在系统启动时初始化。
- `sdk_src(...)`：固件库侧源码。
- `sdk_app_src(...)`：应用侧源码。
- `SSC/Src/digital_io.c` 不参与编译，当前由 `app/gateway_app.c` 提供 `APPL_*` 回调。

默认构建命令形态：

```powershell
cd firmware/ecat_tsn_gateway
cmake -S . -B build_freertos -G Ninja `
  -DBOARD=hpm6e00_full_port `
  -DHPM_BUILD_TYPE=flash_xip `
  -DCMAKE_BUILD_TYPE=debug `
  -DBOARD_SEARCH_PATH=D:\Tools\hpm\sdk_env-main\hpm_app\boards
ninja -C build_freertos
```

## 4. 运行任务结构

`main.c` 只完成板级初始化、创建顶层 EtherCAT 任务和启动 scheduler。运行任务位于
`tasks/`，系统外设初始化位于 `app/gateway_system.c`。

### 4.1 `main()`

职责：

- `board_init()`
- `full_port_lcd_init()`
- 创建顶层 EtherCAT 任务
- 启动 FreeRTOS scheduler

`main()` 不包含外设初始化、协议栈轮询、电机控制或状态打印。

### 4.2 `gateway_ethercat_task`

周期：`1ms`

职责：

- 调用 `gateway_system_init()`
- 启动 EtherCAT application：`bRunApplication = TRUE`
- 每周期调用 `MainLoop()`，维护 SSC/ESM/mailbox/PDO 相关逻辑
- 使用 `vTaskDelayUntil()` 固定 1ms 节拍

这个任务不调用电机控制和状态打印。

### 4.3 `gateway_motor_task`

轮询周期：`1ms`；控制帧周期：`MOTOR_MANAGER_CONTROL_PERIOD_MS`，当前 `1ms`。

职责：调用 `motor_manager_poll()`，并在任务退出时执行 `motor_manager_stop_all()`。

### 4.4 `gateway_status_task`

周期：`GATEWAY_STATUS_PERIOD_MS`，当前 1000ms。

职责：

- 翻转 LEDA
- 调用 `motor_manager_print_status()` 和 `imu_print_status()`

此任务不改变控制输出。

### 4.5 `gateway_imu_task`

周期：`1ms`

职责：

- 从 HDMA circular buffer 消费 UART 数据
- 调用 `yesense_parser_push()`
- 发布最新 IMU 样本
- 更新统计计数

此任务是 IMU 数据接收路径的拥有者。`gateway_app` 仅读取 IMU 最新样本并映射进 TxPDO。

## 5. 核心模块职责

### 5.1 `app/gateway_app.c`

角色：EtherCAT SSC 应用层和 PDO 数据拥有者。

主要职责：

- 实现 SSC 回调：
  - `APPL_GenerateMapping`
  - `APPL_InputMapping`
  - `APPL_OutputMapping`
  - state transition callbacks
- 将 EtherCAT RxPDO 写入 `s_motor_commands`
- 定义 SSC 生成的对象字典变量；`_DIGITAL_IO_` 只在本翻译单元生效
- 将 motor/IMU 状态写入 EtherCAT TxPDO 对象
- 提供电机命令/反馈访问 API：
  - `gateway_app_get_motor_command`
  - `gateway_app_get_motor_command_sequence`
  - `gateway_app_set_motor_feedback`

所有与 EtherCAT 对象字典变量直接交互的逻辑应尽量集中在这里。其他模块不要直接读写 `TargetPosition00x7010` 这类 SSC 对象变量。

### 5.2 `motor/motor_manager.c`

角色：EtherCAT 命令到 ENCOS/RS06 CAN 输出的调度器。

主要职责：

- 从 `gateway_app` 抓取命令快照
- 根据 `motor_config` 找轴配置
- 按轴配置编码 ENCOS、RS06 private 或 RS06 MIT CAN 帧
- 从 `can_if` 拉取 CAN RX 帧
- 解码电机反馈和 RS06 故障报告
- 把反馈写回 `gateway_app`
- 执行命令超时停机保护

当前调度节拍：

- `MOTOR_MANAGER_CONTROL_PERIOD_MS`
- `MOTOR_MANAGER_COMMAND_TIMEOUT_MS`

后续深度优化时，这里是最关键的热点之一。尤其需要按 CAN bus 建立预算，而不是只按轴数估算。

### 5.3 `motor/motor_config.c`

角色：轴到 CAN bus / motor id / host id / 方向的映射表。

当前状态：

- 保留 16 轴映射
- 当前启用 axis0..3，均使用 CAN0
- axis0..2 为 ENCOS id1..3，axis3 为 RS06 MIT id4

建议后续演进方向：

- 把 bring-up 配置、整机配置、生产配置拆成 profile
- 避免在 `motor_config.c` 里手动频繁改 `enabled`
- 当前已检查 bus 范围和初始化状态、motor ID、协议范围及同一 bus 上的重复 ID

### 5.4 `drivers/can/`

角色：硬件无关 CAN API 和 HPM MCAN 后端。

公共 API：

- `can_if_init`
- `can_if_send`
- `can_if_receive`
- `can_if_get_tx_count`
- `can_if_get_rx_count`
- `can_if_get_error_count`
- `can_if_get_drop_count`
- `can_if_get_initialized_mask`

当前重要实现点：

- Classic CAN
- 默认 `CAN_IF_DEFAULT_BITRATE_BPS = 1000000`
- `CAN_IF_ENABLED_BUS_MASK` 默认 `0x01`，只启用 CAN0
- RX ring per bus
- `can_if_receive()` 已改为 round-robin，避免 bus0 长期优先

后续优化重点：

- 给每路 bus 增加独立 RX/TX 预算统计
- 避免 `mcan_transmit_blocking()` 在高频控制周期中造成不可控阻塞
- 建立发送失败/仲裁延迟/错误被动/bus-off 的恢复策略
- 根据 4 电机/路、1Mbps CAN 的目标负载建立控制频率表

### 5.5 `protocols/rs06/rs06_proto.*`

角色：纯协议编解码层。

这个模块不应：

- 初始化 CAN
- 访问 EtherCAT 对象
- 维护调度状态
- 打印运行日志
- 直接知道 FreeRTOS

这个模块应保持可在主机侧单元测试。后续扩展协议时，优先补测试：

- float 到 uint 饱和边界
- private extended id 解析
- MIT standard frame 编码
- DLC/id_format/frame_format 错误输入
- fault/param/device id 响应解析

### 5.6 `drivers/imu/` 与 `protocols/yesense/`

角色：YESENSE IMU 驱动与解析。

主要文件：

- `tasks/gateway_imu_task.c`：IMU 接收任务的周期与生命周期
- `drivers/imu/imu.c`：UART12、HDMA、有限量接收处理、状态发布和状态打印
- `drivers/imu/imu.h`：对外样本/统计 API
- `protocols/yesense/yesense_parser.*`：流式数据帧解析
- `protocols/yesense/yesense_query.*`：设备信息查询帧构造与响应解析
- `protocols/yesense/yesense_types.h`：与硬件无关的样本类型

对外 API：

- `imu_init`
- `imu_process_rx`
- `imu_get_latest`
- `imu_get_stats`
- `imu_print_status`

当前数据所有权：

- DMA buffer 归 `imu.c`
- parser 状态归 `imu.c`
- latest sample 归 `imu.c`
- stats 归 `imu.c`
- PDO 映射只通过 `imu_get_latest()` 和 `imu_get_stats()` 获取快照

后续优化重点：

- 为 `imu_dma_read()` 建立更明确的 overflow 行为记录
- 给 DMA wrap/count 与 parser reset 增加测试或硬件验证记录
- 把设备信息查询结果缓存成结构体，而不是只打印
- 明确 IMU 输出频率与 PDO 周期之间的采样/保持关系

### 5.7 `drivers/tsn/tsn_switch.*`

角色：初始化 TSW 为独立三端口二层交换机。

当前行为：

- 广播帧泛洪到 Port1/2/3
- 未知单播泛洪到 Port1/2/3
- CPU Port 不在转发目的掩码中
- 不配置 TSN RX/TX DMA
- 不注册 TSN 中断，不需要 FreeRTOS 任务
- EtherCAT PDO 和 TSN Ethernet 帧之间不存在数据转换

### 5.8 `SSC/Src/`

角色：Beckhoff SSC、对象字典、协议栈边界。

重要事实：

- `SSC/Src/digital_io.c` 不编译
- `SSC/Src/digital_ioObjects.h` 是对象字典和 PDO mapping 的事实来源
- `app/gateway_app.c` 替代原例程的 APPL callbacks

不要随手清理整个 SSC 目录。后续改 PDO mapping 时，建议：

- 先写清对象布局设计
- 再通过脚本或生成流程修改 `digital_ioObjects.h`
- 同步 ESI/XML/EEPROM image
- 用主站读 mapping 验证

## 6. 当前主要数据流

### 6.1 EtherCAT master 到电机

```text
RDK/SOEM master writes RxPDO
  -> SSC receives output data
  -> APPL_OutputMapping(pData)
  -> copy REAL32 target fields into SSC objects
  -> sync_command_cache(axis)
  -> s_motor_commands[axis]
  -> s_motor_command_sequence++
  -> motor_manager_poll()
  -> gateway_app_get_motor_command(axis)
  -> encode_motion_for_axis()
  -> can_if_send()
  -> HPM MCAN
```

关键约束：

- `s_motor_command_sequence` 是 motor manager 判断是否收到新命令的依据
- 命令超时由 `MOTOR_MANAGER_COMMAND_TIMEOUT_MS` 控制
- 当前直接控制开关为 `MOTOR_MANAGER_ENABLE_MOTION_CONTROL`

### 6.2 电机到 EtherCAT master

```text
MCAN interrupt
  -> can_if_rx_from_fifo()
  -> per-bus rx ring
  -> motor_manager_poll()
  -> can_if_receive()
  -> protocol-specific feedback decode
  -> handle_encos_feedback()/handle_rs06_mit_feedback()
  -> gateway_app_set_motor_feedback()
  -> SSC feedback objects
  -> APPL_InputMapping()
  -> EtherCAT TxPDO
```

关键约束：

- `can_if_receive()` 是轮询读取所有 bus 的公共出口
- 当前每次 `motor_manager_poll()` 最多处理 `MOTOR_MANAGER_RX_BUDGET_PER_POLL` 帧
- 如果后续 8 路 CAN 全开，需要重新评估 RX budget 是否足够

### 6.3 IMU 到 EtherCAT master

```text
UART12 RX
  -> HDMA circular buffer
  -> imu_dma_read()
  -> yesense_parser_push()
  -> imu_publish_sample()
  -> gateway_app sync_imu_objects()
  -> IMU PDO prefix 0x3000..0x3014
  -> APPL_InputMapping()
  -> EtherCAT TxPDO
```

关键约束：

- IMU 最新样本是保持型数据，不是每个 PDO 周期必然新样本
- `ImuStatus0x3000` 用 bit 标记 sample/checksum/format/DMA 异常
- `ImuValidFields0x3001` 表示当前样本哪些字段有效

## 7. 状态所有权

| 状态 | 唯一拥有者 | 读取者 | 写入者 |
| --- | --- | --- | --- |
| EtherCAT command PDO objects | `gateway_app.c` / SSC object dictionary | `gateway_app.c` | `APPL_OutputMapping` |
| Motor command cache | `gateway_app.c` | `motor_manager.c` | `gateway_app.c` |
| Motor feedback PDO objects | `gateway_app.c` / SSC object dictionary | `APPL_InputMapping` | `motor_manager.c` through `gateway_app_set_motor_feedback()` |
| Axis config | `motor_config.c` | `motor_manager.c` | build-time static config |
| CAN RX rings | `can_if_hpm_mcan.c` | `motor_manager.c` through API | MCAN ISR |
| RS06 protocol frame fields | local stack values | `motor_manager.c` | `rs06_proto.c` |
| IMU latest sample/stats | `drivers/imu/imu.c` | `gateway_app.c`, status diagnostics | `gateway_imu_task`, DMA ISR |
| TSN forwarding state | TSW hardware | external Ethernet ports | `tsn_switch_init()` at startup |

原则：其他模块不要跨过 API 直接访问别人的静态状态。跨模块共享状态时，优先提供快照函数或明确的 set/get API。

## 8. 已做的结构优化

本轮及前置整理已经完成以下低风险改动：

- 删除 EtherCAT/TSN raw-frame 双向桥、DMA ring 和私有 60-byte 协议
- TSW 改为独立三端口硬件交换机，不占用运行任务
- `tasks/gateway_task_config.h` 集中任务周期、优先级和栈配置
- EtherCAT、电机、IMU 和状态输出拆为四个独立任务文件
- 驱动和协议已按职责归档
- CAN bitrate 改为 `CAN_IF_DEFAULT_BITRATE_BPS`
- `can_if_receive()` 改为 round-robin bus scan
- `imu_print_status()` 移入 IMU 模块，`main.c` 不再维护 IMU 诊断细节

EtherCAT 电机控制与 TSN 交换数据面完全独立。

## 9. 当前不合理点和建议处理顺序

### 9.1 系统初始化仍集中

现状：

- `main.c` 已只保留板级启动和 scheduler 编排
- EEPROM mirror、TSN switch、ESC/CAN/IMU 初始化集中在 `gateway_system.c`

建议：

1. 仅当 `gateway_system.c` 继续增长时再按 EEPROM、TSN 等硬件职责拆分
2. 保持初始化顺序由 `gateway_system_init()` 单点编排
3. 不把周期任务或协议编解码重新放回系统初始化

不要一次性拆太多。每拆一块都应保持构建可过。

### 9.2 PDO 对象字典维护成本高

现状：

- 16 轴对象和 mapping 在 `digital_ioObjects.h` 中展开
- 手动维护成本高

建议：

1. 先冻结当前 PDO layout 文档
2. 写一个生成脚本或表驱动模板
3. 由脚本生成 object dictionary 片段
4. 同步 ESI/XML/EEPROM image

### 9.3 CAN 调度还缺少负载模型

现状：

- 1ms 接收轮询，1ms 控制发送周期
- 每轴可能发送 motion frame
- RX 每周期预算固定
- 当前 CAN0 启用四个电机

建议：

1. 建立 `bus -> enabled axis count -> tx frames/s -> rx frames/s` 表
2. 对 1Mbps Classic CAN 按各协议实际 standard/extended frame 估算 bus load
3. 将控制帧、反馈帧、查询帧分开预算
4. 根据预算决定 4 电机/路时的控制频率和反馈频率

### 9.4 阻塞发送可能影响 1ms 周期

现状：

- `can_if_send()` 使用 `mcan_transmit_blocking()`

风险：

- 多轴同时发送时，阻塞时间可能吃掉控制周期
- bus error 或仲裁延迟时，周期抖动变大

建议：

1. 先测量 `can_if_send()` 最坏耗时
2. 再考虑非阻塞 TX queue
3. 不要在没有测量前直接引入复杂异步发送架构

### 9.5 IMU 查询和流接收耦合在启动阶段

现状：

- `imu_init()` 初始化 UART 后先 query device info，再启动 DMA
- query 过程中会同时看到 streaming frame

建议：

1. 缓存 query 结果到 `imu_device_info_t`
2. 增加 `imu_get_device_info()`
3. 将 query 超时不作为 IMU 启动失败，除非产品要求必须读到设备信息

### 9.6 配置 profile 还不明确

现状：

- CAN enabled mask 在 `can_if_hpm_mcan.c`
- 轴启用在 `motor_config.c`
- TSN switch 在系统启动时固定启用
- IMU UART/DMA 在 `imu.c`

建议：

1. 保持当前直接宏配置，不急着引入复杂配置系统
2. 先新增 `gateway_board_profile.h` 或 `gateway_product_config.h`
3. 只放真实产品差异：启用 CAN mask、轴配置 profile 和 IMU 开关

## 10. 后续深度优化路线

建议按以下顺序推进：

1. **固定行为基线**
   - 保存当前构建命令
   - 记录 PDO layout
   - 记录 CAN0 四电机 bring-up 行为
   - 记录 IMU 输出频率和错误计数

2. **补协议单元测试**
   - `protocols/rs06` 边界测试
   - `yesense_parser`/`yesense_query` 已有测试继续保留
   - CAN frame invalid input 测试

3. **建立实时预算**
   - 1ms task 中 `MainLoop()`、`motor_manager_poll()`、CAN send 的耗时
   - 每路 CAN bus load
   - RX ring drop/error 统计

4. **优化 CAN 调度**
   - 先做测量和日志
   - 再决定是否引入 TX queue
   - 最后再扩展多路多电机

5. **整理 PDO 生成流程**
   - 当前 mapping 先不要手工大改
   - 生成脚本和 ESI 同步后再扩轴或改字段

6. **拆入口职责**
   - IMU diagnostics 已移出
   - 后续拆 EEPROM/TSN/motor layout 打印
   - 保持 `main.c` 只做启动编排

## 11. 验证建议

每次结构改动至少跑：

```powershell
gcc -std=c99 -Wall -Wextra `
  -Ifirmware/ecat_tsn_gateway/protocols/yesense `
  firmware/ecat_tsn_gateway/protocols/yesense/yesense_parser.c `
  firmware/ecat_tsn_gateway/tests/yesense_parser_test.c `
  -o tmp/yesense_parser_test.exe

gcc -std=c99 -Wall -Wextra `
  -Ifirmware/ecat_tsn_gateway/protocols/yesense `
  firmware/ecat_tsn_gateway/protocols/yesense/yesense_query.c `
  firmware/ecat_tsn_gateway/tests/yesense_query_test.c `
  -o tmp/yesense_query_test.exe

.\tmp\yesense_parser_test.exe
.\tmp\yesense_query_test.exe
ninja -C firmware/ecat_tsn_gateway/build_freertos
```

主机测试和固件编译通过不等于目标板实时行为通过。涉及 CAN 周期、IMU DMA、
EtherCAT 状态机、电机安全输出和 TSN 三端口转发时，还必须做目标板验证。
