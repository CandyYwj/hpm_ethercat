# Performance Budget: HPM gateway whole-system maximum sustainable frequency

- Profile ID: `whole-system-max-frequency`
- Opened: 2026-07-21T18:32+08:00
- Status: `planned`
- Requirement/architecture baseline: Commit `8dd5a8c`; intended 16 motors on
  four Classic CAN 1 Mbit/s buses, 1 kHz FreeRTOS tick, 460800 baud IMU, and
  320-byte RxPDO plus 252-byte TxPDO.

| Metric ID | Trigger and completion | Period/deadline | Jitter/resource limit | Load | Margin | Source |
| --- | --- | --- | --- | --- | --- | --- |
| ECAT-CYCLE | Scheduled SOEM release to process-data receive completion | Sweep period; completion before next release | 0 missed deadlines; report p50/p99/p99.9/max lateness | Full 572-byte process image | At least 20% execution-time margin at accepted rate | Engineering discovery budget |
| ECAT-WKC | One process-data exchange | Every cycle | Expected WKC every cycle; zero unexpected drops | Same as ECAT-CYCLE | No accepted loss | EtherCAT correctness prerequisite |
| FW-CONTROL | Entry to return of `MainLoop()` plus `motor_manager_poll_1ms()` | 1 ms current deadline | Max observed below 800 us; zero deadline misses | Full EtherCAT, IMU and CAN load | Provisional 20% margin | Current 1 kHz scheduler architecture |
| CMD-E2E | EtherCAT output arrival to last required CAN command queued/on wire | 1 ms target | Report p50/p99/p99.9/max; zero command timeout | 16 motors, four buses | Budget requires phase-aligned measurement | User 1 ms goal |
| CAN-LOAD | SOF of first command to EOF of last command per bus | Within selected control period | Sustained utilization target <=70%; no TX failure/drop | Four motors per enabled bus | 30% recovery/error margin | Engineering bus-load target |
| IMU-RX | UART byte production to parsed snapshot publication | No ring overwrite | Zero DMA overrun, DMA error, checksum/format growth attributable to load | Maximum configured IMU output | 2 KiB buffer must cover worst service gap | Driver contract |
| STACK | Minimum task stack high-water mark | Entire stress run | At least 25% stack remaining | Combined worst load | 25% | Engineering resource target |

## Hard Constraints, Soft Targets, and Assumptions

- The effective whole-system control rate is currently capped at 1 kHz by the
  FreeRTOS tick, `gateway_control_task`, `imu_rx`, and motor manager periods.
  Faster EtherCAT cycles can measure communication headroom but do not increase
  motor command consumption without an architecture change.
- Budgets above are provisional discovery gates, not certified WCET or safety
  requirements. Dynamic testing reports maximum observed values only.
- A rate is sustainable only when functional output remains correct: OP is
  maintained, WKC is correct, feedback is fresh, no CAN drop occurs, and IMU
  error counters do not grow.
- Current independent EtherCAT and 1 ms RTOS phases may produce nearly one full
  tick of queueing before CAN work begins. A true <=1 ms end-to-end guarantee
  may require DC/Sync0 phase alignment or event-driven handoff.
- Motor power must remain disabled for EtherCAT-only tests. Powered tests need
  current/velocity limits, mechanical restraint, an independent stop, and a
  staged one-motor-to-16-motor progression.
