<!-- Author: BearKing -->
# C/C++ 编码与命名规范

版本：2026-07-28（试行）

适用范围：本仓库手写的 C/C++ 代码、测试和主机工具。SSC、HPM SDK、SOEM
及其生成文件保持上游风格；它们只允许做与集成或安全直接相关的最小修改。

## 1. 目的与规范词

本规范保证 EtherCAT PDO、CAN 帧、硬件接口和实时路径在重构时仍可审查、构建
和兼容。

- **必须**：正确性、安全性、兼容性或已配置构建所要求。
- **应该**：默认规则；偏离时在代码或评审中说明具体理由。
- **可以**：不破坏一致性时的可选做法。

## 2. 事实来源与优先级

规则按以下优先级裁决：安全与实时约束、外部契约、构建配置、稳定的本地模式、
外部指南。当前证据是：

- `firmware/ecat_tsn_gateway/CMakeLists.txt`：HPM SDK 固件构建；本仓库未显式
  固定 C 标准。
- `host/rdk_ecat_motor_rt/Makefile`：主机 C11、`-Wall -Wextra` 和 SOEM/Linux 构建。
- `firmware/ecat_tsn_gateway/app/gateway_app.h`：320-byte RxPDO 和
  252-byte TxPDO 契约。
- `firmware/ecat_tsn_gateway/protocols/encos/encos_proto.[ch]`、`motor/`：现有
  `lower_snake_case` 函数与 `UPPER_SNAKE_CASE` 宏的主导模式。

仓库未配置 `.clang-format`、`.clang-tidy`、cppcheck 或统一 CI 格式检查；本规范
不声称这些工具已启用。

## 3. 兼容与采用策略

- 已发布或跨模块使用的 PDO/SDO 字段、CAN 字段、对象索引、命令行选项、SOEM/HPM
  符号和生成代码名称**必须**保持不变，除非另有兼容计划。
- 新的手写 C 代码**必须**遵循第 4–12 节；本规范不要求批量重命名或重排旧代码。
- 修改旧代码时，**应该**只在被修改的函数/头文件内改善命名和注释；协议或 ABI
  变更必须拆为单独任务，并列出消费者和回滚方式。

## 4. 语言与工具链

| 项目 | 当前事实 | 新代码规则 |
| --- | --- | --- |
| Linux/SOEM 主机 C | 现有主机已使用 C11 或 GNU C | 必须显式 `-std=c11`；新增警告视为缺陷 |
| HPM 固件 C | SDK 构建未在仓库显式固定语言标准 | 必须兼容该 SDK 的实际编译设置；不得把主机 C11 特性直接假定为可用 |
| C++ | 当前无 C++ 目标或语言版本证据 | 新增目标必须在构建中显式声明语言标准后才可加入；命名规则见第 5 节 |
| 警告 | 主机目标使用 `-Wall -Wextra` | 新主机代码应使用两者；`-Werror` 可用于独立、无历史警告的目标 |
| 格式化 | 未配置工具 | 采用本文件格式规则，不声称自动强制 |

## 5. 文件与命名

| 对象 | C 规则 | C++ 规则 | 示例 |
| --- | --- | --- | --- |
| 文件/目录 | `lower_snake_case` | `lower_snake_case` | `ecat_motor_rt.c` |
| 类型 | 模块前缀 `lower_snake_case`，不用裸 `_t` | `UpperCamelCase` | `gateway_rt_output_pdo` |
| 公共 C 函数 | `<module>_<verb>_<object>` | 建立目标时定义类 API | `gateway_rt_parse_pdo` |
| 文件内函数/变量 | `static lower_snake_case` | 匿名命名空间中的 `lower_snake_case` | `configure_master` |
| 宏/编译期常量 | 模块前缀 `UPPER_SNAKE_CASE` | 优先 `constexpr` | `GATEWAY_RT_INPUT_SIZE_BYTES` |
| 枚举值 | 模块前缀 `UPPER_SNAKE_CASE` | `EnumType::kValue` | `GATEWAY_RT_STATE_READY` |
| 布尔值 | `is_`、`has_`、`can_`、`should_` | 同左 | `has_sample` |

避免 `data`、`info`、`helper`、`utils`、`manager` 等无领域含义的名称。文件内
`static` 函数可省模块前缀，但名称仍必须表达动作和对象。POSIX 将以 `_t` 结尾的
标识符保留给实现；新类型不得使用裸 `_t` 后缀。

| 缩写 | 允许形式 | 说明 |
| --- | --- | --- |
| EtherCAT | `ecat`（文件内）/`EtherCAT`（文档） | 协议名保持原拼写 |
| PDO | `pdo` | 过程数据对象 |
| IMU | `imu` | 惯性测量单元 |
| CAN | `can` | 控制器局域网 |
| SOEM | `soem` | 外部库名，不重命名 |
| WKC | `wkc` | EtherCAT working counter |

## 6. 单位、布尔值与数据布局

- 易混淆单位**必须**写入名称：`cycle_us`、`timeout_ms`、`position_rad`、
  `temperature_c`、`size_bytes`。时间基准应写明，例如 `monotonic_time_us`。
- `*_mask` 只用于实际位掩码；单一真值使用 `bool` 和疑问式名称。
- 网络、PDO、CAN、寄存器和持久化字段**必须**用固定宽度整数；对象/缓冲区长度
  使用 `size_t`，指针差使用 `ptrdiff_t`。
- 原生结构体只有在端序、浮点格式、对齐和大小已明确时才能作为协议视图；这类
  布局必须使用 `_Static_assert` 验证大小，必要时验证偏移。不得仅靠 `packed` 猜测
  可移植性。

## 7. C 头文件、API 与错误处理

- 每个头文件**必须**自包含，使用带模块前缀且非保留的 include guard；不得使用
  双下划线或 `_` 加大写字母。POSIX 明确规定的 feature-test 宏（例如
  `_POSIX_C_SOURCE`）是外部平台契约，可作为唯一例外。
- 公共函数必须记录指针的可空性、借用/所有权、缓冲区容量和输出长度、线程安全、
  阻塞/超时以及失败语义。文件内函数应保持 `static`。
- 错误边界必须使用一种明确语义：布尔值仅用于“成功/失败”且调用方有上下文；
  需要区分原因时使用受文档约束的错误码。不得把失败替换为伪成功。
- `errno` 只能在紧邻 POSIX 边界处解释；高频循环不得无界打印。资源按取得的反序
  释放。

### C++（未来目标）

- C++ 目标必须显式声明语言版本、异常策略和 C ABI 边界；不得让异常穿过 SOEM、
  pthread 或 C 回调。
- 资源应由 RAII 类型管理，时间量应优先使用 `std::chrono`；与 PDO/CAN 等序列化
  契约交互时仍必须使用明确宽度和单位的字段。
- 仅在所有权语义明确时使用智能指针；实时循环不得隐式分配、抛异常或依赖无界
  容器增长。

## 8. 并发、实时与信号

- 每份可变状态**必须**有唯一所有者。SOEM `ecx_contextt` 和其 raw socket 必须
  线程封闭：一个运行期线程拥有全部 `ecx_*` 调用。
- 跨线程统计使用 C11 原子；`volatile` 只用于信号标志、MMIO 或已有平台边界，不能
  作为线程同步。
- 实时周期使用绝对单调时间；不得用相对睡眠抵消工作耗时。周期线程不得打印、做
  文件 I/O、动态分配或无限重试。
- API 和注释必须明确取消、超时、关闭、背压与生命周期。安全停机不能假设“零值
  命令”等同于物理断能，除非网关契约明确如此。

## 9. 格式、注释与测试

- 使用 4 空格缩进、K&R 大括号、每条声明一个逻辑对象；不对无关文件做格式化。
- 行宽应尽量不超过 100 列；无法自然断开的协议字符串、宏和 URL 可例外。
- 注释解释协议来源、单位、并发所有权、硬件限制或反直觉原因；TODO 必须包含负责
  人/问题号和移除条件。
- 新的纯逻辑或布局代码必须有可重复主机测试。协议/实时变更还必须进行目标构建和
  硬件 WKC/时序验证；未运行的验证必须如实记录。

## 10. 示例

| 不推荐 | 问题 | 推荐 |
| --- | --- | --- |
| `int timeout` | 单位未知 | `uint32_t rx_timeout_us` |
| `struct data_t` | 无领域含义且 `_t` 受限 | `gateway_rt_input_pdo` |
| `volatile bool ready` | 不是线程同步 | `atomic_bool is_ready` |
| `get_frame()` | 未说明是否阻塞 | `receive_frame(timeout_us)` |
| `#define SIZE 252` | 无模块/单位 | `GATEWAY_RT_INPUT_SIZE_BYTES` |

## 11. 代码审查清单

- [ ] 名称、单位、布尔值、缩写和宏符合第 5–6 节。
- [ ] 没有误改 PDO/CAN/SDO、ABI、生成代码或外部库契约。
- [ ] 头文件自包含；布局存在必要的编译期断言。
- [ ] 所有权、空值、阻塞、超时、错误和关闭语义可从 API 看出。
- [ ] 并发状态有唯一所有者，未把 `volatile` 当同步原语。
- [ ] 高优先级路径无高频日志、文件 I/O、分配或无界重试。
- [ ] 已运行与改动相称的构建、测试和静态检查；限制已记录。

## 12. 参考

仓库依据见第 2 节。外部资料（Google C++ Style Guide、LLVM Coding
Standards、SEI CERT C/C++、C++ Core Guidelines）仅作为输入；与本仓库的实时、
协议和 ABI 契约冲突时，以这些契约为准。
