# Safety Verification: CAN0 dual ENCOS motor interaction during bring-up

| Verification ID | Requirement | Method/test | Energy boundary and stop condition | Result | Evidence | Deviation/approval |
| --- | --- | --- | --- | --- | --- | --- |
| SV-1 | SR-2 | Power off; measure CANH-to-CANL resistance at the bus endpoints. | Motors de-energized | pending | User measurement | n/a |
| SV-2 | SR-2 | Power one motor at a time and observe control/feedback IDs with analyzer. | Unloaded mechanism; remove power on unexpected motion | pending | CAN capture | n/a |
