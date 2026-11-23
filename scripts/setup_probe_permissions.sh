#!/usr/bin/env bash
# Install USB permissions; this does not connect to or program a target.
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
    printf 'Usage: sudo %s\nInstall CMSIS-DAP/HSLINK USB rules, then reconnect the probe.\n' "$0"
    exit 0
fi
if [[ $# -ne 0 ]]; then
    printf 'Unexpected argument. Use --help for usage.\n' >&2; exit 2
fi
if [[ "$EUID" -ne 0 ]]; then
    printf 'USB rules require root: sudo %s\n' "$0" >&2
    exit 1
fi
install -m 0644 "$script_dir/udev/70-hpm-debug-probes.rules" /etc/udev/rules.d/70-hpm-debug-probes.rules
udevadm control --reload-rules
printf 'USB rules installed. Reconnect the debug probe to apply them.\n'
