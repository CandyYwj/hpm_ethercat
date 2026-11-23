# HSLINK 烧录脚本

`flash_hslink.cmd` 用 HSLINK 的 CMSIS-DAP v2 接口烧录 HPM6E80 全端口板卡。脚本会执行 OpenOCD 的 `program`、`verify` 和 `reset`，只有验证通过才会报告成功。

## 前提条件

- 已在 HPM SDK 环境中运行，`HPM_SDK_BASE` 已设置。
- 已连接 HSLINK 到目标板的调试接口，并为目标板上电。
- 已构建固件；默认 ELF 路径读取 `../build/firmware-elf-path.txt`，由 CMake 自动生成。
- OpenOCD 的资源目录 `%APPDATA%\OpenOCD` 中包含 CMSIS-DAP、HPM6E80 和 HPM6E00EVK 配置。SDK 安装 HSLINK 支持后通常会提供这些文件。

## 用法

在 `firmware\ecat_tsn_gateway` 目录下执行：

```bat
scripts\flash_hslink.cmd
```

烧录其他 ELF：

```bat
scripts\flash_hslink.cmd D:\path\to\firmware.elf
```

不连接或写入设备，只检查路径并显示最终 OpenOCD 命令：

```bat
scripts\flash_hslink.cmd --dry-run
```

如需重新构建，请先在 `build` 目录运行 Ninja；本脚本有意只负责烧录，避免在连接硬件时隐式修改构建产物。
