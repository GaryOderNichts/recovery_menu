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
#include "wupserver.h"
#include "mcp_install.h"
#include "ccr.h"
#include "sci.h"
#include "mcp_misc.h"

#include <string.h>
#include <unistd.h>

#define COLOR_BACKGROUND    0x000000ff
#define COLOR_PRIMARY       0xffffffff
#define COLOR_SECONDARY     0x3478e4ff
#define COLOR_SUCCESS       0x00ff00ff
#define COLOR_ERROR         0xff0000ff

static void option_SetColdbootTitle(void);
static void option_DumpSyslogs(void);
static void option_DumpOtpAndSeeprom(void);
static void option_StartWupserver(void);
static void option_LoadNetConf(void);
static void option_displayDRCPin(void);
static void option_InstallWUP(void);
static void option_EditParental(void);
static void option_FixRegionBrick(void);
static void option_SystemInformation(void);
static void option_Shutdown(void);

extern int ppcHeartBeatThreadId;
extern uint64_t currentColdbootOS;
extern uint64_t currentColdbootTitle;

static int fsaHandle = -1;

/**
 * Number of elements in an array.
 *
 * Includes a static check for pointers to make sure
 * a dynamically-allocated array wasn't specified.
 * Reference: http://stackoverflow.com/questions/8018843/macro-definition-array-size
 */
#define ARRAY_SIZE(x) \
	(((sizeof(x) / sizeof(x[0]))) / \
		(size_t)(!(sizeof(x) % sizeof(x[0]))))

typedef struct Menu {
    const char* name;
    union {
        void (*callback)(void);
        uint64_t tid;
    };
} Menu;

static const Menu mainMenuOptions[] = {
    {"Set Coldboot Title",          {.callback = option_SetColdbootTitle}},
    {"Dump Syslogs",                {.callback = option_DumpSyslogs}},
    {"Dump OTP + SEEPROM",          {.callback = option_DumpOtpAndSeeprom}},
    {"Start wupserver",             {.callback = option_StartWupserver}},
    {"Load Network Configuration",  {.callback = option_LoadNetConf}},
    {"Display DRC Pin",             {.callback = option_displayDRCPin}},
    {"Install WUP",                 {.callback = option_InstallWUP}},
    {"Edit Parental Controls",      {.callback = option_EditParental}},
    {"Fix Region Brick",            {.callback = option_FixRegionBrick}},
    {"System Information",          {.callback = option_SystemInformation}},
    {"Shutdown",                    {.callback = option_Shutdown}},
};

static void drawTopBar(const char* title)
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
    gfx_print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 1, "POWER: Choose");
}

typedef enum {
    MenuFlag_ShowTID        = (1U << 0),
    MenuFlag_NoClearScreen  = (1U << 1),
} MenuFlags;

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
 * @return Selected menu entry index.
 */
static int drawMenu(const char* title, const Menu* menu, size_t count,
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

    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        readSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if (flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) {
                prev_selected = selected;
                selected++;
                if (selected == count)
                    selected = 0;
                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
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

static void waitButtonInput(void)
{
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    gfx_draw_rect_filled(16 - 1, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4,
        854 - 16, CHAR_SIZE_DRC_Y + 2,
        COLOR_BACKGROUND);
    gfx_print(16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 0, "Press EJECT or POWER to proceed...");

    uint8_t cur_flag = 0;
    uint8_t flag = 0;

    while (1) {
        readSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if ((flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) || (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON)) {
                return;
            }

            cur_flag = flag;
        }
    }
}

static int isSystemUsingDebugKeyset(void)
{
    int ret = 0;

    // Check OTP to see if this system is using the Debug keyset.
    // NOTE: Includes "Factory" as well.
    uint8_t* const dataBuffer = IOS_HeapAllocAligned(0xcaff, 0x400, 0x40);
    if (!dataBuffer)
        return ret;

    int res = readOTP(dataBuffer, 0x400);
    if (res >= 0) {
        ret = ((dataBuffer[0x080] & 0x18) != 0x10);
    }

    IOS_HeapFree(0xcaff, dataBuffer);
    return ret;
}

static void option_SetColdbootTitle(void)
{
    static const Menu coldbootTitleOptions[] = {
        {"Back", {0} },
        {"Wii U Menu (JPN)", {.tid = 0x0005001010040000}},
        {"Wii U Menu (USA)", {.tid = 0x0005001010040100}},
        {"Wii U Menu (EUR)", {.tid = 0x0005001010040200}},

        // non-retail systems only
        {"System Config Tool", {.tid = 0x000500101F700500}},
        {"DEVMENU (pre-2.09)", {.tid = 0x000500101F7001FF}},
        {"Kiosk Menu        ", {.tid = 0x000500101FA81000}},
    };

    // Only print the non-retail options if the keyset is debug.
    const int option_count = (isSystemUsingDebugKeyset() ? 7 : 4);

    int rval;
    uint64_t newtid = 0;
    int selected = 0;

    gfx_clear(COLOR_BACKGROUND);
    while (1) {
        uint32_t index = 16 + 8 + 2 + 8;
        gfx_set_font_color(COLOR_PRIMARY);

        // draw current titles
        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * (28+8+8)) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        gfx_printf(16, index, 0, "Current coldboot title:    %08lx-%08lx",
            (uint32_t)(currentColdbootTitle >> 32), (uint32_t)(currentColdbootTitle & 0xFFFFFFFFU));
        index += CHAR_SIZE_DRC_Y + 4;

        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * (28+8+8)) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        gfx_printf(16, index, 0, "Current coldboot os:       %08lx-%08lx",
            (uint32_t)(currentColdbootOS >> 32), (uint32_t)(currentColdbootOS & 0xFFFFFFFFU));
        index += (CHAR_SIZE_DRC_Y + 4) * 2;

        selected = drawMenu("Set Coldboot Title",
            coldbootTitleOptions, option_count, selected,
            MenuFlag_ShowTID | MenuFlag_NoClearScreen, 16, index);
        index += (CHAR_SIZE_DRC_Y + 4) * option_count;

        newtid = coldbootTitleOptions[selected].tid;
        if (newtid == 0)
            return;

        // set the new default title ID
        rval = setDefaultTitleId(newtid);

        if (newtid) {
            index += (CHAR_SIZE_DRC_Y + 4) * 2;

            gfx_draw_rect_filled(16 - 1, index - 1,
                (CHAR_SIZE_DRC_X * (37+8+8+11)) + 2, CHAR_SIZE_DRC_Y + 2,
                COLOR_BACKGROUND);
            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Setting coldboot title id to %08lx-%08lx, rval %d",
                (uint32_t)(newtid >> 32), (uint32_t)(newtid & 0xFFFFFFFFU), rval);
            index += CHAR_SIZE_DRC_Y + 4;

            gfx_draw_rect_filled(16 - 1, index - 1,
                (CHAR_SIZE_DRC_X * 46) + 2, CHAR_SIZE_DRC_Y + 2,
                COLOR_BACKGROUND);
            if (rval < 0) {
                gfx_set_font_color(COLOR_ERROR);
                gfx_print(16, index, 0, "Error! Make sure title is installed correctly.");
            } else {
                gfx_set_font_color(COLOR_SUCCESS);
                gfx_print(16, index, 0, "Success!");
            }
        }
    }
}

static void option_DumpSyslogs(void)
{
    gfx_clear(COLOR_BACKGROUND);

    drawTopBar("Dumping Syslogs...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Creating 'logs' directory...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = FSA_MakeDir(fsaHandle, "/vol/storage_recovsd/logs", 0x600);
    if ((res < 0) && !(res == -0x30016)) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to create directory: %x", res);
        waitButtonInput();
        return;
    }

    gfx_print(16, index, 0, "Opening system 'logs' directory...");
    index += CHAR_SIZE_DRC_Y + 4;

    int dir_handle;
    res = FSA_OpenDir(fsaHandle, "/vol/system/logs", &dir_handle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open system logs: %x", res);
        waitButtonInput();
        return;
    }

    char src_path[500];
    char dst_path[500];
    FSDirectoryEntry dir_entry;
    while (FSA_ReadDir(fsaHandle, dir_handle, &dir_entry) >= 0) {
        if (dir_entry.stat.flags & DIR_ENTRY_IS_DIRECTORY) {
            continue;
        }

        gfx_draw_rect_filled(0, index, SCREEN_WIDTH, 8, COLOR_BACKGROUND);
        gfx_printf(16, index, 0, "Copying %s...", dir_entry.name);

        snprintf(src_path, sizeof(src_path), "/vol/system/logs/" "%s", dir_entry.name);
        snprintf(dst_path, sizeof(dst_path), "/vol/storage_recovsd/logs/" "%s", dir_entry.name);

        res = copy_file(fsaHandle, src_path, dst_path);
        if (res < 0) {
            index += CHAR_SIZE_DRC_Y + 4;
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, 0, "Failed to copy %s: %x", dir_entry.name, res);
            waitButtonInput();

            FSA_CloseDir(fsaHandle, dir_handle);
            return;
        }
    }

    index += CHAR_SIZE_DRC_Y + 4;
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseDir(fsaHandle, dir_handle);
}

static void option_DumpOtpAndSeeprom(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Dumping OTP + SEEPROM...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Creating otp.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    void* dataBuffer = IOS_HeapAllocAligned(0xcaff, 0x400, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "Out of memory!");
        waitButtonInput();
        return;
    }

    int otpHandle;
    int res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/otp.bin", "w", &otpHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to create otp.bin: %x", res);
        waitButtonInput();

        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    gfx_print(16, index, 0, "Reading OTP...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = readOTP(dataBuffer, 0x400);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read OTP: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, otpHandle);
        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    gfx_print(16, index, 0, "Writing otp.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = FSA_WriteFile(fsaHandle, dataBuffer, 1, 0x400, otpHandle, 0);
    if (res != 0x400) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to write otp.bin: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, otpHandle);
        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    FSA_CloseFile(fsaHandle, otpHandle);

    gfx_print(16, index, 0, "Creating seeprom.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    int seepromHandle;
    res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/seeprom.bin", "w", &seepromHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to create seeprom.bin: %x", res);
        waitButtonInput();

        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    gfx_print(16, index, 0, "Reading SEEPROM...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = EEPROM_Read(0, 0x100, (uint16_t*) dataBuffer);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read EEPROM: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, seepromHandle);
        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    gfx_print(16, index, 0, "Writing seeprom.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = FSA_WriteFile(fsaHandle, dataBuffer, 1, 0x200, seepromHandle, 0);
    if (res != 0x200) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to write seeprom.bin: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, seepromHandle);
        IOS_HeapFree(0xcaff, dataBuffer);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseFile(fsaHandle, seepromHandle);
    IOS_HeapFree(0xcaff, dataBuffer);
}

static void option_StartWupserver(void)
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
        gfx_draw_rect_filled(0, index, SCREEN_WIDTH, 8, COLOR_BACKGROUND);
        gfx_printf(16, index, 0, "Waiting for network connection... %ds", 4 - i);
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

static void network_parse_config_value(uint32_t* console_idx, NetConfCfg* cfg, const char* key, const char* value, uint32_t value_len)
{
    if (strncmp(key, "type", sizeof("type")) == 0) {
        gfx_printf(16, *console_idx, 0, "Type: %s", value);
        (*console_idx) += CHAR_SIZE_DRC_Y + 4;

        if (value) {
            if (strncmp(value, "wifi", sizeof("wifi")) == 0) {
                cfg->wl0.if_index = NET_CFG_INTERFACE_TYPE_WIFI;
                cfg->wl0.if_sate = 1;
                cfg->wl0.ipv4Info.mode = NET_CONFIG_IPV4_MODE_AUTO_OBTAIN_IP;
                cfg->wifi.config_method = 0;
            } else if (strncmp(value, "eth", sizeof("eth")) == 0) {
                cfg->eth0.if_index = NET_CFG_INTERFACE_TYPE_ETHERNET;
                cfg->eth0.if_sate = 1;
                cfg->eth0.ipv4Info.mode = NET_CONFIG_IPV4_MODE_AUTO_OBTAIN_IP;
                cfg->ethCfg.negotiation = NET_CFG_ETH_CFG_NEGOTIATION_AUTO;
            }
        }
    } else if (strncmp(key, "ssid", sizeof("ssid")) == 0) {
        gfx_printf(16, *console_idx, 0, "SSID: %s (%lu)", value, value_len);
        (*console_idx) += CHAR_SIZE_DRC_Y + 4;

        if (value) {
            memcpy(cfg->wifi.config.ssid, value, value_len);
            cfg->wifi.config.ssidlength = value_len;
        }
    } else if (strncmp(key, "key", sizeof("key")) == 0) {
        gfx_printf(16, *console_idx, 0, "Key: ******* (%lu)", value_len);
        (*console_idx) += CHAR_SIZE_DRC_Y + 4;

        if (value) {
            memcpy(cfg->wifi.config.privacy.aes_key, value, value_len);
            cfg->wifi.config.privacy.aes_key_len = value_len;
        }
    } else if (strncmp(key, "key_type", sizeof("key_type")) == 0) {
        gfx_printf(16, *console_idx, 0, "Key type: %s", value);
        (*console_idx) += CHAR_SIZE_DRC_Y + 4;

        if (value) {
            if (strncmp(value, "NONE", sizeof("NONE")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_NONE;
            } else if (strncmp(value, "WEP", sizeof("WEP")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_WEP;
            } else if (strncmp(value, "WPA2_PSK_TKIP", sizeof("WPA2_PSK_TKIP")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_WPA2_PSK_TKIP;
            } else if (strncmp(value, "WPA_PSK_TKIP", sizeof("WPA_PSK_TKIP")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_WPA_PSK_TKIP;
            } else if (strncmp(value, "WPA2_PSK_AES", sizeof("WPA2_PSK_AES")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_WPA2_PSK_AES;
            } else if (strncmp(value, "WPA_PSK_AES", sizeof("WPA_PSK_AES")) == 0) {
                cfg->wifi.config.privacy.mode = NET_CFG_WIFI_PRIVACY_MODE_WPA_PSK_AES;
            } else {
                gfx_print(16, *console_idx, 0, "Unknown key type!");
                (*console_idx) += CHAR_SIZE_DRC_Y + 4;
            }
        }
    }
}

static void option_LoadNetConf(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Loading network configuration...");

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

    gfx_print(16, index, 0, "Reading network.cfg...");
    index += CHAR_SIZE_DRC_Y + 4;

    int cfgHandle;
    res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/network.cfg", "r", &cfgHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open network.cfg: %x", res);
        waitButtonInput();
        return;
    }

    FSStat stat;
    res = FSA_StatFile(fsaHandle, cfgHandle, &stat);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to stat file: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, cfgHandle);
        return;
    }

    char* cfgBuffer = (char*) IOS_HeapAllocAligned(0xcaff, stat.size + 1, 0x40);
    if (!cfgBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "Out of memory!");
        waitButtonInput();

        FSA_CloseFile(fsaHandle, cfgHandle);
        return;
    }

    cfgBuffer[stat.size] = '\0';

    res = FSA_ReadFile(fsaHandle, cfgBuffer, 1, stat.size, cfgHandle, 0);
    if (res != stat.size) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read file: %x", res);
        waitButtonInput();

        IOS_HeapFree(0xcaff, cfgBuffer);
        FSA_CloseFile(fsaHandle, cfgHandle);
        return;
    }

    NetConfCfg cfg;
    memset(&cfg, 0, sizeof(cfg));

    // parse network cfg file
    const char* keyPtr = cfgBuffer;
    const char* valuePtr = NULL;
    for (size_t i = 0; i < stat.size; i++) {
        if (cfgBuffer[i] == '=') {
            cfgBuffer[i] = '\0';

            valuePtr = cfgBuffer + i + 1;
        } else if (cfgBuffer[i] == '\n') {
            size_t end = i;
            cfgBuffer[end] = '\0';
            if (cfgBuffer[end - 1] == '\r') {
                end = i - 1;
                cfgBuffer[end] = '\0';
            }

            network_parse_config_value(&index, &cfg, keyPtr, valuePtr, (cfgBuffer + end) - valuePtr);

            keyPtr = cfgBuffer + i + 1;
            valuePtr = NULL;
        }
    }

    // if valuePtr isn't NULL there is another option without a newline at the end
    if (valuePtr) {
        network_parse_config_value(&index, &cfg, keyPtr, valuePtr, (cfgBuffer + stat.size) - valuePtr);
    }

    gfx_print(16, index, 0, "Applying configuration...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = netconf_set_running(&cfg);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to apply configuration: %x", res);
        waitButtonInput();

        IOS_HeapFree(0xcaff, cfgBuffer);
        FSA_CloseFile(fsaHandle, cfgHandle);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    index += CHAR_SIZE_DRC_Y + 4;

    waitButtonInput();

    IOS_HeapFree(0xcaff, cfgBuffer);
    FSA_CloseFile(fsaHandle, cfgHandle);
}

static void option_displayDRCPin(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Display DRC Pin");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Reading DRH mac address...");
    index += CHAR_SIZE_DRC_Y + 4;

    int ccrHandle = IOS_Open("/dev/ccr_cdc", 0);
    if (ccrHandle < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open /dev/ccr_cdc: %x", ccrHandle);
        waitButtonInput();
        return;
    }

    uint8_t pincode[4];
    int res = CCRSysGetPincode(ccrHandle, pincode);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get pincode: %x", res);
        waitButtonInput();

        IOS_Close(ccrHandle);
        return;
    }

    static const char symbol_names[][8] = {
        "spade",
        "heart",
        "diamond",
        "clubs",
    };

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_printf(16, index, 0, "Pincode: %x%x%x%x (%s %s %s %s)",
        pincode[0], pincode[1], pincode[2], pincode[3],
        symbol_names[pincode[0]], symbol_names[pincode[1]], symbol_names[pincode[2]], symbol_names[pincode[3]]);

    waitButtonInput();

    IOS_Close(ccrHandle);
}

static void option_InstallWUP(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Installing WUP");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Make sure to place a valid signed WUP directly in 'sd:/install'");
    index += CHAR_SIZE_DRC_Y + 4;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open /dev/mcp: %x", mcpHandle);
        waitButtonInput();
        return;
    }

    gfx_print(16, index, 0, "Querying install info...");
    index += CHAR_SIZE_DRC_Y + 4;

    MCPInstallInfo info;
    int res = MCP_InstallGetInfo(mcpHandle, "/vol/storage_recovsd/install", &info);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get install info: %x", res);
        waitButtonInput();

        IOS_Close(mcpHandle);
        return;
    }

    gfx_printf(16, index, 0, "Installing title: 0x%016llx...", info.titleId);
    index += CHAR_SIZE_DRC_Y + 4;

    // only install to NAND
    res = MCP_InstallSetTargetDevice(mcpHandle, 0);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "MCP_InstallSetTargetDevice: %x", res);
        waitButtonInput();

        IOS_Close(mcpHandle);
        return;
    }
    res = MCP_SetTargetUsb(mcpHandle, 0);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "MCP_InstallSetTargetDevice: %x", res);
        waitButtonInput();

        IOS_Close(mcpHandle);
        return;
    }

    // TODO: async installations
    res = MCP_InstallTitle(mcpHandle, "/vol/storage_recovsd/install");
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to install: %x", res);
        waitButtonInput();

        IOS_Close(mcpHandle);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    IOS_Close(mcpHandle);
}

static void option_EditParental(void)
{
    static const Menu parentalControlOptions[] = {
        {"Back", {0} },
        {"Disable", {0} },
    };

    int rval;
    int selected = 0;

    gfx_clear(COLOR_BACKGROUND);
    while (1) {
        uint32_t index = 16 + 8 + 2 + 8;
        gfx_set_font_color(COLOR_PRIMARY);

        // draw current parental control info
        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * (29+11)) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        uint8_t enabled = 0;
        int res = SCIGetParentalEnable(&enabled);
        if (res == 1) {
            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Parental Controls: %s", enabled ? "Enabled" : "Disabled");
        } else {
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, 0, "SCIGetParentalEnable failed: %d", res);
        }
        index += CHAR_SIZE_DRC_Y + 4;

        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * (30+3)) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        char pin[5] = "";
        res = SCIGetParentalPinCode(pin, sizeof(pin));
        if (res == 1) {
            gfx_set_font_color(COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Parental Pin Code: %s", pin);
        } else {
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, 0, "SCIGetParentalPinCode failed: %d", res);
        }
        index += (CHAR_SIZE_DRC_Y + 4) * 2;

        gfx_set_font_color(COLOR_PRIMARY);

        selected = drawMenu("Edit Parental Controls",
            parentalControlOptions, ARRAY_SIZE(parentalControlOptions), selected,
            MenuFlag_NoClearScreen, 16, index);
        index += (CHAR_SIZE_DRC_Y + 4) * ARRAY_SIZE(parentalControlOptions);

        if (selected == 0)
            return;

        // Option 1: Disable the parental controls.
        rval = SCISetParentalEnable(0);

        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * (29+11)) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        gfx_printf(16, index, 0, "SCISetParentalEnable(false): %d", rval);
        index += CHAR_SIZE_DRC_Y + 4;

        gfx_draw_rect_filled(16 - 1, index - 1,
            (CHAR_SIZE_DRC_X * 8) + 2, CHAR_SIZE_DRC_Y + 2,
            COLOR_BACKGROUND);
        if (rval != 1) {
            gfx_set_font_color(COLOR_ERROR);
            gfx_print(16, index, 0, "Error!");
        } else {
            gfx_set_font_color(COLOR_SUCCESS);
            gfx_print(16, index, 0, "Success!");
        }
        index += CHAR_SIZE_DRC_Y + 4;
    }
}

static const char region_tbl[7][4] = {
    "JPN", "USA", "EUR", "AUS",
    "CHN", "KOR", "TWN",
};

/**
 * Get region code information.
 * @param productArea_id Product area ID: 0-6
 * @param gameRegion Bitfield of game regions
 * @return 0 on success; negative on error
 */
static int getRegionInfo(int* productArea_id, int* gameRegion)
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

static void option_FixRegionBrick(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Fix Region Brick");

    uint32_t index = 16 + 8 + 2 + 8;

    // Get the system region code, then check if a matching
    // Wii U Menu is installed.
    int productArea_id, gameRegion;
    int res = getRegionInfo(&productArea_id, &gameRegion);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get the system region code: %x", res);
        waitButtonInput();
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
    index += CHAR_SIZE_DRC_Y + 4;
    index += CHAR_SIZE_DRC_Y + 4;

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

    gfx_print(16, index, 0, "Installed Wii U Menu: ");
    const char* menu_region_str;
    if (menu_count == 0 || menu_productArea_id < 0) {
        menu_region_str = "NONE";
    } else if (menu_count > 1) {
        menu_region_str = "MANY";
    } else {
        menu_region_str = region_tbl[menu_productArea_id];
    }

    if (menu_matches_region && menu_is_in_gameRegion && menu_count == 1) {
        // Matching menu found.
        gfx_set_font_color(COLOR_SUCCESS);
    } else {
        gfx_set_font_color(COLOR_ERROR);
    }
    gfx_print(16+(22*CHAR_SIZE_DRC_X), index, 0, menu_region_str);
    index += CHAR_SIZE_DRC_Y + 4;

    if (menu_matches_region && menu_is_in_gameRegion) {
        gfx_set_font_color(COLOR_PRIMARY);
        gfx_print(16, index, 0, "The system region appears to be set correctly.");
        waitButtonInput();
        return;
    }

    // Show the errors.
    if (menu_count == 0 || menu_productArea_id < 0) {
        gfx_print(16, index, 0, "Could not find a Wii U Menu title installed on this system.");
        waitButtonInput();
        return;
    } else if (menu_count > 1) {
        gfx_print(16, index, 0, "Multiple Wii U Menus were found. Someone dun goofed...");
        waitButtonInput();
        return;
    }

    if (!menu_matches_region) {
        gfx_print(16, index, 0, "The system region does not match the installed Wii U Menu.");
        index += CHAR_SIZE_DRC_Y + 4;
    }
    if (!menu_is_in_gameRegion) {
        gfx_print(16, index, 0, "The game region does not match the installed Wii U Menu.");
        index += CHAR_SIZE_DRC_Y + 4;
    }
    index += CHAR_SIZE_DRC_Y + 4;

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_printf(16, index, 0, "Repair the system by setting the region code to %s?", menu_region_str);
    index += CHAR_SIZE_DRC_Y + 4;

    char menuItem[] = "Set Region to XXX";
    menuItem[14] = menu_region_str[0];
    menuItem[15] = menu_region_str[1];
    menuItem[16] = menu_region_str[2];

    const Menu fixRegionBrickOptions[] = {
        {"Cancel", {0} },
        {menuItem, {0} },
    };
    int selected = drawMenu("Fix Region Brick",
        fixRegionBrickOptions, ARRAY_SIZE(fixRegionBrickOptions), selected,
        MenuFlag_NoClearScreen, 16, index);
    if (selected == 0)
        return;
    index += (CHAR_SIZE_DRC_Y*3) + 4;

    // Attempt to set the region code.
    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "IOS_Open(\"/dev/mcp\") failed: %x", mcpHandle);
        waitButtonInput();
        return;
    }

    MCPSysProdSettings sysProdSettings;
    res = MCP_GetSysProdSettings(mcpHandle, &sysProdSettings);
    if (res < 0) {
        IOS_Close(mcpHandle);
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "MCP_GetSysProdSettings() failed: %x", res);
        waitButtonInput();
        return;
    }

    // Set both productArea and gameRegion to the Wii U Menu's region value.
    // NOTE: productArea_id is a bit index, so it needs to be shifted into place.
    sysProdSettings.product_area = (1U << menu_productArea_id);
    sysProdSettings.game_region = (1U << menu_productArea_id);
    res = MCP_SetSysProdSettings(mcpHandle, &sysProdSettings);
    IOS_Close(mcpHandle);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "MCP_SetSysProdSettings() failed: %x", res);
    } else {
        gfx_set_font_color(COLOR_SUCCESS);
        gfx_printf(16, index, 0, "System region set to %s successfully.", menu_region_str);
        waitButtonInput();
        return;
    }

    waitButtonInput();
}

static void option_SystemInformation(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("System Information");

    uint32_t index = 16 + 8 + 2 + 8;

    // parse OTP/SEEPROM for system information
    // 0x000-0x3FF: OTP
    // 0x400-0x5FF: SEEPROM
    // 0x600-0x7FF: misc for version.bin
    void *dataBuffer = IOS_HeapAllocAligned(0xcaff, 0x800, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "Out of memory!");
        waitButtonInput();
        return;
    }
    uint8_t* const otp = (uint8_t*)dataBuffer;
    uint16_t* const seeprom = (uint16_t*)dataBuffer + 0x200;

    int res = readOTP((void*)otp, 0x400);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read OTP: %x", res);
        IOS_HeapFree(0xcaff, dataBuffer);
        waitButtonInput();
        return;
    }

    res = EEPROM_Read(0, 0x100, seeprom);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read EEPROM: %x", res);
        IOS_HeapFree(0xcaff, dataBuffer);
        waitButtonInput();
        return;
    }

    // NOTE: gfx_printf() does not support precision specifiers.
    // We'll have to ensure the strings are terminated manually.
    char buf1[17];
    char buf2[17];

    memcpy(buf1, &seeprom[0xB8], 16);
    buf1[16] = '\0';
    gfx_printf(16, index, 0, "Model:    %s", buf1);
    index += CHAR_SIZE_DRC_Y + 4;

    memcpy(buf1, &seeprom[0xAC], 8);
    buf1[8] = '\0';
    memcpy(buf2, &seeprom[0xB0], 16);
    buf2[16] = '\0';
    gfx_printf(16, index, 0, "Serial:   %s%s", buf1, buf2);
    index += CHAR_SIZE_DRC_Y + 4;

    if (seeprom[0xC4] != 0) {
        gfx_printf(16, index, 0, "Mfg Date: %04x/%02x/%02x %02x:%02x",
            seeprom[0xC4], seeprom[0xC5] >> 8, seeprom[0xC5] & 0xFF,
            seeprom[0xC6] >> 8, seeprom[0xC6] & 0xFF);
        index += CHAR_SIZE_DRC_Y + 4;
    }

    static const char keyset_tbl[4][8] = {"Factory", "Debug", "Retail", "Invalid"};
    gfx_printf(16, index, 0, "Keyset:   %s", keyset_tbl[(otp[0x080] & 0x18) >> 3]);

    index += CHAR_SIZE_DRC_Y + 4;
    index += CHAR_SIZE_DRC_Y + 4;

    memcpy(buf1, &seeprom[0x21], 2);
    buf1[2] = '\0';
    gfx_printf(16, index, 0, "boardType:   %s", buf1);
    index += CHAR_SIZE_DRC_Y + 4;

    const unsigned int sataDevice_id = seeprom[0x2C];
    static const char* const sataDevice_tbl[] = {
        NULL, "Default", "No device", "ROM drive",
        "R drive", "MION", "SES", "GEN2-HDD",
        "GEN1-HDD",
    };
    const char *sataDevice = NULL;
    if (sataDevice_id < 9) {
        sataDevice = sataDevice_tbl[sataDevice_id];
    }
    if (sataDevice) {
        gfx_printf(16, index, 0, "sataDevice:  %u (%s)", sataDevice_id, sataDevice);
    } else {
        gfx_printf(16, index, 0, "sataDevice:  %u", sataDevice_id);
    }
    index += CHAR_SIZE_DRC_Y + 4;

    const unsigned int consoleType_id = seeprom[0x2D];
    static const char* const consoleType_tbl[] = {
        NULL, "WUP", "CAT-R", "CAT-DEV",
        "EV board", "Kiosk", "OrchestraX", "WUIH",
        "WUIH_DEV", "CAT_DEV_WUIH",
    };
    const char* consoleType = NULL;
    if (consoleType_id < 10) {
        consoleType = consoleType_tbl[consoleType_id];
    }
    if (consoleType) {
        gfx_printf(16, index, 0, "consoleType: %u (%s)", consoleType_id, consoleType);
    } else {
        gfx_printf(16, index, 0, "consoleType: %u", consoleType_id);
    }
    index += CHAR_SIZE_DRC_Y + 4;
    index += CHAR_SIZE_DRC_Y + 4;

    int productArea_id = 0; // 0-6, matches title ID
    int gameRegion;
    res = getRegionInfo(&productArea_id, &gameRegion);
    if (res >= 0) {
        // productArea is a single region.
        gfx_printf(16, index, 0, "productArea: %s", region_tbl[productArea_id]);
        index += CHAR_SIZE_DRC_Y + 4;

        // gameRegion is multiple regions.
        gfx_printf(16, index, 0, "gameRegion:  %s %s %s %s %s %s",
            (gameRegion & MCP_REGION_JAPAN)  ? region_tbl[0] : "---",
            (gameRegion & MCP_REGION_USA)    ? region_tbl[1] : "---",
            (gameRegion & MCP_REGION_EUROPE) ? region_tbl[2] : "---",
            (gameRegion & MCP_REGION_CHINA)  ? region_tbl[4] : "---",
            (gameRegion & MCP_REGION_KOREA)  ? region_tbl[5] : "---",
            (gameRegion & MCP_REGION_TAIWAN) ? region_tbl[6] : "---");
        index += CHAR_SIZE_DRC_Y + 4;
    }

    // Wii U Menu version
    // FIXME: CAT-I has all three region versions installed.
    // Need to get the actual productArea from sys_prod.xml.
#define VERSION_BIN_MAGIC_STR "VER\0"
#define VERSION_BIN_MAGIC_U32 0x56455200
    typedef struct _version_bin_t {
        union {
            char c[4];       // [0x000] "VER\0"
            uint32_t u32;   // [0x000] 0x56455200
        } ver_magic;
        uint32_t major;     // [0x004] Major version
        uint32_t minor;     // [0x008] Minor version
        uint32_t revision;  // [0x00C] Revision
        char region;        // [0x010] Region character: 'J', 'U', 'E'

        uint8_t reserved[47];   // [0x011]
    } version_bin_t;
    version_bin_t* const version_bin = (version_bin_t*)((uint8_t*)dataBuffer + 0x600);
    version_bin->ver_magic.u32 = 0;

    char path[] = "/vol/storage_mlc01/sys/title/00050010/10041x00/content/version.bin";
    path[43] = productArea_id + '0';

    int verHandle;
    res = FSA_OpenFile(fsaHandle, path, "r", &verHandle);
    if (res >= 0) {
        // version.bin should always be 64 bytes.
        res = FSA_ReadFile(fsaHandle, version_bin, 1, sizeof(*version_bin), verHandle, 0);
        FSA_CloseFile(fsaHandle, verHandle);
        if (res == sizeof(*version_bin)) {
            // Did we find a valid version.bin?
            if (version_bin->ver_magic.u32 == VERSION_BIN_MAGIC_U32) {
                // Found a valid version.bin.
                gfx_printf(16, index, 0, "Wii U Menu version: %lu.%lu.%lu %c",
                    version_bin->major, version_bin->minor, version_bin->revision, version_bin->region);
                index += CHAR_SIZE_DRC_Y + 4;
            }
        }
    }

    // TODO: productArea, gameRegion, IOSU version, Wii U Menu version
    // - productArea is set to 2 on my CAT-I that's actually set to USA.
    // - gameRegion is set to 0 on all systems I've used.
    // TODO: Use MCP_GetSysProdSettings()?

    IOS_HeapFree(0xcaff, dataBuffer);
    waitButtonInput();
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

    IOS_Shutdown(0);   
}

int menuThread(void* arg)
{
    printf("menuThread running\n");

    // stop ppcHeartbeatThread and reset PPC
    IOS_CancelThread(ppcHeartBeatThreadId, 0);
    resetPPC();

#ifdef DC_INIT
    /* Note: CONFIGURATION_0 is 720p instead of 480p,
       but doesn't shut down the GPU properly? The
       GamePad just stays connected after calling iOS_Shutdown.
       To be safe, let's use CONFIGURATION_1 for now */

    // init display output
    initDisplay(DC_CONFIGURATION_1);

    /* Note about the display configuration struct:
       The returned framebuffer address seems to be AV out only?
       Writing to the hardcoded addresses in gfx.c works for HDMI though */
    //DisplayController_Config dc_config;
    //readDCConfig(&dc_config);
#endif

    // initialize the font
    if (gfx_init_font() != 0) {
        // failed to initialize font
        // can't do anything without a font, so shut down
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

    int selected = 0;
    while (1) {
        selected = drawMenu("Wii U Recovery Menu v0.2 by GaryOderNichts",
            mainMenuOptions, ARRAY_SIZE(mainMenuOptions), selected,
            0, 16, 16+8+2+8);
        if (selected >= 0 && selected < ARRAY_SIZE(mainMenuOptions)) {
            mainMenuOptions[selected].callback();
        }
    }

    return 0;
}
