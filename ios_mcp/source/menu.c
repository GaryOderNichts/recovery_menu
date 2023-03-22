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
#include "socket.h"
#include "netconf.h"
#include "mcp_misc.h"

#include <string.h>
#include <unistd.h>

#include "options/options.h"

static void option_Shutdown(void);

extern int ppcHeartBeatThreadId;

int fsaHandle = -1;

static const Menu mainMenuOptions[] = {
    {"Set Coldboot Title",          {.callback = option_SetColdbootTitle}},
    {"Dump Syslogs",                {.callback = option_DumpSyslogs}},
    {"Dump OTP + SEEPROM",          {.callback = option_DumpOtpAndSeeprom}},
    {"Start wupserver",             {.callback = option_StartWupserver}},
    {"Load Network Configuration",  {.callback = option_LoadNetConf}},
    {"Pair Gamepad",                {.callback = option_PairDRC}},
    {"Install WUP",                 {.callback = option_InstallWUP}},
    {"Edit Parental Controls",      {.callback = option_EditParental}},
    {"Debug System Region",         {.callback = option_DebugSystemRegion}},
    {"System Information",          {.callback = option_SystemInformation}},
    {"Submit System Data",          {.callback = option_SubmitSystemData}},
    {"Shutdown",                    {.callback = option_Shutdown}},
};

/**
 * Draw the top bar.
 * @param title Title
 */
void drawTopBar(const char* title)
{
    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_print((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
}

static void drawBars(const char* title)
{
    drawTopBar(title);

    // draw bottom bar
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
    gfx_print(16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 0, "EJECT: Navigate");
    gfx_print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, GfxPrintFlag_AlignRight, "POWER: Choose");
}

/**
 * Draw a single menu item. Called by drawMenu().
 * @param menuItem Menu item
 * @param selected If non-zero, item is selected
 * @param flags
 * @param x
 * @param y
 */
static void drawMenuItem(const Menu* menuItem, int selected, uint32_t flags, uint32_t x, uint32_t y)
{
    const char *text;
    char buf[64];
    if (!(flags & MenuFlag_ShowTID)) {
        text = menuItem->name;
    } else {
        if (menuItem->tid != 0) {
            snprintf(buf, sizeof(buf), "%s - %08lx-%08lx",
                menuItem->name, (uint32_t)(menuItem->tid >> 32), (uint32_t)(menuItem->tid & 0xFFFFFFFF));
            text = buf;
        } else {
            text = menuItem->name;
        }
    }

    gfx_draw_rect_filled(x - 1, y - 1,
        gfx_get_text_width(text) + 2, CHAR_SIZE_DRC_Y + 2,
        selected ? COLOR_PRIMARY : COLOR_BACKGROUND);

    gfx_set_font_color(selected ? COLOR_BACKGROUND : COLOR_PRIMARY);
    gfx_print(x, y, 0, text);
}

/**
 * Draw a menu and wait for the user to select an option.
 * @param title Menu title
 * @param menu Array of menu entries
 * @param count Number of menu entries
 * @param selected Initial selected item index
 * @param flags
 * @param x
 * @param y
 * @return Selected menu entry index
 */
int drawMenu(const char* title, const Menu* menu, size_t count,
        int selected, uint32_t flags, uint32_t x, uint32_t y)
{
    int redraw = 1;
    int prev_selected = -1;
    if (selected < 0 || selected >= count)
        selected = 0;

    // draw the full menu
    if (!(flags & MenuFlag_NoClearScreen)) {
        gfx_clear(COLOR_BACKGROUND);
    }
    int index = y;
    for (int i = 0; i < count; i++) {
        drawMenuItem(&menu[i], selected == i, flags, x, index);
        index += CHAR_SIZE_DRC_Y + 4;
    }

    if (flags & MenuFlag_ShowGitHubLink) {
        static const int ypos = SCREEN_HEIGHT - (CHAR_SIZE_DRC_Y * 3);
        gfx_set_font_color(COLOR_PRIMARY);
        static const char link_prefix[] = "Check out the GitHub repository at:";
        gfx_print(16, ypos, 0, link_prefix);
        static const int xpos = 16 + CHAR_SIZE_DRC_X * sizeof(link_prefix);
        gfx_set_font_color(COLOR_LINK);
        gfx_print(xpos, ypos, GfxPrintFlag_Underline, "https://github.com/GaryOderNichts/recovery_menu");
    }

    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        SMC_ReadSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if (flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) {
                setNotificationLED(NOTIF_LED_OFF, 250);
                prev_selected = selected;
                selected++;
                if (selected == count)
                    selected = 0;
                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
                setNotificationLED(NOTIF_LED_OFF, 250);
                return selected;
            }
            cur_flag = flag;
        }

        if (redraw) {
            if (prev_selected != selected) {
                // Redraw the previously selected menu item.
                if (prev_selected >= 0) {
                    index = y + ((CHAR_SIZE_DRC_Y + 4) * prev_selected);
                    drawMenuItem(&menu[prev_selected], 0, flags, x, index);
                }

                // Redraw the selected item.
                index = y + ((CHAR_SIZE_DRC_Y + 4) * selected);
                drawMenuItem(&menu[selected], 1, flags, x, index);
            }

            gfx_set_font_color(COLOR_PRIMARY);
            drawBars(title);
            redraw = 0;
        }
    }
}

/**
 * Wait for the user to press a button.
 */
void waitButtonInput(void)
{
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    gfx_draw_rect_filled(16 - 1, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4,
        SCREEN_WIDTH - 16, CHAR_SIZE_DRC_Y + 2,
        COLOR_BACKGROUND);
    gfx_print(16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 0, "Press EJECT or POWER to proceed...");

    uint8_t cur_flag = 0;
    uint8_t flag = 0;

    while (1) {
        SMC_ReadSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if ((flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) || (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON)) {
                setNotificationLED(NOTIF_LED_OFF, 250);
                return;
            }

            cur_flag = flag;
        }
    }
}

/**
 * Initialize the network configuration.
 * @param index [in/out] Starting (and ending) Y position.
 * @return 0 on success; non-zero on error.
 */
int initNetconf(uint32_t* index)
{
    gfx_print(16, *index, 0, "Initializing netconf...");
    *index += CHAR_SIZE_DRC_Y;

    int res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, *index, 0, "Failed to initialize netconf: %x", res);
        waitButtonInput();
        return res;
    }

    gfx_printf(16, *index, 0, "Waiting for network connection... %ds", 5);

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
        gfx_printf(16, *index, GfxPrintFlag_ClearBG, "Waiting for network connection... %ds", 4 - i);
    }

    *index += CHAR_SIZE_DRC_Y;

    if (interface == 0xff) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, *index, 0, "No network connection!");
        waitButtonInput();
        return 1;
    }

    gfx_printf(16, *index, 0, "Connected using %s", (interface == NET_CFG_INTERFACE_TYPE_WIFI) ? "WIFI" : "ETHERNET");
    *index += CHAR_SIZE_DRC_Y;

    uint8_t ip_address[4];
    res = netconf_get_assigned_address(interface, (uint32_t*) ip_address);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, *index, 0, "Failed to get IP address: %x", res);
        waitButtonInput();
        return res;
    }

    gfx_printf(16, *index, 0, "IP address: %u.%u.%u.%u",
        ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    *index += CHAR_SIZE_DRC_Y;

    res = socketInit();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, *index, 0, "Failed to initialize socketlib: %x", res);
        waitButtonInput();
        return res;
    }

    return 0;
}

/**
 * Get region code information.
 * @param productArea_id Product area ID: 0-6
 * @param gameRegion Bitfield of game regions
 * @return 0 on success; negative on error.
 */
int getRegionInfo(int* productArea_id, int* gameRegion)
{
    MCPSysProdSettings sysProdSettings;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0)
        return mcpHandle;

    int res = MCP_GetSysProdSettings(mcpHandle, &sysProdSettings);
    IOS_Close(mcpHandle);
    if (res < 0)
        return res;

    // productArea is a single region.
    if (sysProdSettings.product_area == 0)
        return -1;

    if (productArea_id) {
        *productArea_id = __builtin_ctz(sysProdSettings.product_area);
    }
    if (gameRegion) {
        *gameRegion = sysProdSettings.game_region;
    }
    return 0;
}

static void option_Shutdown(void)
{
    if (fsaHandle > 0) {
        // flush mlc and slc before forcing shutdown
        FSA_FlushVolume(fsaHandle, "/vol/storage_mlc01");
        FSA_FlushVolume(fsaHandle, "/vol/system");

        // unmount sd
        FSA_Unmount(fsaHandle, "/vol/storage_recovsd", 2);

        IOS_Close(fsaHandle);
    }

    // Finalize utils (doesn't really matter at this point since we don't have a clean shutdown anyways)
    //finalizeUtils();

    IOS_Shutdown(0);
}

int menuThread(void* arg)
{
    printf("menuThread running\n");

    // set LED to purple-orange blinking
    setNotificationLED(NOTIF_LED_PURPLE_BLINKING | NOTIF_LED_ORANGE, 0);

    // stop ppcHeartbeatThread and reset PPC
    IOS_CancelThread(ppcHeartBeatThreadId, 0);
    resetPPC();

    // cut power to the disc drive to not eject a disc every eject press
    SMC_SetODDPower(0);

    // Initialize utils
    initializeUtils();

#ifdef DC_INIT
    // (re-)init the graphics subsystem
    GFX_SubsystemInit(0);

    /* Note: CONFIGURATION_0 is 720p instead of 480p,
       but doesn't shut down the GPU properly? The
       GamePad just stays connected after calling iOS_Shutdown.
       To be safe, let's use CONFIGURATION_1 for now */
    DISPLAY_DCInit(DC_CONFIGURATION_1);

    /* Note about the display configuration struct:
       The returned framebuffer address seems to be AV out only?
       Writing to the hardcoded addresses in gfx.c works for HDMI though */
    DC_Config dc_config;
    DISPLAY_ReadDCConfig(&dc_config);
#endif

    // initialize the font
    if (gfx_init_font() != 0) {
        // failed to initialize font
        // can't do anything without a font, so sleep for 5 secs and shut down
        usleep(1000 * 1000 * 5);
        IOS_Shutdown(0);
    }

    // open fsa and mount sdcard
    fsaHandle = IOS_Open("/dev/fsa", 0);
    if (fsaHandle > 0) {
        int res = FSA_Mount(fsaHandle, "/dev/sdcard01", "/vol/storage_recovsd", 2, NULL, 0);
        if (res < 0) {
            printf("Failed to mount SD: %x\n", res);
        }
    } else {
        printf("Failed to open FSA: %x\n", fsaHandle);
    }

    // set LED to purple
    setNotificationLED(NOTIF_LED_PURPLE, 0);

    int selected = 0;
    while (1) {
        selected = drawMenu("Wii U Recovery Menu v" VERSION_STRING " by GaryOderNichts",
            mainMenuOptions, ARRAY_SIZE(mainMenuOptions), selected,
            MenuFlag_ShowGitHubLink, 16, 16+8+2+8);
        if (selected >= 0 && selected < ARRAY_SIZE(mainMenuOptions)) {
            mainMenuOptions[selected].callback();
        }
    }

    return 0;
}
