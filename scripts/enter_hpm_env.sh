#!/usr/bin/env bash
# Source this file to select the Linux HPM SDK, compiler and Python environment.

_hpm_configure_env() {
    local env_root="${HPM_ENV_ROOT:-$HOME/tools/hpm}"
    local sdk="${HPM_SDK_BASE:-$env_root/hpm_sdk}"
    local apps="${HPM_APPS_BASE:-$env_root/hpm_apps}"
    local toolchain="${GNURISCV_TOOLCHAIN_PATH:-$env_root/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-linux}"
    local boards="${BOARD_SEARCH_PATH:-$apps/boards}"
    local drivers="${HPM_APP_DRIVERS:-$apps/apps/full_port/software/drivers}"
    local python="${HPM_PYTHON:-$env_root/venv/bin/python3}"
    local openocd="${HPM_OPENOCD:-$env_root/openocd/bin/openocd}"

    if [[ ! -f "$sdk/cmake/hpm-sdk-config.cmake" ]]; then
        printf 'HPM SDK not found: %s\nSet HPM_SDK_BASE to the SDK directory.\n' "$sdk" >&2
        return 1
    fi
    if [[ ! -x "$toolchain/bin/riscv32-unknown-elf-gcc" ]]; then
        printf 'Linux RISC-V compiler not found: %s\nSet GNURISCV_TOOLCHAIN_PATH to its root directory.\n' "$toolchain" >&2
        return 1
    fi
    if [[ ! -f "$boards/hpm6e00_full_port/board.h" || ! -f "$drivers/inc/full_port_config_drv.h" ]]; then
        printf 'Full-port board/drivers not found.\nBOARD_SEARCH_PATH=%s\nHPM_APP_DRIVERS=%s\n' "$boards" "$drivers" >&2
        return 1
    fi
    if [[ ! -x "$python" ]]; then
        printf 'SDK Python environment not found: %s\nSee docs/42-ubuntu-firmware-build.md.\n' "$python" >&2
        return 1
    fi

    export HPM_ENV_ROOT="$env_root" HPM_SDK_BASE="$sdk" HPM_APPS_BASE="$apps"
    export GNURISCV_TOOLCHAIN_PATH="$toolchain" BOARD_SEARCH_PATH="$boards"
    export HPM_APP_DRIVERS="$drivers" HPM_PYTHON="$python"
    export HPM_OPENOCD="$openocd"
    export HPM_SDK_TOOLCHAIN_VARIANT="${HPM_SDK_TOOLCHAIN_VARIANT:-}"
    export PATH="$(dirname "$openocd"):$(dirname "$python"):$toolchain/bin:$PATH"
}

if ! _hpm_configure_env; then
    unset -f _hpm_configure_env
    return 1 2>/dev/null || exit 1
fi
unset -f _hpm_configure_env

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    printf 'HPM_SDK_BASE=%s\nGNURISCV_TOOLCHAIN_PATH=%s\nBOARD_SEARCH_PATH=%s\nHPM_APP_DRIVERS=%s\nHPM_PYTHON=%s\n' \
        "$HPM_SDK_BASE" "$GNURISCV_TOOLCHAIN_PATH" "$BOARD_SEARCH_PATH" "$HPM_APP_DRIVERS" "$HPM_PYTHON"
    printf 'To apply these variables to your terminal: source scripts/enter_hpm_env.sh\n'
fi
