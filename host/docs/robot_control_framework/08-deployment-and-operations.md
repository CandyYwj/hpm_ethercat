# Ubuntu 部署与运维

## 支持矩阵

| Profile | Ubuntu | ROS 2 | 用途 | 说明 |
| --- | --- | --- | --- | --- |
| `production-jazzy` | 24.04 LTS | Jazzy LTS | 新产品/长期维护 | PREEMPT_RT、固定 RMW、签名制品 |
| `rdk-humble-compat` | 22.04 | Humble/TROS | 当前 RDK 迁移 | 只运行兼容接口，保留旧 launch 回滚 |
| `dev-container` | 22.04/24.04 | 对应发行版 | CI/开发/仿真 | 不宣称满足硬实时；挂载 virtual transport |

截至 2026-08-31，官方 Jazzy deb 包支持 Ubuntu Noble 24.04，发行版支持页列出 Jazzy 至 May 2029；具体供应商 TROS 支持矩阵仍以其发行说明为准。每个制品记录 Ubuntu、内核、ROS、RMW、编译器和依赖 lock。

## 主机目录与权限

```text
/opt/robot/releases/<version>/        # 不可变程序和插件
/opt/robot/current -> releases/<v>    # 原子切换
/etc/robot/<robot_id>/                # 配置、签名、证书（权限 0640）
/var/lib/robot/<robot_id>/            # 校准、A/B 状态、回放索引
/var/log/journal/                     # 结构化系统日志
/var/lib/robot/captures/              # 有配额的 raw trace/MCAP
```

运行用户不拥有代码目录写权限。配置变更先写临时文件、校验 schema/hash/signature 和总线预算，再原子 rename；失败保留旧配置。密钥使用系统密钥环或受限文件，不写入日志和录包。

## systemd 运行单元

建议拆成：

1. `robot-rt@<id>.service`：`robot_rt`，`After=network-pre.target`，`Restart=on-failure`，配置 `CPUSchedulingPolicy=fifo`、优先级、`CPUAffinity`、`LimitMEMLOCK`、`WatchdogSec` 和最小 capabilities。
2. `robot-control@<id>.service`：ROS 2 bridge/controller manager，依赖 `robot-rt` ready，普通 executor；不能拥有 raw EtherCAT/CAN 设备。
3. `robot-ops@<id>.service`：诊断、指标、trace、录包，可独立重启；磁盘满只停录包。
4. `robot-app.target`：规划、感知、手柄、电源和 UI，按机器人配置启动。

单元启动顺序必须先 `SAFE -> PROBING -> READY`，而不是服务启动即 arm。停止顺序先撤销命令和进入 SAFE，再关闭 ROS/RT；关机超时由硬件 watchdog 接管。

## 容器策略

开发、仿真、编译和非实时 ROS 应用优先容器化。硬实时 `robot_rt` 默认使用宿主 systemd；若必须容器化，明确固定 image digest、`--network=host`（仅控制网需要时）、cpuset、实时 runtime、`CAP_NET_RAW/CAP_SYS_NICE/CAP_IPC_LOCK`、`memlock` 和设备映射，并在目标内核上重跑 jitter/HIL 门禁。容器能启动不代表满足实时预算。

## 网络与时钟

- EtherCAT/raw socket 网卡只做链路配置，不分配管理 IP；管理、ROS/DDS 和 EtherCAT 接口分开。
- PTP/chrony 同步主机；保留硬件时间、单调接收时间和 ROS 时间，不用 wall clock 参与 watchdog。
- DDS XML 固定网卡、发现策略、传输和安全；`ROS_DOMAIN_ID`、机器人 namespace 和控制网 ACL 由部署 manifest 注入。
- CAN 使用 SocketCAN/MCAN 对应的 bitrate、FD 数据 bitrate、restart-ms 和 error frame 策略；启动前验证物理终端和接口状态。

## 配置、发布与回滚

发布包是不可变 bundle：

```text
manifest + compiled_profile + protocol_lock + robot_interfaces
        + binaries/plugins + URDF + tests/evidence + signature
```

发布步骤：离线 lint/预算 -> 签名校验 -> 停机进入 SAFE -> 备份当前配置/设备参数 -> A/B 安装 -> 只读 probe -> 单设备 canary -> 健康门 -> 显式 arm。健康门至少检查连续 N 个周期的 deadline/WKC/CRC、反馈 freshness、温度和安全输入。任一阈值失败原子回到上一 slot，保留 trace 和原因。

注意主机回滚不能自动回滚已写入设备的 CAN ID、协议模式或持久参数；这些操作必须有读回备份、版本记录和人工确认。

## 运维命令与变更审计

`robotctl` 提供 doctor/status/config/trace/capture/device/recover/upgrade 子命令。默认只读；危险命令要求 operator 身份、理由、目标 `device_uid`、当前安全状态和 request ID。所有配置、模式、清错、标定和升级事件记录操作者、时间、旧/新 hash、结果和回滚信息。

## 资源与告警

systemd/cgroup 为 RT、ROS、录包和感知设定 CPU/内存/文件描述符上限。告警最小集合：RT deadline miss、命令 stale、反馈 stale、WKC/CAN bus-off、IPC CRC/drop、磁盘/内存/温度、配置 drift、证书即将过期。告警动作先撤销运动 lease，再通知运维；不让告警系统本身阻塞 RT。
