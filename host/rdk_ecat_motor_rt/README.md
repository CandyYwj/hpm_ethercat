# 16-axis EtherCAT cyclic master

This is the current SOEM v2 EtherCAT master for the HPM gateway. It keeps cycle
timing and EtherCAT socket ownership explicit and targets Linux on the RDK.

New code in this directory follows the repository [C/C++ coding standard](../../docs/25-c-cpp-coding-standard.md).

## Why this is not `select()`-based

SOEM owns one raw EtherCAT socket, and a process-data exchange is one ordered
`ecx_send_processdata()` / `ecx_receive_processdata()` transaction. Running
`select()` directly on SOEM internals cannot add useful parallelism and can make
the result depend on SOEM's private port implementation. The program instead
uses one exclusive SOEM owner with absolute-time releases:

```text
release -> prepare fixed 320 B output -> send -> receive -> WKC/deadline stats
        -> clock_nanosleep(TIMER_ABSTIME) -> next release
```

This removes the old master's concurrent `ecx_readstate()` call, relative
`usleep()` drift, and high-frequency console output from the cyclic path.

## Build and local layout test

```bash
cd host/rdk_ecat_motor_rt
make test
make
```

`make test` needs only a C11 compiler. `make` additionally needs SOEM v2 at
`SOEM_DIR` and links against the Linux SOEM library.

## Run

```bash
sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --rx-timeout-us 500
```

`--arm-all` is deliberately mandatory. The current gateway enables every motor
marked active in `motor_config.c` after the first command PDO. PDO axis n maps
to global CAN motor ID n + 1; the current profile enables CAN0 and CAN1 motors.
The reference command applies low-gain velocity control only to CAN0 IDs 1, 2,
3, and 13, leaving every other PDO slot zero. It is a communication/timing
baseline, not a motion generator.

The program verifies the runtime PDO contract before OP:

| Direction | Bytes | Layout |
| --- | ---: | --- |
| Master -> gateway | 320 | 16 x `(position, velocity, torque, kp, kd)` float |
| Gateway -> master | 252 | 60-byte IMU prefix + 16 x `(position, velocity, torque)` float |

It reports per-second cycle rate plus cumulative WKC failures, deadline misses,
maximum exchange time, and maximum lateness. A WKC mismatch or a deadline miss
is evidence to investigate, not a condition silently retried or ignored.

## Optional scheduling

`--rt-priority N` requests `SCHED_FIFO` priority 1..99. It is optional because
the target machine must reserve CPU capacity for NIC IRQ handling and other
control work. The program warns and continues if the request is not permitted.

## Scope and known safety boundary

Only the cyclic task calls SOEM after setup. The program does not expose the
raw PDO buffer to another thread and does not call `ecx_readstate()` in parallel
with `ecx_receive_processdata()`.

This host-side program cannot make the current gateway's auto-enable behavior
per-axis. A production arm/disarm contract requires a gateway protocol change
(for example, an explicit per-axis armed bit or a separate safety object).
