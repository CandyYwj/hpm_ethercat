<!-- Author: BearKing -->
# RS06 电机协议接入计划：解耦架构与性能优化

> 当前状态：EtherCAT 侧已经改成 16 轴 MIT 风格 PDO。
> 主站下发 `16 * 5 * float32 = 320 bytes`，从站反馈 `16 * 3 * float32 = 192 bytes`。
> 本计划只解决下一步：把 RS06 电机通过 CAN 接到这个 16 轴控制模型下面。

## 1. 总体结论

推荐先接入 **RS06 私有协议 29-bit 扩展帧**，不要第一步就强制把电机切到 MIT 协议模式。

原因：

1. RS06 默认就是私有协议，接上电机后不需要先永久切协议，调试风险更低。
2. 私有协议已经支持 MIT 风格运控帧：`position + velocity + torque_ff + kp + kd`，刚好能承接现在 EtherCAT PDO 的 5 个控制量。
3. 私有协议还支持使能、停止、清错、读写参数、保存参数、设置 CAN ID，后续功能更完整。
4. 真正的 EtherCAT 对象字典不用再大改，CAN 层只需要读取 `gateway_app_get_motor_command()`，然后把反馈写回 `gateway_app_set_motor_feedback()`。

最终结构应该是：

```text
EtherCAT master
  |
  | RxPDO: 16-axis command, 320 bytes
  v
gateway_app.c
  |
  | generic motor command / feedback
  v
motor_manager.c
  |
  | axis schedule, state machine, safety, timeout
  v
rs06_motor.c
  |
  | RS06 command model <-> RS06 CAN frames
  v
rs06_proto.c
  |
  | pure encode/decode, no hardware dependency
  v
can_if_hpm_mcan.c
  |
  | HPM MCAN hardware
  v
RS06 motors
```

## 2. 不要把 RS06 代码放进 `gateway_app.c`

`gateway_app.c` 现在的职责应该保持很干净：

- `APPL_OutputMapping()`：把主站 PDO 数据拆成 16 个电机命令。
- `APPL_InputMapping()`：把 16 个电机反馈打包给主站。
- `gateway_app_get_motor_command()`：给应用层读取命令。
- `gateway_app_set_motor_feedback()`：给应用层写入反馈。

它不应该知道：

- CAN 有几路。
- RS06 是扩展帧还是标准帧。
- CAN ID 怎么编码。
- 电机是否在线。
- 电机怎么使能、停止、清错。

原因是 SSC Tool 生成的文件和 EtherCAT PDO 后面还可能再变，如果把 RS06 协议写进 `gateway_app.c`，以后改对象字典时很容易把电机协议也带乱。

## 3. 新增目录和文件

建议新增目录：

```text
firmware/ecat_tsn_gateway/motor/
  motor_types.h
  motor_config.h
  motor_config.c
  motor_manager.h
  motor_manager.c
  motor_safety.h
  motor_safety.c

firmware/ecat_tsn_gateway/can_if/
  can_if.h
  can_if_hpm_mcan.h
  can_if_hpm_mcan.c

firmware/ecat_tsn_gateway/rs06/
  rs06_proto.h
  rs06_proto.c
  rs06_motor.h
  rs06_motor.c
```

各文件职责如下：

| 文件 | 职责 | 为什么这样拆 |
|---|---|---|
| `motor_types.h` | 定义通用电机命令、反馈、状态、错误码 | 不绑定 RS06，后续 ENCOS 或其他电机也能复用 |
| `motor_config.*` | 16 轴配置表：轴号、CAN bus、RS06 CAN ID、方向、限幅 | 配置集中管理，避免散落在代码里 |
| `motor_manager.*` | 16 轴调度、状态机、超时、安全保护 | 它是 EtherCAT 和电机协议之间的中间层 |
| `motor_safety.*` | 命令限幅、掉线处理、故障聚合 | 安全逻辑独立，便于审查和测试 |
| `can_if.h` | 抽象 CAN 帧和 CAN 发送接收接口 | 上层不依赖 HPM SDK 具体 API |
| `can_if_hpm_mcan.*` | HPM6E80 MCAN 初始化、发送、接收中断、错误统计 | 硬件相关代码集中在一处 |
| `rs06_proto.*` | RS06 帧编解码，纯函数 | 可以在 PC 上单元测试，不碰硬件 |
| `rs06_motor.*` | RS06 电机状态机：使能、停止、清错、运控、解析反馈 | 协议动作和单个电机状态放在一起 |

## 4. 通用数据模型

当前 EtherCAT PDO 已经是 MIT 风格，所以内部也用同样的通用结构：

```c
typedef struct {
    float target_position;
    float target_velocity;
    float feedforward_torque;
    float kp;
    float kd;
} motor_command_t;

typedef struct {
    float actual_position;
    float actual_velocity;
    float actual_torque;
    float temperature;
    uint32_t fault;
    uint32_t timestamp_ms;
    bool online;
} motor_feedback_t;
```

注意：EtherCAT PDO 现在只反馈 3 个 float：位置、速度、力矩。

所以 `temperature / fault / online` 暂时不进 PDO，可以先用于串口打印和内部保护。后面如果主站需要看这些诊断量，再通过 SDO 或扩展 TxPDO 加进去。

## 5. RS06 协议接入方式

### 5.1 第一阶段使用 RS06 私有协议

RS06 私有协议使用 29-bit 扩展帧：

```text
CAN ID bits [28:24] = 通信类型
CAN ID bits [23:8]  = 数据区，含主机 ID / 力矩 / 故障 / 模式等
CAN ID bits [7:0]   = 目标电机 CAN ID
Data[0:7]           = 与通信类型相关的 8 字节数据
```

最重要的控制帧是通信类型 `0x01`：

```text
主控 -> 电机：运控模式

CAN ID:
  type = 0x01
  data = torque_ff 的 uint16 编码值
  id   = motor_id

Data:
  Byte0-1 = target_position，uint16，范围 -12.57 ~ +12.57 rad
  Byte2-3 = target_velocity，uint16，范围 -50 ~ +50 rad/s
  Byte4-5 = kp，uint16，范围 0 ~ 5000
  Byte6-7 = kd，uint16，范围 0 ~ 100
```

反馈帧是通信类型 `0x02`：

```text
电机 -> 主控：状态反馈

CAN ID:
  type = 0x02
  data = 电机 ID + 故障位 + 模式状态
  id   = host_id

Data:
  Byte0-1 = actual_position，uint16，范围 -12.57 ~ +12.57 rad
  Byte2-3 = actual_velocity，uint16，范围 -50 ~ +50 rad/s
  Byte4-5 = actual_torque，uint16，范围 -36 ~ +36 Nm
  Byte6-7 = temperature * 10
```

这样做的好处是：EtherCAT 下发的 5 个 float 可以直接转成 RS06 私有运控帧，不需要改主站，也不需要改 PDO。

### 5.2 第二阶段再支持 RS06 MIT 11-bit 标准帧

等私有协议跑通后，可以增加一个可选后端：

```c
typedef enum {
    RS06_PROTOCOL_PRIVATE = 0,
    RS06_PROTOCOL_MIT_11BIT = 1,
} rs06_protocol_mode_t;
```

这样同一个 `rs06_motor.c` 可以根据配置选择：

- 私有协议：默认推荐，功能完整，适合调试。
- MIT 11-bit：帧更短，可能节省总线时间，但需要确认电机已经切换协议并重启。

不要第一步就上 MIT 11-bit，因为协议切换需要断电重启，调试复杂度更高。

## 6. 关键接口设计

### 6.1 CAN 抽象层

`can_if.h`：

```c
typedef enum {
    CAN_ID_STANDARD = 0,
    CAN_ID_EXTENDED = 1,
} can_id_type_t;

typedef struct {
    uint8_t bus;
    can_id_type_t id_type;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t timestamp_us;
} can_frame_t;

bool can_if_init(void);
bool can_if_send(const can_frame_t *frame);
bool can_if_receive(can_frame_t *frame);
uint32_t can_if_get_tx_count(uint8_t bus);
uint32_t can_if_get_rx_count(uint8_t bus);
uint32_t can_if_get_error_count(uint8_t bus);
```

这一层只管 CAN，不知道 RS06。

### 6.2 RS06 编解码层

`rs06_proto.h`：

```c
typedef struct {
    uint8_t motor_id;
    uint8_t host_id;
    float position;
    float velocity;
    float torque;
    float kp;
    float kd;
} rs06_control_t;

typedef struct {
    uint8_t motor_id;
    float position;
    float velocity;
    float torque;
    float temperature;
    uint32_t fault;
    uint8_t mode;
} rs06_feedback_t;

bool rs06_encode_enable(uint8_t motor_id, uint8_t host_id, can_frame_t *frame);
bool rs06_encode_stop(uint8_t motor_id, uint8_t host_id, bool clear_fault, can_frame_t *frame);
bool rs06_encode_control_private(const rs06_control_t *cmd, can_frame_t *frame);
bool rs06_decode_feedback_private(const can_frame_t *frame, rs06_feedback_t *feedback);
```

这一层只做协议编解码，不操作数组、不操作主循环、不操作 MCAN 寄存器。

### 6.3 单电机适配层

`rs06_motor.h`：

```c
typedef enum {
    RS06_STATE_DISABLED = 0,
    RS06_STATE_INIT,
    RS06_STATE_ENABLE_SENT,
    RS06_STATE_RUNNING,
    RS06_STATE_FAULT,
    RS06_STATE_OFFLINE,
} rs06_motor_state_t;

typedef struct {
    uint8_t axis;
    uint8_t bus;
    uint8_t motor_id;
    uint8_t host_id;
    rs06_protocol_mode_t protocol;
    rs06_motor_state_t state;
    uint32_t last_rx_ms;
    uint32_t last_tx_ms;
    uint32_t fault;
} rs06_motor_t;

void rs06_motor_init(rs06_motor_t *motor, const motor_axis_config_t *cfg);
bool rs06_motor_make_control_frame(rs06_motor_t *motor,
                                   const motor_command_t *cmd,
                                   can_frame_t *frame);
bool rs06_motor_handle_frame(rs06_motor_t *motor,
                             const can_frame_t *frame,
                             motor_feedback_t *feedback);
bool rs06_motor_make_enable_frame(rs06_motor_t *motor, can_frame_t *frame);
bool rs06_motor_make_stop_frame(rs06_motor_t *motor, can_frame_t *frame);
```

### 6.4 16 轴管理层

`motor_manager.h`：

```c
bool motor_manager_init(void);
void motor_manager_poll_1ms(void);
void motor_manager_on_can_rx(const can_frame_t *frame);
void motor_manager_stop_all(void);
void motor_manager_print_status(void);
```

主循环里只加一行：

```c
while (bRunApplication == TRUE) {
    MainLoop();
    motor_manager_poll_1ms();
    ...
}
```

这样 `main.c` 不需要知道 RS06 的细节。

## 7. 16 轴总线分组与性能预算

RS06 默认 CAN 2.0、1Mbps、8 字节数据帧。这个限制非常重要。

每个电机每个控制周期至少需要：

```text
1 帧控制：主控 -> 电机
1 帧反馈：电机 -> 主控
```

也就是每轴每周期 2 帧。

扩展帧 8 字节 CAN 2.0 实际线上时间粗略按 `130~160 bit` 估算，考虑位填充、仲裁、帧间隔后更接近上限。

所以：

```text
单轴每周期 ≈ 2 * 150 bit = 300 bit
1Mbps 下单轴 ≈ 300 us
16 轴单总线 ≈ 4.8 ms，不可能 1kHz
```

推荐分组：

| 目标周期 | 推荐总线数 | 每路电机数 | 说明 |
|---|---:|---:|---|
| 1ms 真全量控制 | 8 路 CAN | 2 轴/路 | 最稳，留出错误重发和调度余量 |
| 1ms 勉强可测 | 4 路 CAN | 4 轴/路 | 标准帧可能接近，扩展帧风险大 |
| 2ms 控制周期 | 4 路 CAN | 4 轴/路 | 比较合理 |
| 4ms 控制周期 | 2 路 CAN | 8 轴/路 | 可用于低速验证 |
| 8ms 以上 | 1 路 CAN | 16 轴/路 | 只适合功能验证，不适合高动态控制 |

如果新板子真的引出了 8 路 MCAN，建议一开始就按：

```text
MCAN0: Axis 0, 1
MCAN1: Axis 2, 3
MCAN2: Axis 4, 5
MCAN3: Axis 6, 7
MCAN4: Axis 8, 9
MCAN5: Axis 10, 11
MCAN6: Axis 12, 13
MCAN7: Axis 14, 15
```

如果硬件只方便用 4 路 CAN，则建议先把 `motor_manager` 控制周期设成 2ms，不要强行 1ms。

## 8. 性能优化策略

### 8.1 运行期不动态分配内存

所有对象静态分配：

```c
static rs06_motor_t s_motors[16];
static motor_command_t s_command_shadow[16];
static motor_feedback_t s_feedback_shadow[16];
static can_frame_t s_tx_queue[8][16];
```

原因：控制周期内不能 `malloc/free`，否则时延不可控。

### 8.2 CAN ID 和缩放系数预计算

初始化时预计算：

- enable frame ID
- stop frame ID
- control frame base ID
- motor_id -> axis 查找表
- float 转 uint 的 scale 和 offset

周期内只做：

```text
限幅 -> 乘法 -> 转整数 -> 写 8 字节
```

不要在 1ms 周期里查表、拼字符串、打印大量日志。

### 8.3 中断只搬运数据，不做协议逻辑

MCAN RX ISR 只做：

1. 从 MCAN RX FIFO 取帧。
2. 放入 lock-free 或关中断保护的 ring buffer。
3. 计数。
4. 退出中断。

协议解析放在 `motor_manager_poll_1ms()` 或更高优先级任务里做。

原因：如果 ISR 里解析协议、写 EtherCAT 对象、打印日志，会影响 EtherCAT 主循环。

### 8.4 命令快照，避免读到半包

`APPL_OutputMapping()` 会更新 16 轴命令。`motor_manager` 读取时建议使用快照接口：

```c
bool gateway_app_copy_motor_commands(motor_command_t dst[16]);
```

或者在 `motor_manager_poll_1ms()` 开头一次性复制 16 轴命令到本地 shadow。

原因：不要在发送 16 帧 CAN 的过程中，一边读一边被新的 PDO 覆盖。

### 8.5 非 OP 状态不发运控帧

只有满足下面条件才允许发控制帧：

```text
EtherCAT AL state == OP
bEcatOutputUpdateRunning == TRUE
收到过主站 Output PDO
电机状态 == RUNNING
命令未超限
CAN 总线未 bus-off
```

否则发停止帧或保持 disabled。

### 8.6 打印降频

周期控制里禁止每帧打印。

建议 1 秒打印一次汇总：

```text
MOTOR: op=1 cmdHz=1000 canTx=32000 canRx=31880 off=0 fault=0 buserr=0
BUS0: tx=4000 rx=3998 err=0 max_rx_gap=2ms
Axis0: id=1 state=RUNNING p=... v=... t=...
```

调单个电机时可以开启详细日志，但必须用宏控制：

```c
#define MOTOR_DEBUG_VERBOSE 0
```

## 9. 安全策略

必须先实现这些保护，再接真实负载：

| 条件 | 动作 | 原因 |
|---|---|---|
| EtherCAT 未到 OP | 不使能电机 | 防止主站未控制时电机动作 |
| 主站 Output 超过 100ms 未更新 | 停止所有电机 | 防止主站掉线后保持旧命令 |
| 单轴 CAN 反馈超过 50ms 未更新 | 标记 offline，停止该轴 | 防止电机失联 |
| CAN bus-off | 停止该总线所有轴，尝试恢复 CAN，但不自动重新使能 | bus-off 通常表示物理层或接线问题 |
| 命令超出范围 | 限幅，计数，必要时拒绝使能 | 防止主站参数错误 |
| RS06 故障位非 0 | 停止该轴，等待显式清错 | 电机保护优先 |
| 温度超过报警阈值 | 降额或停止 | 防止过温 |

建议默认限幅：

```text
position: -12.57 ~ +12.57 rad
velocity: -50 ~ +50 rad/s
torque_ff: -36 ~ +36 Nm
kp: 0 ~ 5000
kd: 0 ~ 100
```

## 10. 接入主循环的位置

`main.c` 初始化顺序建议：

```text
board_init()
board_timer_create()
EtherCAT/ESC init
MainInit()
APPL_GenerateMapping()
can_if_init()
motor_manager_init()
进入 while 主循环
```

主循环：

```c
while (bRunApplication == TRUE) {
    MainLoop();
    motor_manager_poll_1ms();

    if (s_ms1000 >= 1000) {
        s_ms1000 = 0;
        motor_manager_print_status();
        ...
    }
}
```

不要把 CAN 初始化放在 SSC 生成文件里，避免 SSC Tool 重新生成时被覆盖。

## 11. CMake 接入

`firmware/ecat_tsn_gateway/CMakeLists.txt` 需要增加：

```cmake
sdk_inc(motor)
sdk_inc(can_if)
sdk_inc(rs06)

sdk_src(motor/motor_config.c)
sdk_src(motor/motor_manager.c)
sdk_src(motor/motor_safety.c)
sdk_src(can_if/can_if_hpm_mcan.c)
sdk_src(rs06/rs06_proto.c)
sdk_src(rs06/rs06_motor.c)
```

如果先做 PC 单元测试，可以把 `rs06_proto.c` 做成纯 C 文件，不依赖 HPM SDK。

## 12. 分阶段实施计划

### 阶段 0：确认硬件

目标：确认新板子到底引出了几路 MCAN，以及每路 CAN 的收发器、终端电阻、引脚复用。

要做：

1. 在原理图或板级头文件里确认 MCAN0~MCAN7 是否可用。
2. 确认每路 CAN 收发器供电和 STB/EN 引脚。
3. 确认终端电阻是否可控。
4. 确认 RS06 默认波特率为 1Mbps。

验收标准：

```text
每路 CAN 都能用 USB-CAN 或另一块板收到测试帧。
```

### 阶段 1：CAN 抽象层

目标：先不接电机，只验证 HPM MCAN 收发。

要做：

1. 新增 `can_if/`。
2. 初始化 1Mbps CAN 2.0。
3. 支持标准帧和扩展帧。
4. 支持 RX FIFO 中断。
5. 支持错误计数和 bus-off 状态读取。
6. 用 USB-CAN 验证发送扩展帧和接收扩展帧。

验收标准：

```text
HPM -> USB-CAN: 能看到 29-bit 扩展帧，DLC=8，数据正确。
USB-CAN -> HPM: 固件 rx_count 增加，帧 ID 和数据正确。
```

### 阶段 2：RS06 编解码纯函数

目标：不碰硬件，先把帧格式写对。

要做：

1. 新增 `rs06_proto.*`。
2. 实现 float <-> uint 编码函数。
3. 实现通信类型 3：使能。
4. 实现通信类型 4：停止、清错。
5. 实现通信类型 1：运控帧。
6. 实现通信类型 2：反馈解析。
7. 写几个固定输入的测试用例。

验收标准：

```text
输入 position=0, velocity=0, torque=0, kp=0, kd=0 时，编码值落在中点或零点预期位置。
输入 position=12.57 时编码接近 65535。
输入 position=-12.57 时编码接近 0。
反馈帧能正确解析出 motor_id、position、velocity、torque、temperature、fault。
```

### 阶段 3：单电机闭环

目标：只接 1 个 RS06，不接 16 轴。

要做：

1. `motor_config.c` 只启用 Axis0。
2. 上电默认 disabled。
3. 主站进入 OP 后，先发送使能帧。
4. 收到正常反馈后进入 RUNNING。
5. 周期发送运控帧。
6. 收到反馈后写入 `gateway_app_set_motor_feedback(0, ...)`。

验收标准：

```text
TwinCAT 或主站能看到 Axis0 ActualPosition/ActualVelocity/ActualTorque 变化。
串口能看到 axis0 state=RUNNING, rx_count 持续增加。
断开 CAN 后 50ms 内 axis0 offline，并停止发送运控。
EtherCAT 退出 OP 后电机停止。
```

### 阶段 4：多总线 16 轴

目标：把 16 个轴按配置表分配到多路 CAN。

要做：

1. 完整填写 `motor_axis_config_t s_axis_config[16]`。
2. 每路 CAN 独立 tx/rx/error 计数。
3. `motor_manager` 每周期按 bus 分组发送。
4. 每个 bus 控制自己的发送节奏，不要 16 轴串行堵在一路 CAN 上。
5. 串口每秒打印 bus 统计。

验收标准：

```text
16 轴在线。
每轴反馈时间戳持续刷新。
主站能看到 16 轴反馈。
CAN error_count 不持续增加。
控制周期内没有明显超时。
```

### 阶段 5：安全和诊断

目标：让系统从“能跑”变成“可调试、可保护”。

要做：

1. 增加命令限幅计数。
2. 增加主站命令超时。
3. 增加单轴反馈超时。
4. 增加 RS06 fault 解析。
5. 增加 bus-off 恢复逻辑。
6. 增加 stop all 和 clear fault 接口。
7. 后续通过 SDO 暴露诊断对象。

验收标准：

```text
拔 CAN 线、电机报错、主站退出 OP、主站停止刷新命令时，固件动作符合安全策略。
```

## 13. 第一版最小可运行目标

第一版不要贪多，做到下面这些就够：

1. 只支持 RS06 私有协议。
2. 只支持运控、使能、停止、清错、反馈解析。
3. 先支持 1 个电机，再扩到 16 个。
4. 不做参数保存、不改 CAN ID、不切协议。
5. EtherCAT PDO 不再改。
6. TSN bridge 继续保持关闭。

第一版成功后，主站应该看到：

```text
Axis0 TargetPosition/TargetVelocity/FFTorque/Kp/Kd 下发
    -> HPM 发 RS06 CAN 控制帧
    -> RS06 返回反馈帧
    -> HPM 更新 ActualPosition/ActualVelocity/ActualTorque
    -> EtherCAT 主站收到 TxPDO 反馈
```

## 14. 推荐开发顺序

严格按这个顺序做：

1. `can_if_hpm_mcan.c`：单路 CAN 收发测试。
2. `rs06_proto.c`：RS06 帧编解码测试。
3. `rs06_motor.c`：单电机状态机。
4. `motor_manager.c`：单轴接入 `gateway_app`。
5. `main.c`：加入 `motor_manager_init()` 和 `motor_manager_poll_1ms()`。
6. 单电机实测。
7. 多路 CAN 初始化。
8. 16 轴配置表。
9. 多电机实测。
10. 安全保护和诊断对象。

这个顺序的原因是：每一步都可以单独验证。不要直接一口气写 16 轴，否则出问题时分不清是 EtherCAT、CAN 硬件、RS06 帧格式、总线负载还是状态机的问题。

## 15. 需要特别注意的坑

1. RS06 私有协议是 29-bit 扩展帧，USB-CAN 工具必须打开扩展帧显示。
2. RS06 默认 1Mbps，HPM MCAN 也必须配置 1Mbps。
3. 反馈帧的目标 ID 是 host_id，不是 motor_id，解析时不要只按低 8 位找电机。
4. 力矩前馈在控制帧 CAN ID 的 data 字段里，不在 Data[0:7] 里。
5. `printf` 不能放在 1ms 控制路径。
6. EtherCAT 到 OP 之前不要使能电机。
7. 主站停止发 Output PDO 后必须停机。
8. 单路 CAN 跑 16 轴 1kHz 不现实，必须分总线或降低周期。
9. 不要改 SSC 生成文件来接 CAN，重新生成会覆盖。
10. 不要第一步就做改 CAN ID、保存 Flash、设置零点，这些都属于危险操作，应放到后面显式命令里。

