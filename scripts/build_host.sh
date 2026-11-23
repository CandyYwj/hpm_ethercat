#!/usr/bin/env bash
# Build the Linux ROS 2 C++ packages and their compile databases.
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
clean_cache=false
for argument in "$@"; do
    case "$argument" in
        --clean-cache) clean_cache=true ;;
        --help|-h)
            printf 'Usage: %s [--clean-cache]\n' "$0"
            printf 'Build messages, motor control and gamepad with native Linux C++17.\n'
            printf 'Overrides: HPM_ROS_SETUP, SOEM_DIR, HPM_HOST_PYTHON, HPM_HOST_BUILD_JOBS.\n'
            exit 0 ;;
        *) printf 'Unknown argument: %s\n' "$argument" >&2; exit 2 ;;
    esac
done

ros_setup="${HPM_ROS_SETUP:-}"
if [[ -z "$ros_setup" ]]; then
    for candidate in \
        "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash" \
        "/opt/tros/${ROS_DISTRO:-humble}/setup.bash" \
        /opt/ros/jazzy/setup.bash /opt/ros/humble/setup.bash; do
        if [[ -f "$candidate" ]]; then ros_setup="$candidate"; break; fi
    done
fi
if [[ ! -f "$ros_setup" ]]; then
    printf 'ROS 2 setup.bash not found. Set HPM_ROS_SETUP to your ROS installation.\n' >&2
    exit 1
fi
# ROS setup scripts can reference unset environment variables.
set +u
source "$ros_setup"
set -u

export SOEM_DIR="${SOEM_DIR:-$HOME/tools/ethercat/soem}"
if [[ ! -f "$SOEM_DIR/include/soem/soem.h" || ! -f "$SOEM_DIR/include/soem/ec_options.h" ]]; then
    printf 'SOEM headers not found in %s\nUse the installed SOEM v2 prefix, including generated ec_options.h.\n' "$SOEM_DIR" >&2
    exit 1
fi
if ! command -v colcon >/dev/null; then
    printf 'colcon is required to build the ROS workspace.\n' >&2; exit 1
fi
host_python="${HPM_HOST_PYTHON:-/usr/bin/python3}"
if [[ ! -x "$host_python" ]]; then
    printf 'ROS Python executable not found: %s\n' "$host_python" >&2; exit 1
fi
export CMAKE_BUILD_PARALLEL_LEVEL="${HPM_HOST_BUILD_JOBS:-4}"

printf 'ROS setup: %s\nSOEM prefix: %s\n' "$ros_setup" "$SOEM_DIR"
cd -- "$repo_root/host/hpm_gateway_ros2_ws"
build=(colcon build --symlink-install
    --packages-up-to hpm_gateway_control hpm_gateway_gamepad)
if "$clean_cache"; then build+=(--cmake-clean-cache); fi
build+=(--cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DPython3_EXECUTABLE=$host_python")
"${build[@]}"
"$host_python" "$script_dir/update_compile_commands.py"
printf '\nHost build OK. VS Code reads the control and gamepad compile databases automatically.\n'
