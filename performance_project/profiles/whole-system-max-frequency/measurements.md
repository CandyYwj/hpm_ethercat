# Measurements: HPM gateway whole-system maximum sustainable frequency

## Reproducible Environment

- Hardware/clock/power mode: HPM6E80 full-port board and RDK X5; exact board
  revisions, MCU clocks, RDK governor and temperature must be recorded per run.
- Firmware/binary/compiler: Commit `8dd5a8c`; record `demo.elf` SHA-256,
  HPM SDK/compiler versions and optimization level before testing.
- RTOS/cache/configuration: FreeRTOS tick 1 kHz; IMU DMA buffer 2 KiB
  non-cacheable; UART12 460800 baud; current CAN mask enables only CAN7.
- Load and environmental conditions: Current firmware enables only axis14.
  This is not a 16-motor load; each additional stage must record enabled axes,
  feedback rate, IMU output selection and motor power state.
- Instrumentation and measured overhead: Not measured. Per-cycle `printf` and
  relative `usleep` in `ecat_motor_test` are prohibited for performance runs.

## Measurement Protocol

### Prerequisites

1. Resolve functional failures first: motor feedback must be nonzero/fresh when
   a motor is expected online, and IMU DMA overrun must remain zero at baseline.
2. Disable command/feedback console formatting in the cyclic thread. Record
   timestamps and counters in a bounded memory buffer and print after stop.
3. Use `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` on RDK. Record
   CPU affinity, scheduler policy/priority, NIC IRQ affinity and CPU governor.
4. Instrument HPM with the RISC-V cycle counter or two GPIO markers around
   `MainLoop()` and `motor_manager_poll_1ms()`. Measure marker overhead alone.
5. Use a CAN analyzer or logic analyzer as an independent clock for per-bus SOF,
   EOF, utilization and response timing.

### Frequency sweep

For each stage, sweep periods `10000, 5000, 2000, 1250, 1000 us`; communication-
only testing may continue to `800` and `500 us`. Run 60 seconds per discovery
point, stop at the first failure, then run the highest passing whole-system rate
for 30 minutes (at 1 kHz this is 1.8 million cycles). Repeat the final point for
at least three cold boots.

1. EtherCAT only, motor power off, IMU disconnected.
2. EtherCAT plus IMU at its maximum intended output configuration.
3. Add one powered motor at low limits; then four motors on one CAN bus.
4. Add one motor per CAN bus; then four motors per bus on all four buses.
5. Repeat with the final intended feedback rates and simultaneous IMU traffic.

At every point collect cycle release lateness, send/receive duration, WKC,
firmware control execution time, end-to-end command latency, each CAN bus load,
feedback age, IMU frame/error counters, task stack high-water marks and thermal
state. Report min/p50/p99/p99.9/max and violation count, not only average.

### Stop conditions

Stop the run immediately on OP loss, unexpected WKC, any deadline miss, command
timeout, CAN TX/drop/error growth, stale feedback, IMU DMA overrun/error growth,
stack margin below 25%, unexpected motion, current-limit event or thermal alarm.

| Run | Metric ID | Samples/duration | Min | Median | High percentile | Max observed | Violations | Artifact |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Baseline | All | Not run | - | - | - | - | - | Pending target instrumentation |
