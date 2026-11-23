# Copyright (c) 2024 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

# Language servers use this database for exact include paths, definitions, and
# compiler flags. Ninja and Makefile generators write it into the build tree.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CONFIG_EEPROM_EMULATION 1) # using eeprom emulation component
set(CONFIG_FREERTOS 1)
set(CONFIG_FREERTOS_TIMER_RESOURCE_GPTMR 1)

# Set the firmware name here; build/flash/debug tools resolve the output below.
set(APP_NAME ss)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(ecat_tsn_gateway)

# Export the exact target path. Consumers must not guess a name or pick an old ELF.
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/firmware-elf-path.txt"
    CONTENT "$<TARGET_FILE:${APP_ELF_NAME}>\n")

# Match the HPM SDK EtherCAT + FreeRTOS interrupt model. The existing SSC
# hardware layer keeps GPTMR0; FreeRTOS uses the board's GPTMR6 resource.
sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=0)
sdk_compile_definitions(-DUSE_SYSCALL_INTERRUPT_PRIORITY=1)

option(GATEWAY_PERFORMANCE_BUILD "Build gateway with O2 while keeping debug symbols" OFF)
if(GATEWAY_PERFORMANCE_BUILD)
    sdk_compile_options(-O2 -g)
    sdk_compile_definitions(-DGATEWAY_PERFORMANCE_BUILD=1)
endif()

# ---- SSC source files ----
sdk_inc(SSC/Src)
sdk_src(SSC/Src/coeappl.c)
sdk_src(SSC/Src/ecatappl.c)
sdk_src(SSC/Src/ecatcoe.c)
sdk_src(SSC/Src/ecatslv.c)
sdk_src(SSC/Src/mailbox.c)
sdk_src(SSC/Src/objdef.c)
sdk_src(SSC/Src/sdoserv.c)
# NOTE: SSC/Src/digital_io.c is NOT compiled; app/gateway_app.c replaces it.

# ---- Path to hpm_app drivers ----
set(HPM_APP_DRIVERS "" CACHE PATH "Path to hpm_app/software/drivers")
if(NOT HPM_APP_DRIVERS)
    if(NOT BOARD_SEARCH_PATH)
        message(FATAL_ERROR "Set BOARD_SEARCH_PATH or HPM_APP_DRIVERS")
    endif()
    get_filename_component(HPM_APP_ROOT "${BOARD_SEARCH_PATH}" DIRECTORY)
    set(HPM_APP_DRIVERS "${HPM_APP_ROOT}/software/drivers")
endif()
if(NOT EXISTS "${HPM_APP_DRIVERS}/inc/full_port_config_drv.h")
    message(FATAL_ERROR "Invalid HPM_APP_DRIVERS: ${HPM_APP_DRIVERS}")
endif()

# ---- ESC hardware abstraction (from hpm_app drivers) ----
sdk_inc(${HPM_APP_DRIVERS}/src/full_ecat_port)
sdk_src(${HPM_APP_DRIVERS}/src/full_ecat_port/hpm_ecat_hw.c)
sdk_src(${HPM_APP_DRIVERS}/src/full_ecat_port/hpm_ecat_phy.c)

if(CONFIG_EEPROM_EMULATION)
    sdk_src(${HPM_APP_DRIVERS}/src/full_ecat_port/hpm_ecat_e2p_emulation.c)
endif()

# ---- Full-port drivers ----
sdk_inc(${HPM_APP_DRIVERS}/inc)
sdk_inc(${HPM_APP_DRIVERS}/inc/tsn)
sdk_app_src(${HPM_APP_DRIVERS}/src/full_port_led_drv.c)
sdk_app_src(${HPM_APP_DRIVERS}/src/full_port_lcd_drv.c)
sdk_app_src(${HPM_APP_DRIVERS}/src/full_port_log_drv.c)

# ---- Gateway modules ----
sdk_inc(./)
sdk_inc(app)
sdk_inc(tasks)
sdk_inc(drivers/can)
sdk_inc(drivers/imu)
sdk_inc(drivers/tsn)
sdk_inc(protocols/robstride)
sdk_inc(protocols/yesense)
sdk_inc(motor)
sdk_src(app/gateway_app.c)
sdk_src(app/gateway_system.c)
sdk_src(tasks/gateway_tasks.c)
sdk_src(tasks/gateway_perf.c)
sdk_src(tasks/gateway_ethercat_task.c)
sdk_src(tasks/gateway_motor_task.c)
sdk_src(tasks/gateway_imu_task.c)
sdk_src(tasks/gateway_status_task.c)
sdk_src(drivers/can/can_if_hpm_mcan.c)
sdk_src(drivers/tsn/tsn_switch.c)
sdk_src(protocols/robstride/robstride_proto.c)
sdk_src(motor/motor_config.c)
sdk_src(motor/motor_manager.c)
sdk_src(protocols/yesense/yesense_parser.c)
sdk_src(protocols/yesense/yesense_query.c)
sdk_src(drivers/imu/imu.c)

# ---- Gateway sources ----
sdk_app_src(main.c)

generate_ide_projects()
