# RS06 MIT axis 3 feedback remains zero at the EtherCAT master

- Case ID: `rs06-mit-no-feedback`
- Opened: 2026-07-28T00:00+08:00
- Status: `fixed`

## Problem statement

The RDK cyclic master reports valid EtherCAT exchange and IMU data, but all
motor feedback values, including axis 3 (CAN0, motor ID 4), remain zero after
the RS06 axes were switched to MIT mode.

## Expected behavior

The axis-3 command is continuously transmitted as position 3.0 rad, velocity
and torque 0, Kp 0.1, Kd 0.1. A valid RS06 MIT feedback frame should update
axis 3 in the EtherCAT input PDO.

## Boundaries

This investigation changes only the MIT feedback routing configuration and
decoder selection. It does not change command payload encoding or motor safety
behavior.

## Links

- [Investigation](investigation.md)
- [Experience report](experience.md)
