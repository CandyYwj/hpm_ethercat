# Experience: RDK motor control pull awk failure

## Summary

The pull script failed because a nonessential remote diagnostic used awk through
too many quoting layers. The transfer logic was made more robust by removing
awk from the remote command and cleaning partial remote archives on error.

## Symptom Versus Root Cause

- Visible symptom: `ssh failed with exit code 2 while creating ...tar.gz`.
- Actual root cause: the diagnostic awk print command was malformed remotely.
- Why they were easy to confuse: the script labels the whole remote command as archive creation, but the archive had already been created before awk failed.

## Evidence That Mattered

- User output included `archive=/root/hpm_gateway/.rdk-motor-control-pull-...tar.gz` before the awk error.
- The failing command was the only remote line containing `%`.
- Removing awk preserved the useful file count while removing the fragile parser layer.

## Investigation Lessons

- Useful signals: command ordering in the here-string and the exact `awk` error token.
- Misleading signals or rejected hypotheses: the high-level thrown message made it look like `tar` failed.
- Fastest distinguishing experiment: replace the diagnostic awk with shell `printf` plus `wc -l`.

## Fix and Validation

Fix: `scripts/pull_rdk_motor_control.ps1` now prints the file count without awk
and removes the remote temporary archive if the remote command exits with an
error. Validation completed locally for PowerShell parsing and dry-run. End-to-end
remote pull still needs to be rerun from a terminal that can SSH to the RDK.

## Prevention

- Code/design guard: avoid awk/sed snippets for trivial diagnostics inside nested PowerShell/SSH here-strings.
- Test or monitor: after changing remote shell snippets, run the real SSH path or at least print the final remote command during review.
- Documentation/process update: mention that dry-run does not validate remote shell quoting.

## Reusable Checklist

- [ ] Keep remote SSH commands simple when they pass through PowerShell.
- [ ] Do not let optional diagnostics decide transfer success unless their failure matters.
- [ ] Add remote cleanup for temporary files created before later command failures.

## Unresolved Risks

The tool environment could not SSH to `192.168.108.178`, so the fixed script
needs one user-side end-to-end pull run.
