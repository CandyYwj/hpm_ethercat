#!/usr/bin/env bash
#
# rdk_link_setup.sh - Configure a USB-to-Ethernet adapter as a direct link to
# the RDK, or tear the link down.
#
# By default the RDK is expected at 192.168.108.178 (the address used by the
# deploy scripts).  A direct point-to-point cable therefore needs this machine
# on the same subnet, e.g. 192.168.108.155/24.  Because the built-in NIC is
# often also on 192.168.108.0/22, the /24 route created here is more specific
# and will correctly prefer the USB adapter for the RDK host.
#
# Usage (run as root):
#   sudo ./rdk_link_setup.sh up     [iface]      # bring the direct link up
#   sudo ./rdk_link_setup.sh down   [iface]      # remove the link config
#   sudo ./rdk_link_setup.sh status [iface]      # show current link state
#
# Environment overrides:
#   RDK_IP=192.168.108.178   LINK_IP=192.168.108.155   PREFIX=24
#
# Idempotent: `up` on an already-configured link is a no-op.
set -eu

ACTION="${1:-status}"
IFACE="${2:-}"

RDK_IP="${RDK_IP:-192.168.108.178}"
LINK_IP="${LINK_IP:-192.168.108.155}"
PREFIX="${PREFIX:-24}"

# Pick the USB-to-Ethernet adapter (enx*) when no interface was given.
if [ -z "$IFACE" ]; then
    IFACE="$(ip -o link show 2>/dev/null | awk '/^[0-9]+: enx/{print $2}' | tr -d ':' | head -n1)"
fi

if [ -z "$IFACE" ]; then
    echo "error: no enx* USB-Ethernet interface found; pass one explicitly" >&2
    exit 1
fi

if ! ip link show "$IFACE" >/dev/null 2>&1; then
    echo "error: interface '$IFACE' does not exist" >&2
    exit 1
fi

CUR="$(ip -o -4 addr show dev "$IFACE" 2>/dev/null | awk '{print $4}' | head -n1 || true)"

status() {
    echo "interface : $IFACE"
    echo "link state: $(cat /sys/class/net/$IFACE/carrier 2>/dev/null || echo down)"
    echo "ipv4      : ${CUR:-<none>}"
    echo "rdk target: $RDK_IP"
}

case "$ACTION" in
    up)
        if [ "$(id -u)" -ne 0 ]; then
            echo "error: 'up' must run as root (sudo)" >&2
            exit 1
        fi
        ip link set "$IFACE" up
        if [ -z "$CUR" ]; then
            ip addr add "$LINK_IP/$PREFIX" dev "$IFACE"
            echo "[OK] assigned $LINK_IP/$PREFIX to $IFACE"
        else
            echo "[OK] $IFACE already has $CUR; nothing to assign"
        fi
        ip route replace "$RDK_IP/32" dev "$IFACE" 2>/dev/null || true
        echo "--- verify ---"
        ping -c 2 -W 2 "$RDK_IP" || echo "warning: $RDK_IP did not respond (is the cable to the RDK plugged in?)"
        ;;
    down)
        if [ "$(id -u)" -ne 0 ]; then
            echo "error: 'down' must run as root (sudo)" >&2
            exit 1
        fi
        ip route del "$RDK_IP/32" dev "$IFACE" 2>/dev/null || true
        if [ -n "$CUR" ]; then
            ip addr del "$CUR" dev "$IFACE"
            echo "[OK] removed $CUR from $IFACE"
        else
            echo "[OK] $IFACE had no IPv4 address to remove"
        fi
        ;;
    status)
        status
        ;;
    *)
        echo "usage: $0 {up|down|status} [iface]" >&2
        exit 2
        ;;
esac
