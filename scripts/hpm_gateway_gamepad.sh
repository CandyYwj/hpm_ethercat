#!/usr/bin/env bash
# Author: BearKing
#
# hpm_gateway_gamepad.sh - Boot-time launcher for the event-driven gamepad node.
#
# Sources the TROS environment and the colcon workspace, sets ROS_DOMAIN_ID,
# then execs the gamepad node. The node auto-detects joysticks via udev, so
# no fixed device path is needed.
#
# Usage (run as root):
#   /usr/local/bin/hpm_gateway_gamepad.sh
set -e

TROS_SETUP="${TROS_SETUP:-/opt/tros/humble/setup.bash}"
WS_SETUP="${WS_SETUP:-/root/hpm_gateway_ros2_ws/install/setup.bash}"
DOMAIN_ID="${ROS_DOMAIN_ID:-42}"

if [ ! -f "$TROS_SETUP" ]; then
    echo "error: $TROS_SETUP not found (TROS not installed?)" >&2
    exit 1
fi
if [ ! -f "$WS_SETUP" ]; then
    echo "error: $WS_SETUP not found (run 'colcon build' first)" >&2
    exit 1
fi

export ROS_DOMAIN_ID="$DOMAIN_ID"
# shellcheck disable=SC1090
source "$TROS_SETUP"
# shellcheck disable=SC1090
source "$WS_SETUP"

echo "[hpm_gateway_gamepad] starting event-driven gamepad node"
exec ros2 run hpm_gateway_gamepad gamepad_node
