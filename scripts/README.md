<!-- Author: BearKing -->
# RDK 主机代码部署

## Ubuntu 固件编译

本机 Linux SDK 与工具链位于 `~/tools/hpm`，日常构建直接执行：

```bash
./scripts/build_firmware.sh
```

脚本自动加载 SDK、板级驱动和独立 Python 环境。`--configure-only` 仅配置，
`--clean` 刷新缓存并完整重编，`--dry-run` 检查依赖并打印命令。
需要当前终端使用交叉编译器时执行 `source scripts/enter_hpm_env.sh`。

固件名称只在工程 `CMakeLists.txt` 中设置 `APP_NAME`；构建和下载脚本
读取 CMake 生成的 `build/firmware-elf-path.txt`，无须同步修改文件名。
`./scripts/download_firmware.sh --dry-run` 可检查将要烧录的 ELF，
`./scripts/download_firmware.sh --check` 可在不接板、未编译固件时检查工具环境。
本机 HPM OpenOCD 已安装在 `~/tools/hpm/openocd`。首次普通用户烧录前执行
`sudo ./scripts/setup_probe_permissions.sh` 安装 USB 权限规则，再连接探针，
去掉 `--dry-run` 执行烧录。

版本、依赖位置、VS Code 配置和验证记录见
[`docs/42-ubuntu-firmware-build.md`](../docs/42-ubuntu-firmware-build.md)。

## RDK 部署

本机 Ubuntu 主机端构建（ROS 消息、控制节点、手柄节点）：

```bash
./scripts/build_host.sh
```

脚本自动加载 ROS 和本机 SOEM 安装前缀，并生成 VS Code 所需的主机编译数据库。
切换依赖后可使用 `--clean-cache`。详情见
[`docs/43-ubuntu-host-build.md`](../docs/43-ubuntu-host-build.md)。

`deploy_host_to_rdk.ps1` 将本仓库的 `host` 目录同步到 RDK。默认目标为：

```text
root@192.168.108.178:/root/hpm_gateway/host
```

脚本调用系统自带的 OpenSSH `scp` 和 `ssh`，运行时由终端交互输入密码；密码不会保存在脚本或仓库中。

远端更新过程是可回退的：先把新代码解压到临时目录，成功后将当前 `host` 重命名为带时间戳的备份，最后切换新目录。旧目录不会被删除。

## 使用

从仓库根目录运行：

```powershell
.\scripts\deploy_host_to_rdk.ps1
```

只检查本地路径和依赖、不创建归档或连接 RDK：

```powershell
.\scripts\deploy_host_to_rdk.ps1 -DryRun
```

指定其他目标：

```powershell
.\scripts\deploy_host_to_rdk.ps1 -RdkIp 192.168.108.178 -UserName root -RemoteRoot /root/hpm_gateway
```

如果本机 PowerShell 的执行策略阻止脚本，可在本次调用中临时绕过：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_host_to_rdk.ps1
```

## ROS 2 工作空间源码同步

这两个脚本只同步 ROS 2 工作空间的 `src`，不会传输可重新生成的
`build`、`install` 和 `log`：

```text
RDK:   /root/hpm_gateway_ros2_ws/src
本地:  host\hpm_gateway_ros2_ws\src
```

首次使用时先从 RDK 拉取当前源码：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1
```

本地修改并确认后推送回 RDK：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1
```

只检查路径和依赖、不连接 RDK：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1 -DryRun
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 -DryRun
```

指定其他目标：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1 -RdkIp 192.168.108.178 -UserName root -RemoteWorkspace /root/hpm_gateway_ros2_ws
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 -RdkIp 192.168.108.178 -UserName root -RemoteWorkspace /root/hpm_gateway_ros2_ws
```

拉取时，已有本地 `src` 会重命名为 `src.backup-时间戳-ID`。推送时，已有
远端 `src` 会移动到工作空间外的 `/root/hpm_gateway_ros2_ws.backups/`；
新目录切换失败时脚本会把旧 `src` 恢复。推送和拉取都要求 `src` 中至少存在一个 ROS 2
`package.xml`，并且推送要求存在 `hpm_gateway_msgs/package.xml`，避免用空目录
或不完整源码覆盖工作空间。

Linux 下用等价的 bash 版推送（行为与 `.ps1` 一致）：

```bash
./scripts/deploy_rdk_ros2_workspace.sh          # 默认 192.168.108.178
./scripts/deploy_rdk_ros2_workspace.sh 192.168.108.118
./scripts/deploy_rdk_ros2_workspace.sh --dry-run

# 第二台目标机（用户 wz，工作区在 /home/wz/...）：
RDK_USER=wz REMOTE_WORKSPACE=/home/wz/hpm_gateway_ros2_ws \
  ./scripts/deploy_rdk_ros2_workspace.sh 192.168.111.70
```

完整流程、换目标、部署后重建与回滚见
[`docs/31-code-deploy-tutorial.md`](../docs/31-code-deploy-tutorial.md)。

## 固件编译与下载

固件只维护 `firmware/ecat_tsn_gateway/build` 一套构建目录（O2 优化，
`GATEWAY_PERFORMANCE_BUILD=ON`，保留调试符号）。

编译（配置 + 构建）：

```powershell
.\scripts\build_firmware.ps1
# 或
.\scripts\build_firmware.cmd
```

清空后重新编译：

```powershell
.\scripts\build_firmware.ps1 -Clean
```

只打印命令不执行：

```powershell
.\scripts\build_firmware.ps1 -DryRun
```

下载到开发板（默认 CMSIS-DAP 探针，ELF 为
由 `firmware\ecat_tsn_gateway\build\firmware-elf-path.txt` 自动确定）：

```powershell
.\scripts\download_firmware.ps1
```

指定其他探针 / 速度 / ELF：

```powershell
.\scripts\download_firmware.ps1 -Probe jlink -Speed 4000
.\scripts\download_firmware.ps1 -Elf path\to\other.elf -NoVerify -DryRun
```

常用组合：先编译再下载

```powershell
.\scripts\build_firmware.ps1; .\scripts\download_firmware.ps1
```

## 快速设置开发环境

仓库中提供了便捷的环境配置脚本，用于在当前终端会话中设置 `HPM_SDK_BASE`、
`GNURISCV_TOOLCHAIN_PATH` 及工具链路径，便于直接运行构建和烧写脚本：

- PowerShell（临时会话）：

```powershell
.\scripts\enter_hpm_env.ps1
```

要把变量持久化到当前用户环境（影响新打开的终端），使用 `-Persist`：

```powershell
.\scripts\enter_hpm_env.ps1 -Persist
```

- CMD（临时会话）：

```cmd
.\scripts\enter_hpm_env.cmd
```

持久化（影响新开启的命令行窗口）

```cmd
.\scripts\enter_hpm_env.cmd --persist
```

注意：持久化操作会调用 `setx` 修改用户环境变量，只影响新终端。脚本默认
从仓库上两级目录（例如仓库与 `hpm_sdk`、`toolchains` 在同一父目录）定位
SDK 和工具；如果你的目录布局不同，请在运行前手动设置 `HPM_SDK_BASE`。

## RDK 实时配置（开机自启，绑定 CPU 核）

EtherCAT 从站的实时性依赖 NIC 中断与 ECAT 工作线程落在同一个核上。节点侧
用 `hpm_gateway_control_eth0.yaml` 的 `rt_cpu` 固定线程；下面的脚本在开机时
把网卡中断也固定到同一核。

### 文件

- `rdk_rt_setup.sh`：探测 eth0 中断号并把 `smp_affinity` 设为指定核（默认核 3）。
- `rdk_rt_setup.service`：systemd 服务，开机自动执行上面的脚本。

### 安装（在 RDK 上执行，root）

```bash
# 1. 把脚本拷到 RDK（在本机 PowerShell 执行）
scp scripts/rdk_rt_setup.sh scripts/rdk_rt_setup.service root@<RDK_IP>:/root/

# 2. 在 RDK 上安装并启用服务
install -m 0755 /root/rdk_rt_setup.sh /usr/local/bin/rdk_rt_setup.sh
install -m 0644 /root/rdk_rt_setup.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now rdk_rt_setup.service

# 3. 验证
systemctl status rdk_rt_setup.service
cat /proc/interrupts | grep eth0        # 计数应累积到目标核
IRQ=$(grep -E " eth0" /proc/interrupts | awk -F: '{print $1}' | tr -d ' ')
cat /proc/irq/$IRQ/smp_affinity         # 应显示目标核的位掩码（如 8 = 核3）
```

### 手动执行

```bash
sudo /usr/local/bin/rdk_rt_setup.sh           # 默认核 3，接口 eth0
sudo /usr/local/bin/rdk_rt_setup.sh 3 eth0    # 显式指定
sudo /usr/local/bin/rdk_rt_setup.sh -1        # 恢复默认（全部核）
```

### 与节点配置对齐

`hpm_gateway_control_eth0.yaml` 的 `rt_cpu` 必须与脚本绑定的核一致（例如都是 3）：

```yaml
rt_priority: 80
rt_cpu: 3
```

## RDK 开机自启 ROS 节点（systemd 服务）

RDK 上电后自动启动 `hpm_gateway_control`（EtherCAT 主站节点）。服务以 root
运行（raw socket 需要），并**在 `rdk_rt_setup.service` 之后启动**，保证 NIC
中断已先绑核。

### 文件

- `hpm_gateway_node.sh`：source TROS + 工作区 `install/setup.bash`，设
  `ROS_DOMAIN_ID=42`，然后 `exec ros2 launch`。可用环境变量切换 launch 文件
  （默认 EtherCAT 模式，安全模式为 `HPM_GATEWAY_LAUNCH=hpm_gateway_control.launch.py`）。
- `hpm_gateway_node.service`：systemd 服务单元，失败自动重启、日志进 journal。

### 安装（在 RDK 上执行，root）

```bash
# 1. 拷到 RDK（本机 PowerShell）
scp scripts/hpm_gateway_node.sh scripts/hpm_gateway_node.service root@<RDK_IP>:/root/

# 2. 在 RDK 上安装并启用
install -m 0755 /root/hpm_gateway_node.sh /usr/local/bin/hpm_gateway_node.sh
install -m 0644 /root/hpm_gateway_node.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable --now hpm_gateway_node.service

# 3. 验证 / 看日志
systemctl status hpm_gateway_node.service
journalctl -u hpm_gateway_node.service -f
```

### 管理

```bash
systemctl restart hpm_gateway_node.service      # 重启节点
systemctl stop hpm_gateway_node.service         # 停止（断开 EtherCAT）
systemctl disable --now hpm_gateway_node.service  # 取消开机自启
```

### 注意

- 节点只在收到**第一条命令帧**后才使能全部已配置轴，开机后电机不会自转；
  但服务常驻意味着**任何时刻发布的命令都会被电机执行**，安全纪律不变。
- EtherCAT 网口名以 `ip -br link` 实测为准，改 `hpm_gateway_control_eth0.yaml`
  的 `interface_name`；绑错口会导致节点反复重启。
- 工作区必须先 `colcon build`（`install/setup.bash` 不存在时脚本直接退出，
  服务每 5 s 重试）
