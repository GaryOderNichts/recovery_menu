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
    { "Shutdown", option_Shutdown },
};
static const int numMainMenuOptions = sizeof(mainMenuOptions) / sizeof(mainMenuOptions[0]);

static void waitButtonInput(void)
{
    gfx_set_font_color(COLOR_PRIMARY);
    gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
    gfx_printf(16, SCREEN_HEIGHT - 16, 0, "Press EJECT or POWER to proceed");

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

static void option_SetColdbootTitle(void)
{
    static const char* coldbootTitleOptions[] = {
        "Back",
        "Wii U Menu (JPN) - 00050010-10040000",
        "Wii U Menu (USA) - 00050010-10040100",
        "Wii U Menu (EUR) - 00050010-10040200",
    };

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

                if (selected == 4) {
                    selected = 0;
                }

                redraw = 1;
            } else if (flag & SYSTEM_EVENT_FLAG_POWER_BUTTON) {
                switch (selected) {
                case 0:
                    return;
                case 1:
                    newtid = 0x0005001010040000llu;
                    rval = setDefaultTitleId(newtid);
                    break;
                case 2:
                    newtid = 0x0005001010040100llu;
                    rval = setDefaultTitleId(newtid);
                    break;
                case 3:
                    newtid = 0x0005001010040200llu;
                    rval = setDefaultTitleId(newtid);
                    break;
                }

                redraw = 1;
            }

            cur_flag = flag;
        }

        if (redraw) {
            gfx_clear(COLOR_BACKGROUND);
            uint32_t index = 16 + 8 + 2 + 8;

            // draw current titles
            gfx_printf(16, index, 0, "Current coldboot title:    0x%016llx", currentColdbootTitle);
            index += 8 + 4;

            gfx_printf(16, index, 0, "Current coldboot os:       0x%016llx", currentColdbootOS);
            index += (8 + 4) * 2;

            // draw options
            for (int i = 0; i < 4; i++) {
                gfx_draw_rect_filled(16 - 1, index - 1,
                    gfx_get_text_width(coldbootTitleOptions[i]) + 2, 8 + 2,
                    selected == i ? COLOR_PRIMARY : COLOR_BACKGROUND);

                gfx_set_font_color(selected == i ? COLOR_BACKGROUND : COLOR_PRIMARY);
                gfx_printf(16, index, 0, coldbootTitleOptions[i]);

                index += 8 + 4;
            }

            gfx_set_font_color(COLOR_PRIMARY);

            if (newtid) {
                index += (8 + 4) * 2;
                gfx_printf(16, index, 0, "Setting coldboot title id to 0x%016llx, rval %d", newtid, rval);
                index += 8 + 4;
                if (rval < 0) {
                    gfx_set_font_color(COLOR_ERROR);
                    gfx_printf(16, index, 0, "Error! Make sure title is installed correctly.");
                } else {
                    gfx_set_font_color(COLOR_SUCCESS);
                    gfx_printf(16, index, 0, "Success!");
                }
            }

            // draw top bar
            gfx_set_font_color(COLOR_PRIMARY);
            const char* title = "Set Coldboot Title";
            gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
            gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

            // draw bottom bar
            gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
            gfx_printf(16, SCREEN_HEIGHT - 16, 0, "EJECT: Navigate ");
            gfx_printf(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 16, 1, "POWER: Choose");

            redraw = 0;
        }
    }
}

static void option_DumpSyslogs(void)
{
    gfx_clear(COLOR_BACKGROUND);

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Dumping Syslogs...";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Creating 'logs' directory...");
    index += 8 + 4;

    int res = FSA_MakeDir(fsaHandle, "/vol/storage_recovsd/logs", 0x600);
    if ((res < 0) && !(res == -0x30016)) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to create directory: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Opening system 'logs' directory...");
    index += 8 + 4;

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
            index += 8 + 4;
            gfx_set_font_color(COLOR_ERROR);
            gfx_printf(16, index, 0, "Failed to copy %s: %x", dir_entry.name, res);
            waitButtonInput();

            FSA_CloseDir(fsaHandle, dir_handle);
            return;
        }
    }

    index += 8 + 4;
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_printf(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseDir(fsaHandle, dir_handle);
}

static void option_DumpOtpAndSeeprom(void)
{
    gfx_clear(COLOR_BACKGROUND);

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Dumping OTP + SEEPROM...";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Creating otp.bin...");
    index += 8 + 4;

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
    index += 8 + 4;

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
    index += 8 + 4;

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
    index += 8 + 4;

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
    index += 8 + 4;

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
    index += 8 + 4;

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

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Running wupserver...";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Initializing netconf...");
    index += 8 + 4;

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

    index += 8 + 4;

    if (interface == 0xff) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "No network connection!");
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Connected using %s", (interface == NET_CFG_INTERFACE_TYPE_WIFI) ? "WIFI" : "ETHERNET");
    index += 8 + 4;

    uint8_t ip_address[4];
    res = netconf_get_assigned_address(interface, (uint32_t*) ip_address);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get IP address: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "IP address: %d.%d.%d.%d", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    index += 8 + 4;

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
    index += 8 + 4;

    waitButtonInput();

    gfx_set_font_color(COLOR_PRIMARY);
    gfx_printf(16, index, 0, "Stopping wupserver...");
    index += 8 + 4;

    wupserver_deinit();
}

static void network_parse_config_value(uint32_t* console_idx, NetConfCfg* cfg, const char* key, const char* value, uint32_t value_len)
{
    if (strncmp(key, "type", sizeof("type")) == 0) {
        gfx_printf(16, *console_idx, 0, "Type: %s", value);
        (*console_idx) += 8 + 4;

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
        (*console_idx) += 8 + 4;

        if (value) {
            memcpy(cfg->wifi.config.ssid, value, value_len);
            cfg->wifi.config.ssidlength = value_len;
        }
    } else if (strncmp(key, "key", sizeof("key")) == 0) {
        gfx_printf(16, *console_idx, 0, "Key: ******* (%d)", value_len);
        (*console_idx) += 8 + 4;

        if (value) {
            memcpy(cfg->wifi.config.privacy.aes_key, value, value_len);
            cfg->wifi.config.privacy.aes_key_len = value_len;
        }
    } else if (strncmp(key, "key_type", sizeof("key_type")) == 0) {
        gfx_printf(16, *console_idx, 0, "Key type: %s", value);
        (*console_idx) += 8 + 4;

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
                (*console_idx) += 8 + 4;
            }
        }
    }
}

static void option_LoadNetConf(void)
{
    gfx_clear(COLOR_BACKGROUND);

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Loading network configuration...";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Initializing netconf...");
    index += 8 + 4;

    int res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize netconf: %x", res);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Reading network.cfg...");
    index += 8 + 4;

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
            cfgBuffer[i] = '\0';

            network_parse_config_value(&index, &cfg, keyPtr, valuePtr, (cfgBuffer + i) - valuePtr);

            keyPtr = cfgBuffer + i + 1;
            valuePtr = NULL;
        }
    }

    // if valuePtr isn't NULL there is another option without a newline at the end
    if (valuePtr) {
        network_parse_config_value(&index, &cfg, keyPtr, valuePtr, (cfgBuffer + stat.size) - valuePtr);
    }

    gfx_printf(16, index, 0, "Applying configuration...");
    index += 8 + 4;

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
    index += 8 + 4;

    waitButtonInput();

    IOS_HeapFree(0xcaff, cfgBuffer);
    FSA_CloseFile(fsaHandle, cfgHandle);
}

static void option_displayDRCPin(void)
{
    gfx_clear(COLOR_BACKGROUND);

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Display DRC Pin";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Reading DRH mac address...");
    index += 8 + 4;

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

    static const char* symbol_names[] = {
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

    // draw top bar
    gfx_set_font_color(COLOR_PRIMARY);
    const char* title = "Installing WUP";
    gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
    gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "Make sure to place a valid signed WUP directly in 'sd:/install'");
    index += 8 + 4;

    int mcpHandle = IOS_Open("/dev/mcp", 0);
    if (mcpHandle < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to open /dev/mcp: %x", mcpHandle);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Querying install info...");
    index += 8 + 4;

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
    index += 8 + 4;

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
            index += 8 + 4;

            gfx_set_font_color(COLOR_PRIMARY);

            char pin[5] = "";
            res = SCIGetParentalPinCode(pin, sizeof(pin));
            if (res == 1) {
                gfx_printf(16, index, 0, "Parental Pin Code: %s", pin);
            } else {
                gfx_set_font_color(COLOR_ERROR);
                gfx_printf(16, index, 0, "SCIGetParentalPinCode failed: %d", res);
            }
            index += (8 + 4) * 2;

            gfx_set_font_color(COLOR_PRIMARY);

            // draw options

            // Back button
            gfx_draw_rect_filled(16 - 1, index - 1,
                gfx_get_text_width("Back") + 2, 8 + 2,
                !selected ? COLOR_PRIMARY : COLOR_BACKGROUND);

            gfx_set_font_color(!selected ? COLOR_BACKGROUND : COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Back");

            index += 8 + 4;

            // Disable button
            gfx_draw_rect_filled(16 - 1, index - 1,
                gfx_get_text_width("Disable") + 2, 8 + 2,
                selected ? COLOR_PRIMARY : COLOR_BACKGROUND);

            gfx_set_font_color(selected ? COLOR_BACKGROUND : COLOR_PRIMARY);
            gfx_printf(16, index, 0, "Disable");

            index += 8 + 4;

            gfx_set_font_color(COLOR_PRIMARY);

            if (toggled) {
                index += 8 + 4;
                
                gfx_printf(16, index, 0, "SCISetParentalEnable(false): %d", rval);
                index += 8 + 4;

                if (rval != 1) {
                    gfx_set_font_color(COLOR_ERROR);
                    gfx_printf(16, index, 0, "Error!");
                } else {
                    gfx_set_font_color(COLOR_SUCCESS);
                    gfx_printf(16, index, 0, "Success!");
                }

                index += 8 + 4;
            }

            // draw top bar
            gfx_set_font_color(COLOR_PRIMARY);
            const char* title = "Edit Parental Controls";
            gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
            gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

            // draw bottom bar
            gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
            gfx_printf(16, SCREEN_HEIGHT - 16, 0, "EJECT: Navigate ");
            gfx_printf(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 16, 1, "POWER: Choose");

            redraw = 0;
        }
    }
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
            uint32_t index = 16 + 8 + 2 + 8;
            for (int i = 0; i < numMainMenuOptions; i++) {
                gfx_draw_rect_filled(16 - 1, index - 1,
                    gfx_get_text_width(mainMenuOptions[i].name) + 2, 8 + 2,
                    selected == i ? COLOR_PRIMARY : COLOR_BACKGROUND);

                gfx_set_font_color(selected == i ? COLOR_BACKGROUND : COLOR_PRIMARY);
                gfx_printf(16, index, 0, mainMenuOptions[i].name);

                index += 8 + 4;
            }

            gfx_set_font_color(COLOR_PRIMARY);

            // draw top bar
            gfx_set_font_color(COLOR_PRIMARY);
            const char* title = "Wii U Recovery Menu v0.1 by GaryOderNichts";
            gfx_printf((SCREEN_WIDTH / 2) + (gfx_get_text_width(title) / 2), 8, 1, title);
            gfx_draw_rect_filled(8, 16 + 8, SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);

            // draw bottom bar
            gfx_draw_rect_filled(8, SCREEN_HEIGHT - (16 + 8 + 2), SCREEN_WIDTH - 8 * 2, 2, COLOR_SECONDARY);
            gfx_printf(16, SCREEN_HEIGHT - 16, 0, "EJECT: Navigate ");
            gfx_printf(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 16, 1, "POWER: Choose");

            redraw = 0;
        }
    }

    return 0;
}
