<!-- Author: BearKing -->
# HPM Gateway 文档索引

Ubuntu 固件构建环境与使用方法：
[42-ubuntu-firmware-build.md](42-ubuntu-firmware-build.md)。

Ubuntu ROS 2 主机包构建与 VS Code 双工具链索引：
[43-ubuntu-host-build.md](43-ubuntu-host-build.md)。

## 当前状态 (2026-07-29)

工程是“EtherCAT 电机从站 + YESENSE IMU + 独立三端口 TSN 硬件交换机”。

| 模块 | 状态 |
|------|------|
| 16轴 MIT PDO (`gateway_app.c`) | ✅ 已实现 |
| SSC 对象字典 (`digital_ioObjects.h`) | ✅ 已手改为 16 轴布局 |
| EtherCAT → OP 状态机 | 已实现；本轮重构待上板回归 |
| TSN 三端口交换 (`drivers/tsn/tsn_switch.c`) | 已实现；待实机转发验证 |
| EtherCAT/TSN raw-frame 桥 | ✅ 已删除 |
| CAN0 四电机通信 | 已实现，1 kHz 控制；本轮重构待上板回归 |
| SSC Tool 重新生成 | ❌ 待做 (当前手改) |
| ESI/EEPROM 16轴同步 | ❌ 待做 |

## 文档地图

### 新架构（当前）

| 文档 | 内容 |
|------|------|
| [12-mit-motor-16axis-pdo-design.md](12-mit-motor-16axis-pdo-design.md) | **MIT PDO 设计** — 16轴对象字典、尺寸、Slot 方案 |
| [13-mit-motor-16axis-implementation-guide.md](13-mit-motor-16axis-implementation-guide.md) | **实施指南** — 方案A/B 选择、代码改动清单、避坑 |
| [10-ssc-protocol-detail.md](10-ssc-protocol-detail.md) | **SSC 源码逐行分析** — 13章覆盖全部 SSC 文件 |
| [08-motor-can-integration-plan.md](08-motor-can-integration-plan.md) | **CAN 接入计划** — RS06/ENCOS 双协议适配 |
| [15-rs06-motor-protocol-reference.md](15-rs06-motor-protocol-reference.md) | **RS06 协议参考** — 私有+CANopen+MIT 三协议完整帧定义 |
| [16-encos-motor-protocol-reference.md](16-encos-motor-protocol-reference.md) | **ENCOS 协议参考** — 运控/位置/速度/力矩+配置命令 |
| [26-rdk-usb-ethernet-link.md](26-rdk-usb-ethernet-link.md) | **RDK 网线联调** — USB 转网口在 PC 侧，网线直连 RDK |
| [27-rdk-usb-eth-to-pc.md](27-rdk-usb-eth-to-pc.md) | **RDK 网线联调** — USB 转网口在 RDK 侧，网线直连个人 PC |
| [28-ros2-container-motor-control.md](28-ros2-container-motor-control.md) | **ROS2 容器控制电机** — `ros2_humble.sh` 全部命令与 send 参数 |
| [29-rdkx5-ros2-deployment.md](29-rdkx5-ros2-deployment.md) | **RDK X5 部署** — 在已有 TROS/ROS2 的 RDK 上部署电机控制环境（网络/部署/SOEM/编译/实时/启动/验证） |
| [30-wlan-nmcli-guide.md](30-wlan-nmcli-guide.md) | **WLAN 管理** — 用 `nmcli` 扫描/连接/断开/开关 Wi-Fi、优先级与静态 IP（RDK NetworkManager） |
| [31-code-deploy-tutorial.md](31-code-deploy-tutorial.md) | **代码部署教程** — host 代码与 ROS2 工作区源码推送到 RDK 的完整流程（Windows/Linux、换 IP、重建、回滚、排障） |
| [32-humble-container-deploy.md](32-humble-container-deploy.md) | **Humble 容器跨机部署** — 无状态容器原理、Dockerfile 双目标构建（dev/standalone，方案C）、拷贝工作区+拉镜像（方案A）、容器内改动时 commit+save/load（方案B）、踩坑与验证清单 |
| [33-systemd-guide.md](33-systemd-guide.md) | **systemd 详解** — unit 三段结构、Type/After/Wants、重启与停止策略、journald 日志、排障，以及 `rdk_rt_setup.service` / `hpm_gateway_node.service` 逐行讲解 |
| [34-rdk-gamepad-node.md](34-rdk-gamepad-node.md) | **RDK 手柄节点** — 热插拔感知 + 发布 `sensor_msgs/Joy` 的三种实现（joy/joy_linux、SDL2、evdev 自研），systemd 自启与验证清单 |

### 历史参考（原始网关）

以下文档描述旧版“60 字节 EtherCAT↔TSN 透传网关”。对应固件代码已经删除，
这些文档只用于追溯历史，不代表当前实现：

| 文档 | 内容 | 状态 |
|------|------|------|
| [01-项目综述.md](01-项目综述.md) | 旧架构综述 | ⚠️ 过时 |
| [02-相对例程改动说明.md](02-相对例程改动说明.md) | vs 原始例程 | ⚠️ 过时 |
| [03-自定义协议修改指南.md](03-自定义协议修改指南.md) | 协议修改方法 | ⚠️ 过时 |
| [04-控制流详解.md](04-控制流详解.md) | 旧控制流 | ⚠️ 过时 |
| [05-数据流详解.md](05-数据流详解.md) | 旧数据流 | ⚠️ 过时 |
| [06-调试与验证指南.md](06-调试与验证指南.md) | 调试方法 | ⚠️ 过时 |
| [07-相对ecat_io的EtherCAT改动.md](07-相对ecat_io的EtherCAT改动.md) | vs ecat_io | ⚠️ 过时 |
| [09-ethercat-pdo-sdo-modification-guide.md](09-ethercat-pdo-sdo-modification-guide.md) | PDO/SDO 通用修改 | 部分有效 |
| [11-frame-flow-hardware-to-app.md](11-frame-flow-hardware-to-app.md) | 帧流转详图 | 部分有效 |
| [hpm_ethercat_tutorial.md](hpm_ethercat_tutorial.md) | HPM EtherCAT 教程 | 参考 |

## 核心代码入口

```text
firmware/ecat_tsn_gateway/
  main.c                        ← 板级入口和 scheduler 启动
  CMakeLists.txt                ← 固件构建配置
  app/
    gateway_app.c               ← SSC对象定义和16轴MIT PDO回调 ★ 核心
    gateway_system.c            ← ESC/CAN/IMU/TSN初始化编排
    gateway_app.h               ← MIT结构体 + 公开API
  tasks/                        ← EtherCAT/Motor/IMU/Status任务
  drivers/                      ← CAN/IMU/TSN硬件驱动
  motor/                        ← 轴配置和控制调度
  protocols/                    ← ENCOS/RS06/YESENSE编解码
  SSC/Src/
    digital_ioObjects.h         ← 16轴 PDO 对象字典 (手改)
    ecatslv.c/h                 ← ESM 状态机
    ecatappl.c/h                ← PDO映射 + MainLoop
    ecat_def.h                  ← PDO 尺寸配置
    objdef.c/h                  ← 对象字典读写
    sdoserv.c/h                 ← SDO 服务
    mailbox.c/h                 ← 邮箱协议
    ecatcoe.c/h                 ← CoE 协议
    coeappl.c/h                 ← CoE 应用层
  drivers/tsn/
    tsn_switch.c                ← 三端口硬件交换初始化
host/rdk_ecat_motor_rt/         ← 当前SOEM周期主站
```

## 下一步工作

1. SSC Tool 重新生成并同步 ESI/EEPROM
2. 电机安全保护层
3. 实时性能和任务栈监测
4. TSN 三端口转发实机验证
