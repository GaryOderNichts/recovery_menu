#include "InstallWUP.h"

#include "menu.h"
#include "gfx.h"
#include "mcp_install.h"
#include "imports.h"
#include "utils.h"

void option_InstallWUP(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Installing WUP");
    setNotificationLED(NOTIF_LED_RED_BLINKING, 0);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Make sure to place a valid signed WUP directly in 'sd:/install'");
    index += CHAR_SIZE_DRC_Y + 4;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        printf_error(index, "Failed to open /dev/mcp: %x", mcpHandle);
        return;
    }

    gfx_print(16, index, 0, "Querying install info...");
    index += CHAR_SIZE_DRC_Y + 4;

    MCPInstallInfo info;
    int res = MCP_InstallGetInfo(mcpHandle, "/vol/storage_recovsd/install", &info);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "Failed to get install info: %x", res);
        return;
    }

    gfx_printf(16, index, 0, "Installing title: %08lx-%08lx...",
        (uint32_t)(info.titleId >> 32), (uint32_t)(info.titleId & 0xFFFFFFFFU));
    index += CHAR_SIZE_DRC_Y + 4;

    // only install to NAND
    res = MCP_InstallSetTargetDevice(mcpHandle, 0);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "MCP_InstallSetTargetDevice: %x", res);
        return;
    }
    res = MCP_InstallSetTargetUsb(mcpHandle, 0);
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "MCP_InstallSetTargetUsb: %x", res);
        return;
    }

    // TODO: async installations
    res = MCP_InstallTitle(mcpHandle, "/vol/storage_recovsd/install");
    if (res < 0) {
        IOS_Close(mcpHandle);
        printf_error(index, "Failed to install: %x", res);
        return;
    }

    setNotificationLED(NOTIF_LED_PURPLE, 0);
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    IOS_Close(mcpHandle);
}
