<!-- Author: BearKing -->
# RS06 协议接入状态与调试说明

## 1. 当前已经接入了什么

本次接入分成两层：

1. `firmware/ecat_tsn_gateway/rs06/rs06_proto.*`
   - 只做 RS06 协议编解码。
   - 不知道 EtherCAT，也不直接操作 HPM MCAN。
   - 已支持 RS06 私有 29-bit 扩展帧：
     - 设备 ID 查询，类型 `0x00`
     - 运控帧，类型 `0x01`
     - 电机反馈，类型 `0x02`
     - 使能，类型 `0x03`
     - 停止/清错，类型 `0x04`
     - 设零点，类型 `0x06`
     - 修改 CAN ID，类型 `0x07`
     - 读参数，类型 `0x11`
     - 写 float 参数，类型 `0x12`
     - 故障上报解析，类型 `0x15`
     - 保存参数，类型 `0x16`
     - 主动上报开关，类型 `0x18`
     - 切换协议，类型 `0x19`
   - 也预留了 RS06 MIT 11-bit 标准帧：
     - 使能
     - 停止
     - 设零
     - MIT 运控
     - MIT 反馈解析

2. `firmware/ecat_tsn_gateway/motor/motor_manager.*`
   - 从 `gateway_app` 读 EtherCAT 16 轴命令。
   - 调用 `rs06_proto` 生成 RS06 CAN 帧。
   - 调用 `can_if` 收发 CAN。
   - 收到反馈后调用 `gateway_app_set_motor_feedback()` 写回 EtherCAT TxPDO。

这样以后换 ENCOS 或者补 RS06 参数功能，不需要改 SSC 生成文件，也不需要把协议塞进 `gateway_app.c`。

## 2. 当前默认是否会让电机转

默认不会。

在 `firmware/ecat_tsn_gateway/motor/motor_config.h` 里：

```c
#define MOTOR_MANAGER_ENABLE_DEVICE_QUERY     1
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE      0
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL   0
```

含义：

- `MOTOR_MANAGER_ENABLE_DEVICE_QUERY = 1`
  - 上电后会周期性发 RS06 设备 ID 查询。
  - 这是安全查询，不是使能，不会让电机运动。

- `MOTOR_MANAGER_ENABLE_AUTO_ENABLE = 0`
  - 不自动发送使能帧。

- `MOTOR_MANAGER_ENABLE_MOTION_CONTROL = 0`
  - 不发送 MIT 运控帧。
  - 即使 EtherCAT 主站下发了位置、速度、力矩、Kp、Kd，也暂时不会发给电机。

这个默认值是为了先验证 CAN 物理层和 RS06 反馈，不让电机意外动作。

## 3. 默认 16 轴到 8 路 CAN 的映射

配置在 `firmware/ecat_tsn_gateway/motor/motor_config.c`。

默认是每路 CAN 两个电机：

| EtherCAT axis | CAN bus | RS06 motor_id |
|---:|---:|---:|
| 0 | CAN0 | 1 |
| 1 | CAN0 | 2 |
| 2 | CAN1 | 1 |
| 3 | CAN1 | 2 |
| 4 | CAN2 | 1 |
| 5 | CAN2 | 2 |
| 6 | CAN3 | 1 |
| 7 | CAN3 | 2 |
| 8 | CAN4 | 1 |
| 9 | CAN4 | 2 |
| 10 | CAN5 | 1 |
| 11 | CAN5 | 2 |
| 12 | CAN6 | 1 |
| 13 | CAN6 | 2 |
| 14 | CAN7 | 1 |
| 15 | CAN7 | 2 |

所以你现在只测 CAN7，默认 RS06 电机 ID 如果是 `1`，它会对应 EtherCAT 的 `axis14`。

## 4. CAN7 调试时应该看到什么

启动后会看到类似：

```text
RS06 manager: host=0xFD axes=16 map=2 motors/bus query=1 autoEnable=0 motion=0
CAN: initMask=0xFF | C0 tx=... rx=... | ... | C7 tx=... rx=...
RS06: online=0/16 tx=... rx=... motion=0
```

如果 CAN7 接了 RS06，且电机 ID 是 `1` 或 `2`，收到有效反馈后会打印前 16 条：

```text
RS06 RX: axis=14 bus=7 id=1 pos=... vel=... tq=... temp=... fault=0x00 mode=...
```

同时：

- `CAN` 状态里的 `C7 rx` 会增加。
- `RS06 online` 会从 `0/16` 变成 `1/16` 或 `2/16`。
- EtherCAT TxPDO 对应轴会更新真实反馈。

如果只看到 `C7 rx` 增加，但 `RS06 online` 不增加，通常说明：

1. 收到的不是 RS06 私有协议反馈帧。
2. 电机反馈的 `host_id` 不是 `0xFD`。
3. 电机 ID 不是当前配置表里的 `1` 或 `2`。
4. 电机当前协议不是 RS06 私有 29-bit 扩展帧。

## 5. 什么时候打开真正运控

确认以下条件都满足后再打开：

1. CAN7 能收到 RS06 反馈。
2. `RS06 online` 能正确增加。
3. 主站看到 axis14 或 axis15 的 TxPDO 反馈正确。
4. 电机固定牢靠，急停/断电手段可用。

然后改 `motor_config.h`：

```c
#define MOTOR_MANAGER_ENABLE_AUTO_ENABLE      1
#define MOTOR_MANAGER_ENABLE_MOTION_CONTROL   1
```

重新编译下载后，manager 会：

1. 收到 EtherCAT 输出命令后，记录命令时间。
2. 自动给配置启用的 RS06 发送使能帧。
3. 周期发送 RS06 私有协议运控帧。
4. 如果超过 `MOTOR_MANAGER_COMMAND_TIMEOUT_MS` 没有新的 EtherCAT 命令，就停止继续发送运控帧。

## 6. 关键文件

| 文件 | 作用 |
|---|---|
| `rs06/rs06_proto.h` | RS06 协议接口、范围、类型定义 |
| `rs06/rs06_proto.c` | RS06 帧 ID、数据打包、反馈解析 |
| `motor/motor_config.h` | 安全开关、host_id、周期配置 |
| `motor/motor_config.c` | 16 轴到 8 路 CAN 的映射 |
| `motor/motor_manager.c` | 查询、收反馈、写 EtherCAT TxPDO、可选运控发送 |
| `can_if/can_if_hpm_mcan.c` | HPM MCAN 硬件底层 |
| `application/gateway_app.c` | EtherCAT PDO 和内部 16 轴命令/反馈缓存 |

