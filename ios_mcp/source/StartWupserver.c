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
    index = initNetconf(index);
    if (index == 0) {
        // An error occurred while initializing netconf.
        waitButtonInput();
        return;
    }

    wupserver_init();

    index += 4;
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Wupserver running. Press EJECT or POWER to stop.");
    index += CHAR_SIZE_DRC_Y + 4;

    waitButtonInput();

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_print(16, index, 0, "Stopping wupserver...");
    index += CHAR_SIZE_DRC_Y + 4;

    wupserver_deinit();
}
