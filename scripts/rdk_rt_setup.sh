#!/usr/bin/env bash
# Author: BearKing
#
# rdk_rt_setup.sh - Pin the EtherCAT NIC interrupts to a dedicated CPU core.
#
# The EtherCAT worker thread is pinned with rt_cpu in
# hpm_gateway_control_eth0.yaml; this script pins the NIC interrupts to the
# SAME core so frame-arrival interrupts land locally instead of waking the
# worker thread on another core.
#
# Usage (run as root):
#   ./rdk_rt_setup.sh [core] [interface]
#   CORE=3 IFACE=eth0 ./rdk_rt_setup.sh
#
#   core       -1: restore default all-core affinity (ff)
#   core    >= 0 : pin to that core (mask 1<<core). Default: 3.
#   interface     : NIC name to match in /proc/interrupts. Default: eth0.
#
# Idempotent: safe to run on every boot.
set -eu

CORE="${1:-${CORE:-3}}"
IFACE="${2:-${IFACE:-eth0}}"

if [ "$(id -u)" -ne 0 ]; then
    echo "error: must run as root to write /proc/irq/*/smp_affinity" >&2
    exit 1
fi

# Affinity mask for the requested core; -1 restores the default (all cores).
if [ "$CORE" -lt 0 ]; then
    MASK="ff"
    LABEL="all cores"
else
    MASK="$(printf '%x' "$((1 << CORE))")"
    LABEL="core ${CORE}"
fi

# irqbalance would fight the manual affinity; disable it if present.
if systemctl list-unit-files 2>/dev/null | grep -q '^irqbalance'; then
    systemctl stop irqbalance 2>/dev/null || true
    systemctl disable irqbalance 2>/dev/null || true
fi

# Collect every IRQ line that names the interface (handles multi-queue NICs
# like eth0-0/eth0-1 in addition to the plain eth0 line).
mapfile -t LINES < <(grep -E "(^| )${IFACE}(\+|-|$)" /proc/interrupts || true)
if [ "${#LINES[@]}" -eq 0 ]; then
    echo "error: no IRQ found for '${IFACE}' in /proc/interrupts" >&2
    exit 1
fi

for LINE in "${LINES[@]}"; do
    IRQ_NUM="$(printf '%s' "$LINE" | awk -F: '{print $1}' | tr -d ' ')"
    AFFINITY="/proc/irq/${IRQ_NUM}/smp_affinity"
    if [ ! -w "$AFFINITY" ]; then
        echo "warning: cannot write ${AFFINITY}, skipping" >&2
        continue
    fi
    echo "$MASK" > "$AFFINITY"
    echo "irq ${IRQ_NUM} (${IFACE}): smp_affinity=${MASK} (${LABEL})"
done

echo "--- current interrupts for ${IFACE} ---"
grep -E "(^| )${IFACE}(\+|-|$)" /proc/interrupts
