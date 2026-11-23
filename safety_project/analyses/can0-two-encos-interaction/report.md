# Safety Analysis Report: CAN0 dual ENCOS motor interaction during bring-up

## Principal Hazards and Safety Strategy

The gateway sends independent standard-ID ENCOS control frames for axis0/ID1
and axis1/ID2. One motor stopping when another is connected is therefore not
explained by the axis mapping alone. Treat the condition as a CAN physical
layer, actual-ID, or power integrity fault until analyzer evidence distinguishes
it.

## Architecture and Independence Findings

CAN command timeout is not independent of a failed CAN bus. An independent
motor power disconnect is required during wiring and identifier changes.

## Verification Status and Evidence Limits

No two-motor analyzer trace, termination measurement, or supply-voltage record
has yet been collected. Software-only review cannot prove the physical bus.

## Open Hazards, Assumptions, and Residual Risk

Open hazards: actual IDs, termination placement, wiring polarity/common ground,
and power capacity remain unverified. Do not run a loaded mechanism until the
pending verification steps pass.

## Required Review and Approval

Requires the responsible lab operator to approve the physical two-motor test.
