/*
 * gateway_system.c - Board, EtherCAT, CAN, motor, IMU, and TSN switch setup.
 * Author: BearKing
 */

#include "gateway_system.h"

#include "SSC/Src/applInterface.h"
#include "board.h"
#include "can_if.h"
#include "gateway_app.h"
#include "hpm_ecat_hw.h"
#include "imu.h"
#include "motor_manager.h"
#include "tsn_switch.h"

#include <stdio.h>
#include <string.h>

#if defined(ESC_EEPROM_EMULATION) && ESC_EEPROM_EMULATION
#include "hpm_ecat_e2p_emulation.h"
#include "hpm_ppor_drv.h"
extern unsigned char aEepromData[];
#endif

/* Custom full-port board hooks are implemented in hpm_app/boards. */
void board_init_ethercat(ESC_Type *ptr);
void board_init_switch_led(void);

#if defined(ESC_EEPROM_EMULATION) && ESC_EEPROM_EMULATION && \
    defined(ECAT_SYNC_EEPROM_IMAGE_ON_BOOT) && ECAT_SYNC_EEPROM_IMAGE_ON_BOOT
static BOOL gateway_eeprom_image_matches(void)
{
    uint8_t word_data[EEPROM_WRITE_SIZE];
    uint32_t word_index;

    for (word_index = 0U; word_index < (ESC_EEPROM_SIZE / EEPROM_WRITE_SIZE); word_index++) {
        if (ecat_flash_eeprom_read(word_index, word_data) != status_success) {
            return FALSE;
        }

        if (memcmp(word_data,
                   &aEepromData[word_index * EEPROM_WRITE_SIZE],
                   EEPROM_WRITE_SIZE) != 0) {
            return FALSE;
        }
    }

    return TRUE;
}

static void gateway_refresh_eeprom_image(void)
{
    uint32_t word_index;

    printf("EEPROM image mismatch, refreshing flash mirror...\r\n");
    ecat_flash_eeprom_format();

    for (word_index = 0U;
         word_index < (ESC_EEPROM_SIZE / EEPROM_WRITE_SIZE);
         word_index++) {
        if (ecat_flash_eeprom_write(word_index,
                                    &aEepromData[word_index * EEPROM_WRITE_SIZE])
            != status_success) {
            printf("EEPROM refresh FAILED at word %lu\r\n", (unsigned long)word_index);
            while (1) {
            }
        }
    }

    ecat_flash_eeprom_flush();
    printf("EEPROM image refreshed, resetting MCU...\r\n");
    ppor_sw_reset(HPM_PPOR, 2);

    while (1) {
    }
}
#endif

static void validate_pdo_mapping(void)
{
    UINT16 input_size_bytes;
    UINT16 output_size_bytes;

    APPL_GenerateMapping(&input_size_bytes, &output_size_bytes);
    printf("PDO: %u bytes in, %u bytes out\r\n",
           (unsigned int)input_size_bytes,
           (unsigned int)output_size_bytes);
    if ((input_size_bytes != GATEWAY_TXPDO_INPUT_BYTES)
        || (output_size_bytes != MIT_MOTOR_COMMAND_BYTES)) {
        printf("PDO size WARNING: expected %u bytes in, %u bytes out\r\n",
               (unsigned int)GATEWAY_TXPDO_INPUT_BYTES,
               (unsigned int)MIT_MOTOR_COMMAND_BYTES);
    }
}

bool gateway_system_init(void)
{
    uint16_t ecat_init_status;

    board_init_ethercat(HPM_ESC);
    board_init_switch_led();

    HPM_ESC->PHY_CFG0 = (HPM_ESC->PHY_CFG0 & ~ESC_PHY_CFG0_PHY_OFFSET_VAL_MASK)
                       | ESC_PHY_CFG0_PHY_OFFSET_VAL_SET(1);

    if (!tsn_switch_init()) {
        printf("TSN switch init FAILED\r\n");
        return false;
    }

    ecat_init_status = ecat_hardware_init(HPM_ESC);
    if (ecat_init_status != 0U) {
        printf("ESC hardware init FAILED: %u\r\n", (unsigned int)ecat_init_status);
        return false;
    }

#if defined(ESC_EEPROM_EMULATION) && ESC_EEPROM_EMULATION && \
    defined(ECAT_SYNC_EEPROM_IMAGE_ON_BOOT) && ECAT_SYNC_EEPROM_IMAGE_ON_BOOT
    if (!gateway_eeprom_image_matches()) {
        gateway_refresh_eeprom_image();
    }
#endif

    printf("ESC hardware OK\r\n");
    MainInit();

#if defined(ESC_EEPROM_EMULATION) && ESC_EEPROM_EMULATION
    pAPPL_EEPROM_Read = ecat_eeprom_emulation_read;
    pAPPL_EEPROM_Write = ecat_eeprom_emulation_write;
    pAPPL_EEPROM_Reload = ecat_eeprom_emulation_reload;
    pAPPL_EEPROM_Store = ecat_eeprom_emulation_store;
#endif

    validate_pdo_mapping();

    if (!can_if_init()) {
        printf("CAN init FAILED: no MCAN bus initialized\r\n");
        return false;
    }
    printf("CAN init OK: initMask=0x%02lX, Classic CAN extended frame %lubps\r\n",
           can_if_get_initialized_mask(),
           (unsigned long)CAN_IF_DEFAULT_BITRATE_BPS);

    if (!motor_manager_init()) {
        printf("motor manager init FAILED\r\n");
        return false;
    }

    if (!imu_init()) {
        printf("IMU init FAILED: UART12/HDMA channel 30\r\n");
        return false;
    }
    printf("IMU init OK: UART12 PF16/PF17, 460800 8N1, RX DMA\r\n");

    return true;
}
