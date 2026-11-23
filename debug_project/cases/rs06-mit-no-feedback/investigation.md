# Investigation: RS06 MIT axis 3 feedback remains zero

## Baseline observation

The supplied RDK log reports about 1000 cycles/s, zero deadline misses, and
only one cumulative WKC error after more than 12,000 cycles. It prints IMU
data, while axes 0 through 15 remain at zero position, velocity, and torque.

## Evidence chain

1. `host/rdk_ecat_motor_rt/ecat_motor_rt.c` assigns axis 3 position 3.0 rad,
   velocity 0, torque 0, Kp 0.1, and Kd 0.1 before `run_cyclic()`.
2. `run_cyclic()` copies `master.output` to the EtherCAT output PDO every
   cycle.
3. The host's printed `axis03` value is read from the EtherCAT input PDO, so
   it is motor feedback, not an echo of `master.output.axis[3]`.
4. Firmware updates MIT feedback only when a classic, standard-ID, DLC-8 CAN
   frame has ID `MOTOR_MANAGER_HOST_ID` (`0xFD`) and contains the motor ID in
   data byte 0.
5. The captured MIT motion payload `9E 8C 80 00 01 00 48 00` decodes to about
   3.00 rad position, 0 rad/s velocity, Kp raw `0x001` (about 1.22), Kd raw
   `0x004` (about 0.098), and near-zero torque. It proves the command path is
   active, but does not encode the requested Kp 0.1.
6. The supplied HPM UART log reports `CAN init OK: initMask=0x0F`, proving
   CAN0 through CAN3 initialized successfully. It also repeatedly reports
   `cmd=0 seq=0`, `tx=0`, and `rx=0`.
7. `motor_manager_poll_1ms()` sends motion frames only after an EtherCAT
   output-PDO update changes the command sequence. With sequence zero, the
   command-seen gate remains false and no CAN frame is requested.
8. Device query and safe feedback poll are both configured as zero, so the
   firmware does not probe motors autonomously before a PDO command arrives.

## Hypotheses

| ID | Hypothesis | Supporting evidence | Distinguishing observation | Status |
| --- | --- | --- | --- | --- |
| H1 | EtherCAT output PDO had not reached the board when the UART log was captured | `cmd=0 seq=0` directly shows no output mapping update | Run the RDK master and observe `cmd=1` and a changing sequence | Confirmed |
| H2 | CAN initialization or driver failure prevented transmission | CAN TX remains zero | `initMask=0x0F` proves initialization, but the command gate prevents reaching the driver | Rejected for this log |
| H3 | Motor feedback uses a CAN ID or layout other than the decoder expects | No feedback is available while no command is sent | Reassess only after command gate opens and CAN TX rises | Deferred |

## Next experiment

Run the RDK master while collecting one board UART heartbeat. The expected
transition is `cmd=1`, a nonzero command sequence, and increasing `can0: tx`.
Only then can CAN reception and MIT feedback decoding be assessed.

## Root cause

The reported absence of CAN traffic is caused by the EtherCAT command gate,
not by MCAN initialization. At the time of the board log no output PDO had
updated the application command cache (`cmd=0 seq=0`), so the motor manager
intentionally did not transmit a CAN enable or motion frame. In addition, the
configured disabled device-query and safe-feedback-poll options mean no
autonomous motor discovery traffic is generated.

## Follow-up observation

A later board status line reports RS06-MIT axis 3 with `target=3.000000` while
its received position, online state, and feedback count remain zero. This
proves the EtherCAT command gate is now open and the command cache reached the
motor manager. It does not prove that the physical CAN transmit or motor
enable/feedback path succeeded.

The host source subsequently shown assigns axis 3 position 0 and velocity 0.4.
That source does not match the board's cached target 3.0, so the RDK executable
being run is stale or a different master is writing the PDO.

The next decisive evidence is the same heartbeat's `motor:` and `can0:` lines:
nonzero `cmd` and increasing CAN0 TX establish physical transmit attempts;
CAN0 RX and axis-3 feedback then distinguish missing motor response from a
decoder mismatch.

## Host-side observation

The RDK log shows a successful build, gateway OP state, expected WKC 3, zero
WKC errors over more than 6,000 cycles, and continuously changing IMU ticks.
This proves the HPM-to-RDK EtherCAT input PDO path is operational. All motor
feedback fields remain zero, which means the motor feedback section of that
PDO was not updated; it does not identify the CAN-side cause.

The firmware only calls `gateway_app_set_motor_feedback()` after
`rs06_mit_decode_feedback()` accepts a frame. The decoder requires a standard
CAN frame, DLC at least 8, CAN ID 0xFD, and a motor ID in data byte 0 that maps
to the configured axis. A CAN RX count without a matching decoded frame will
therefore not reach EtherCAT. The next required artifact is one complete
received CAN frame (bus, standard/extended flag, numeric ID, DLC, and 8 data
bytes) together with the matching board heartbeat.

## CAN-side observation

A later board log reports `cmd=1`, a continuously changing command sequence,
and 653,488 successful motor-manager send calls. CAN0 has received 163,480
frames, while motor-manager `rx` and `fb` remain zero. Therefore the MCU CAN
RX ISR and ring buffer are active, but none of the received CAN0 frames match
the configured ENCOS or RS06 MIT feedback decoders.

`can0: tx=0` is not evidence that no transmit request occurred: that counter is
incremented only by the MCAN transmit interrupt, whereas the motor-manager TX
counter increments after `mcan_transmit_blocking()` returns success.

CAN1 through CAN3 are enabled but have no received frames and accumulate more
than ten million error events each. If only the CAN0 motor is wired during this
test, the enabled-bus mask must be limited to CAN0 before further runtime
measurements; otherwise the floating buses create continuous error interrupts.

## Captured MIT feedback and fix

The CAN capture provides a standard-ID feedback frame for motor 4:

```
CAN ID 0x000, DLC 8: 04 1D EE 7F 97 FB 01 40
```

It is a valid MIT feedback frame: byte 0 is motor ID 4; the decoded values are
approximately position -9.6308 rad, velocity -0.1587 rad/s, torque -0.0791 Nm,
and temperature 32.0 C.  The CAN ID is `0x000`, not the firmware's previously
configured `MOTOR_MANAGER_HOST_ID` of `0xFD`. This differentiates the decoder
mismatch from missing CAN reception.

The firmware now stores the observed MIT feedback ID (`0x000`) in each RS06
MIT axis configuration. `handle_can_rx()` tries each enabled MIT configuration
instead of a single global host ID, then routes accepted feedback by the
captured `(bus, data[0] motor ID)`. `handle_rs06_mit_feedback()` also verifies
that the decoded feedback ID equals that axis's configured host ID before
updating EtherCAT feedback.

`ninja -C firmware/ecat_tsn_gateway/build` completed successfully after the
change. Hardware verification remains required: flash this build, command
axis 3, and confirm that `RS06-MIT axis3` reports `online=1`, `fb>0`, and
nonzero `pos` matching the motor response.
