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
static void option_SystemInformation(void);
static void option_Shutdown(void);

extern int ppcHeartBeatThreadId;
extern uint64_t currentColdbootOS;
extern uint64_t currentColdbootTitle;

static int fsaHandle = -1;

static struct {
    const char* name;
    void (*callback)(void);
} mainMenuOptions[] = {
    { "Set Coldboot Title", option_SetColdbootTitle },
    { "Dump Syslogs", option_DumpSyslogs },
    { "Dump OTP + SEEPROM", option_DumpOtpAndSeeprom },
    { "Start wupserver", option_StartWupserver },
    { "Load Network Configuration", option_LoadNetConf },
    { "Display DRC Pin", option_displayDRCPin },
    { "Install WUP", option_InstallWUP, },
    { "Edit Parental Controls", option_EditParental, },
    { "System Information", option_SystemInformation, },
    { "Shutdown", option_Shutdown },
};
static const int numMainMenuOptions = sizeof(mainMenuOptions) / sizeof(mainMenuOptions[0]);

static void drawTopBar(const char* title)
{
    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
}

static void drawBars(const char* title)
{
    drawTopBar(title);

    // draw bottom bar
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
    gfx_printf(16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 0, "EJECT: Navigate");
    gfx_printf(SCREEN_WIDTH - 16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 1, "POWER: Choose");
}

static void waitButtonInput(void)
{
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
    gfx_printf(16, SCREEN_HEIGHT - CHAR_SIZE_DRC_Y - 4, 0, "Press EJECT or POWER to proceed...");

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
    typedef struct _coldbootTitleOptions_t {
        const char* desc;
        uint64_t tid;
    } coldbootTitleOptions_t;

    static const coldbootTitleOptions_t coldbootTitleOptions[] = {
        {"Back", 0},
        {"Wii U Menu (JPN)", 0x0005001010040000},
        {"Wii U Menu (USA)", 0x0005001010040100},
        {"Wii U Menu (EUR)", 0x0005001010040200},

        // non-retail systems only
        {"System Config Tool", 0x000500101F700500},
        {"DEVMENU (pre-2.09)", 0x000500101F7001FF},
        {"Kiosk Menu        ", 0x000500101FA81000},
    };

    // Only print the non-retail options if the keyset is debug.
    const int option_count = (isSystemUsingDebugKeyset() ? 7 : 4);

    int rval;
    uint64_t newtid = 0;

    int redraw = 1;
    int selected = 0;

    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        readSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if (flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) {
                selected++;

                if (selected == option_count) {
                    selected = 0;
                }

                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
                newtid = coldbootTitleOptions[selected].tid;
                if (newtid == 0)
                    return;

                rval = setDefaultTitleId(newtid);
                redraw = 1;
            }

            cur_flag = flag;
        }

        if (redraw) {
            gfx_clear(COLOR_BACKGROUND);
            uint32_t index = 16 + 8 + 2 + 8;

            // draw current titles
            gfx_printf(16, index, 0, "Current coldboot title:    %08x-%08x",
                (uint32_t)(currentColdbootTitle >> 32), (uint32_t)(currentColdbootTitle & 0xFFFFFFFFU));
            index += CHAR_SIZE_DRC_Y + 4;

            gfx_printf(16, index, 0, "Current coldboot os:       %08x-%08x",
                (uint32_t)(currentColdbootOS >> 32), (uint32_t)(currentColdbootOS & 0xFFFFFFFFU));
            index += (CHAR_SIZE_DRC_Y + 4) * 2;

            // draw options
            for (int i = 0; i < option_count; i++) {
                char buf[64];
                if (coldbootTitleOptions[i].tid != 0) {
                    // Append the title ID in hi-lo format.
                    snprintf(buf, sizeof(buf), "%s - %08lx-%08lx", coldbootTitleOptions[i].desc,
                        (uint32_t)(coldbootTitleOptions[i].tid >> 32),
                        (uint32_t)(coldbootTitleOptions[i].tid & 0xFFFFFFFFU));
                } else {
                    // No title ID. Use the option by itself.
                    snprintf(buf, sizeof(buf), "%s", coldbootTitleOptions[i].desc);
                }

                gfx_draw_rect_filled(16 - 1, index - 1,
                    gfx_get_text_width(buf) + 2, CHAR_SIZE_DRC_Y + 2,
                    selected == i ? COLOR_PRIMARY : COLOR_BACKGROUND);

                gfx_set_font_color(selected == i ? COLOR_BACKGROUND : COLOR_PRIMARY);
                gfx_printf(16, index, 0, buf);

                index += CHAR_SIZE_DRC_Y + 4;
            }

            gfx_set_font_color(COLOR_PRIMARY);

            if (newtid) {
                index += (CHAR_SIZE_DRC_Y + 4) * 2;
                gfx_printf(16, index, 0, "Setting coldboot title id to %08x-%08x, rval %d",
                    (uint32_t)(newtid >> 32), (uint32_t)(newtid & 0xFFFFFFFFU), rval);
                index += CHAR_SIZE_DRC_Y + 4;
                if (rval < 0) {
                    gfx_set_font_color(COLOR_ERROR);
                    gfx_printf(16, index, 0, "Error! Make sure title is installed correctly.");
                } else {
                    gfx_set_font_color(COLOR_SUCCESS);
                    gfx_printf(16, index, 0, "Success!");
                }
            }

            drawBars("Set Coldboot Title");
            redraw = 0;
        }
    }
}

static void option_DumpSyslogs(void)
{
    gfx_clear(COLOR_BACKGROUND);

    drawTopBar("Dumping Syslogs...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Creating 'logs' directory...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = FSA_MakeDir(fsaHandle, "/vol/storage_recovsd/logs", 0x600);
    if ((res < 0) && !(res == -0x30016)) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to create directory: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Opening system 'logs' directory...");
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
    gfx_printf(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseDir(fsaHandle, dir_handle);
}

static void option_DumpOtpAndSeeprom(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Dumping OTP + SEEPROM...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Creating otp.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    void* dataBuffer = IOS_HeapAllocAligned(0xcaff, 0x400, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Out of memory!");
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

    gfx_printf(16, index, 0, "Reading OTP...");
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

    gfx_printf(16, index, 0, "Writing otp.bin...");
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

    gfx_printf(16, index, 0, "Creating seeprom.bin...");
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

    gfx_printf(16, index, 0, "Reading SEEPROM...");
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

    gfx_printf(16, index, 0, "Writing seeprom.bin...");
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
    gfx_printf(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseFile(fsaHandle, seepromHandle);
    IOS_HeapFree(0xcaff, dataBuffer);
}

static void option_StartWupserver(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Running wupserver...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Initializing netconf...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize netconf: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Waiting for network connection... 5s");

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
        gfx_printf(16, index, 0, "No network connection!");
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
    gfx_printf(16, index, 0, "Wupserver running. Press EJECT or POWER to stop.");
    index += CHAR_SIZE_DRC_Y + 4;

    waitButtonInput();

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_printf(16, index, 0, "Stopping wupserver...");
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
        gfx_printf(16, *console_idx, 0, "SSID: %s (%d)", value, value_len);
        (*console_idx) += CHAR_SIZE_DRC_Y + 4;

        if (value) {
            memcpy(cfg->wifi.config.ssid, value, value_len);
            cfg->wifi.config.ssidlength = value_len;
        }
    } else if (strncmp(key, "key", sizeof("key")) == 0) {
        gfx_printf(16, *console_idx, 0, "Key: ******* (%d)", value_len);
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
                gfx_printf(16, *console_idx, 0, "Unknown key type!");
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
    gfx_printf(16, index, 0, "Initializing netconf...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize netconf: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Reading network.cfg...");
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
        gfx_printf(16, index, 0, "Out of memory!");
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

    gfx_printf(16, index, 0, "Applying configuration...");
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
    gfx_printf(16, index, 0, "Done!");
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
    gfx_printf(16, index, 0, "Reading DRH mac address...");
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
    gfx_printf(16, index, 0, "Make sure to place a valid signed WUP directly in 'sd:/install'");
    index += CHAR_SIZE_DRC_Y + 4;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open /dev/mcp: %x", mcpHandle);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Querying install info...");
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
    gfx_printf(16, index, 0, "Done!");
    waitButtonInput();

    IOS_Close(mcpHandle);
}

static void option_EditParental(void)
{
    int redraw = 1;
    int selected = 0;

    int toggled = 0;
    int rval = 0;

    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        readSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if (flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) {
                selected = !selected;
                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
                if (selected) {
                    rval = SCISetParentalEnable(0);
                    toggled = 1;
                    redraw = 1;
                } else {
                    return;
                }
            }

            cur_flag = flag;
        }

        if (redraw) {
            gfx_clear(COLOR_BACKGROUND);
            uint32_t index = 16 + 8 + 2 + 8;

            uint8_t enabled = 0;
            int res = SCIGetParentalEnable(&enabled);
            if (res == 1) {
                gfx_printf(16, index, 0, "Parental Controls: %s", enabled ? "Enabled" : "Disabled");
            } else {
                gfx_set_font_color(COLOR_ERROR);
                gfx_printf(16, index, 0, "SCIGetParentalEnable failed: %d", res);
            }
            index += CHAR_SIZE_DRC_Y + 4;

            gfx_set_font_color(COLOR_PRIMARY);

            char pin[5] = "";
            res = SCIGetParentalPinCode(pin, sizeof(pin));
            if (res == 1) {
                gfx_printf(16, index, 0, "Parental Pin Code: %s", pin);
            } else {
                gfx_set_font_color(COLOR_ERROR);
                gfx_printf(16, index, 0, "SCIGetParentalPinCode failed: %d", res);
            }
            index += (CHAR_SIZE_DRC_Y + 4) * 2;

            gfx_set_font_color(COLOR_PRIMARY);

            // draw options

            // Back button
            gfx_draw_rect_filled(16 - 1, index - 1,
                gfx_get_text_width("Back") + 2, CHAR_SIZE_DRC_Y + 2,
                !selected ? COLOR_PRIMARY : COLOR_BACKGROUND);

            gfx_set_font_color(!selected ? COLOR_BACKGROUND : COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Back");

            index += CHAR_SIZE_DRC_Y + 4;

            // Disable button
            gfx_draw_rect_filled(16 - 1, index - 1,
                gfx_get_text_width("Disable") + 2, CHAR_SIZE_DRC_Y + 2,
                selected ? COLOR_PRIMARY : COLOR_BACKGROUND);

            gfx_set_font_color(selected ? COLOR_BACKGROUND : COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Disable");

            index += CHAR_SIZE_DRC_Y + 4;

            gfx_set_font_color(COLOR_PRIMARY);

            if (toggled) {
                index += CHAR_SIZE_DRC_Y + 4;
                
                gfx_printf(16, index, 0, "SCISetParentalEnable(false): %d", rval);
                index += CHAR_SIZE_DRC_Y + 4;

                if (rval != 1) {
                    gfx_set_font_color(COLOR_ERROR);
                    gfx_printf(16, index, 0, "Error!");
                } else {
                    gfx_set_font_color(COLOR_SUCCESS);
                    gfx_printf(16, index, 0, "Success!");
                }

                index += CHAR_SIZE_DRC_Y + 4;
            }

            drawBars("Edit Parental Controls");
            redraw = 0;
        }
    }
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
        gfx_printf(16, index, 0, "Out of memory!");
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

    int productArea_id = 0;    // 0-6, matches title ID
    MCPSysProdSettings sysProdSettings;
    sysProdSettings.product_area = 0;
    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle >= 0) {
        res = MCP_GetSysProdSettings(mcpHandle, &sysProdSettings);
        if (res >= 0) {
            static const char region_tbl[7][4] = {
                "JPN", "USA", "EUR", "AUS",
                "CHN", "KOR", "TWN",
            };

            // productArea is a single region.
            if (sysProdSettings.product_area != 0) {
                productArea_id = __builtin_ctz(sysProdSettings.product_area);
                if (productArea_id < 7) {
                    gfx_printf(16, index, 0, "productArea: %s", region_tbl[productArea_id]);
                    index += CHAR_SIZE_DRC_Y + 4;
                }
            }

            // gameRegion is multiple regions.
            gfx_printf(16, index, 0, "gameRegion:  %s %s %s %s %s %s",
                (sysProdSettings.game_region & MCP_REGION_JAPAN)  ? region_tbl[0] : "---",
                (sysProdSettings.game_region & MCP_REGION_USA)    ? region_tbl[1] : "---",
                (sysProdSettings.game_region & MCP_REGION_EUROPE) ? region_tbl[2] : "---",
                (sysProdSettings.game_region & MCP_REGION_CHINA)  ? region_tbl[4] : "---",
                (sysProdSettings.game_region & MCP_REGION_KOREA)  ? region_tbl[5] : "---",
                (sysProdSettings.game_region & MCP_REGION_TAIWAN) ? region_tbl[6] : "---");
            index += CHAR_SIZE_DRC_Y + 4;
        } else {
            // Failed to get sys_prod settings.
            // Clear the product area.
            sysProdSettings.product_area = 0;
        }
        IOS_Close(mcpHandle);
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
                gfx_printf(16, index, 0, "Wii U Menu version: %u.%u.%u %c",
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

    int redraw = 1;
    int selected = 0;

    uint8_t cur_flag = 0;
    uint8_t flag = 0;
    while (1) {
        readSystemEventFlag(&flag);
        if (cur_flag != flag) {
            if (flag & SYSTEM_EVENT_FLAG_EJECT_BUTTON) {
                selected++;

                if (selected == numMainMenuOptions) {
                    selected = 0;
                }

                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
                if (mainMenuOptions[selected].callback) {
                    mainMenuOptions[selected].callback();
                    redraw = 1;
                }
            }

            cur_flag = flag;
        }

        if (redraw) {
            gfx_clear(COLOR_BACKGROUND);

            // draw options
            uint32_t index = 16+8+2+8;
            for (int i = 0; i < numMainMenuOptions; i++) {
                gfx_draw_rect_filled(16 - 1, index - 1,
                    gfx_get_text_width(mainMenuOptions[i].name) + 2, CHAR_SIZE_DRC_Y + 2,
                    selected == i ? COLOR_PRIMARY : COLOR_BACKGROUND);

                gfx_set_font_color(selected == i ? COLOR_BACKGROUND : COLOR_PRIMARY);
                gfx_printf(16, index, 0, mainMenuOptions[i].name);

                index += CHAR_SIZE_DRC_Y + 4;
            }

            gfx_set_font_color(COLOR_PRIMARY);
            drawBars("Wii U Recovery Menu v0.2 by GaryOderNichts");
            redraw = 0;
        }
    }

    return 0;
}
