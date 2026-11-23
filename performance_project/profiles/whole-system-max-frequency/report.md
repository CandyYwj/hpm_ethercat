# Performance Report: HPM gateway whole-system maximum sustainable frequency

## Scope and Evidence Limits

No maximum sustainable whole-system frequency has been measured yet. This
profile defines the protocol and documents implementation limits. A finite
dynamic run will establish a maximum observed passing rate for the tested load,
not a proven WCET or universal maximum.

## Budget Comparison

| Metric ID | Budget | Baseline | Current | Margin | Status | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| ECAT-CYCLE | No misses | Not measured | Not measured | Unknown | blocked | Cyclic logger currently perturbs timing |
| ECAT-WKC | No unexpected WKC | Not measured | Not measured | Unknown | not-run | Target run required |
| FW-CONTROL | <800 us max observed | Not measured | Not measured | Unknown | not-run | Cycle-counter/GPIO instrumentation required |
| CMD-E2E | 1 ms target | Not measured | Not measured | Unknown | blocked | Only axis14/CAN7 enabled; feedback is zero |
| CAN-LOAD | <=70% sustained target | Not measured | Not measured | Unknown | blocked | Four-bus/four-motor configuration not enabled |
| IMU-RX | Zero errors/overruns | Overrun reported | Overrun reported | None | fail | Supplied RDK output shows DMA overrun |

## Bottleneck Evidence and Changes

- The current 1 kHz RTOS tick and 1 ms task delays cap useful control updates at
  1 kHz even if EtherCAT is exchanged faster.
- EtherCAT payload is 572 bytes/cycle, about 4.576 Mbit/s at 1 kHz before line
  overhead, so the 100 Mbit physical link is unlikely to be the first limit.
- Classic CAN is the likely throughput limit. At roughly 150 bits worst-case per
  extended 8-byte frame, four 1 kHz command frames consume about 60% of one
  1 Mbit/s bus. Four additional 1 kHz feedback frames would exceed capacity;
  feedback must be measured and reduced or phase-scheduled.
- `mcan_transmit_blocking()` serializes work in the 1 ms control task and must be
  timed under the actual four-bus load before claiming 1 kHz headroom.
- Current firmware enables only CAN7 and axis14, so a 16-axis EtherCAT command
  test does not exercise a 16-motor physical system.

## Functional Regression and Remaining Risks

Motor feedback is currently all zero and the IMU reports DMA overrun. Those are
functional failures, so current runs cannot establish a valid maximum system
frequency. Resolve them before the sweep. Retain motor safety limits and verify
PDO/CAN/IMU correctness at every frequency point; a faster error path is not a
performance improvement.
