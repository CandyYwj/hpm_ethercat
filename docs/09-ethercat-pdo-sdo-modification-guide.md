<!-- Author: BearKing -->
# EtherCAT PDO 与 SDO 修改指南

本文面向当前 `firmware/ecat_tsn_gateway` 工程，说明如果要修改 EtherCAT 从站的 PDO 和 SDO，应该改哪些文件、每一步的作用是什么、以及修改后如何验证。

当前工程使用 HPM6E80 内置 ESC 作为 EtherCAT Slave Controller，使用 Beckhoff SSC 作为从站协议栈。业务层通过 `application/gateway_app.c` 接入 SSC 的应用回调。

## 1. 当前 EtherCAT 代码结构

### 1.1 关键文件

| 文件 | 作用 |
| --- | --- |
| `firmware/ecat_tsn_gateway/main.c` | 工程入口，初始化 board、ESC、SSC、TSN，并循环调用 `MainLoop()` |
| `firmware/ecat_tsn_gateway/application/gateway_app.c` | 当前真正生效的 SSC 应用层回调，包含 PDO 映射和 PDO 数据复制 |
| `firmware/ecat_tsn_gateway/SSC/Src/digital_ioObjects.h` | 对象字典定义，包含 PDO mapping、PDO 对象、普通 SDO 对象 |
| `firmware/ecat_tsn_gateway/SSC/Src/ecatappl.c` | SSC 栈主逻辑，包含 `MainInit()`、`MainLoop()`、`PDO_InputMapping()`、`PDO_OutputMapping()` |
| `firmware/ecat_tsn_gateway/SSC/Src/ecat_def.h` | SSC 配置宏，例如 `MAX_PD_INPUT_SIZE`、`MAX_PD_OUTPUT_SIZE` |
| `firmware/ecat_tsn_gateway/SSC/ESI/ECAT_IO.xml` | EtherCAT 主站导入的 ESI 文件，必须和固件对象字典匹配 |
| `firmware/ecat_tsn_gateway/CMakeLists.txt` | 编译源文件列表 |

特别注意：

```cmake
# NOTE: SSC/Src/digital_io.c is NOT compiled
sdk_src(application/gateway_app.c)
```

也就是说，`SSC/Src/digital_io.c` 是 SSC 工具生成的模板，但当前工程没有编译它。实际生效的是 `application/gateway_app.c`。

### 1.2 当前 PDO 布局

当前 PDO 是两边各 60 字节。

Master 到 Slave，称为 RxPDO：

```text
0x1C12 -> 0x1600
0x1600 -> 0x7010:0, 120 bit
          0x7011:0, 120 bit
          0x7012:0, 120 bit
          0x7013:0, 120 bit
```

Slave 到 Master，称为 TxPDO：

```text
0x1C13 -> 0x1A00
0x1A00 -> 0x6000:0, 120 bit
          0x6001:0, 120 bit
          0x6002:0, 120 bit
          0x6003:0, 120 bit
```

每个对象 120 bit，即 15 字节。4 个对象合计 60 字节。

## 2. PDO 修改方法

PDO 修改至少涉及 5 类内容：

1. PDO mapping 对象，例如 `0x1600`、`0x1A00`
2. 被映射的 PDO 数据对象，例如 `0x7010`、`0x6000`
3. 应用层 `APPL_InputMapping()` 和 `APPL_OutputMapping()`
4. SSC 最大过程数据长度配置
5. ESI XML 文件

只改固件不改 ESI，或者只改 ESI 不改固件，主站很容易出现 SM 长度不匹配，导致从站进不了 SAFEOP 或 OP。

### 2.1 理解 PDO mapping 数值格式

当前代码中有这样的 mapping：

```c
0x70100078
```

它的格式是：

```text
bits 31..16: index
bits 15..8 : subindex
bits 7..0  : bit length
```

所以：

```text
0x70100078
  index    = 0x7010
  subindex = 0
  length   = 0x78 bit = 120 bit = 15 byte
```

如果要映射一个 16 bit 对象 `0x7010:0`：

```c
0x70100010
```

如果要映射一个 32 bit 对象 `0x7011:0`：

```c
0x70110020
```

### 2.2 修改 RxPDO，也就是 Master 到 Slave

RxPDO 相关对象在 `digital_ioObjects.h` 中：

```text
0x1C12: SyncManager 2 assignment
0x1600: RxPDO mapping
0x7010-0x7013: GatewayOutputData
```

如果要把当前 60 字节透明 buffer 改成结构化控制命令，可以设计成：

| 对象 | 类型 | 长度 | 含义 |
| --- | --- | --- | --- |
| `0x7010:0` | `UINT16` | 16 bit | control word |
| `0x7011:0` | `UINT8` | 8 bit | motor id |
| `0x7012:0` | `INT32` | 32 bit | target position |
| `0x7013:0` | `INT32` | 32 bit | target velocity |
| `0x7014:0` | `INT16` | 16 bit | target torque |
| `0x7015:0` | `UINT16` | 16 bit | command sequence |

对应 `0x1600` mapping 可以改成：

```c
typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 SI1;
    UINT32 SI2;
    UINT32 SI3;
    UINT32 SI4;
    UINT32 SI5;
    UINT32 SI6;
} OBJ_STRUCT_PACKED_END TOBJ1600;

PROTO TOBJ1600 OutputCounterProcessDataMapping0x1600
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    6,
    0x70100010,
    0x70110008,
    0x70120020,
    0x70130020,
    0x70140010,
    0x70150010
}
#endif
;
```

同时要增加或修改 `0x7010` 到 `0x7015` 的对象定义。示例：

```c
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x7010 =
    { DEFTYPE_UNSIGNED16, 0x10, ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING };
OBJCONST UCHAR OBJMEM aName0x7010[] = "ControlWord\000\377";
#endif
PROTO UINT16 ControlWord0x7010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;
```

每个对象还必须加入 `ApplicationObjDic[]`：

```c
{NULL, NULL, 0x7010,
 {DEFTYPE_UNSIGNED16, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x7010,
 aName0x7010,
 &ControlWord0x7010,
 NULL, NULL, 0x0000},
```

### 2.3 修改 TxPDO，也就是 Slave 到 Master

TxPDO 相关对象：

```text
0x1C13: SyncManager 3 assignment
0x1A00: TxPDO mapping
0x6000-0x6003: GatewayInputData
```

推荐把电机反馈设计成结构化对象：

| 对象 | 类型 | 长度 | 含义 |
| --- | --- | --- | --- |
| `0x6000:0` | `UINT16` | 16 bit | status word |
| `0x6001:0` | `UINT8` | 8 bit | motor id |
| `0x6002:0` | `INT32` | 32 bit | actual position |
| `0x6003:0` | `INT32` | 32 bit | actual velocity |
| `0x6004:0` | `INT16` | 16 bit | actual torque |
| `0x6005:0` | `UINT16` | 16 bit | error code |
| `0x6006:0` | `UINT16` | 16 bit | feedback sequence |

对应 `0x1A00` mapping：

```c
typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 SI1;
    UINT32 SI2;
    UINT32 SI3;
    UINT32 SI4;
    UINT32 SI5;
    UINT32 SI6;
    UINT32 SI7;
} OBJ_STRUCT_PACKED_END TOBJ1A00;

PROTO TOBJ1A00 InputCounterProcessDataMapping0x1A00
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    7,
    0x60000010,
    0x60010008,
    0x60020020,
    0x60030020,
    0x60040010,
    0x60050010,
    0x60060010
}
#endif
;
```

### 2.4 修改 APPL_InputMapping 和 APPL_OutputMapping

当前实现是透明复制 60 字节：

```c
memcpy((uint8_t *)pData, g_pdo_input_buf, 60);
memcpy(g_pdo_output_buf, (uint8_t *)pData, 60);
```

如果改成结构化 PDO，建议定义明确的 packed 结构体，例如：

```c
typedef struct __attribute__((packed)) {
    UINT16 control_word;
    UINT8 motor_id;
    INT32 target_position;
    INT32 target_velocity;
    INT16 target_torque;
    UINT16 command_seq;
} gateway_rxpdo_t;

typedef struct __attribute__((packed)) {
    UINT16 status_word;
    UINT8 motor_id;
    INT32 actual_position;
    INT32 actual_velocity;
    INT16 actual_torque;
    UINT16 error_code;
    UINT16 feedback_seq;
} gateway_txpdo_t;
```

然后在 `APPL_OutputMapping()` 里解析 master 命令：

```c
void APPL_OutputMapping(UINT16 *pData)
{
    const gateway_rxpdo_t *rxpdo = (const gateway_rxpdo_t *)pData;

    /* TODO: 将 rxpdo->control_word、target_position 等同步给电机控制模块 */
}
```

在 `APPL_InputMapping()` 里写入反馈：

```c
void APPL_InputMapping(UINT16 *pData)
{
    gateway_txpdo_t *txpdo = (gateway_txpdo_t *)pData;

    memset(txpdo, 0, sizeof(*txpdo));
    txpdo->status_word = 0;
    txpdo->motor_id = 1;
    txpdo->actual_position = 0;
    txpdo->actual_velocity = 0;
    txpdo->actual_torque = 0;
    txpdo->error_code = 0;
    txpdo->feedback_seq++;
}
```

如果暂时仍想保留透明网关协议，就把 `60` 替换成统一宏，例如：

```c
#define GATEWAY_PDO_TOTAL_SIZE 60
```

以后改长度时只改一处，避免 `gateway_app.c`、`gateway_bridge.c`、`gateway_protocol.h` 里出现多个不一致的长度。

### 2.5 修改最大 PDO 长度

当前 `ecat_def.h`：

```c
#define MAX_PD_INPUT_SIZE  0x40
#define MAX_PD_OUTPUT_SIZE 0x40
```

`0x40` 是 64 字节。当前 60 字节没有超。

如果新的 input 或 output PDO 超过 64 字节，必须改大：

```c
#define MAX_PD_INPUT_SIZE  0x80
#define MAX_PD_OUTPUT_SIZE 0x80
```

否则 SSC 检查 SM 长度时会失败，从站可能停在 PREOP 或 SAFEOP。

## 3. SDO 修改方法

SDO 是 CoE 对象字典访问。只要对象加入 `ApplicationObjDic[]`，主站就可以通过 SDO 读写它。

### 3.1 添加只读 SDO

示例：添加固件版本 `0x8000:0`，类型 `UINT32`。

在 `digital_ioObjects.h` 中定义：

```c
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x8000 =
    { DEFTYPE_UNSIGNED32, 0x20, ACCESS_READ };
OBJCONST UCHAR OBJMEM aName0x8000[] = "FirmwareVersion\000\377";
#endif

PROTO UINT32 FirmwareVersion0x8000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0x00010000
#endif
;
```

加入 `ApplicationObjDic[]`：

```c
{NULL, NULL, 0x8000,
 {DEFTYPE_UNSIGNED32, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x8000,
 aName0x8000,
 &FirmwareVersion0x8000,
 NULL, NULL, 0x0000},
```

### 3.2 添加可读写 SDO

示例：添加 CAN bitrate 配置 `0x8001:0`，类型 `UINT32`。

```c
#ifdef _OBJD_
OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x8001 =
    { DEFTYPE_UNSIGNED32, 0x20, ACCESS_READWRITE };
OBJCONST UCHAR OBJMEM aName0x8001[] = "CanBitrate\000\377";
#endif

PROTO UINT32 CanBitrate0x8001
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 1000000
#endif
;
```

加入对象字典：

```c
{NULL, NULL, 0x8001,
 {DEFTYPE_UNSIGNED32, 0 | (OBJCODE_VAR << 8)},
 &sEntryDesc0x8001,
 aName0x8001,
 &CanBitrate0x8001,
 NULL, NULL, 0x0000},
```

主站即可通过 SDO 写：

```text
0x8001:0 = 1000000
```

### 3.3 SDO 写入后触发动作

如果某个 SDO 写入后要立即触发动作，比如重新配置 CAN、更新电机 ID，可以使用 SSC 提供的 CoE write indication。

函数指针在 `SSC/Src/applInterface.h`：

```c
pAPPL_CoeWriteInd
```

注意：`MainInit()` 会把它清空，所以必须在 `MainInit()` 之后重新赋值。

示例：

```c
static void gateway_coe_write_ind(UINT16 index, UINT8 subindex, BOOL complete_access)
{
    (void)subindex;
    (void)complete_access;

    if (index == 0x8001) {
        /* CanBitrate0x8001 已被 SSC 写入，此处通知 CAN 模块重新配置 */
    }
}
```

在 `main.c` 中：

```c
MainInit();
pAPPL_CoeWriteInd = gateway_coe_write_ind;
```

如果要在 SDO 读取前刷新数据，可以使用：

```c
pAPPL_CoeReadInd
```

例如读取 `0x8010` 前，把最新电机状态同步到对象变量。

### 3.4 哪些参数适合放 SDO

适合放 SDO 的内容：

- CAN bitrate
- RS06 node id
- ENCOS node id
- 控制模式
- 电机参数
- 超时时间
- 软件版本
- 错误统计
- 调试开关

不适合放 SDO 的内容：

- 高频目标位置
- 高频目标速度
- 高频反馈位置
- 高频反馈电流
- 周期性控制字

高频实时数据应放 PDO，低频配置和诊断数据应放 SDO。

## 4. ESI XML 修改

固件对象字典改完后，必须同步更新：

```text
firmware/ecat_tsn_gateway/SSC/ESI/ECAT_IO.xml
```

ESI 中至少要同步：

- Object Dictionary
- RxPDO mapping
- TxPDO mapping
- SyncManager assignment
- 数据类型和 bit length
- ProductCode / RevisionNo 如需区分版本，也要更新

如果 ESI 不同步，主站仍会按旧布局配置 SM2/SM3，常见现象：

- 主站扫描得到从站，但进不了 OP
- PREOP 到 SAFEOP 失败
- SAFEOP 到 OP 失败
- AL status code 指向 invalid input mapping 或 invalid output mapping
- `APPL_OutputMapping()` 不被调用
- PDO 数据错位

## 5. 推荐的电机控制对象规划

接 RS06 和 ENCOS 时，推荐从透明 60 字节 buffer 逐步改成结构化对象。

### 5.1 推荐 RxPDO

| Index | Name | Type | Direction |
| --- | --- | --- | --- |
| `0x7010` | ControlWord | `UINT16` | Master 到 Slave |
| `0x7011` | TargetMotorId | `UINT8` | Master 到 Slave |
| `0x7012` | TargetPosition | `INT32` | Master 到 Slave |
| `0x7013` | TargetVelocity | `INT32` | Master 到 Slave |
| `0x7014` | TargetTorque | `INT16` | Master 到 Slave |
| `0x7015` | CommandSeq | `UINT16` | Master 到 Slave |

### 5.2 推荐 TxPDO

| Index | Name | Type | Direction |
| --- | --- | --- | --- |
| `0x6000` | StatusWord | `UINT16` | Slave 到 Master |
| `0x6001` | ActualMotorId | `UINT8` | Slave 到 Master |
| `0x6002` | ActualPosition | `INT32` | Slave 到 Master |
| `0x6003` | ActualVelocity | `INT32` | Slave 到 Master |
| `0x6004` | ActualTorque | `INT16` | Slave 到 Master |
| `0x6005` | ErrorCode | `UINT16` | Slave 到 Master |
| `0x6006` | FeedbackSeq | `UINT16` | Slave 到 Master |

### 5.3 推荐 SDO

| Index | Name | Type | 用途 |
| --- | --- | --- | --- |
| `0x8000` | MotorProtocolSelect | `UINT8` | 选择 RS06 或 ENCOS |
| `0x8001` | CanBitrate | `UINT32` | CAN 波特率 |
| `0x8002` | Rs06NodeId | `UINT8` | RS06 节点 ID |
| `0x8003` | EncosNodeId | `UINT8` | ENCOS 节点 ID |
| `0x8010` | ControlMode | `UINT8` | 位置/速度/力矩模式 |
| `0x8020` | CommandTimeoutMs | `UINT16` | 控制超时 |
| `0x8100` | CanRxCount | `UINT32` | 诊断计数 |
| `0x8101` | CanTxCount | `UINT32` | 诊断计数 |
| `0x8102` | CanErrorCount | `UINT32` | 诊断计数 |

## 6. 修改后的验证步骤

### 6.1 编译验证

重新构建固件，确认没有重复符号、对象字典结构错误、类型未定义等问题。

重点关注：

- `APPL_GenerateMapping`
- `ApplicationObjDic`
- `TOBJ1600`
- `TOBJ1A00`
- 新增对象变量

### 6.2 启动日志验证

串口应看到：

```text
ESC hardware OK
PDO: xx bytes in, yy bytes out
Gateway running
```

确认 `xx`、`yy` 与你期望的 PDO 字节数一致。

### 6.3 主站状态验证

主站应能让从站依次进入：

```text
INIT -> PREOP -> SAFEOP -> OP
```

如果失败，优先看：

- ESI 是否更新
- `0x1600` / `0x1A00` mapping 是否正确
- `0x1C12` / `0x1C13` 是否指向正确 PDO
- `MAX_PD_INPUT_SIZE` / `MAX_PD_OUTPUT_SIZE` 是否足够
- 对象 bit length 总和是否和 SM 长度一致

### 6.4 PDO 收发验证

Master 写 RxPDO 后，应能看到：

```text
APPL_StartOutputHandler called
APPL_OutputMapping #1 called
```

如果 `APPL_OutputMapping()` 不触发，通常是：

- 从站没有进 OP
- output PDO 长度不匹配
- SM2 配置不正确
- master 没有周期性发送 output PDO

### 6.5 SDO 验证

用主站读写新增 SDO：

```text
read  0x8001:0
write 0x8001:0
read  0x8001:0
```

如果读不到：

- 对象是否加入 `ApplicationObjDic[]`
- index 是否冲突
- access 权限是否正确
- ESI 是否同步

如果写入后动作没有触发：

- `pAPPL_CoeWriteInd` 是否在 `MainInit()` 后赋值
- index 判断是否正确
- 写入对象是否为 `ACCESS_READWRITE`

## 7. 常见问题

### 7.1 改了 `digital_io.c` 但没有效果

当前工程没有编译 `SSC/Src/digital_io.c`，所以改它不会影响固件。应改：

```text
firmware/ecat_tsn_gateway/application/gateway_app.c
firmware/ecat_tsn_gateway/SSC/Src/digital_ioObjects.h
```

### 7.2 代码里 PDO 是 60 字节，主站却显示别的长度

说明 ESI 或主站工程没有更新。重新导入 `ECAT_IO.xml`，并清理主站缓存。

### 7.3 从站只能到 PREOP

通常是 mailbox 或对象字典基础信息能工作，但 PDO/SM 配置还没通过。

检查：

- `0x1C12`
- `0x1C13`
- `0x1600`
- `0x1A00`
- ESI 中的 PDO 长度

### 7.4 从站到 SAFEOP 但进不了 OP

常见原因：

- output PDO 没有被 master 周期性发送
- watchdog 超时
- `APPL_StartOutputHandler()` 返回错误
- SM2 长度和 `nPdOutputSize` 不一致

### 7.5 PDO 数据错位

常见原因：

- C 结构体没有 packed
- PDO mapping 顺序和结构体字段顺序不一致
- bit length 写错，例如 16 bit 写成 8 bit
- master 侧变量类型和从站不一致

## 8. 推荐修改顺序

1. 先只新增 SDO，不动 PDO，验证 SDO 读写。
2. 再新增一个很小的结构化 PDO，例如 control word 和 status word。
3. 确认从站能进 OP，确认 `APPL_OutputMapping()` 能触发。
4. 再逐步加入位置、速度、力矩、错误码等字段。
5. 最后再把 PDO 对接 CAN 电机控制模块。

不要一次性大改 PDO、SDO、CAN、电机状态机。EtherCAT 从站调试时，最容易出问题的是 ESI 和对象字典不一致，建议每次只改一小步。
