# 设备模型与扩展方法

## 为什么需要统一模型

总线和厂商协议是实现细节；控制器需要的是带单位、时间、质量和能力的语义信号。设备类型（例如电机或 IMU）与设备实例（某个序列号/关节）必须分开，实例不能用数组位置或 CAN ID 充当业务身份。

### 稳定标识

- `device_uid`：跨重启稳定，推荐 `vendor/product/serial` 或明确的人工 UUID。
- `semantic_name`：机器人模型中的名称，例如 `arm/joint_3`、`imu/base`。
- `endpoint_key`：实际总线地址 `(transport, bus, address, id_format)`，只在配置层使用。
- `protocol_ref`：`reverse-dns-id@wire-major`，例如 `com.robstride.rs06.private@1`。

配置加载时拒绝重复 `device_uid`、重复 endpoint、语义名称冲突、未知总线、越界限值、单位不一致和预算超限。

## Canonical 数据模型

所有 driver 向上提供固定、类型化的结构；RT 不使用字符串 key/value：

```text
CommandFrame {
  header: magic, schema, sequence, source_id, monotonic_ns, valid_until_ns, crc32
  devices[]: { device_uid, mode, enable, position, velocity, effort,
               feedforward, valid_mask }
}

StateFrame {
  header: sequence, sample_time_ns, receive_time_ns, crc32
  devices[]: { device_uid, position, velocity, effort, status_bits,
               fault_bits, quality, feedback_sequence }
}

SensorSample {
  device_uid, sample_time_ns, receive_time_ns, sequence, frame_id,
  valid_fields, values, covariance, quality, age_ns
}
```

数值统一 SI 单位（rad、rad/s、N·m、m/s²、rad/s），字段名也写单位；拒绝 NaN/Inf。无效值使用 `valid_fields/quality`，不能把旧值伪装成新样本。`sample_time` 与 `receive_time` 必须同时保留，时钟源和转换不确定度写入诊断。

### 执行器能力

至少声明：position、velocity、effort/torque、enable、mode、fault、reset；每种能力带命令范围、变化率、周期、反馈新鲜度、控制模式和资源 claim。电源、夹爪、LED 等非周期设备不强行伪装成关节，可使用 lifecycle driver + service/action，但仍经过 authority 和 safety supervisor。

### 传感器能力

至少声明：测量字段、单位、坐标系/frame_id、采样率、延迟、时间源、协方差/质量、校准版本、缓存深度和丢帧策略。DMA/ISR 只把字节放入有界 ring；解析和滤波按预算在 RT 或非 RT 明确层执行。

## 设备描述文件

人工 YAML 只是输入，不能在 RT 运行时解析。推荐 JSON Schema + 语义 lint，编译成带 SHA-256 的 `compiled_profile.bin`：

```yaml
schema_version: 1
device_uid: vendor.product.serial-001
semantic_name: arm/joint_3
class: actuator/joint
identity:
  vendor: RobStride
  product: RS06
  firmware: ">=2.1,<3.0"
transport:
  kind: can
  bus: can0
  bitrate: 1000000
  id: 13
  id_format: extended
protocol: com.robstride.rs06.private@1
capabilities: [position, velocity, torque, enable, fault]
signals:
  position: {type: float32, unit: rad, range: [-12.57, 12.57], freshness_ms: 5}
  velocity: {type: float32, unit: rad/s, range: [-30.0, 30.0]}
  torque: {type: float32, unit: N*m, range: [-20.0, 20.0]}
timing:
  period_us: 1000
  phase_us: 0
  tx_frames: 1
  rx_frames: 1
  deadline_us: 800
  priority: 20
safety:
  command_timeout_ms: 100
  fail_action: disable
  max_velocity: 10.0
  max_torque: 8.0
calibration:
  direction: 1
  position_offset_rad: 0.0
  revision: zero-2026-01
ros2_control:
  joint: arm_joint_3
  command_interfaces: [position, velocity, effort]
  state_interfaces: [position, velocity, effort]
```

字段规范：

- `schema_version`、`protocol`、固件匹配、配置 hash 必填；缺失即不能 ACTIVE。
- endpoint、bitrate、帧格式和设备身份必须可被离线工具验证；禁止内嵌脚本或任意 `.so` 路径。
- `timing` 用于 admission，`safety` 用于生成 RT 限制；ROS/URDF 限制不能取代 RT 再校验。
- 生成静态 device table、ROS 参数、URDF 映射、测试样例和诊断枚举；启动时打印 fingerprint。

## Driver 生命周期

设备 driver 的管理面与实时面分开：

```text
configure -> discover -> identify -> prepare -> ready
                                      -> activate -> read_rt/write_rt
                                      -> deactivate -> recover -> shutdown
```

管理面允许阻塞、重试、参数读写和日志；`read_rt/write_rt` 只处理固定结构，不能分配、锁、等待、访问 ROS 或解析配置。未知设备进入 `QUARANTINED`，只读探测和诊断，不能自动使能。

## 新增执行器步骤

1. 填写 descriptor：身份、endpoint、协议、能力、单位、限位、校准、周期、watchdog 和 fail-action。
2. 运行 schema/lint、endpoint 冲突检查、总线 admission 和资源 claim 检查。
3. 复用或实现纯 codec；实现 driver 生命周期，把 wire 字段映射到 canonical command/state。
4. 在 scheduler 注册周期 slot、相位、优先级和 RX/TX budget；Safety 在写帧前做限幅、斜率、finite、状态和新鲜度检查。
5. 先 virtual transport/SIL，再 loopback、单设备低速 HIL、逐总线扩展；测试断线、复位、反向、限位、急停和电源掉电。
6. 在 `ros2_control` 导出标准 command/state interface；使能、模式切换、清错使用 lifecycle/service/action，不把厂商寄存器变成公共运动接口。
7. 更新拓扑、配置锁、诊断、运行手册和回滚包；先 read-only/shadow，再 canary，最后放量。

## 新增传感器步骤

1. 声明字段、SI 单位、frame、采样率/延迟、时间源、valid mask、协方差、校准和 stale 策略。
2. transport/driver 采用 DMA 或非阻塞读；ISR 只入 ring，RT 每周期消费有限字节，非 RT 处理低频配置。
3. 每个样本写入 `sample_time`、`receive_time`、sequence、quality、age 和丢帧/CRC 计数；保留 latest snapshot 供控制，保留有界 ring 供录包。
4. 发布优先使用 `sensor_msgs`/`geometry_msgs`/`tf2`；缺失的稳定语义字段才进入版本化 `robot_interfaces`。高频大数据使用有界 best-effort/共享内存，不让 DDS 回调解析原始帧。
5. 加入仿真传感器、回放、校准和 HIL；验证时钟漂移、TF、协方差、断流、重复/乱序和温度异常。

## 资源与模式切换

`ros2_control` resource manager 负责控制器 claim，但最终资源检查在 RT 启动和 mode switch 边界再次执行。模式切换顺序为：撤销旧 lease -> 请求 RT 在 cycle boundary 停止 -> 应用静态 mode table -> 读取确认状态 -> 新 controller claim -> 重新激活。失败回退到 SAFE，不在活动周期中修改 codec、endpoint 或 PDO 布局。
