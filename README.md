# HPM6E80 EtherCAT Motor Gateway

本项目把 HPM6E80 full-port 板作为 EtherCAT 电机从站，同时启用板载 TSW 的三端口
二层交换功能。

## 当前数据面

```text
EtherCAT master
  -> ESC / SSC
  -> 320-byte RxPDO
  -> 1 kHz motor control
  -> CAN0: 3 x ENCOS + 1 x RS06 MIT

CAN motor feedback + YESENSE IMU
  -> 252-byte TxPDO
  -> EtherCAT master

TSN Port1 <-> TSW hardware <-> TSN Port2/Port3
```

EtherCAT 与 TSN 相互独立。旧的 EtherCAT PDO 与 raw Ethernet 双向转换、私有
`0x88B5` 协议、CPU DMA 接收环和 `gateway_bridge_poll()` 已从固件删除。

## 运行任务

| 任务 | 周期 | 职责 |
| --- | ---: | --- |
| EtherCAT | 1 ms | SSC `MainLoop()` |
| Motor | 1 ms poll / 1 ms control | CAN 收包和 1 kHz 控制 |
| IMU | 1 ms | 有界消费 UART HDMA 数据 |
| Status | 1000 ms | LED 和低频状态输出 |

TSN 交换不需要任务。广播和未知单播由硬件泛洪到 Port1/2/3，CPU Port 不在转发
目的掩码中，也不启用 TSN RX/TX DMA 或 TSN 中断。

## 目录

```text
firmware/ecat_tsn_gateway/
  app/                 system initialization and EtherCAT PDO application
  tasks/               FreeRTOS task entry points
  drivers/can/         HPM MCAN backend
  drivers/imu/         YESENSE UART/HDMA driver
  drivers/tsn/         three-port hardware switch initialization
  motor/               axis configuration and motor scheduler
  protocols/           ENCOS, RS06 and YESENSE codecs
  SSC/                 Beckhoff SSC and generated object dictionary
  tests/               host-side protocol tests
host/rdk_ecat_motor_rt/ current SOEM cyclic master and PDO layout test
```

## 构建

Ubuntu 环境已配置时，在仓库根目录执行：

```bash
./scripts/build_firmware.sh
```

依赖版本与环境配置见
[`docs/42-ubuntu-firmware-build.md`](docs/42-ubuntu-firmware-build.md)。

Windows 构建示例：

```powershell
cmake -S firmware/ecat_tsn_gateway `
  -B firmware/ecat_tsn_gateway/build_freertos `
  -G Ninja `
  -DBOARD=hpm6e00_full_port `
  -DHPM_BUILD_TYPE=flash_xip `
  -DCMAKE_BUILD_TYPE=debug `
  -DBOARD_SEARCH_PATH=D:\Tools\hpm\sdk_env-main\hpm_app\boards

ninja -C firmware/ecat_tsn_gateway/build_freertos
```

详细结构与实时约束见
[`docs/24-code-structure-optimization.md`](docs/24-code-structure-optimization.md)。
