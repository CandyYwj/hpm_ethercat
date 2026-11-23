# Ubuntu 主机程序构建与 VS Code 索引

本机是 Ubuntu 24.04，已有 ROS 2 Jazzy。主机包已用原生 GCC/C++17 编译通过；
HPM 固件继续使用独立的 RISC-V 工具链。现有 RDK 部署的 Humble/TROS 配置按原部署文档使用。

## 红线的原因与修复

之前 VS Code 只读取 HPM 固件的 `compile_commands.json`，ROS 2 翻译单元没有
对应的原生编译命令；工作区也没有构建过，自定义消息头文件尚未生成，SOEM 未安装。

构建脚本现在通过 `scripts/update_compile_commands.py` 合并三个数据库：

```text
firmware/ecat_tsn_gateway/build/compile_commands.json
host/hpm_gateway_ros2_ws/build/hpm_gateway_control/compile_commands.json
host/hpm_gateway_ros2_ws/build/hpm_gateway_gamepad/compile_commands.json
```

编辑器只读取合并结果 `.vscode/compile_commands.json`，每条记录保留自己的
编译器和参数。该文件由构建自动更新，并已被 Git 忽略。

控制节点的两个 `.cpp` 使用 `/usr/bin/c++`、C++17、ROS 和生成消息的实际头文件路径。
固件的 `.c` 使用 HPM RISC-V 编译器及 SDK 参数。根配置不再固定 `compilerPath`
或 RISC-V 参数，避免覆盖其他翻译单元的编译器。未匹配文件使用插件自动发现的编译器。
无需关闭诊断或在业务源码中补伪声明。

2026-10-15 的编辑器日志曾明确报告：`gateway_ecat_api.cpp` 在固件数据库中
未找到，转而使用根目录的 HPM 配置。随后调试日志还捕获到把 `-mabi=ilp32`
传给 `/usr/bin/c++` 的失败查询。改为单一数据库并去掉显式编译器覆盖后，
实时日志确认当前 ROS 文件使用 `linux-gcc-x64`、C++17 和 `_GNU_SOURCE=1`。
后者关系到 `cpu_set_t`、`CPU_ZERO`、`CPU_SET` 等 GNU/Linux 声明的可见性。

## Ctrl 跳转

跳转除了需要当前文件解析正确，还需要目标源码进入索引。`browse.path` 已加入
ROS 工作区源码、生成消息头、SOEM 源码与安装头文件、ROS 安装头文件。

- 项目函数和已安装的模板定义可跳到源文件或头文件。
- SOEM 的源码位于 `~/tools/ethercat/SOEM/src`，可用于寻找其函数实现。
- `sched_setaffinity` 等系统库接口在系统头文件中有声明；进入其实现需要额外的
  libc 源码。仅安装二进制库时，通常只能跳转到声明。

## 已配置的依赖

```text
/opt/ros/jazzy/                         本机 ROS 安装
~/tools/ethercat/SOEM/                  官方 SOEM v2.0.0 源码与 build/
~/tools/ethercat/soem/                  用户目录 SOEM 安装前缀
~/tools/ethercat/environment-manifest.json
```

SOEM 来源：[官方 v2.0.0](https://github.com/OpenEtherCATsociety/SOEM/tree/v2.0.0)。
源码归档 SHA-256：`1f1f38464c5542d8cd1b1d8f27d9af6422d03b9364a9f31ad66f1919f2898f16`。

SOEM v2 的 `ec_options.h` 在构建时生成。安装前缀包含它和完整公开头文件，
因此本机的 `SOEM_DIR` 使用 `~/tools/ethercat/soem`。

## 日常使用

在仓库根目录：

```bash
./scripts/build_host.sh
```

该命令自动加载 ROS，设置 SOEM 路径，构建 `hpm_gateway_msgs`、
`hpm_gateway_control`、`hpm_gateway_gamepad`，并生成编译数据库。
脚本只编译，不启动 EtherCAT、手柄节点或发送电机命令。

VS Code 中也可以选择 `Terminal → Run Task → Build ROS 2 host (Ubuntu)`。
默认 `Ctrl+Shift+B` 仍为 HPM 固件构建。

切换 ROS 发行版、SOEM 安装位置或遇到旧 CMake 缓存时：

```bash
HPM_ROS_SETUP=/opt/ros/jazzy/setup.bash \
SOEM_DIR="$HOME/tools/ethercat/soem" \
./scripts/build_host.sh --clean-cache
```

支持 `HPM_HOST_PYTHON` 指定 ROS 使用的 Python（默认 `/usr/bin/python3`），
`HPM_HOST_BUILD_JOBS` 指定每个 CMake 构建的并行数（默认 4）。

如删除了 ROS 工作区的 `build/` 或 `install/`，重新执行构建以恢复数据库和消息头。
如 VS Code 仍显示旧诊断，执行 `Developer: Reload Window`，必要时执行
`C/C++: Reset IntelliSense Database`。

只更新合并索引、无需重新编译时，可以执行：

```bash
python3 scripts/update_compile_commands.py
```

## 验证记录

- 三个 ROS 包构建成功；控制节点两个源文件和手柄四个源文件均完成原生编译。
- 自定义 `hpm_gateway_msgs/msg/*.hpp` 已生成，所有目标数据库的编译器与头文件目录存在。
- SOEM 头文件有匿名结构的 pedantic 警告，手柄源码有两处忽略 `read()` 返回值的警告；本次未修改其运行逻辑。
- 已验证编译和代码索引配置；未连接 EtherCAT 硬件，也未验证 RDK/Humble 的运行行为。
