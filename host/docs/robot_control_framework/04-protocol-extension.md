# 协议扩展与插件契约

## 五层边界

```text
Transport -> Framing -> Codec -> Device Profile -> Driver/Scheduler
```

- **Transport**：SocketCAN/CAN-FD、EtherCAT raw socket、UART/RS-485、UDP、SPI/GPIO；负责端口配置、非阻塞收发、硬件时间戳、过滤器、总线状态和错误计数，不理解电机语义。
- **Framing**：DLC/长度、分片、转义、CRC、重同步和序号；不能等待无界字节或调用设备 API。
- **Codec**：wire bytes 与 typed frame 的纯函数；负责端序、位域、缩放、范围、CRC 和错误码。
- **Device Profile**：某型号设备的寄存器、能力、模式和时序；不把 profile 差异散落在 scheduler。
- **Driver/Scheduler**：生命周期、周期 slot、命令仲裁、反馈 freshness 和安全动作。

现有 `protocols/rs06`、`encos`、`yesense` 的“纯编解码，不碰 CAN/FreeRTOS/ROS”模式应保留；`gateway_app`/SSC 回调只做 PDO 快照拷贝和序号处理。

## 协议描述与稳定 ID

每个协议目录至少包含：

```text
protocols/<reverse-dns-id>/<wire-major>/
  spec.yaml             # 线缆规范、预算、错误和兼容性
  codec.h/.c            # RT-safe 编解码
  profile.yaml          # 设备型号映射（可多个）
  vectors/              # golden frames
  tests/                # 单测、属性、fuzz、回放
```

注册键是 `(protocol_id, wire_major, codec_abi)`，例如 `com.robstride.rs06.private@1`。`spec.yaml` 必须声明：

- wire semver、schema hash、支持的 transport/frame 格式、DLC/位速率和最坏帧预算；
- 端序、位域、单位、范围、饱和/拒绝策略、保留位和 CRC；
- command/feedback/discovery/service 的时序、幂等性、超时、重试上限和故障动作；
- 设备固件兼容矩阵、探测/识别规则、能力位和安全默认值；
- golden frame 来源（抓包、供应商文档、版本）和隐私处理方式。

## RT-safe 接口

接口可以是稳定 C ABI，也可以是静态 C++ vtable；语义示例：

```cpp
struct ProtocolDescriptor {
  const char *protocol_id;
  uint16_t wire_major;
  uint16_t codec_abi;
  uint8_t schema_hash[32];
  uint64_t capabilities;

  ProbeResult (*probe)(const FrameView &, ProbeReply *) noexcept;
  CodecError (*validate_frame)(const FrameView &) noexcept;
  CodecError (*encode_cyclic_rt)(const CanonicalCommand &, FrameOut *) noexcept;
  CodecError (*decode_cyclic_rt)(const FrameView &, CanonicalState *) noexcept;
  CostEstimate (*estimate_cost)(const DeviceConfig &) noexcept;
};
```

RT 函数必须定长、可重入、无分配、无锁、无阻塞、无异常、无 printf/DDS，并返回明确错误码和 consumed length。探测、参数读写、固件升级等管理函数可以阻塞，但只能在非 RT 控制面运行。不要把 `float` 的 NaN/Inf 当作“未提供”；由 valid mask 表示。

## 注册、加载与安全

- host 可用 `pluginlib/class_loader` 或稳定 `.so` C ABI；只在 configure 阶段 `dlopen`，校验签名、ABI、schema hash、依赖和 allowlist，随后冻结不可变 vtable。
- `robot_rt` 启动前完成所有 plugin load、设备发现和静态表生成；进入 ACTIVE 后禁止 `dlclose`、热替换或从 ROS 参数加载任意路径。
- 固件使用编译期 registry/生成表，不支持动态装载；安全关键协议优先静态内置。
- 注册器拒绝重复 ID/版本、缺能力/单位/限值、未知 transport 或预算超限。发现成功不等于授权，设备 allowlist 和 operator approval 是独立条件。

## 新增协议门禁

1. **协议卡/RFC**：定义 wire 字段、时序、错误、重试/幂等、安全停机和预算，分配不可变 ID 与 major。
2. **参考向量**：正常、边界、短帧、错误 ID/CRC、保留位、旧版本、NaN/Inf 和设备复位；记录抓包来源。
3. **纯 codec**：双向 encode/decode、范围策略和错误码；不修改业务 manager 的大 switch。
4. **虚拟 transport**：注入丢帧、乱序、重复、延迟、损坏、bus-off 和设备重启；验证状态机和超时。
5. **设备 profile/注册**：声明能力、周期、frame filter、资源和 ROS canonical mapping；生成配置和诊断枚举。
6. **验证**：主机单测、property/fuzz、ASan/UBSan、交叉编译、WCET、总线负载、loopback/HIL、长稳和安全审查。
7. **发布**：manifest + codec + profile + generated artifacts + tests + hash/signature 组成不可变 bundle；先 read-only/shadow，再单设备 canary，再全量。

## 版本与协商

独立管理五类版本：wire semver、plugin ABI、device firmware、descriptor schema、ROS API/IDL。wire major 变化必须新建 profile/codec；minor 只能添加可忽略字段/能力；patch 只修实现，禁止同版本重新解释字段。

启动 probe 记录实际 firmware、wire revision、能力 bitmap、schema hash 和 PDO layout hash。双方不支持协商时只能使用明确的兼容 adapter/allowlist，不能“看起来像”就解析。EtherCAT PDO/object dictionary 变化必须同步递增 ESI RevisionNo、更新 ESI/XML/EEPROM 镜像和主站缓存。

## 发布、回滚与不可逆命令

插件和 descriptor 采用 A/B slot，切换前停机、校验签名/依赖/预算/设备身份，并保留 N-1 版本。激活后健康门要求连续 N 个周期无 deadline/WKC/CRC/超时、反馈新鲜且故障为零；失败自动回到上一 slot 并保留 raw trace。

设备参数写入（CAN ID、波特率、协议模式、save 等）可能持久且不可原子回退：先读取备份、使用非持久配置验证、回读确认后才显式保存；非幂等命令超时不得盲目重发。主机回滚、设备参数恢复和硬件安全停机分别记录，不能假设一个动作覆盖全部。

## 当前仓库的适配注意

- 当前 `can_if_frame_t` 只有 8 字节，虽有 CAN-FD 枚举；扩展必须显式支持最多 64 字节、DLC 映射和新 ABI，禁止静默截断。
- 当前 MCAN 实现使用非阻塞 TX FIFO；接入 RT core 前仍应测量每 bus 队列容量、满载丢帧/重试语义和最坏提交时间，并把它们写入 transport 契约。
- 16 轴 PDO、ESI 和对象字典是独立契约；布局改变时必须更新 revision/hash 和主站兼容测试。
- 协议 dispatch 使用 `(transport,bus,address,profile)` O(1) 查表，避免每周期线性试探不同协议。
