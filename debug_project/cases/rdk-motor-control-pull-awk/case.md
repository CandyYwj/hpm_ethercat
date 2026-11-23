# RDK motor control pull awk failure

- Case ID: `rdk-motor-control-pull-awk`
- Opened: 2026-07-31T10:48+08:00
- Status: `fixed`
- Owner: Codex

## Problem Statement

### Expected behavior

`scripts/pull_rdk_motor_control.ps1` creates a remote archive of
`/root/hpm_gateway/host/rdk_motor_control`, downloads it, extracts it locally,
and switches it into `host/rdk_motor_control`.

### Reported behavior

The remote archive was created, then the remote command failed with:

```text
awk: line 1: syntax error at or near %
ssh failed with exit code 2 while creating /root/hpm_gateway/.rdk-motor-control-pull-<id>.tar.gz.
```

### Impact and scope

The pull script could not complete. The failure was limited to the diagnostic
file-count print in the remote command; it did not indicate that the remote
directory was missing or that `tar` failed.

## Environment

- Version/commit: working tree script added for RDK motor-control sync
- Build/configuration: Windows PowerShell invoking OpenSSH to RDK Ubuntu
- Hardware/OS/runtime: local Windows PowerShell, remote RDK Linux shell
- Frequency: observed once from user terminal, deterministic for the command shape
- First observed: 2026-07-31
- Recent changes: new `scripts/pull_rdk_motor_control.ps1`

## Boundaries and Completion Criteria

Completion criteria: explain why the remote `awk` command failed, remove that
failure path, and verify the script parses and reaches a different stage than
the reported `awk` failure.

## Links

- [Investigation](investigation.md)
- [Experience report](experience.md)
- [Artifacts](artifacts/)
