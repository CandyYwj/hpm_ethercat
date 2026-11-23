<!-- Author: BearKing -->
# 相对 `ecat_io` 的 EtherCAT 改动

本文只讨论 EtherCAT 这一侧，不讨论 TSN 交换机部分。对照基线是：

- `D:\Tools\hpm\sdk_env-main\hpm_app\software\apps\ecat_io`

这个基线本质上是一个标准的 HPM EtherCAT 数字 IO 从站示例，而当前工程已经把它改造成了一个“报文型 EtherCAT 从站”，用于给 TSN 网关提供 60 字节双向 PDO 通道。

## 1. 先说结论

相对 `ecat_io`，当前工程在 EtherCAT 侧的改动可以概括成四类：

1. 把应用语义从“数字 IO”改成了“网关报文通道”。
2. 把 PDO 模型从“单个 32-bit 变量”改成了“60 字节双向块数据”。
3. 把应用回调从“直接读写 GPIO”改成了“和桥接缓冲区交互”。
4. 把主站测试方式从“32-bit 输入输出验证”改成了“持续 PDO 帧收发 + OP 过渡期喂 PDO”。

## 2. 应用定位变了

`ecat_io` 的 EtherCAT 语义很简单：

- 输入 PDO 对应板子 DIP/输入引脚状态。
- 输出 PDO 对应板子 LED/输出引脚状态。
- 本质是一个 8DI + 8DO 示例。

当前工程的 EtherCAT 语义已经完全变了：

- 输入 PDO 承载 `Jetson -> HPM -> RDK` 的网关消息。
- 输出 PDO 承载 `RDK -> HPM -> Jetson` 的网关消息。
- GPIO 不再是 EtherCAT 应用层的核心业务对象。

也就是说，`ecat_io` 是“控制 IO”，当前工程是“传输消息”。

## 3. PDO 对象字典的核心变化

这是 EtherCAT 侧最关键的改动。

## 3.1 `ecat_io` 的 PDO 很小

在 `ecat_io/SSC/Src/digital_ioObjects.h` 里：

- `0x6000 = UINT32 InputCounter`
- `0x7010 = UINT32 OutputCounter`
- `0x1A00` 只映射一个 `0x6000:0`
- `0x1600` 只映射一个 `0x7010:0`

所以它本质上是：

- 输入 4 字节
- 输出 4 字节

## 3.2 当前工程的 PDO 变成了 60 字节双向缓冲区

在 `firmware/ecat_tsn_gateway/SSC/Src/digital_ioObjects.h` 里：

- 输入对象从 `0x6000` 扩成 `0x6000 ~ 0x6003`
- 输出对象从 `0x7010` 扩成 `0x7010 ~ 0x7013`
- 每个对象 15 字节
- 4 个对象拼成 60 字节

映射关系也随之变化：

- `0x1A00 = 4 个输入片段`
- `0x1600 = 4 个输出片段`

所以 EtherCAT 主站现在看到的是：

- `OBits = 480`
- `IBits = 480`

而不再是 `32 / 32`。

## 3.3 为什么不是直接定义一个 60 字节对象

这是一个很实际的 SSC 工程问题。

当前实现没有把整个 PDO 直接做成一个大块 `UINT8[60]` 单对象，而是拆成 `4 x OCTET_STRING[15]`，原因是：

1. 这更容易和 SSC 生成的对象字典结构兼容。
2. 映射项长度、对象类型和 ESI/EEPROM 保持一致更直接。
3. 现有实现已经验证可稳定进入 `OP`。

所以这里不是随便切成 4 段，而是一个面向 SSC/ESI 工具链的工程折中。

## 4. 应用回调的职责变了

## 4.1 `ecat_io` 的回调直接操作应用变量

在 `ecat_io/application/digital_io.c` 中：

- `APPL_InputMapping()` 直接把 `InputCounter0x6000` 拷到 PDO
- `APPL_OutputMapping()` 直接把 PDO 拷到 `OutputCounter0x7010`
- `APPL_Application()` 再做：
  - `InputCounter0x6000 = APPL_GetDipSw();`
  - `APPL_SetLed(OutputCounter0x7010);`

也就是说它的应用层路径是：

- EtherCAT PDO <-> 局部变量 <-> GPIO

## 4.2 当前工程的回调只负责和桥接层交换

在 `firmware/ecat_tsn_gateway/application/gateway_app.c` 中：

- `APPL_OutputMapping()`
  - 把 60 字节输出 PDO 拷到 `g_pdo_output_buf`
  - 置位 `g_pdo_output_updated`
- `APPL_InputMapping()`
  - 如果 `g_pdo_input_updated`
  - 就把 `g_pdo_input_buf` 拷到输入 PDO
  - 然后清标志

这里已经不再有：

- `APPL_GetDipSw()`
- `APPL_SetLed()`
- 直接 GPIO 读写

它的应用层路径变成了：

- EtherCAT PDO <-> 桥接缓冲区 <-> `gateway_bridge.c`

这说明 EtherCAT 应用层已经从“终端设备逻辑”改成了“消息搬运逻辑”。

## 5. 主循环结构也变了

## 5.1 `ecat_io` 是纯 EtherCAT 主循环

`ecat_io/main.c` 的核心循环几乎只有：

```c
while (bRunApplication == TRUE) {
    MainLoop();
    ...
}
```

外围逻辑主要是：

- LED 心跳
- 按键消抖
- 打印按键事件

它没有额外的数据面轮询模块。

## 5.2 当前工程是 EtherCAT + 桥接的双主循环模型

当前工程 `main.c` 的主循环是：

```c
while (bRunApplication == TRUE) {
    MainLoop();
    gateway_bridge_poll();
    ...
}
```

其中：

- `MainLoop()` 仍然是 SSC/EtherCAT 栈主循环
- `gateway_bridge_poll()` 是新增的网关数据面轮询

从 EtherCAT 视角看，最大的差别是：

- `ecat_io` 的 EtherCAT 数据在应用回调里就闭环结束了
- 当前工程的 EtherCAT 数据在应用回调里只是进入桥接层，后面还要继续去 TSN 侧

## 6. 设备元数据做了定制

EtherCAT 设备元数据虽然没大改 Vendor/Product，但已经不再是原始数字 IO 身份。

## 6.1 `ecat_def.h`

相对 `ecat_io`：

- `REVISION_NUMBER`
  - 从 `0x00000001` 改为 `0x00000002`
- `DEVICE_NAME`
  - 从 `digital_io` 改为 `ecat_tsn_gateway`
- `DEVICE_SW_VERSION`
  - 当前工程代码里是 `2.0`

这说明当前从站虽然 Product Code 沿用，但软件身份已经明确升级为网关版本。

## 6.2 SSC 配置 XML

在 `SSC/Config/HPM_ECAT_IO_Config.xml` 中：

- `Configuration Name` 从 `HPMicro ECAT Digital IO` 改为 `HPMicro ECAT TSN Gateway`
- `Version` 从 `1.0.0.0` 改为 `2.0.0.0`
- `DEVICE_NAME` 从 `ECAT_IO` 改为 `ecat_tsn_gateway`
- `REVISION_NUMBER` 改成 `0x00000002`

这些改动不是装饰，它们决定了：

- 生成出来的 `ecat_def.h`
- ESI 元数据
- EEPROM 镜像内容

## 6.3 EEPROM 镜像随之变化

当前工程 `SSC/Src/eeprom.h` 已经不是 `ecat_io` 的 EEPROM 内容，而是匹配新对象字典和新字符串表的镜像。

EtherCAT 里这点非常关键：

- 对象字典
- ESI
- EEPROM

三者必须一致。

## 7. `OP` 进入条件虽然逻辑没改，但现实用法变了

`ecat_io` 这类简单示例在主站侧通常只要普通状态切换就能工作，因为：

- 它的应用输出很轻
- 过程数据规模小
- 示例主站往往按最简单模式驱动

当前工程虽然 `OP_PD_REQUIRED` 仍然是开启的，但联调时发现：

- 仅仅请求 `SAFE-OP -> OP` 不够
- 主站必须在过渡期间持续发 PDO

所以当前工程在主站测试程序里专门新增了：

- `wait_for_op_with_pdo()`

这并不是 `SSC` 配置本身变了，而是“当前 EtherCAT 应用已经不再是 4 字节灯控例程，它更依赖真实 PDO 流量来完成 OP 过渡”。

## 8. 主站测试程序也完全不是一个东西

## 8.1 `ecat_io` 自带 Linux_Test 的目标

`ecat_io/Linux_Test/ecat_master_test.c` 的目标是：

- 扫描从站
- 切到 OP
- 周期性读取 32-bit 输入
- 周期性写入 32-bit 输出
- 做 LED 跑马灯

它测试的是：

- 从站 PDO 4 字节输入输出是否正常

## 8.2 当前工程主站测试程序的目标

`host/rdk_ecat_master/ecat_gateway_test.c` 的目标是：

- 扫描网关从站
- 校验 `480/480 bits` PDO
- 在 OP 过渡阶段持续发 PDO
- 每周期构造 60 字节网关帧
- 解析来自 Jetson 方向的 60 字节网关帧

也就是说它测试的是：

- EtherCAT 作为“消息承载通道”是否正常

而不再是：

- EtherCAT 作为“32-bit IO 变量映射”是否正常

## 9. 相对 `ecat_io`，没变的部分

为了避免误解，也要说明哪些 EtherCAT 栈骨架基本没动。

### 9.1 Beckhoff SSC 主框架没换

仍然是：

- SSC v5.13
- `MainInit()`
- `MainLoop()`
- `APPL_*` 回调模型

### 9.2 基础从站身份没完全重做

以下内容仍然延续了原始工程的基础设定：

- `VENDOR_ID = 0x0048504D`
- `PRODUCT_CODE = 0x00000001`
- mailbox/DC/CoE 的大部分配置框架
- ESC 硬件访问模式

所以这不是“重写了一套 EtherCAT 从站”，而是“在 `ecat_io` 的 SSC 基线之上，重构了应用层和 PDO 结构”。

## 10. 最本质的差异

如果只用一句话概括，相对 `ecat_io` 的 EtherCAT 改动就是：

- `ecat_io` 把 EtherCAT 当成“控制板上 IO 的接口”
- 当前工程把 EtherCAT 当成“60 字节应用报文通道”

因此所有关键改动都会围绕三件事展开：

1. PDO 从 `4B/4B` 扩成 `60B/60B`
2. 应用回调从 `GPIO` 改成 `bridge buffer`
3. 主站联调从“变量读写”改成“持续报文收发”

## 11. 如果以后还要继续改 EtherCAT 层，优先看哪些文件

如果你以后继续基于当前工程改 EtherCAT，而不是改 TSN，优先看下面这些文件：

- `firmware/ecat_tsn_gateway/application/gateway_app.c`
- `firmware/ecat_tsn_gateway/SSC/Src/digital_ioObjects.h`
- `firmware/ecat_tsn_gateway/SSC/Src/ecat_def.h`
- `firmware/ecat_tsn_gateway/SSC/Config/HPM_ECAT_IO_Config.xml`
- `firmware/ecat_tsn_gateway/SSC/ESI/ECAT_IO.xml`
- `firmware/ecat_tsn_gateway/SSC/Src/eeprom.h`
- `host/rdk_ecat_master/ecat_gateway_test.c`

如果你要改的是 PDO 长度、对象语义、设备名、版本号、OP 行为验证方式，这几个文件一定会联动。
