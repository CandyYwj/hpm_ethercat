#!/usr/bin/env bash
# Build the HPM6E80 gateway with the Linux toolchain.
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
configure_only=false
fresh=false
dry_run=false

usage() {
    printf 'Usage: %s [--configure-only] [--clean] [--dry-run]\n' "$0"
    printf '  --configure-only  Generate Ninja files and compile_commands.json only.\n'
    printf '  --clean           Refresh the CMake cache and rebuild all objects.\n'
    printf '  --dry-run         Validate dependencies and print commands without building.\n'
    printf 'Environment overrides: see scripts/enter_hpm_env.sh; HPM_BUILD_JOBS defaults to 8.\n'
}

for argument in "$@"; do
    case "$argument" in
        --configure-only) configure_only=true ;;
        --clean) fresh=true ;;
        --dry-run) dry_run=true ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'Unknown argument: %s\n' "$argument" >&2; usage >&2; exit 2 ;;
    esac
done

source "$script_dir/enter_hpm_env.sh"
for command_name in cmake ninja; do
    if ! command -v "$command_name" >/dev/null; then
        printf '%s is required. Install cmake and ninja-build.\n' "$command_name" >&2
        exit 1
    fi
done
"$HPM_PYTHON" -c 'import yaml, jinja2'

build_dir="$repo_root/firmware/ecat_tsn_gateway/build"
configure=(cmake -S "$repo_root/firmware/ecat_tsn_gateway" -B "$build_dir" -G Ninja
    -DBOARD=hpm6e00_full_port
    "-DBOARD_SEARCH_PATH=$BOARD_SEARCH_PATH"
    "-DHPM_APP_DRIVERS=$HPM_APP_DRIVERS"
    -DHPM_BUILD_TYPE=flash_xip -DCMAKE_BUILD_TYPE=debug
    -DGATEWAY_PERFORMANCE_BUILD=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    "-DPYTHON_EXECUTABLE=$HPM_PYTHON")
build=(cmake --build "$build_dir" --parallel "${HPM_BUILD_JOBS:-8}")
if "$fresh"; then
    configure+=(--fresh)
    build+=(--clean-first)
fi

printf 'SDK: %s\nBoard: hpm6e00_full_port\nCompiler: %s\nDrivers: %s\n' \
    "$HPM_SDK_BASE" "$GNURISCV_TOOLCHAIN_PATH/bin/riscv32-unknown-elf-gcc" "$HPM_APP_DRIVERS"
if "$dry_run"; then
    printf '%q ' "${configure[@]}"; printf '\n'
    if ! "$configure_only"; then
        printf '%q ' "${build[@]}"; printf '\n'
    fi
    exit 0
fi

"${configure[@]}"
"$HPM_PYTHON" "$script_dir/update_compile_commands.py"
if "$configure_only"; then
    exit 0
fi
"${build[@]}"
IFS= read -r elf_path < "$build_dir/firmware-elf-path.txt"
if [[ ! -f "$elf_path" ]]; then
    printf 'Build finished but ELF was not generated: %s\n' "$elf_path" >&2
    exit 1
fi
printf '\nBuild OK: %s\n' "$elf_path"
"$GNURISCV_TOOLCHAIN_PATH/bin/riscv32-unknown-elf-size" "$elf_path"
