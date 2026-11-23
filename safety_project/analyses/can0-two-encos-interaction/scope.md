# Safety Scope: CAN0 dual ENCOS motor interaction during bring-up

- Analysis ID: `can0-two-encos-interaction`
- Opened: 2026-07-28T21:43+08:00
- Status: `open`
- Lifecycle/version baseline: CAN0-only ENCOS bring-up firmware, 2026-07-28
- Required qualified reviewers/approvers: TBD

## System Boundary and External Dependencies

HPM gateway, CAN0 wiring, two ENCOS motors configured as IDs 1 and 2, their
power supplies, and the EtherCAT host command path. The CAN analyzer is an
observation tool, not a safety interlock.

## People, Assets, Modes, Environment, and Reasonable Misuse

People must stay clear of moving shafts; test with an unloaded mechanism and a
reachable independent power disconnect. Modes are powered wiring inspection,
one-motor command test, and two-motor command test.

## Energy and Hazard Sources

Motor electrical power and shaft motion. Communication loss, duplicate CAN
identifiers, incorrect CAN wiring/termination, and supply collapse can cause
loss of commanded motion or uncommanded/erratic motion.

## Safe State by Mode

Before changing wiring or identifiers: disable motor power. During a bus fault,
remove motor power using the independent disconnect; do not rely on CAN stop
commands because the bus may be unavailable.

## Assumptions and Applicable Standards

Motor-ID labels and termination state have not yet been independently verified.
This is a bring-up analysis, not a certification assessment.
