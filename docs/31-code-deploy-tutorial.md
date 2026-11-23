<!-- Author: BearKing -->
# 代码推送到不同机器（RDK）教程

本文说明如何把本仓库的代码从开发机推送到地瓜派 RDK（目标机），覆盖
**host 代码** 和 **ROS2 工作区源码** 两类推送，Windows 与 Linux 开发机两种场景，
以及换 IP / 换机器、部署后重建、回滚与排障。

> 前置网络联调见 [26-rdk-usb-ethernet-link.md](26-rdk-usb-ethernet-link.md)、
> [27-rdk-usb-eth-to-pc.md](27-rdk-usb-eth-to-pc.md)、
> [30-wlan-nmcli-guide.md](30-wlan-nmcli-guide.md)。
> ROS2 部署整体流程见 [29-rdkx5-ros2-deployment.md](29-rdkx5-ros2-deployment.md)。

## 1. 部署模型

```text
开发机（源）                          RDK（目标机）
┌──────────────────────┐               ┌──────────────────────────┐
│ host/                │ ──tar+scp──▶  │ /root/hpm_gateway/host   │
│ host/hpm_gateway_    │ ──tar+scp──▶  │ /root/hpm_gateway_       │
│   ros2_ws/src        │               │   ros2_ws/src            │
└──────────────────────┘               └──────────────────────────┘
```

- **host 代码**：整个 `host/` 目录（`rdk_ecat_motor_rt`、SOEM 主站等）。
- **ROS2 工作区源码**：只同步 `host/hpm_gateway_ros2_ws/src`，
  **不传** 可再生成的 `build/install/log`。

所有脚本在远端采用 **“临时目录解压 → 旧目录改名为时间戳备份 → 原子切换”**
的流程，旧目录**不会被删除**；切换失败会自动恢复旧目录。

### 脚本清单

| 脚本 | 平台 | 方向 | 内容 |
| --- | --- | --- | --- |
| `deploy_host_to_rdk.ps1` | Windows | 推送 | `host/` → RDK `/root/hpm_gateway/host` |
| `deploy_host_to_rdk.sh` | Linux | 推送 | `host/` → RDK `/root/hpm_gateway/host` |
| `deploy_rdk_ros2_workspace.ps1` | Windows | 推送 | `host/hpm_gateway_ros2_ws/src` → RDK |
| `deploy_rdk_ros2_workspace.sh` | Linux | 推送 | 同上（Linux 版） |
| `pull_rdk_ros2_workspace.ps1` | Windows | 拉取 | RDK `src` ← 本地（首次建库/同步回来） |

## 2. 前置条件

### 2.1 网络可达

- PC 与 RDK 必须在同一网段。仓库默认 RDK 管理地址是 `192.168.108.178`；
  开发机 `eno1` 通常是 `192.168.108.156/22`，同网段直达。
- 确认方法：

```bash
ip route get 192.168.108.178     # 应显示 dev eno1（或对应网卡）
ping -c 4 192.168.108.178       # 通
ssh root@192.168.108.178 'echo ok'   # 能登录
```

### 2.2 工具

| 平台 | 需要的命令 |
| --- | --- |
| Linux | `tar`、`scp`、`ssh` |
| Windows PowerShell | `tar.exe`、`scp.exe`、`ssh.exe`（Windows 10+ 自带 OpenSSH） |

脚本启动时会检查工具是否存在，缺一个就报错退出。

### 2.3 SSH 认证

- 默认用户 `root`，端口 22，**密码交互认证**（脚本用 `BatchMode=no`，会提示输入密码）。
- 密码只在你和 ssh 之间交互，**不会**出现在脚本或仓库里。
- 可选：配置 SSH 密钥免密（`ssh-copy-id root@<ip>`）后每次部署不用输密码。

> **在 AI 工具沙箱里跑这些脚本时注意**：沙箱没有 TTY，`ssh` 无法弹出密码输入
> （会回退到 `ssh-askpass` 并失败）。此时请在**你自己的终端**运行脚本，
> 或授权工具在真实终端（非沙箱）运行，由你手动输入密码。

## 3. 指定目标机器（换 IP）

所有脚本默认目标是 `192.168.108.178`。换机器/换 IP 用参数覆盖：

| 平台 | host 代码 | ROS2 工作区 |
| --- | --- | --- |
| Windows | `deploy_host_to_rdk.ps1 -RdkIp <ip>` | `deploy_rdk_ros2_workspace.ps1 -RdkIp <ip>` |
| Linux | `deploy_host_to_rdk.sh <ip>` | `deploy_rdk_ros2_workspace.sh <ip>` |

PowerShell 完整参数（都带默认值）：

```text
-RdkIp <ip>           目标 IP，默认 192.168.108.178
-UserName <user>      SSH 用户，默认 root
-RemoteRoot <path>    host 远端父目录，默认 /root/hpm_gateway
-RemoteWorkspace <p>  ROS2 远端工作区，默认 /root/hpm_gateway_ros2_ws
-DryRun               只检查，不打包、不传输、不连接
```

Linux `.sh` 还支持环境变量覆盖：`RDK_USER`、`REMOTE_ROOT`、`REMOTE_WORKSPACE`。

### 3.1 第二台目标机：192.168.111.70（用户 wz）

仓库还维护一台目标机（用户 `wz`，**非 root**），ROS2 工作区与默认 RDK 不同：

| 项 | 默认 RDK | 第二台目标机 |
| --- | --- | --- |
| IP | 192.168.108.178 | 192.168.111.70 |
| SSH 用户 | root | wz |
| ROS2 工作区 | /root/hpm_gateway_ros2_ws | /home/wz/hpm_gateway_ros2_ws |

Linux 推送 ROS2 源码（密码交互认证，需在自己终端运行）：

```bash
RDK_USER=wz REMOTE_WORKSPACE=/home/wz/hpm_gateway_ros2_ws \
  ./scripts/deploy_rdk_ros2_workspace.sh 192.168.111.70
```

Windows 对应：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 `
  -RdkIp 192.168.111.70 -UserName wz -RemoteWorkspace /home/wz/hpm_gateway_ros2_ws
```

> 2026-08-24 实测：推送成功，远端旧 `src` 备份在
> `/home/wz/hpm_gateway_ros2_ws.backups/`。

## 4. 推送 host 代码

从仓库根目录运行。

**Windows PowerShell：**

```powershell
# 只检查本地路径和依赖，不连 RDK
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_host_to_rdk.ps1 -DryRun

# 推送到默认目标 192.168.108.178（会提示输密码）
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_host_to_rdk.ps1

# 推到其他机器
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_host_to_rdk.ps1 -RdkIp 192.168.108.118
```

**Linux：**

```bash
./scripts/deploy_host_to_rdk.sh                 # 默认 192.168.108.178
./scripts/deploy_host_to_rdk.sh --dry-run
./scripts/deploy_host_to_rdk.sh 192.168.108.118 # 换目标
```

脚本输出示例（成功）：

```text
Source: .../host
Target: root@192.168.108.178:/root/hpm_gateway/host
Creating host source archive...
Uploading archive to RDK...
Switching remote host directory...
backup=/root/hpm_gateway/host.backup-20260818-101500
directories=rdk_ecat_motor_rt tools ...
files=1234
[OK] Host code deployed.
```

## 5. 推送 / 拉取 ROS2 工作区源码

只同步 `src`，不传 `build/install/log`。**推送要求本地 `src` 至少含一个
`package.xml`，且必须有 `hpm_gateway_msgs/package.xml`**，防止用空目录覆盖。

### 5.1 首次：从 RDK 拉取当前源码

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\pull_rdk_ros2_workspace.ps1
```

拉取时，已有本地 `src` 会重命名为 `src.backup-时间戳-ID`。

### 5.2 本地改完：推送到 RDK

**Windows PowerShell：**

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 -DryRun
powershell -ExecutionPolicy Bypass -File .\scripts\deploy_rdk_ros2_workspace.ps1 -RdkIp 192.168.108.118
```

**Linux：**

```bash
./scripts/deploy_rdk_ros2_workspace.sh                # 默认 192.168.108.178
./scripts/deploy_rdk_ros2_workspace.sh --dry-run
./scripts/deploy_rdk_ros2_workspace.sh 192.168.108.118
```

脚本输出示例（成功）：

```text
Source: .../host/hpm_gateway_ros2_ws/src
Target: root@192.168.108.178:/root/hpm_gateway_ros2_ws/src
Scope: src only (build, install, and log are excluded)
Creating ROS 2 source archive...
Uploading archive to RDK...
Switching remote ROS 2 source directory...
backup=/root/hpm_gateway_ros2_ws.backups/src.backup-20260818-114152-3775
packages=2
[OK] ROS 2 workspace source deployed.
```

> Windows `tar` 不保留 Unix 可执行位，脚本会自动在远端
> `chmod +x` 脚本 `hpm_gateway_control/scripts/send_reference_motor_commands.py`。

## 6. 部署后：RDK 上重建（让新代码生效）

推送只更新源码，**不会自动编译**。在 RDK 上（本机或另开 SSH）执行：

### 6.1 host 代码

```bash
cd ~/hpm_gateway/host/rdk_ecat_motor_rt
make            # 依赖 SOEM v2；默认 SOEM_DIR=~/Tools/EtherCat/SOEM，链接 -lsoem(/usr/local)
sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --print-feedback
```

接口名按实际 EtherCAT 网口调整（示例 `enp4s0`）。

### 6.2 ROS2 工作区

```bash
cd ~/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
export SOEM_DIR=/usr/local          # 用安装后的头文件/库，避免缺生成的 soem/ec_options.h
colcon build --symlink-install
source install/setup.bash
```

改过 `SOEM_DIR` 或换过 SOEM 后，先清缓存再编：

```bash
rm -rf build/hpm_gateway_control install/hpm_gateway_control
```

## 7. 回滚（远端备份位置）

推送成功后旧目录保留在以下位置，确认新代码有问题时可直接换回：

| 代码 | 备份位置（RDK 上） |
| --- | --- |
| host | `/root/hpm_gateway/host.backup-<时间戳>` |
| ROS2 src | `/root/hpm_gateway_ros2_ws.backups/src.backup-<时间戳>` |
| ROS2 src（第二台目标机 192.168.111.70） | `/home/wz/hpm_gateway_ros2_ws.backups/src.backup-<时间戳>` |

回滚示例：

```bash
# 在 RDK 上
mv /root/hpm_gateway/host /root/hpm_gateway/host.broken
mv /root/hpm_gateway/host.backup-20260818-101500 /root/hpm_gateway/host
# 再重新 make / colcon build 即可
```

## 8. 常见问题排查

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `Network is unreachable` | 沙箱/环境没网络 | 放开网络访问，或直接在用户终端跑 |
| `Permission denied (publickey,password)` 或 `ssh_askpass` 失败 | 沙箱无 TTY，密码无法交互 | 用真实终端运行，手动输密码 |
| 输错 3 次密码 | 密码错误 | 重跑脚本，确认 RDK root 密码 |
| `ssh: connect to host ... port 22: Connection refused` | RDK 未开机/SSH 未开 | 检查 RDK 电源与网络（见 2.1） |
| ping 超时（非 unreachable） | 网段不对或 RDK 网口没配 IP | 见 [26](26-rdk-usb-ethernet-link.md)/[27](27-rdk-usb-eth-to-pc.md) |
| 推送 ROS2 报缺 `package.xml` | 本地 `src` 不完整 | 先 `pull_rdk_ros2_workspace.ps1` 拉回来 |
| RDK 上 `make` 报 SOEM 相关错误 | SOEM 没装或路径不对 | 见 [29](29-rdkx5-ros2-deployment.md) 第 5 节 |
| `colcon build` 报 `soem/ec_options.h` | SOEM_DIR 未指向 /usr/local | `export SOEM_DIR=/usr/local` 后重编 |

## 9. 参考

- 脚本本体：`scripts/deploy_*.ps1`、`scripts/deploy_*.sh`、`scripts/pull_*.ps1`
- 脚本使用简表：`scripts/README.md`
- 网络联调：`docs/26-rdk-usb-ethernet-link.md`、`docs/27-rdk-usb-eth-to-pc.md`、`docs/30-wlan-nmcli-guide.md`
- ROS2 部署全流程：`docs/29-rdkx5-ros2-deployment.md`
