#include "LoadNetConf.h"

#include "menu.h"
#include "gfx.h"
#include "netconf.h"
#include "fsa.h"

#include <string.h>

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

void option_LoadNetConf(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Loading network configuration...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Initializing netconf...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = netconf_init();
    if (res <= 0) {
        printf_error(index, "Failed to initialize netconf: %x", res);
        return;
    }

    gfx_print(16, index, 0, "Reading network.cfg...");
    index += CHAR_SIZE_DRC_Y + 4;

    int cfgHandle;
    res = FSA_OpenFile(fsaHandle, "/vol/storage_recovsd/network.cfg", "r", &cfgHandle);
    if (res < 0) {
        printf_error(index, "Failed to open network.cfg: %x", res);
        return;
    }

    FSStat stat;
    res = FSA_StatFile(fsaHandle, cfgHandle, &stat);
    if (res < 0) {
        FSA_CloseFile(fsaHandle, cfgHandle);
        printf_error(index, "Failed to stat file: %x", res);
        return;
    }

    char* cfgBuffer = (char*) IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, stat.size + 1, 0x40);
    if (!cfgBuffer) {
        FSA_CloseFile(fsaHandle, cfgHandle);
        print_error(index, "Out of memory!");
        return;
    }

    cfgBuffer[stat.size] = '\0';

    res = FSA_ReadFile(fsaHandle, cfgBuffer, 1, stat.size, cfgHandle, 0);
    if (res != stat.size) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, cfgBuffer);
        FSA_CloseFile(fsaHandle, cfgHandle);
        printf_error(index, "Failed to read file: %x", res);
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
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, cfgBuffer);
        FSA_CloseFile(fsaHandle, cfgHandle);
        printf_error(index, "Failed to apply configuration: %x", res);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    index += CHAR_SIZE_DRC_Y + 4;

    waitButtonInput();

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, cfgBuffer);
    FSA_CloseFile(fsaHandle, cfgHandle);
}
