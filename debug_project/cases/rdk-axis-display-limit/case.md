# RDK 16-axis log only displays four axes

- Case ID: `rdk-axis-display-limit`
- Opened: 2026-07-21T17:36+08:00
- Status: `root-cause-found`
- Owner: firmware team

## Problem Statement

### Expected behavior

Running with `active_axes=16` should make all 16 command and feedback values
observable when detailed logging is requested.

### Reported behavior

`CMD` and `FB` lines show axes 0 through 3. The command line then prints
`...+12`, which was interpreted as only four axes being active.

### Impact and scope

The behavior affects terminal presentation only. Command generation still
iterates over all active axes, and the EtherCAT PDO remains 16-axis.

## Environment

- Version/commit: `8dd5a8c`
- Build/configuration: `host/rdk_ecat_motor/ecat_motor_test`, 16 active axes
- Hardware/OS/runtime: RDK X5, Linux aarch64, SOEM
- Frequency: Every ten-cycle status print
- First observed: 2026-07-21
- Recent changes: Host-side IMU/PDO display update

## Boundaries and Completion Criteria

Do not change PDO or control behavior for this diagnosis. Completion requires
proving whether the four-axis limit is in command generation, PDO sizing, or
logging. A display change requires separate authorization.

## Links

- [Investigation](investigation.md)
- [Experience report](experience.md)
- [Artifacts](artifacts/)
