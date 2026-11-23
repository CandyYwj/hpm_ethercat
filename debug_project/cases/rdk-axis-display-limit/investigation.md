# Investigation: RDK 16-axis log only displays four axes

## Reproduction Baseline

- Steps: Run `sudo ./ecat_motor_test eth0 16` and observe periodic status.
- Input/workload: 16 active axes, default cycle setting.
- Expected result: Terminal makes all requested axes observable.
- Actual result: `CMD` and `FB` show axes 0..3; `CMD` appends `...+12`.
- Reproduction rate: Every status print in the supplied log.
- Observation window: 218 EtherCAT cycles.

## Known Facts and Constraints

- `argv[2]` is parsed into `g_active_axes`; the supplied value is 16.
- Command generation iterates while `i < g_active_axes && i < g_max_axes`.
- Both command and feedback print loops contain an independent `ai < 4` cap.
- `...+12` is emitted only when `g_active_axes > 4`, proving the runtime value
  is 16 and the remaining axes are intentionally summarized.

## System and Failure Path

CLI `16` -> `g_active_axes=16` -> 16-axis command generation -> 320-byte
RxPDO. The data remains correct through command generation; information is
first omitted in the terminal formatting loops capped by `ai < 4`.

## Hypotheses

| ID | Hypothesis | Supporting evidence | Falsifier | Next experiment | Status |
| --- | --- | --- | --- | --- | --- |
| H1 | Only four axes are active | Only four values are visible | Runtime reports 16 and summarizes 12 more | Inspect parser and generation loop | Rejected |
| H2 | PDO supports only four axes | Four feedback values are visible | Mapping reports 16-axis layout and generation uses `g_max_axes` | Inspect mapping calculation | Rejected |
| H3 | Logging intentionally caps output at four axes | `ai < 4` appears in both print loops | Removing only that cap would not change visible count | Source inspection | Confirmed |

## Experiment Log

| Time | Change or probe | Controlled variables | Observation | Interpretation | Artifact |
| --- | --- | --- | --- | --- | --- |
| 2026-07-21 | Read-only source inspection | No runtime or PDO changes | Command and feedback formatters both use `ai < 4`; command generation does not | Display limit is independent from active-axis count | `host/rdk_ecat_motor/ecat_motor_test.c` |

## Root Cause

- Symptom: Only axes 0..3 are printed for a 16-axis invocation.
- Trigger: Any `g_active_axes` value greater than four.
- Failure mechanism: Status formatting terminates at `ai == 4` and folds the
  remainder into `...+N`.
- Direct root cause: Hard-coded `ai < 4` conditions in both status print loops.
- Contributing factors: The CLI argument name suggests active count but there
  is no separate display-count option; feedback lacks an explicit `...+N`.
- Detection/containment gaps: Startup does not explain that periodic output is
  a four-axis summary.
- Distinguishing evidence: `...+12` can only be printed when the runtime active
  count is 16, while command generation loops over that full count.
- Counterfactual check: A display-only loop over all 16 axes would reveal all
  values without changing command bytes, PDO sizes, or EtherCAT state.

## Fix and Verification

- Fix: Not applied; the user requested diagnosis.
- Why it breaks the causal chain: A future display-only fix must remove or
  configure the four-axis formatter cap.
- Original reproduction result: Root cause reproduced by source conditions and
  the supplied `...+12` output.
- Regression result: Not run because production behavior was not modified.
- Performance/resource result: Printing 16 axes inside the cyclic thread may
  add significant jitter; a fix should keep detailed printing out of that path.
- Remaining risks: DMA overrun is a separate reported condition and is not
  explained by the axis display limit.
