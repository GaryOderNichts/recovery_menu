/*
 *   Copyright (C) 2022 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "StartWupserver.h"

#include "gfx.h"
#include "menu.h"
#include "netconf.h"
#include "socket.h"
#include "wupserver.h"

#include <stdint.h>
#include <unistd.h>

void option_StartWupserver(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Running wupserver...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Initializing netconf...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize netconf: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Waiting for network connection... %ds", 5);

    NetConfInterfaceTypeEnum interface = 0xff;
    for (int i = 0; i < 5; i++) {
        if (netconf_get_if_linkstate(NET_CFG_INTERFACE_TYPE_WIFI) == NET_CFG_LINK_STATE_UP) {
            interface = NET_CFG_INTERFACE_TYPE_WIFI;
            break;
        }

        if (netconf_get_if_linkstate(NET_CFG_INTERFACE_TYPE_ETHERNET) == NET_CFG_LINK_STATE_UP) {
            interface = NET_CFG_INTERFACE_TYPE_ETHERNET;
            break;
        }

        usleep(1000 * 1000);
        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Waiting for network connection... %ds", 4 - i);
    }

    index += CHAR_SIZE_DRC_Y + 4;

    if (interface == 0xff) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "No network connection!");
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Connected using %s", (interface == NET_CFG_INTERFACE_TYPE_WIFI) ? "WIFI" : "ETHERNET");
    index += CHAR_SIZE_DRC_Y + 4;

    uint8_t ip_address[4];
    res = netconf_get_assigned_address(interface, (uint32_t*) ip_address);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get IP address: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "IP address: %u.%u.%u.%u",
        ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    index += CHAR_SIZE_DRC_Y + 4;

    res = socketInit();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize socketlib: %x", res);
        waitButtonInput();
        return;
    }

    wupserver_init();

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Wupserver running. Press EJECT or POWER to stop.");
    index += CHAR_SIZE_DRC_Y + 4;

    waitButtonInput();

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_print(16, index, 0, "Stopping wupserver...");
    index += CHAR_SIZE_DRC_Y + 4;

    wupserver_deinit();
}
