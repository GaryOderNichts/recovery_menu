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

#include "DebugSystemRegion.h"

#include "fsa.h"
#include "gfx.h"
#include "mcp_misc.h"
#include "menu.h"
#include "utils.h"

#include <stdint.h>

void option_DebugSystemRegion(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Debug System Region");

    uint32_t index = 16 + 8 + 2 + 8;

    // Get the system region code, then check if a matching
    // Wii U Menu is installed.
    int productArea_id, gameRegion;
    int res = getRegionInfo(&productArea_id, &gameRegion);
    if (res < 0) {
        printf_error(index, "Failed to get the system region code: %x", res);
        return;
    }

    // productArea
    gfx_set_font_color(COLOR_PRIMARY);
    if (productArea_id >= 0 && productArea_id < ARRAY_SIZE(region_tbl)) {
        gfx_printf(16, index, 0, "System region code:   %s", region_tbl[productArea_id]);
    } else {
        gfx_printf(16, index, 0, "System region code:   %d", productArea_id);
    }
    index += CHAR_SIZE_DRC_Y + 4;

    // gameRegion
    gfx_printf(16, index, 0, "Game region code:     %s %s %s %s %s %s",
        (gameRegion & MCP_REGION_JAPAN)  ? region_tbl[0] : "---",
        (gameRegion & MCP_REGION_USA)    ? region_tbl[1] : "---",
        (gameRegion & MCP_REGION_EUROPE) ? region_tbl[2] : "---",
        (gameRegion & MCP_REGION_CHINA)  ? region_tbl[4] : "---",
        (gameRegion & MCP_REGION_KOREA)  ? region_tbl[5] : "---",
        (gameRegion & MCP_REGION_TAIWAN) ? region_tbl[6] : "---");
    index += ((CHAR_SIZE_DRC_Y + 4) * 2);

    // Wii U Menu path ('x' is at path[43])
    char path[] = "/vol/storage_mlc01/sys/title/00050010/10040x00/code/app.xml";

    // Check if Wii U Menu for this region exists.
    int menu_matches_region = 0;
    int menu_is_in_gameRegion = 0;
    int menu_productArea_id = -1;
    int menu_count = 0;
    int fileHandle;

    path[43] = productArea_id + '0';
    res = FSA_OpenFile(fsaHandle, path, "r", &fileHandle);
    if (res >= 0) {
        menu_matches_region = 1;
        menu_productArea_id = productArea_id;
        menu_count = 1; // TODO: Check for others anyway?
        FSA_CloseFile(fsaHandle, fileHandle);
    }

    if (!menu_matches_region) {
        // Check if another Wii U Menu is installed.
        for (int i = 0; i < ARRAY_SIZE(region_tbl); i++) {
            if (i == productArea_id)
                continue;

            path[43] = '0' + i;
            res = FSA_OpenFile(fsaHandle, path, "r", &fileHandle);
            if (res >= 0) {
                menu_count++;
                menu_productArea_id = i;
                FSA_CloseFile(fsaHandle, fileHandle);
            }
        }
    }

    // Is the menu region in gameRegion?
    menu_is_in_gameRegion = (gameRegion & (1U << menu_productArea_id));

    static const char inst_menu[] = "Installed Wii U Menu:";
    gfx_print(16, index, 0, inst_menu);
    const char* menu_region_str;
    if (menu_count == 0 || menu_productArea_id < 0) {
        menu_region_str = "NONE";
        gfx_set_font_color(COLOR_ERROR);
    } else if (menu_count > 1) {
        menu_region_str = "MANY";
        gfx_set_font_color(COLOR_ERROR);
    } else {
        menu_region_str = region_tbl[menu_productArea_id];
        gfx_set_font_color(COLOR_SUCCESS);
    }

    gfx_print(16 + (CHAR_SIZE_DRC_X * sizeof(inst_menu)), index, 0, menu_region_str);
    index += CHAR_SIZE_DRC_Y + 4;

    if (menu_matches_region && menu_is_in_gameRegion) {
        gfx_set_font_color(COLOR_SUCCESS);
        gfx_print(16, index, 0, "The system region appears to be set correctly.");
        waitButtonInput();
        return;
    }

    // Show the errors.
    if (menu_count == 0 || menu_productArea_id < 0) {
        print_error(index, "Could not find a Wii U Menu title installed on this system.");
        return;
    }

    if (menu_count > 1) {
        print_error(index, "Multiple Wii U Menus were found. Someone dun goofed...");
        return;
    }

    gfx_set_font_color(COLOR_ERROR);
    if (!menu_matches_region) {
        gfx_printf(16, index, 0, "The %s region does not match the installed Wii U Menu.", "system");
        index += CHAR_SIZE_DRC_Y + 4;
    }
    if (!menu_is_in_gameRegion) {
        gfx_printf(16, index, 0, "The %s region does not match the installed Wii U Menu.", "game");
        index += CHAR_SIZE_DRC_Y + 4;
    }
    index += CHAR_SIZE_DRC_Y + 4;

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_printf(16, index, 0, "Repair the system by setting the region code to %s?", menu_region_str);
    index += CHAR_SIZE_DRC_Y + 4;

    static const Menu fixRegionBrickOptions[] = {
        {"Cancel", {0} },
        {"Fix Region", {0} },
    };
    int selected = drawMenu("Debug System Region",
        fixRegionBrickOptions, ARRAY_SIZE(fixRegionBrickOptions), 0,
        MenuFlag_NoClearScreen, 16, index);
    if (selected == 0)
        return;
    index += (CHAR_SIZE_DRC_Y*(ARRAY_SIZE(fixRegionBrickOptions)+1)) + 4;

    // Attempt to set the region code.
    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        printf_error(index, "IOS_Open(\"/dev/mcp\") failed: %x", mcpHandle);
        return;
    }

    MCPSysProdSettings sysProdSettings;
    res = MCP_GetSysProdSettings(mcpHandle, &sysProdSettings);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "MCP_GetSysProdSettings() failed: %x", res);
        return;
    }

    // Set both productArea and gameRegion to the Wii U Menu's region value.
    // NOTE: productArea_id is a bit index, so it needs to be shifted into place.
    sysProdSettings.product_area = (1U << menu_productArea_id);
    sysProdSettings.game_region = (1U << menu_productArea_id);
    res = MCP_SetSysProdSettings(mcpHandle, &sysProdSettings);
    IOS_Close(mcpHandle);
    if (res < 0) {
        printf_error(index, "MCP_SetSysProdSettings() failed: %x", res);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_printf(16, index, 0, "System region set to %s successfully.", menu_region_str);
    waitButtonInput();
}
