# 接口契约与命名基线

本文把架构原则收敛成可供实现和评审引用的契约。字段名、错误码和 topic 是建议的 v1 基线；在代码落地前需要生成 IDL、C ABI 头文件和 schema，并通过契约测试锁定。

## 版本层次

不要把所有版本拼成一个字符串。运行时同时记录：

```text
robot_interfaces_schema   # ROS msg/srv/action 与字段语义
ipc_schema                # 进程间固定帧布局
device_descriptor_schema  # YAML/JSON 输入格式
protocol_wire_version     # 线缆字节语义
protocol_codec_abi        # host/firmware 函数表 ABI
device_firmware_version   # 设备实际固件
pdo_layout_revision       # EtherCAT PDO/ESI 布局
```

每层都有兼容规则和 hash。破坏性字段变化创建新 major/v2；minor 只能追加可忽略字段；不能用同一版本重新解释旧字段。启动健康报告必须同时显示这些版本和 `config_fingerprint`。

## IPC 固定帧

共享内存和 UDS 使用相同的 wire layout；不要直接共享 C++ 对象或包含指针的结构体。推荐 little-endian、显式宽度类型和 64 字节对齐：

```c
struct robot_ipc_header {
  uint32_t magic;             /* constant, rejects random bytes */
  uint16_t schema_version;
  uint16_t frame_type;        /* command/state/event/heartbeat */
  uint32_t header_bytes;
  uint32_t payload_bytes;
  uint64_t sequence;
  uint64_t monotonic_ns;
  uint64_t valid_until_ns;    /* zero only for non-command frames */
  uint32_t robot_id_hash;
  uint32_t flags;              /* valid/safe/stale bits */
  uint32_t payload_crc32;
  uint32_t header_crc32;
};
```

约束：

- `payload_bytes` 必须等于该 `frame_type/schema_version` 的已知大小；多余字节不得静默解释。
- 生产者先写 payload 和 header 字段，最后以 release store 发布 sequence/slot；消费者 acquire 后再次校验 magic、大小、CRC 和 `valid_until`。
- command 使用双槽 latest-value；state 使用 latest snapshot 或固定容量 ring；event 使用独立 ring。每种通道统计覆盖、丢弃、CRC 错、版本拒绝和高水位。
- 序号回退、重复或跨 robot 的帧拒绝；时钟只用 `CLOCK_MONOTONIC` 做超时，ROS 时间仅用于展示/录包。

## Canonical 命令与状态

### 命令语义

```text
CommandFrame v1
  source_id: bounded string/uint32 identity (not a CAN id)
  lease_id: uint64, expires with valid_until_ns
  sequence: monotonic per source
  mode: position | velocity | effort | stop
  enable: false by default
  valid_mask: explicit field/axis validity
  values: SI units; finite and within profile limits
```

命令接收分三道检查：ROS ingress（类型、权限、来源） -> `command_mux`（资源 claim、优先级、lease、deadman、sequence） -> RT（状态、限位、斜率、反馈 age、deadline）。任何一道失败都不执行运动输出，并留下稳定 `reason_code`。

### 状态语义

```text
StateFrame v1
  sample_time_ns       # device/measurement time
  receive_time_ns      # transport/host arrival time
  publish_time_ns      # ROS projection time, optional
  feedback_sequence
  quality: valid | stale | degraded | invalid
  status_bits, fault_bits, counters
```

`quality=stale/invalid` 时仍可发布最后一帧用于诊断，但控制器必须拒绝把它当作新鲜反馈。所有温度、电压、位置和力矩字段声明单位、精度、范围和缺省策略。

## ROS 2 API 基线

统一 namespace：`/robot/<robot_id>/...`。新 API 优先标准消息，设备私有信息进入版本化扩展：

| 用途 | 推荐接口 | QoS/语义 |
| --- | --- | --- |
| 关节状态 | `sensor_msgs/JointState` 或 `control_msgs` 状态 | best-effort、latest、采样时间 |
| 轨迹 | `control_msgs/action/FollowJointTrajectory` | reliable action、取消/抢占/结果码 |
| 高频设定 | `robot_interfaces/msg/CommandFrame` | keep-last 1、best-effort、volatile、deadline/lifespan |
| IMU/温度 | `sensor_msgs/Imu`、`sensor_msgs/Temperature` | SensorDataQoS、quality/age 诊断 |
| 健康 | `diagnostic_msgs/DiagnosticArray` + health msg | reliable、稳定 reason/event |
| arm/mode/reset | 版本化 service/action | reliable、权限、幂等 request_id |
| 静态描述 | `robot_interfaces/msg/RobotDescription` | reliable + transient-local、hash |

推荐 topic：

```text
/robot/r01/joint_states
/robot/r01/control/command
/robot/r01/control/arm
/robot/r01/imu/data
/robot/r01/health
/robot/r01/diagnostics
/robot/r01/rt_metrics
```

运动 topic 不使用 `TRANSIENT_LOCAL`；它只适合描述/标定等晚加入者数据。命令消息不能用无界数组/字符串；大图像/点云使用有界 QoS 或专用共享内存通道。

## 错误模型

错误码分域且稳定，至少包括：

```text
IPC_SCHEMA_MISMATCH, IPC_CRC_ERROR, COMMAND_EXPIRED, COMMAND_NO_LEASE,
COMMAND_OUT_OF_RANGE, RESOURCE_CLAIM_CONFLICT, RT_DEADLINE_MISS,
BUS_WKC_ERROR, BUS_OFF, FRAME_FORMAT_ERROR, FRAME_CRC_ERROR,
DEVICE_NOT_IDENTIFIED, DEVICE_FEEDBACK_STALE, DEVICE_FAULT,
SAFETY_INPUT_OPEN, CONFIG_SIGNATURE_INVALID, CONFIG_BUDGET_EXCEEDED
```

每个错误包含 severity、first_seen、last_seen、count、latch/clear policy、fail-action 和恢复建议。不要把所有失败折叠成 `ok=false`，也不要以可变日志文本作为机器接口。

## 兼容与弃用

- `hpm_gateway_msgs` 和 `/hpm_gateway/...` 作为 legacy v0 adapter 保留，禁止新设备直接依赖；每个字段写明转换、精度损失和弃用版本。
- 新 `robot_interfaces/v1` 只接受 logical/device UID，不暴露 CAN ID/PDO offset；厂商诊断使用独立扩展消息。
- adapter 双向转换必须有单一 writer、sequence 映射和过期语义；旧/新同时运行时默认 shadow/read-only，避免双重发送。
- 删除 legacy 前必须有迁移统计、至少一个回滚版本、文档和现场验证记录。
