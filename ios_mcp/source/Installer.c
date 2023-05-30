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

#include "menu.h"
#include "imports.h"
#include "gfx.h"
#include "utils.h"
#include "fsa.h"
#include "mcp_install.h"
#include "mcp_misc.h"

#include <string.h>
#include <unistd.h>

static int install_title(const char *path, uint64_t tid, int mcpHandle, int index)
{
    gfx_printf(16, index, GfxPrintFlag_ClearBG, "Installing title: %08lx-%08lx...",
        (uint32_t)(tid >> 32), (uint32_t)(tid & 0xFFFFFFFFU));

    gfx_draw_rect_filled(16, index + (CHAR_SIZE_DRC_Y + 4), strnlen("Installing title: 00000000-00000000... Done!", 64) * CHAR_SIZE_DRC_X, CHAR_SIZE_DRC_Y, COLOR_BACKGROUND);

    int x = 16 + (strnlen("Installing title: 00000000-00000000... ", 64) * CHAR_SIZE_DRC_X);

    // only install to NAND
    int res = MCP_InstallSetTargetDevice(mcpHandle, 0);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(x, index, 0, "MCP_InstallSetTargetDevice: %x", res);
        return 1;
    }
    res = MCP_InstallSetTargetUsb(mcpHandle, 0);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(x, index, 0, "MCP_InstallSetTargetUsb: %x", res);
        return 1;
    }

    // TODO: async installations
    res = MCP_InstallTitle(mcpHandle, path);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(x, index, 0, "Failed: %x", res);
        return 1;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(x, index, 0, "Done!");
    gfx_set_font_color(COLOR_PRIMARY);
    return 0;
}

static void try_batch_install(int mcpHandle)
{
    uint32_t index = 16 + 8 + 2 + 8;
    int dir_handle;
    char path[1024] = "/vol/storage_recovsd/install/";
    int res = FSA_OpenDir(fsaHandle, path, &dir_handle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Failed to open install folder: %x", res);
        return;
    }

    FSDirectoryEntry dir_entry;
    size_t maxPathSize = strnlen(path, 64);
    char *inPath = path + maxPathSize;
    maxPathSize = 1023 - maxPathSize;
    MCPInstallInfo info;
    int ret;
    int i = 0;
    int j = 0;

    while (FSA_ReadDir(fsaHandle, dir_handle, &dir_entry) >= 0) {
        if (!(dir_entry.stat.flags & DIR_ENTRY_IS_DIRECTORY)) {
            continue;
        }

        strncpy(inPath, dir_entry.name, maxPathSize);
        ret = MCP_InstallGetInfo(mcpHandle, path, &info);
        if (ret < 0) {
            continue;
        }

        ret = install_title(path, info.titleId, mcpHandle, index);
        ++i;
        if (ret) {
            break;
        }

        if (j < 20) {
            ++j;
            index += CHAR_SIZE_DRC_Y + 4;
        } else {
            j = 0;
            index = 16 + 8 + 2 + 8;
        }
    }

    if (!i) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, GfxPrintFlag_ClearBG, "No installable titles found!");
    }
}

void option_InstallWUP(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("WUP Installer");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Inititalising, please wait...");

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        // TODO: Remove previous line
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open /dev/mcp: %x", mcpHandle);
        waitButtonInput();
        return;
    }

    MCPInstallInfo info;
    int res = MCP_InstallGetInfo(mcpHandle, "/vol/storage_recovsd/install", &info);
    res < 0 ? try_batch_install(mcpHandle) : install_title("/vol/storage_recovsd/install", info.titleId, mcpHandle, index);

    IOS_Close(mcpHandle);
    waitButtonInput();
}
