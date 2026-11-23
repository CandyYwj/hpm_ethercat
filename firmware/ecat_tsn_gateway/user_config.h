/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Author: BearKing
 */

/* EEPROM emulation component configuration */

#ifndef _USER_CONFIG_H
#define _USER_CONFIG_H

#include "ecat_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define E2P_DEBUG_LEVEL        E2P_DEBUG_LEVEL_WARN
#define EEPROM_MAX_VAR_CNT     (ESC_EEPROM_SIZE / EEPROM_WRITE_SIZE)

#ifdef __cplusplus
}
#endif

#endif
