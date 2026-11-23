# Ubuntu 固件编译环境

本文记录本仓库在 Ubuntu 24.04 x86_64 上实际完成的 HPM6E80 固件构建。

## 已安装的环境

依赖位于当前用户的 `~/tools/hpm/`，不需要修改系统 Python 或 `/usr/local`。

```text
~/tools/hpm/
├── hpm_sdk/                         官方 SDK v1.11.0 的构建依赖源码
├── hpm_apps/                        官方 hpm_apps v1.11.0 的 full-port 板级与驱动源码
│   ├── boards/hpm6e00_full_port/
│   └── apps/full_port/software/drivers/
├── toolchains/
│   └── rv32imac_zicsr_zifencei_multilib_b_ext-linux/
├── venv/                            SDK 专用 Python 环境
├── openocd/                         HPM OpenOCD、脚本和用户目录 HID 运行库
├── downloads/                       原始下载文件及 full-port 文件清单
└── environment-manifest.json        已安装版本、来源与下载文件 SHA-256
```

| 组件 | 本机验证的版本 |
| --- | --- |
| HPM SDK | 1.11.0 |
| hpm_apps full-port 板级和驱动 | v1.11.0 |
| HPM GNU 工具链 | 2023.10.18 Linux 包，GCC 13.2.0 |
| CMake / Ninja | 3.28.3 / 1.11.1 |
| Python | 3.12.3，独立 venv，安装 SDK `scripts/requirements.txt` |
| OpenOCD | HPM hpm_xpi_v0.4.0，0.12.0+dev-snapshot (2025-12-22-02:41) |

来源：[HPM SDK v1.11.0](https://github.com/hpmicro/hpm_sdk/tree/v1.11.0)、
[hpm_apps v1.11.0](https://github.com/hpmicro/hpm_apps/tree/v1.11.0)、
[HPM GNU 工具链](https://github.com/hpmicro/riscv-gnu-toolchain/releases/tag/2023.10.18)。

官方 hpm_apps 的驱动位于 `apps/full_port/software/drivers`，与旧 Windows
环境的 `hpm_app/software/drivers` 布局不同。构建脚本显式设置
`HPM_APP_DRIVERS`，不需要修改项目的固件源码或复制驱动到仓库内。

## 固件名称与自动选择

固件名称在 `firmware/ecat_tsn_gateway/CMakeLists.txt` 的 `set(APP_NAME ...)`
中设置。普通代码修改后直接重新编译，无须修改任何产物路径。

CMake 每次配置生成 `build/firmware-elf-path.txt`，内容为当前目标 ELF 的
绝对路径。Linux/Windows 构建、下载脚本读取此文件；VS Code 调试通过
`${command:cmake.launchTargetPath}` 获取 CMake 目标路径。以后改名只需要
修改工程中的 `APP_NAME` 并重新构建，旧产物即使存在也不会被脚本自动选中。

无需修改 SDK 的 `application.cmake`。本项目显式指定名称，独立于 SDK 默认值。

## 日常编译

在仓库根目录执行：

```bash
./scripts/build_firmware.sh
```

脚本自动设置 SDK、交叉编译器和 Python 路径，使用唯一的
`firmware/ecat_tsn_gateway/build` 构建目录，启用 O2、调试符号和
`compile_commands.json`。默认并行数为 8，可通过 `HPM_BUILD_JOBS` 调整。

```bash
# 仅生成构建文件和代码索引
./scripts/build_firmware.sh --configure-only

# 刷新 CMake 缓存，并清理、重编全部对象；切换 Windows/Linux 或 SDK 后使用
./scripts/build_firmware.sh --clean

# 检查依赖并显示命令
./scripts/build_firmware.sh --dry-run

# 指定编译并行数
HPM_BUILD_JOBS=4 ./scripts/build_firmware.sh
```

构建参数：

```text
BOARD=hpm6e00_full_port
HPM_BUILD_TYPE=flash_xip
CMAKE_BUILD_TYPE=debug
GATEWAY_PERFORMANCE_BUILD=ON
```

产物：

```text
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.elf
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.bin
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.map
firmware/ecat_tsn_gateway/build/output/<APP_NAME>.asm
firmware/ecat_tsn_gateway/build/firmware-elf-path.txt
firmware/ecat_tsn_gateway/build/compile_commands.json
```

名称改动后，产物文件名会按 `APP_NAME` 自动变化。

## 烧录入口

HPM OpenOCD 已安装到 `~/tools/hpm/openocd`，`~/.local/bin/openocd` 指向启动脚本。
官方 Linux 包 SHA-256 已校验为
`dab7629a4de59a9c05f8d6c0db0ad4e900006f761efc20377946a420aae11566`。
来源：[hpm_xpi_v0.4.0](https://github.com/hpmicro/riscv-openocd/releases/tag/hpm_xpi_v0.4.0)。

系统尚未安装 `libhidapi-hidraw0`，已从 Ubuntu noble 软件源下载该包并提取到
`openocd/runtime/`；启动脚本自动加载此私有运行库。运行库配置不需要 sudo，
无需修改系统 Python 或系统动态链接器配置。安装记录在
`~/tools/hpm/environment-manifest.json`。

```bash
# 不接开发板，检查 OpenOCD 能否启动及所需配置文件是否存在
./scripts/download_firmware.sh --check

# 显示当前 ELF 和烧录命令，不连接探针，不要求 OpenOCD 已安装
./scripts/download_firmware.sh --dry-run

# 配置 USB 权限并连接 HSLINK/CMSIS-DAP JTAG 后执行
./scripts/download_firmware.sh
```

普通用户访问 USB 探针的规则已准备并通过 `udevadm verify` 检查；安装到
`/etc/udev/rules.d` 需要管理员权限，本次会话未能免密 sudo，尚未执行：

```bash
sudo ./scripts/setup_probe_permissions.sh
```

该命令只安装针对 CMSIS-DAP/HSLINK 的 USB/HID 权限规则并重新加载规则，
不访问或烧录目标板。当前用户已属于 `plugdev` 组，安装后重新插入探针即可。

默认 OpenOCD 路径为 `~/tools/hpm/openocd/bin/openocd`，可通过
`HPM_OPENOCD` 指定其他可执行文件。支持 `--elf PATH` 显式指定镜像，
`--speed KHZ` 设置 JTAG 速率（默认 1000 kHz）。

## VS Code

- Ubuntu 下 `Ctrl+Shift+B` 执行默认 `Build ecat_tsn_gateway` 任务，自动配置并编译。
- `Terminal → Run Task → CMake Configure` 仅执行配置和索引生成。
- C/C++ 默认 `Ubuntu (HPM + ROS 2)` 配置读取 `.vscode/compile_commands.json`，
  每个固件源文件仍使用其实际 RISC-V 编译命令；构建脚本自动更新合并数据库。
  原 Windows 配置保留为 `HPM6E80 (Windows)`。
- CMake Tools 已配置本机 SDK、板级驱动、工具链和 Python 环境。
- 调试 ELF 路径自动读取 CMake 目标；`Flash (HS-Link OpenOCD)` 任务先编译，
  再调用平台对应的下载脚本。
- 若编辑器仍显示旧的头文件诊断，执行 `C/C++: Reset IntelliSense Database`，
  必要时执行 `Developer: Reload Window`。

## 自定义依赖位置

默认目录可以通过环境变量覆盖。例如使用原 Windows 环境配套的板级代码：

```bash
export HPM_SDK_BASE=/path/to/hpm_sdk
export GNURISCV_TOOLCHAIN_PATH=/path/to/linux-toolchain
export BOARD_SEARCH_PATH=/path/to/hpm_app/boards
export HPM_APP_DRIVERS=/path/to/hpm_app/software/drivers
export HPM_PYTHON=/path/to/venv/bin/python3
./scripts/build_firmware.sh --clean
```

需要在当前终端直接使用编译器时：

```bash
source scripts/enter_hpm_env.sh
riscv32-unknown-elf-gcc --version
```

编辑器的默认路径使用 `${userHome}/tools/hpm`。迁移依赖位置后，也需要同步
调整 `.vscode/settings.json` 和 `.vscode/c_cpp_properties.json` 中的路径。

## 本次验证结果与边界

完成 110 个编译/链接步骤，生成 RISC-V 固件，产物名称以当前 `APP_NAME` 为准。
首次完整构建报告 FLASH 使用 180208 B，AHB SRAM 使用 20 KiB。

O2 完整编译输出 14 条 SSC 警告，包括 `mailbox.c` 的 `-Warray-bounds`、
`sdoserv.c` 的整数转换和 packed 成员对齐警告，以及未使用变量和地址比较警告。
其中数组边界警告涉及 10 字节邮箱错误报文缓冲区按 `TMBX *` 访问的写法。
这些警告未阻止链接，本次未修改 SSC。
完整构建日志见 `firmware/ecat_tsn_gateway/build/ubuntu-build.log`。

本次使用的是官方 full-port 板级源码；原 Windows 环境若有额外板级改动，
部署前需要对照确认。OpenOCD 已安装并通过离线检查，尚未连接探针或烧录开发板；烧录和
EtherCAT OP、CAN、电机、IMU 的运行验证需要后续在目标板上完成。

产物名称自动选择已验证：只把 `APP_NAME` 改成临时名称，构建和烧录
`--dry-run` 均选择新 ELF，随后恢复为 `rg`。Windows 脚本和硬件调试尚未实测。
