<!-- Author: BearKing -->
# RS06 当前代码逻辑说明：CAN7 单电机位置往复测试

本文档记录当前工程已经跑通的 RS06 电机控制链路。

当前状态不是最终 16 轴量产模式，而是 **CAN7 单电机调试模式**：

- EtherCAT PDO 已经是 16 轴 MIT 风格对象。
- CAN 底层目前只初始化 CAN7。
- RS06 当前电机 ID 是 `127`。
- 当前只启用 `axis14 -> CAN7 -> RS06 id127`。
- 已经能读取反馈。
- 已经能使能电机。
- 已经能做简单位置往复测试。

---

## 1. 总体数据链路

当前软件链路如下：

```text
main.c
  |
  | 1ms timer
  v
motor_manager_poll_1ms()
  |
  +--> can_if_receive()
  |      |
  |      +--> rs06_private_decode_feedback()
  |      +--> rs06_private_decode_device_id_response()
  |
  +--> gateway_app_set_motor_feedback()
  |
  +--> rs06_private_encode_enable()
  +--> rs06_private_encode_motion_control()
         |
         v
       can_if_send()
         |
         v
       HPM MCAN7
         |
         v
       RS06 motor id127
```

分层原则：

| 层 | 文件 | 责任 |
|---|---|---|
| EtherCAT 应用层 | `application/gateway_app.c` | 解析 PDO，缓存 16 轴命令和反馈 |
| 电机管理层 | `motor/motor_manager.c` | 轴映射、状态机、周期调度、打印 |
| 电机配置层 | `motor/motor_config.*` | axis 到 CAN bus/motor id 的映射和测试参数 |
| RS06 协议层 | `rs06/rs06_proto.*` | RS06 CAN 帧编码和解码 |
| CAN 硬件层 | `can_if/can_if_hpm_mcan.c` | HPM MCAN 初始化、收发、中断、统计 |

---

## 2. main.c 启动流程

入口在 `firmware/ecat_tsn_gateway/main.c`。

主要流程：

1. `board_init()`
   - 初始化板级基础外设。

2. `board_timer_create(1, timer_isr)`
   - 创建 1ms 定时器。
   - `timer_isr()` 里累加：
     - `s_ms10`
     - `s_ms100`
     - `s_ms1000`

3. `ecat_hardware_init(HPM_ESC)`
   - 初始化 EtherCAT ESC。

4. `MainInit()`
   - 初始化 SSC 协议栈。

5. `APPL_GenerateMapping()`
   - 计算 PDO 大小。
   - 当前应为：

   ```text
   PDO: 192 bytes in, 320 bytes out
   ```

   含义：

   - `320 bytes out`：主站到从站，16 轴命令，每轴 5 个 float。
   - `192 bytes in`：从站到主站，16 轴反馈，每轴 3 个 float。

6. `can_if_init()`
   - 初始化 CAN。
   - 当前只启用 CAN7，所以启动后应看到：

   ```text
   CAN init OK: initMask=0x80
   ```

7. `motor_manager_init()`
   - 初始化 RS06 电机管理层。
   - 发送第一次设备 ID 查询。
   - 发送第一次安全反馈轮询。

8. 主循环：

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

注意：变量名叫 `s_ms10`，但现在每 1ms 递增一次，主循环里会把积累的 tick 全部消费掉，所以 `motor_manager_poll_1ms()` 实际按 1ms 调度。

---

## 3. CAN 底层逻辑

文件：

```text
firmware/ecat_tsn_gateway/can_if/can_if_hpm_mcan.c
```

当前关键配置：

```c
#define CAN_IF_BAUDRATE        1000000UL
#define CAN_IF_ENABLED_BUS_MASK 0x80UL
```

含义：

- CAN 波特率：`1Mbps`
- 当前只初始化 CAN7
- `0x80 = bit7`，所以 `initMask=0x80`

为什么暂时只开 CAN7：

1. 当前只接了一台 RS06 到 CAN7。
2. 如果 CAN0-CAN6 没接线也初始化，它们发帧没有 ACK，会产生大量 error interrupt。
3. 只开 CAN7 可以让调试日志干净，快速确认物理链路。

CAN 接收逻辑：

1. MCAN 中断收到帧。
2. ISR 调用 `can_if_rx_from_fifo()`。
3. 收到的 CAN 帧被塞进 ring buffer。
4. 上层通过 `can_if_receive()` 取帧。

CAN 发送逻辑：

1. 上层准备 `can_if_frame_t`。
2. 调用 `can_if_send()`。
3. 底层转为 `mcan_tx_frame_t`。
4. 调用 `mcan_transmit_blocking()` 发送。

当前日志怎么看：

```text
CAN: initMask=0x80 | ... | C7 tx=0 rx=6319 err=0 drop=0
```

说明：

- `initMask=0x80`：只启用 CAN7。
- `C7 rx` 增长：CAN7 正在收到电机回复。
- `C7 err=0`：CAN 物理层没有错误。
- `drop=0`：接收环形缓冲区没有溢出。

---

## 4. RS06 协议层逻辑

文件：

```text
firmware/ecat_tsn_gateway/rs06/rs06_proto.h
firmware/ecat_tsn_gateway/rs06/rs06_proto.c
```

这一层只做协议，不做状态机。

### 4.1 私有协议 CAN ID

RS06 私有协议使用 29-bit 扩展帧：

```text
bits[28:24] = communication type
bits[23:8]  = data field
bits[7:0]   = target id
```

代码里用：

```c
uint32_t rs06_private_make_id(uint8_t type, uint16_t data, uint8_t target_id);
bool rs06_private_parse_id(uint32_t id, rs06_private_id_fields_t *fields);
```

当前用到的类型：

| 类型 | 名称 | 当前用途 |
|---:|---|---|
| `0x00` | Device ID | 查询设备 ID |
| `0x01` | Motion control | 发送位置控制 |
| `0x02` | Feedback | 解析位置/速度/力矩/温度 |
| `0x03` | Enable | 使能电机 |
| `0x04` | Stop/status | 调试早期用于触发反馈 |

### 4.2 设备 ID 查询

编码函数：

```c
rs06_private_encode_device_id_query()
```

用途：

- 查询 RS06 是否在线。
- 获取电机返回的 UID。

你当前看到：

```text
RS06 device: bus=7 id=127 uid=0x20503020A0133312
```

说明：

- 电机在 CAN7。
- 电机 ID 是 `127`。
- UID 是 `0x20503020A0133312`。

### 4.3 反馈解析

解码函数：

```c
rs06_private_decode_feedback()
```

反馈帧 type 是 `0x02`。

解析内容：

| 字段 | 单位 |
|---|---|
| `position_rad` | rad |
| `velocity_rad_s` | rad/s |
| `torque_nm` | Nm |
| `temperature_c` | 摄氏度 |
| `fault_bits` | 故障位 |
| `mode` | 电机模式 |

当前日志：

```text
RS06 axis14 ... pos=2.441113 target=2.741881 err=... vel=... tq=... temp=32.000000 fault=0x00 mode=2
```

说明：

- 当前实际位置约 `2.44 rad`
- 当前目标位置由测试模式给出
- `fault=0x00`：无故障
- `mode=2`：电机处于运行模式

### 4.4 运控帧编码

编码函数：

```c
rs06_private_encode_motion_control()
```

输入结构：

```c
typedef struct {
    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float kp;
    float kd;
} rs06_motion_cmd_t;
```

当前测试模式发送：

```c
cmd.position_rad = command_target_rad;
cmd.velocity_rad_s = 0.0f;
cmd.torque_nm = 0.0f;
cmd.kp = MOTOR_MANAGER_HOLD_KP;
cmd.kd = MOTOR_MANAGER_HOLD_KD;
```

也就是：

```text
位置目标 + 零速度 + 零前馈力矩 + KP/KD
```

---

## 5. motor_config 当前配置

文件：

```text
firmware/ecat_tsn_gateway/motor/motor_config.h
firmware/ecat_tsn_gateway/motor/motor_config.c
```

### 5.1 功能开关

当前配置：

```c
#define MOTOR_MANAGER_ENABLE_DEVICE_QUERY        1
#define MOTOR_MANAGER_ENABLE_SAFE_FEEDBACK_POLL  1
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE         0
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL      0
#define MOTOR_MANAGER_ENABLE_POSITION_HOLD_TEST  1
```

含义：

| 宏 | 当前值 | 含义 |
|---|---:|---|
| `MOTOR_MANAGER_ENABLE_DEVICE_QUERY` | 1 | 周期查询设备 ID |
| `MOTOR_MANAGER_ENABLE_SAFE_FEEDBACK_POLL` | 1 | 使能前用 stop/status 触发反馈 |
| `MOTOR_MANAGER_ENABLE_AUTO_ENABLE` | 0 | 不使用 EtherCAT 主站自动使能逻辑 |
| `MOTOR_MANAGER_ENABLE_MOTION_CONTROL` | 0 | 不使用主站下发命令做控制 |
| `MOTOR_MANAGER_ENABLE_POSITION_HOLD_TEST` | 1 | 使用本地单轴位置测试模式 |

注意：

当前电机能动，是因为 `POSITION_HOLD_TEST` 测试模式在发控制帧，不是因为 `MOTION_CONTROL` 主站控制打开了。

### 5.2 测试参数

当前配置：

```c
#define MOTOR_MANAGER_CONTROL_PERIOD_MS       2U
#define MOTOR_MANAGER_HOLD_KP                 10.0f
#define MOTOR_MANAGER_HOLD_KD                 1.0f
#define MOTOR_MANAGER_STEP_AMPLITUDE_RAD      0.30f
#define MOTOR_MANAGER_STEP_START_DELAY_MS     2000U
#define MOTOR_MANAGER_STEP_INTERVAL_MS        3000U
```

含义：

| 参数 | 当前值 | 含义 |
|---|---:|---|
| 控制周期 | 2ms | 每 2ms 发送一次位置控制帧 |
| KP | 10.0 | 位置刚度 |
| KD | 1.0 | 速度阻尼 |
| 阶跃幅度 | 0.30 rad | 约 17.2 度 |
| 启动延迟 | 2000ms | 捕获当前位置后先保持 2 秒 |
| 切换间隔 | 3000ms | 每 3 秒切换目标位置 |

### 5.3 轴映射

当前只启用了一个轴：

```text
axis14 -> CAN7 -> RS06 motor_id 127
```

其它 axis 暂时 disabled。

原因：

1. 当前只接了一台 RS06。
2. 先验证单轴闭环控制。
3. 等单轴稳定后，再逐步恢复 8 路 CAN、16 轴映射。

---

## 6. motor_manager 状态机

文件：

```text
firmware/ecat_tsn_gateway/motor/motor_manager.c
```

### 6.1 每个轴的状态

核心结构：

```c
typedef struct {
    bool online;
    bool enable_sent;
    uint8_t fault_bits;
    rs06_motor_mode_t mode;

    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t device_id_count;
    uint32_t feedback_count;

    uint32_t last_rx_ms;
    uint32_t last_feedback_ms;

    float position_rad;
    float velocity_rad_s;
    float torque_nm;
    float temperature_c;

    bool hold_target_valid;
    float hold_base_rad;
    float command_target_rad;
    uint32_t hold_capture_ms;
    bool step_high;
} motor_axis_state_t;
```

重要字段：

| 字段 | 含义 |
|---|---|
| `online` | 是否收到过设备 ID 或反馈 |
| `enable_sent` | 是否已经发过使能帧 |
| `fault_bits` | RS06 反馈里的故障位 |
| `mode` | RS06 当前模式 |
| `feedback_count` | 收到 type2 反馈数量 |
| `position_rad` | 最新实际位置 |
| `hold_base_rad` | 测试模式捕获的初始位置 |
| `command_target_rad` | 当前发送给电机的位置目标 |
| `step_high` | 当前是否处于高位目标 |

### 6.2 初始化

函数：

```c
bool motor_manager_init(void)
```

做的事情：

1. 清空所有轴状态。
2. 清空内部时间计数。
3. 打印配置。
4. 发送设备 ID 查询。
5. 发送一次安全反馈轮询。

启动日志类似：

```text
RS06 manager: host=0xFD axes=16 query=1 safeFbPoll=1 holdTest=1 autoEnable=0 motion=0
```

### 6.3 1ms 周期函数

函数：

```c
void motor_manager_poll_1ms(void)
```

每 1ms 被 `main.c` 调用。

内部流程：

1. `s_time_ms++`
2. `handle_can_rx()`
   - 从 CAN ring buffer 取帧。
   - 解析 RS06 device id / feedback / param / fault。
3. 每 1000ms 调用 `send_device_queries()`
4. 每 1000ms 调用 `send_safe_feedback_polls()`
5. 每 2ms 调用 `send_position_hold_controls()`
6. 如果以后打开主站控制，再调用 `send_motion_controls()`

当前真正让电机动的是：

```c
send_position_hold_controls();
```

---

## 7. 当前位置往复测试逻辑

函数：

```c
send_position_hold_controls()
```

### 7.1 捕获基准位置

第一次收到 type2 反馈后，执行：

```c
hold_base_rad = pos;
command_target_rad = pos;
hold_capture_ms = s_time_ms;
step_high = false;
hold_target_valid = true;
```

也就是把当前电机位置记录为基准点。

日志：

```text
RS06 step test base captured: axis=14 base=... rad amp=0.300000 rad
```

### 7.2 使能电机

如果还没发过使能：

```c
rs06_private_encode_enable()
can_if_send()
enable_sent = true;
```

日志：

```text
RS06 enable sent: axis=14 bus=7 id=127
```

收到反馈后：

```text
mode=2
```

说明电机进入运行模式。

### 7.3 目标位置切换

捕获基准位置后，先等待：

```text
MOTOR_MANAGER_STEP_START_DELAY_MS = 2000ms
```

然后每：

```text
MOTOR_MANAGER_STEP_INTERVAL_MS = 3000ms
```

切换一次目标：

```text
base
base + 0.30 rad
base
base + 0.30 rad
...
```

当前幅度：

```text
0.30 rad = 17.2 deg
```

### 7.4 控制帧内容

每 2ms 发送一次：

```c
cmd.position_rad = command_target_rad;
cmd.velocity_rad_s = 0.0f;
cmd.torque_nm = 0.0f;
cmd.kp = 10.0f;
cmd.kd = 1.0f;
```

这个控制模式本质是：

```text
目标位置保持 + 阻尼
```

---

## 8. 串口打印如何判断状态

### 8.1 CAN 状态

```text
CAN: initMask=0x80 | ... | C7 tx=0 rx=6319 err=0 drop=0
```

判断：

| 字段 | 正常值 | 含义 |
|---|---|---|
| `initMask` | `0x80` | 只启用 CAN7 |
| `C7 rx` | 持续增长 | 正在收到电机反馈 |
| `C7 err` | `0` | CAN 物理层正常 |
| `C7 drop` | `0` | 接收没有溢出 |

### 8.2 RS06 总状态

```text
RS06: online=1/16 tx=6319 rx=6318 fb=6305 hold=1 motion=0
```

判断：

| 字段 | 含义 |
|---|---|
| `online=1/16` | 当前 16 轴配置里有 1 个在线 |
| `tx` | motor_manager 发出的 RS06 帧数量 |
| `rx` | motor_manager 识别到的 RS06 帧数量 |
| `fb` | type2 反馈帧数量 |
| `hold=1` | 当前本地位置测试模式开启 |
| `motion=0` | 主站 PDO 控制模式关闭 |

### 8.3 单轴状态

```text
RS06 axis14 bus=7 id=127 online=1 dev=13 fb=6305 pos=2.441113 target=2.741881 err=0.300768 vel=-0.048065 tq=0.000549 temp=32.000000 fault=0x00 mode=2 age=1ms
```

判断：

| 字段 | 含义 |
|---|---|
| `axis14` | EtherCAT 第 14 轴 |
| `bus=7` | CAN7 |
| `id=127` | RS06 motor id |
| `dev` | 设备 ID 回复数量 |
| `fb` | type2 反馈数量 |
| `pos` | 当前实际位置 |
| `target` | 当前目标位置 |
| `err` | `target - pos` |
| `vel` | 当前速度 |
| `tq` | 当前力矩 |
| `temp` | 温度 |
| `fault` | 故障位 |
| `mode` | 电机模式 |
| `age` | 距离上一次反馈的时间 |

正常时：

```text
fault=0x00
mode=2
age=1ms 左右
C7 err=0
```

---

## 9. 当前已验证通过的点

目前已经验证：

1. EEPROM 加载正常。
2. EtherCAT SSC 启动正常。
3. 16 轴 PDO 大小正确：

   ```text
   PDO: 192 bytes in, 320 bytes out
   ```

4. CAN7 初始化正常：

   ```text
   initMask=0x80
   ```

5. CAN7 物理通信正常：

   ```text
   C7 err=0
   ```

6. RS06 设备 ID 查询正常：

   ```text
   id=127
   uid=0x20503020A0133312
   ```

7. RS06 type2 反馈正常。
8. RS06 使能正常：

   ```text
   mode=2
   ```

9. RS06 私有协议位置运控帧正常。
10. 单轴位置往复测试已经能让电机转动。

---

## 10. 后续如何切到 EtherCAT 主站控制

当前电机动起来，是本地测试模式控制：

```c
#define MOTOR_MANAGER_ENABLE_POSITION_HOLD_TEST 1
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL     0
```

后续要让 EtherCAT 主站控制，需要改成：

```c
#define MOTOR_MANAGER_ENABLE_POSITION_HOLD_TEST 0
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE        1
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL     1
```

然后控制数据来自 EtherCAT RxPDO：

| EtherCAT 命令 | RS06 运控字段 |
|---|---|
| `TargetPosition` | `position_rad` |
| `TargetVelocity` | `velocity_rad_s` |
| `FFTorque` | `torque_nm` |
| `Kp` | `kp` |
| `Kd` | `kd` |

反馈写回 EtherCAT TxPDO：

| RS06 反馈 | EtherCAT 反馈 |
|---|---|
| `position_rad` | `ActualPosition` |
| `velocity_rad_s` | `ActualVelocity` |
| `torque_nm` | `ActualTorque` |

建议切换主站控制前，先让主站只发 axis14 的小幅目标，例如：

```text
position = 当前反馈位置附近
velocity = 0
torque_ff = 0
kp = 10
kd = 1
```

不要一开始让主站发全 16 轴，也不要给大 KP。

---

## 11. 后续扩展到 8 路 CAN / 16 轴

扩展顺序建议：

1. 保持 `CAN_IF_ENABLED_BUS_MASK = 0x80`，先把 CAN7 单轴完全验证完。
2. 接第二台电机到 CAN7，启用 axis15。
3. 再逐个打开 CAN6、CAN5、...、CAN0。
4. 每打开一路 CAN，先只做设备 ID 查询和反馈读取。
5. 每一路 CAN 都稳定后，再打开对应轴的运控。
6. 最后把：

   ```c
   #define CAN_IF_ENABLED_BUS_MASK 0xFFUL
   ```

   恢复成 8 路全开。

不要一口气打开 16 个电机。这样出问题时能快速定位是：

- 某一路 CAN 接线问题
- 某个电机 ID 配错
- 某个轴方向反了
- 某个电机参数或协议不一致
- 总线负载过高

---

## 12. 安全注意事项

1. 修改 `STEP_AMPLITUDE_RAD` 前，先确认电机固定牢靠。
2. 修改 `HOLD_KP` 前，先确认不会撞限位。
3. `fault != 0x00` 时不要继续发送运控。
4. `C7 err` 增长时先停机排查 CAN 线。
5. `age` 明显变大时说明反馈丢失，应停机。
6. 切到主站控制前，确认主站初始目标位置等于当前实际位置附近。
7. 先小幅度、低 KP、单轴验证，再扩展多轴。

