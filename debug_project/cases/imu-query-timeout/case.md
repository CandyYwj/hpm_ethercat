# YESENSE startup queries time out while streaming works

- Case ID: `imu-query-timeout`
- Opened: 2026-07-21T19:02+08:00
- Status: `investigating`
- Owner: TBD

## Problem Statement

### Expected behavior

Firmware queries product/software, baud rate, output frequency, output content,
and algorithm mode before starting UART12 RX DMA. Valid replies are printed once
and normal streaming then continues.

### Reported behavior

All five queries time out. RX DMA subsequently decodes about 200 valid streaming
frames per second with plausible acceleration, angular-rate, Euler, and quaternion
values.

### Impact and scope

Startup device metadata is unavailable. Normal IMU streaming and EtherCAT PDO data
remain operational.

## Environment

- Version/commit: working tree based on `8dd5a8c`
- Build/configuration: HPM6E80, `hpm6e00_full_port`, FreeRTOS, flash XIP debug
- Hardware/OS/runtime: HPM6E00 full-port board, UART12 PF16/PF17, YESENSE IMU
- Frequency: 5/5 startup query classes time out; streaming succeeds continuously
- First observed: 2026-07-21 after adding startup information queries
- Recent changes: query protocol module and pre-DMA polling receive path added

## Boundaries and Completion Criteria

Do not write IMU RAM or Flash configuration. Preserve UART12 HDMA30 streaming,
the 2 KiB non-cacheable ring, and EtherCAT PDO behavior. Completion requires a
distinguishing runtime observation, a root-cause fix, host regression tests, clean
cross-build, target flash, and target log showing valid queried information.

## Links

- [Investigation](investigation.md)
- [Experience report](experience.md)
- [Artifacts](artifacts/)
