# Hazards: CAN0 dual ENCOS motor interaction during bring-up

## Losses

| Loss ID | Unacceptable loss | Affected people/assets | Rationale |
| --- | --- | --- | --- |
| L-1 | Unexpected or uncontrolled shaft motion during wiring diagnosis | People near the mechanism; motor and attached equipment | CAN and power faults can invalidate software control. |

## Hazards and Causal Scenarios

| Hazard ID | Hazardous system state | Mode/context | Loss | Existing control | Status |
| --- | --- | --- | --- | --- | --- |
| H-1 | Motor remains energized while CAN wiring, termination, or IDs are changed | Dual-motor CAN0 bring-up | L-1 | Host timeout stop; independent disconnect required | open |

| Scenario ID | Hazard | Cause/trigger | Failure propagation | Detection gap | Proposed control |
| --- | --- | --- | --- | --- | --- |
| S-1 | H-1 | CANH/CANL/GND error, excessive termination, duplicate actual ID, or supply droop after motor 2 is attached | Bus error or reset prevents intended command/stop behavior | A stopped motor does not identify the electrical cause | De-energize before changes; measure termination and use a CAN analyzer before motion test. |
