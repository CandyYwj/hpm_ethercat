# Experience: RDK 16-axis log only displays four axes

## Summary

The program controls the requested axis count but intentionally summarizes only
four axes in its periodic terminal output. Runtime presentation must not be used
as the sole proof of PDO width or command-generation coverage.

## Symptom Versus Root Cause

- Visible symptom: A 16-axis run shows four command and feedback entries.
- Actual root cause: Both format loops have a hard-coded `ai < 4` limit.
- Why they were easy to confuse: The output did not label itself as a summary,
  and only the command line included the `...+12` remainder marker.

## Evidence That Mattered

The decisive signals were `...+12`, the full-axis command-generation loop, and
the separate four-axis caps in the two logging loops.

## Investigation Lessons

- Useful signals: Parsed active-axis count, PDO-derived maximum, loop bounds.
- Misleading signals or rejected hypotheses: Four visible rows did not imply a
  four-axis PDO or four-axis command path.
- Fastest distinguishing experiment: Inspect generation and formatting loop
  bounds independently.

## Fix and Validation

No fix was applied during diagnosis. A safe fix should publish snapshots from
the cyclic thread and print all requested axes from a non-real-time thread.

## Prevention

- Code/design guard: Give summary count and display count separate names.
- Test or monitor: Assert structure/PDO sizes and test formatter output for 1,
  4, and 16 axes.
- Documentation/process update: State explicitly when console output is folded.

## Reusable Checklist

- [x] Verify CLI active-axis parsing.
- [x] Compare command-generation and print loop bounds.
- [ ] Measure cyclic jitter before enabling verbose 16-axis logging.

## Unresolved Risks

The independently reported IMU DMA overrun remains unresolved and requires a
separate investigation.
