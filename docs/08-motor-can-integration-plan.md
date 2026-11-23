<!-- Author: BearKing -->
# RS06 与 ENCOS 电机 CAN 接入落地计划

> **状态更新 (2026-07-09):** 16 轴 MIT PDO 已实现在 `gateway_app.c`。本文聚焦如何将 RS06/ENCOS 电机适配到 MIT 控制范式下，通过 CAN 总线接入。

## 1. 当前进度与上下文

| 模块 | 状态 |
|------|------|
| 16轴 MIT EtherCAT PDO | ✅ 已实现 (`gateway_app.c`) |
| SSC 对象字典 | ✅ 手改为 16 轴 |
| ECAT → OP 状态机 | ✅ 正常 |
| CAN 驱动层 | ❌ 未实现 |
| RS06 CAN 协议适配 | ❌ 未实现 |
| ENCOS CAN 协议适配 | ❌ 未实现 |
| 电机安全保护 | ❌ 未实现 |

16 轴 MIT PDO 的 EtherCAT 侧回调已在 [gateway_app.c](..\firmware\ecat_tsn_gateway\application\gateway_app.c) 中就位:

```c
// 命令方向: ECAT PDO → g_commands[16] (mit_command_t[16])
void APPL_OutputMapping(UINT16 *pData) { ... }

// 反馈方向: g_feedback[16] → ECAT PDO
void APPL_InputMapping(UINT16 *pData) { ... }
```

CAN 层需要做的就是: **`g_commands[i]` → CAN 帧 → 电机,  电机反馈 → CAN 帧 → `g_feedback[i]`**。

## 2. 文档依据

- `docs/rob-stride_release-main/Product Literature/RS06/RS06User Manual260428.pdf`
- `docs/rob-stride_release-main/Product Literature/canopen/driver_v1.1.eds`
- `docs/encos/ENCOS+调试技术手册+V1.19EAP.pdf`
- `hpm_sdk/samples/drivers/mcan`
- `hpm_sdk/samples/canopen/301/master`

## 3. 电机协议速查

| 电机 | 默认协议 | 帧格式 | 波特率 | MIT 切换 |
|------|---------|--------|--------|---------|
| RS06 | 私有协议 | 29-bit 扩展帧 | 1Mbps | 支持 (通信类型 25) |
| ENCOS | 私有协议 | 11-bit 标准帧 | 1Mbps | 不支持 (CANopen 暂未启用, 仅私有/伺服模式) |

**关键结论:** RS06 可切换到 MIT 模式，ENCOS 需要做一层 MIT → 私有协议转换。

## 4. 总体架构

```
EtherCAT 周期 (1ms)
  │
  ▼
APPL_OutputMapping → g_commands[16]
  │
  ├── 轴类型 = MIT (RS06切到MIT模式)
  │     └── CAN FD 直接发送 MIT 帧 (零转换)
  │
  └── 轴类型 = ENCOS (私有协议)
        └── MIT → ENCOS 协议转换 → CAN FD 发送
              pos/vel/kp/kd → 力位混控/伺服模式帧
  │
  ▼
CAN ISR 接收反馈
  │
  ├── MIT 电机 → 直接填入 g_feedback[i]
  └── ENCOS → 解析私有帧 → 填入 g_feedback[i]
  │
  ▼
APPL_InputMapping → g_feedback[16] → ECAT PDO
```

### 新增模块

```text
firmware/ecat_tsn_gateway/can_gateway/
  motor_can.h / motor_can.c           ← MCAN 初始化、收发、filter
  mit_protocol.h / mit_protocol.c     ← MIT CAN 帧编码/解码 (通用)
  rs06_mit.h / rs06_mit.c            ← RS06 切 MIT 模式 + 参数配置
  encos_adapter.h / encos_adapter.c  ← MIT → ENCOS 私有协议转换
  motor_manager.h / motor_manager.c  ← 16轴统一调度 + 安全保护
```

### 模块职责

| 模块 | 职责 |
|------|------|
| `motor_can.*` | MCAN 初始化、CAN FD 收发、RX ISR 分发、错误统计 |
| `mit_protocol.*` | MIT CAN 帧编解码: `mit_command_t` ↔ CAN frame, 反馈帧 ↔ `mit_feedback_t` |
| `rs06_mit.*` | RS06 上电初始化、切 MIT 模式、参数只读验证 |
| `encos_adapter.*` | MIT float 值 → ENCOS uint16+缩放, ENCOS 反馈 → MIT float |
| `motor_manager.*` | 16轴统一调度、使能/停止、超时保护、错误聚合 |

## 5. MIT CAN 帧格式 (通用)

参照标准 MIT Cheetah 协议:

```
主机 → 电机 (CAN ID = motor_id):
  [0-1]  p_des    float16   期望位置
  [2-3]  v_des    float16   期望速度
  [4-5]  kp       float16   位置刚度
  [6-7]  kd       float16   速度阻尼
  [8-9]  t_ff     float16   前馈力矩

电机 → 主机 (CAN ID = motor_id + 0x80):
  [0-1]  p_act    float16   实际位置
  [2-3]  v_act    float16   实际速度
  [4-5]  i_act    float16   实际电流
```

> **注意:** 如果你的电机 MIT 实现使用 float32 而非 float16，CAN 帧会从 10 bytes 变成 20+8 bytes。需根据驱动器实际协议确认。

## 6. CAN 总线拓扑 (16 电机)

HPM6E80 有 8 路 MCAN (CAN FD)。16 电机的推荐分配:

```
MCAN0 ──── CAN FD Bus 0 ──── Motor 0, 1, 2, 3   (Node 1-4)
MCAN1 ──── CAN FD Bus 1 ──── Motor 4, 5, 6, 7   (Node 1-4)
MCAN2 ──── CAN FD Bus 2 ──── Motor 8, 9,10,11   (Node 1-4)
MCAN3 ──── CAN FD Bus 3 ──── Motor 12,13,14,15   (Node 1-4)
```

| 参数 | 值 |
|------|-----|
| 每总线电机数 | 4 |
| 每周期 CAN 帧数/总线 | 8 (4发 + 4收) |
| CAN FD 5Mbps 总线占用 | ~200 µs |
| EtherCAT 周期余量 | 充足 (1ms - 0.2ms = 0.8ms) |

## 7. RS06 适配: 切 MIT 模式

RS06 支持三种协议切换 (通信类型 25):

```text
控制模式值: 0=私有协议, 1=CANopen, 2=MIT协议
```

### 上电初始化流程

```text
1. 获取设备 ID (确认电机在线)
2. 停止/清错 (确保安全状态)
3. 读取当前协议模式
4. 如果非 MIT → 发送协议切换指令 (通信类型 25, 数据=2)
5. 电机重启 / 重新枚举
6. 验证: 读取协议模式 = MIT
7. 周期发送 MIT 控制帧
```

### RS06 MIT 模式控制帧

RS06 切换到 MIT 模式后, 控制帧格式可能和标准 MIT 有细微差异 (参考 RS06 手册中通信类型 1 的 MIT 子模式)。需要确认:
- float 精度 (16 vs 32)
- 单位换算 (rad vs 编码器脉冲)
- 是否需要单独的使能/停止 CAN 帧

## 8. ENCOS 适配: MIT → 私有协议转换

ENCOS 不支持 MIT 模式, 需在 HPM 固件中做转换。

### 转换映射

| MIT 命令字段 | ENCOS 对应模式 | 转换逻辑 |
|-------------|---------------|---------|
| pos, kp, kd | 力位混控 | pos → 目标位置, kp→刚度, kd→阻尼 |
| vel | 速度模式 | vel → 目标速度 |
| trq_ff | 电流/力矩模式 | trq_ff → 目标电流 |

### 数值转换

```c
// MIT (float, SI单位) → ENCOS (uint16, 编码器单位)
uint16_t encos_pos = mit_pos_rad * encoder_cpr / (2.0f * PI);
uint16_t encos_vel = mit_vel_rps * encoder_cpr * 60.0f;
uint16_t encos_trq = mit_trq_Nm * torque_scale_factor;

// ENCOS 反馈 → MIT
float mit_pos = encos_pos_raw * (2.0f * PI) / encoder_cpr;
float mit_vel = encos_vel_raw / (encoder_cpr * 60.0f);
float mit_trq = encos_current_raw * torque_constant;
```

## 9. 与现有主循环集成

当前 `main.c` 的主循环:

```c
while (bRunApplication == TRUE) {
    MainLoop();           // ECAT: ESM + PDO
    gateway_bridge_poll();// TSN (当前 ENABLE_TSN_BRIDGE=0, 不执行)
    // 1s 心跳打印
}
```

CAN 接入后:

```c
while (bRunApplication == TRUE) {
    MainLoop();              // ECAT 栈
    motor_manager_poll();    // ★ 新增: 电机调度
    // 1s 心跳打印
}
```

`motor_manager_poll()` 内部:

```c
void motor_manager_poll(void) {
    // ① 如果 ECAT 在 OP 且有新命令 → 发送 CAN 帧
    if (bEcatOutputUpdateRunning && gateway_app_motor_command_updated()) {
        for (int i = 0; i < 16; i++) {
            const mit_command_t *cmd = gateway_app_get_motor_command(i);
            motor_can_send_mit_frame(i, cmd);  // 发 CAN 帧
        }
        gateway_app_clear_motor_command_updated();
    }

    // ② 收集 CAN 反馈 → 写入 g_feedback
    for (int i = 0; i < 16; i++) {
        if (motor_feedback_available(i)) {
            mit_feedback_t fb = motor_get_feedback(i);
            gateway_app_set_motor_feedback(i, fb.position, fb.velocity, fb.torque);
        }
    }

    // ③ 超时检测 → 紧急停止
    motor_manager_check_timeouts();
}
```

## 10. 安全策略

| 条件 | 动作 |
|------|------|
| 上电初始化 | 所有电机默认 disabled |
| EtherCAT 未进入 OP | 禁止电机使能 |
| 主站命令超过 100ms 未更新 | 电机停机 (零力矩) |
| CAN 反馈超过 50ms 未更新 | 标记 offline，禁止继续使能 |
| CAN bus-off | 恢复 CAN，不自动重新使能 |
| 目标值超限 | 限幅并记录计数 |
| 电机故障反馈 | 停机，等待显式清错 |

**禁止自动执行的操作 (需主动调用):**
- 改 CAN ID
- 改波特率
- 保存参数到 Flash
- 设置零点
- 切换协议模式

## 11. 开发阶段

### 阶段 1: CAN 基础链路
- MCAN 初始化、pinmux、时钟
- 标准帧/扩展帧自收发验证
- CAN FD 模式确认

### 阶段 2: MIT 协议层
- `mit_protocol.c`: float ↔ CAN frame 编解码
- 单电机 MIT 控制自测 (用 USB-CAN 监听)

### 阶段 3: RS06 接入
- 上电初始化序列
- 切 MIT 模式
- 运控验证

### 阶段 4: ENCOS 接入
- MIT → ENCOS 协议转换
- 数值 scaling 校准
- 运控验证

### 阶段 5: 多电机联调
- 16 电机同时在线
- 总线负载监控
- 控制周期抖动检查

### 阶段 6: 安全保护
- 超时停机
- Bus-off 恢复
- 错误聚合上报

## 12. 推荐提交顺序

1. 新增 `can_gateway/` 目录和 `motor_can.*` 空壳
2. MCAN 初始化 + 自收发 `motor_can.*`
3. `mit_protocol.*` MIT 帧编解码 + 单电机测试
4. `motor_manager.*` 统一接口 + 安全框架
5. RS06 适配 `rs06_mit.*`
6. ENCOS 适配 `encos_adapter.*`
7. 接入 `main.c` 主循环
8. 多电机联调 + 安全保护

## 13. 当前建议

**第一步: `motor_can.c` MCAN 基础收发验证。** 先不接电机，用 USB-CAN 工具确认 HPM 能正确发送和接收 CAN 帧。这是后续所有工作的基础。
