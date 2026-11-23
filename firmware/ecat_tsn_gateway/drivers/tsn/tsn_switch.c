/*
 * tsn_switch.c - Standalone three-port TSN hardware switch.
 * Author: BearKing
 */

#include "tsn_switch.h"

#include "board.h"
#include "hpm_tsw_drv.h"
#include <stdio.h>

#define TSN_SWITCH_EXTERNAL_PORT_MASK \
    (tsw_dst_port_1 | tsw_dst_port_2 | tsw_dst_port_3)
#define TSN_SWITCH_RGMII_TX_DELAY  0x8U
#define TSN_SWITCH_MDIO_CLOCK_DIV  19U

static bool configure_external_port(uint8_t port, uint8_t mac_byte)
{
    uint8_t mac_address[6] = {
        mac_byte, mac_byte, mac_byte, mac_byte, mac_byte, mac_byte,
    };

    tsw_port_gpr(BOARD_TSN,
                 port,
                 tsw_port_speed_1000mbps,
                 tsw_port_phy_itf_rgmii,
                 TSN_SWITCH_RGMII_TX_DELAY,
                 0U);

    return (tsw_ep_disable_mac_ctrl(BOARD_TSN, port, TSW_RXFIFO_E1) == status_success)
        && (tsw_ep_set_mac_addr(BOARD_TSN, port, mac_address, true) == status_success)
        && (tsw_ep_set_mac_mode(BOARD_TSN, port, tsw_mac_mode_gmii) == status_success)
        && (tsw_ep_set_mdio_config(BOARD_TSN, port, TSN_SWITCH_MDIO_CLOCK_DIV)
            == status_success)
        && (tsw_ep_enable_mac_ctrl(BOARD_TSN, port, TSW_RXFIFO_E1) == status_success);
}

static void configure_frame_forwarding(void)
{
    /*
     * These SDK setters OR destination bits. Clear the masks first so traffic
     * is never copied to the CPU port by configuration left from another mode.
     */
    tsw_set_broadcast_frame_action(BOARD_TSN, tsw_dst_port_null);
    tsw_set_unknown_frame_action(BOARD_TSN, tsw_dst_port_null);
    tsw_set_broadcast_frame_action(BOARD_TSN, TSN_SWITCH_EXTERNAL_PORT_MASK);
    tsw_set_unknown_frame_action(BOARD_TSN, TSN_SWITCH_EXTERNAL_PORT_MASK);
}

bool tsn_switch_init(void)
{
    board_init_tsw();
    board_tsw_phy_set(TSW_TSNPORT_PORT1, false);
    board_tsw_phy_set(TSW_TSNPORT_PORT2, false);
    board_tsw_phy_set(TSW_TSNPORT_PORT3, false);

    tsw_clear_cam(BOARD_TSN);
    board_delay_ms(1000U);
    tsw_set_cam_vlan_port(BOARD_TSN);

    if (!configure_external_port(TSW_TSNPORT_PORT1, 0x11U)
        || !configure_external_port(TSW_TSNPORT_PORT2, 0x22U)
        || !configure_external_port(TSW_TSNPORT_PORT3, 0x33U)) {
        return false;
    }

    configure_frame_forwarding();

    board_tsw_phy_set(TSW_TSNPORT_PORT1, true);
    board_tsw_phy_set(TSW_TSNPORT_PORT2, true);
    board_tsw_phy_set(TSW_TSNPORT_PORT3, true);

    printf("TSN switch enabled: ports 1/2/3, hardware forwarding, CPU port excluded\r\n");
    return true;
}
