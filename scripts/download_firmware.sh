#!/usr/bin/env bash
# Flash the ELF selected by CMake; --dry-run never connects to a probe.
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
dry_run=false
check_only=false
elf_path=""
speed=1000

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) dry_run=true; shift ;;
        --check) check_only=true; shift ;;
        --elf|--speed)
            if [[ $# -lt 2 ]]; then
                printf 'Missing value for %s\n' "$1" >&2; exit 2
            fi
            if [[ "$1" == --elf ]]; then elf_path="$2"; else speed="$2"; fi
            shift 2 ;;
        --help|-h)
            printf 'Usage: %s [--check] [--dry-run] [--elf PATH] [--speed KHZ]\n' "$0"
            printf 'Default ELF: generated build/firmware-elf-path.txt; probe: HSLINK/CMSIS-DAP JTAG.\n'
            printf '  --check    Verify tools and local configuration without requiring firmware or a board.\n'
            exit 0 ;;
        *) printf 'Unknown argument: %s\n' "$1" >&2; exit 2 ;;
    esac
done
if [[ ! "$speed" =~ ^[1-9][0-9]*$ ]]; then
    printf 'JTAG speed must be a positive integer in kHz.\n' >&2; exit 2
fi

source "$script_dir/enter_hpm_env.sh"
openocd="$HPM_OPENOCD"
if "$check_only"; then
    if [[ ! -x "$openocd" ]]; then
        printf 'HPM OpenOCD not found: %s\n' "$openocd" >&2; exit 1
    fi
    for config_file in \
        "$repo_root/firmware/ecat_tsn_gateway/scripts/hslink_hpm6e00.cfg" \
        "$HPM_SDK_BASE/boards/openocd/probes/cmsis_dap.cfg" \
        "$HPM_SDK_BASE/boards/openocd/soc/hpm6e80-single-core.cfg" \
        "$HPM_SDK_BASE/boards/openocd/boards/hpm6e00evk.cfg" \
        "$HPM_ENV_ROOT/openocd/share/openocd/scripts/interface/cmsis-dap.cfg"; do
        if [[ ! -f "$config_file" ]]; then
            printf 'OpenOCD configuration missing: %s\n' "$config_file" >&2; exit 1
        fi
    done
    "$openocd" --version
    printf 'HPM6E80 / CMSIS-DAP configuration files found. No probe connection attempted.\n'
    exit 0
fi
if [[ -z "$elf_path" ]]; then
    path_file="$repo_root/firmware/ecat_tsn_gateway/build/firmware-elf-path.txt"
    if [[ ! -f "$path_file" ]]; then
        printf 'Run scripts/build_firmware.sh first; missing %s\n' "$path_file" >&2
        exit 1
    fi
    IFS= read -r elf_path < "$path_file"
fi
if [[ ! -f "$elf_path" ]]; then
    printf 'Firmware ELF not found: %s\n' "$elf_path" >&2; exit 1
fi
elf_path="$(realpath -- "$elf_path")"
if [[ "$elf_path" == *'{'* || "$elf_path" == *'}'* || "$elf_path" == *$'\n'* ]]; then
    printf 'ELF path contains unsupported Tcl characters.\n' >&2; exit 2
fi

command_args=("$openocd"
    -s "$HPM_ENV_ROOT/openocd/share/openocd/scripts"
    -s "$HPM_SDK_BASE/boards/openocd"
    -f "$repo_root/firmware/ecat_tsn_gateway/scripts/hslink_hpm6e00.cfg"
    -c "adapter speed $speed"
    -c "program {$elf_path} verify reset exit")
printf 'ELF: %s\n' "$elf_path"
if "$dry_run"; then
    printf '%q ' "${command_args[@]}"; printf '\n'
    exit 0
fi
if [[ ! -x "$openocd" ]]; then
    printf 'HPM OpenOCD not found: %s\nInstall it or set HPM_OPENOCD.\n' "$openocd" >&2
    exit 1
fi
exec "${command_args[@]}"
