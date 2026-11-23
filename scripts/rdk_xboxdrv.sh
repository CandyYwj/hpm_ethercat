#!/usr/bin/env bash
# Author: BearKing
#
# rdk_xboxdrv.sh - Watchdog wrapper around xboxdrv for the 2.4G XBOX360 clone
# receiver (vendor 0x413d / product 0x2104) on the RDK.
#
# The RDK's BSP kernel (6.1) xpad lacks the 0x413d entry, so the kernel only
# sees the keyboard interface. xboxdrv grabs the vendor HID interface
# (class 0xff/93/1) with libusb, decodes the standard XBOX360 protocol, and
# exposes it as a uinput joystick at /dev/input/js0.
#
# This script is a HOT-PLUG watchdog: xboxdrv blocks while the receiver is
# present and exits when it is unplugged; the loop retries, so a replug
# recreates js0 automatically. Run it as a systemd service
# (rdk_xboxdrv.service), which makes the whole chain hot-pluggable.
#
# Verify manually first:
#   sudo apt install -y xboxdrv
#   sudo /usr/local/bin/rdk_xboxdrv.sh
#   ls /dev/input/js0        # expect js0; test with: jstest /dev/input/js0
set -e

XBOXDRV_BIN="${XBOXDRV_BIN:-xboxdrv}"
VID="${XBOXDRV_VID:-413d}"
PID="${XBOXDRV_PID:-2104}"
RETRY_SEC="${XBOXDRV_RETRY_SEC:-2}"

if ! command -v "$XBOXDRV_BIN" >/dev/null 2>&1; then
    echo "error: xboxdrv not installed (apt install xboxdrv)" >&2
    exit 1
fi

# Run forever. xboxdrv exits when the receiver is unplugged; wait and retry so
# the next plug is picked up without any manual restart.
while true; do
    if ! "$XBOXDRV_BIN" --device-by-id "$VID:$PID" --type xbox360 \
        --mimic-xpad --detach-kernel-driver --silent; then
        echo "[rdk_xboxdrv] controller not present; retrying in ${RETRY_SEC}s"
    fi
    sleep "$RETRY_SEC"
done
