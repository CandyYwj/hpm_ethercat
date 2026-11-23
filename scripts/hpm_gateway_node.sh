#!/usr/bin/env bash
# Author: BearKing
#
# hpm_gateway_node.sh - Boot-time launcher for the hpm_gateway_control node.
#
# Sources the TROS environment and the colcon workspace, sets ROS_DOMAIN_ID,
# then execs `ros2 launch`. Runs as root so the EtherCAT raw socket is allowed.
#
# The launch file is selected by HPM_GATEWAY_LAUNCH (default: EtherCAT mode).
#   hpm_gateway_control_eth0.launch.py  EtherCAT mode (drives motors)
#   hpm_gateway_control.launch.py       safe mode (ROS2 only, no EtherCAT)
#
# Usage (run as root):
#   /usr/local/bin/hpm_gateway_node.sh
#   HPM_GATEWAY_LAUNCH=hpm_gateway_control.launch.py /usr/local/bin/hpm_gateway_node.sh
set -e

TROS_SETUP="${TROS_SETUP:-/opt/tros/humble/setup.bash}"
WS_SETUP="${WS_SETUP:-/root/hpm_gateway_ros2_ws/install/setup.bash}"
LAUNCH="${HPM_GATEWAY_LAUNCH:-hpm_gateway_control_eth0.launch.py}"
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

echo "[hpm_gateway_node] launching: ros2 launch hpm_gateway_control $LAUNCH"
exec ros2 launch hpm_gateway_control "$LAUNCH"
