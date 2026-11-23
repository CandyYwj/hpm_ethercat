# Investigation: RDK motor control pull awk failure

## Reproduction Baseline

- Steps: Run `.\scripts\pull_rdk_motor_control.ps1 -RdkIp 192.168.108.178`.
- Input/workload: remote `/root/hpm_gateway/host/rdk_motor_control`.
- Expected result: archive is created, downloaded, extracted, and switched into local `host\rdk_motor_control`.
- Actual result: remote command printed `archive=...`, then failed with `awk: line 1: syntax error at or near %`.
- Reproduction rate: one user-observed run; local tool could not reach SSH after the fix.
- Observation window: 2026-07-31.

## Known Facts and Constraints

- The failure happened after `tar -czf` printed the remote archive path.
- The only command after archive creation that used `%` was the diagnostic `awk` print:
  `awk '{printf "files=%s\n", $1}'`.
- The file-count output is not required for the data transfer contract.
- The script is invoked through PowerShell, OpenSSH, a remote Linux shell, and `awk`, creating multiple quoting/interpolation layers.

## System and Failure Path

Last known-good state: remote `tar` created `/root/hpm_gateway/.rdk-motor-control-pull-<id>.tar.gz`.
First known-bad state: remote `awk` parsed the diagnostic print expression incorrectly and exited 2.
Data/control flow: PowerShell here-string -> `ssh.exe` command argument -> remote shell -> `awk` program.

## Hypotheses

| ID | Hypothesis | Supporting evidence | Falsifier | Next experiment | Status |
| --- | --- | --- | --- | --- | --- |
| H1 | Multi-layer quoting corrupted the remote awk script. | Error points at `%`; the only `%` is inside the awk format string; archive creation had already succeeded. | Removing awk still produces the same `%` syntax error. | Replace awk with plain `printf 'files='; find ... | wc -l`. | supported |
| H2 | Remote directory or tar failed. | The script reports failure while creating the archive. | User output includes `archive=/root/hpm_gateway/.rdk-motor-control-pull-...tar.gz` before the awk error. | Inspect command order. | rejected |

## Experiment Log

| Time | Change or probe | Controlled variables | Observation | Interpretation | Artifact |
| --- | --- | --- | --- | --- | --- |
| 2026-07-31 | Read reported output and `pull_rdk_motor_control.ps1`. | Same remote path and script. | Failure occurs after archive print; diagnostic awk is the failing command. | Root cause is the diagnostic awk command, not tar. | User terminal output |
| 2026-07-31 | Replace awk with `printf 'files='` plus `find ... | wc -l`; add remote error cleanup trap. | Same script contract and paths. | PowerShell parser passed; dry-run passed. | Local tool output |
| 2026-07-31 | Attempt actual pull from tool environment. | Same `-RdkIp 192.168.108.178`. | SSH connection failed earlier with `Permission denied`; no awk error observed. | Tool environment cannot complete remote validation; user terminal should rerun. | Local tool output |

## Root Cause

- Symptom: pull script aborts with remote `awk` syntax error near `%`.
- Trigger: executing the diagnostic file-count line through PowerShell -> SSH -> remote shell -> awk.
- Failure mechanism: the awk program text is fragile across multiple quoting/interpolation layers and reaches remote awk in a malformed form.
- Direct root cause: unnecessary `awk '{printf "files=%s\n", $1}'` in the remote command.
- Contributing factors: diagnostic output was part of the success path; remote archive cleanup only ran after a fully successful archive command.
- Detection/containment gaps: dry-run did not exercise remote shell quoting.
- Distinguishing evidence: archive path printed before the error; `%` exists only in the awk format string.
- Counterfactual check: removing awk removes the `%` parsing surface while preserving the archive/download contract.

## Fix and Verification

- Fix: replace the awk file-count line with `printf 'files='` followed by `find "$remote_dir" -type f | wc -l`; add a remote trap that removes the temporary archive when the remote command exits unsuccessfully.
- Why it breaks the causal chain: no awk program or `%` format string is passed through the multi-layer command path.
- Original reproduction result: user observed awk syntax error.
- Regression result: PowerShell parser passed and dry-run passed locally; actual pull could not be completed from the tool environment because SSH connection was denied before command execution.
- Performance/resource result: not applicable.
- Remaining risks: user terminal should rerun the actual pull to verify remote transfer end to end.
