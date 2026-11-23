# CLAUDE.md

## Project

This repository contains an HPM6E80 EtherCAT motor gateway and its current
SOEM host. The firmware exposes a 16-axis PDO contract, currently drives eight
motors across CAN0 and CAN1 (4 per bus: 3 x ENCOS + 1 x RS06 MIT), publishes
YESENSE IMU data, and configures the board TSW as an independent three-port
Ethernet switch.

```text
EtherCAT RxPDO (320 B) -> motor manager -> CAN0/CAN1 -> 6 x ENCOS + 2 x RS06 MIT
CAN feedback + IMU -> EtherCAT TxPDO (252 B)
TSN Port1 <-> TSW hardware <-> TSN Port2/Port3
```

EtherCAT and TSN are independent. Do not reintroduce the deleted 0x88B5 raw
frame bridge, TSN CPU DMA receive path, TSN interrupt, or PDO conversion.

## Firmware Build

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

The build requires `HPM_SDK_BASE` and `GNURISCV_TOOLCHAIN_PATH` from the HPM
SDK environment.

## Runtime Tasks

| Task | Period | Responsibility |
| --- | ---: | --- |
| EtherCAT | 1 ms | SSC `MainLoop()` only |
| Motor | 1 ms poll, 1 ms control | CAN receive and 1 kHz motor output |
| IMU | 1 ms | Bounded UART HDMA consumption and parsing |
| Status | 1000 ms | LED and low-rate diagnostics |

The high-rate tasks must not print, allocate dynamically, perform file I/O, or
add unbounded retries. TSN switching is hardware data-plane work and has no
FreeRTOS task.

## Source Ownership

| Path | Responsibility |
| --- | --- |
| `app/gateway_app.c` | SSC callbacks, object definitions, PDO command/feedback ownership |
| `app/gateway_system.c` | Ordered ESC, TSN, CAN, motor, and IMU initialization |
| `tasks/` | FreeRTOS task entry points and lifecycle |
| `drivers/can/` | HPM MCAN backend and ISR-owned RX rings |
| `drivers/imu/` | UART12/HDMA transport and latest IMU snapshot |
| `drivers/tsn/` | One-time three-port TSW initialization |
| `motor/` | Axis configuration and motor scheduling |
| `protocols/` | Hardware-independent ENCOS, RS06, and YESENSE codecs |
| `SSC/` | Beckhoff SSC and generated object dictionary boundary |
| `host/rdk_ecat_motor_rt/` | Current SOEM cyclic master and PDO layout test |

`_DIGITAL_IO_` must only be defined in `app/gateway_app.c`; that translation
unit owns the generated SSC object definitions. Other modules access motor PDO
state through `gateway_app.h`, not generated object names.

Do not broadly refactor SSC, HPM SDK, SOEM, or generated files. Keep custom
changes in project-owned modules unless integration correctness requires a
vendor-boundary change.

## Current Contracts

- RxPDO: 320 bytes, 16 x 5 `REAL32` motor commands.
- TxPDO: 252 bytes, 60-byte IMU prefix plus 16 x 3 `REAL32` feedback values.
- Motor command timeout: 100 ms.
- Enabled CAN buses: CAN0 and CAN1 only (`CAN_IF_ENABLED_BUS_MASK 0x03` in
  `drivers/can/can_if_hpm_mcan.c`); the interface supports 8 MCAN buses.
- Current active layout (8 enabled axes in `motor/motor_config.c`):
  - CAN0: axis0..2 ENCOS IDs 1..3, axis12 RS06 MIT ID 13.
  - CAN1: axis3..5 ENCOS IDs 4..6, axis13 RS06 MIT ID 14.
  - axis6..11 and axis14..15 remain disabled (unwired CAN2/CAN3).
- TSN broadcast and unknown unicast destinations: Port1, Port2, Port3 only.
- TSN CPU port, RX/TX DMA, and TSN IRQ remain disabled.

## Validation

Build and flash from the single `build` directory (O2, `GATEWAY_PERFORMANCE_BUILD=ON`):

```powershell
cmake -S firmware/ecat_tsn_gateway -B firmware/ecat_tsn_gateway/build -G Ninja `
  -DBOARD=hpm6e00_full_port -DBOARD_SEARCH_PATH=D:/Tools/hpm/sdk_env-main/hpm_app/boards `
  -DHPM_BUILD_TYPE=flash_xip -DCMAKE_BUILD_TYPE=debug -DGATEWAY_PERFORMANCE_BUILD=ON
ninja -C firmware/ecat_tsn_gateway/build
```

For pure host-side YESENSE logic, rebuild the standalone tests from
`firmware/ecat_tsn_gateway/tests/yesense_parser_test.c` and
`yesense_query_test.c`.

`host/rdk_ecat_motor_rt` provides `make test` for the PDO layout. A successful
build does not replace target validation of EtherCAT OP, CAN timing, IMU DMA,
motor timeout behavior, or physical TSN forwarding.
