# Safety Requirements: CAN0 dual ENCOS motor interaction during bring-up

| Requirement ID | Hazard/scenario | Requirement and safe response | Detection/reaction time | Allocation/independence | Verification | Status |
| --- | --- | --- | --- | --- | --- | --- |
| SR-1 | H-1/S-1 | Motor power shall be removed before changing CAN wiring or IDs. A CAN fault shall be handled with the independent power disconnect, not only a CAN stop command. | Before physical change / immediate on fault | Operator and independent power path | Wiring procedure and observed disconnect | open |
| SR-2 | H-1/S-1 | Before a two-motor motion command, the unpowered CAN bus shall measure approximately 60 ohms between CANH and CANL and each motor shall be shown to respond only to its intended CAN ID. | Before motion test | Physical bus and test procedure | Resistance measurement and one-motor ID tests | open |
