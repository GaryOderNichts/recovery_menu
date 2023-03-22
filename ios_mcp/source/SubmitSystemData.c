/*
 *   Copyright (C) 2022-2023 GaryOderNichts
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

#include "SubmitSystemData.h"

#include "menu.h"
#include "gfx.h"
#include "imports.h"
#include "mdinfo.h"
#include "netconf.h"
#include "socket.h"
#include "utils.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

// POST data
struct post_data {
    char system_model[16];  // [0x000] seeprom[0xB8]
    char system_serial[16]; // [0x010] seeprom[0xAC] + seeprom[0xB0] - need to mask the last 3 digits
    uint8_t mfg_date[6];    // [0x020] seeprom[0xC4]
    uint8_t productArea;    // [0x026]
    uint8_t gameRegion;     // [0x027]
    uint32_t sec_level;     // [0x028] otp[0x080]
    uint16_t boardType;     // [0x02C] seeprom[0x21]
    uint16_t boardRevision; // [0x02E] seeprom[0x22]
    uint16_t bootSource;    // [0x030] seeprom[0x23]
    uint16_t ddr3Size;      // [0x032] seeprom[0x24]
    uint16_t ddr3Speed;     // [0x034] seeprom[0x25]
    uint16_t sataDevice;    // [0x036] seeprom[0x2C]
    uint16_t consoleType;   // [0x038] seeprom[0x2D]
    uint8_t reserved1[6];   // [0x03A]
    uint8_t reserved2[80];  // [0x040]

    // [0x090]
    struct {
        uint32_t mid_prv;   // [0x090] Manufacturer and product revision
        uint32_t blockSize; // [0x094] Block size
        uint64_t numBlocks; // [0x098] Number of blocks
        char name1[128];    // [0x0A0] Product name
    } mlc;

    // [0x120]
    uint8_t otp_sha256[32]; // [0x120] OTP SHA-256 hash (to prevent duplicates)
};  // size == 0x140 (320)

struct post_data_hashed {
    struct post_data data;
    uint8_t post_sha256[32];    // [0x140] SHA-256 hash of post_data, with adjustments
};  // size == 0x160 (352)

void option_SubmitSystemData(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Submit System Data");

    uint32_t index = 16 + 8 + 2 + 8;

    // parse OTP/SEEPROM for system information
    // 0x000-0x3FF: OTP
    // 0x400-0x5FF: SEEPROM
    // 0x600-0x7FF: post_data_hashed
    void* dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x800, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "Out of memory!");
        waitButtonInput();
        return;
    }
    if (read_otp_seeprom(dataBuffer, index) != 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    static const char *const desc_lines[] = {
        "This will submit statistical data to the developers of recovery_menu,",
        "which will help to determine various statistics about Wii U consoles,",
        "e.g. eMMC manufacturers. The submitted data may be publicly accessible",
        "but personally identifying information will be kept confidential.",
        NULL,
        "Information that will be submitted:",
        "* System model",
        "* System serial number (excluding the last 3 digits)",
        "* Manufacturing date",
        "* Region information",
        "* Security level (keyset), sataDevice, consoleType",
        "* boardType, boardRevision, bootSource, ddr3Size, ddr3Speed",
        "* MLC manufacturer, revision, name, and size",
        "* SHA-256 hash of OTP (to prevent duplicates)",
        NULL,
        "Do you want to submit your console's system data?",
    };
    gfx_set_font_color(COLOR_PRIMARY);
    for (unsigned int i = 0; i < ARRAY_SIZE(desc_lines); i++) {
        if (desc_lines[i]) {
            gfx_print(16, index, 0, desc_lines[i]);
        }
        index += CHAR_SIZE_DRC_Y;
    }
    index += 4;

    static const Menu submitSystemDataOptions[] = {
        {"Cancel", {0} },
        {"Submit Data", {0} },
    };
    int selected = drawMenu("Submit System Data",
        submitSystemDataOptions, ARRAY_SIZE(submitSystemDataOptions), 0,
        MenuFlag_NoClearScreen, 16, index);
    if (selected == 0)
        return;
    index += (CHAR_SIZE_DRC_Y*(ARRAY_SIZE(submitSystemDataOptions)+1)) + 4;

    /** Get the data and submit it. **/
    uint8_t* const otp = (uint8_t*)dataBuffer;
    uint16_t* const seeprom = (uint16_t*)dataBuffer + 0x200;
    struct post_data_hashed* const pdh = (struct post_data_hashed*)dataBuffer + 0x600;
    struct post_data* const pd = &pdh->data;

    // Copy in the POST data.
    memset(pd, 0, sizeof(*pd));
    memcpy(pd->system_model, &seeprom[0xB8], sizeof(pd->system_model));
    memcpy(pd->mfg_date, &seeprom[0xC4], sizeof(pd->mfg_date));
    memcpy(&pd->sec_level, &otp[0x080], sizeof(pd->sec_level));
    pd->boardType = seeprom[0x21];
    pd->boardRevision = seeprom[0x22];
    pd->bootSource = seeprom[0x23];
    pd->sataDevice = seeprom[0x2C];
    pd->consoleType = seeprom[0x2D];

    // System serial number
    // NOTE: Assuming code+serial doesn't exceed 15 chars (plus NULL).
    snprintf(pd->system_serial, sizeof(pd->system_serial), "%s%s",
        (const char*)&seeprom[0xAC], (const char*)&seeprom[0xB0]);
    // Mask the last 3 digits of the system serial number.
    for (unsigned int i = sizeof(pd->system_serial)-1; i > 3; i--) {
        if (pd->system_serial[i] <= 0x20) {
            pd->system_serial[i] = 0;
            continue;
        }
        // Found printable text.
        // Mask the last three digits.
        pd->system_serial[i-0] = '*';
        pd->system_serial[i-1] = '*';
        pd->system_serial[i-2] = '*';
        break;
    }

    // Region information
    int productArea_id, gameRegion;
    int res = getRegionInfo(&productArea_id, &gameRegion);
    if (res == 0) {
        // Region info obtained. (If not obtained, these will be 0.)
        pd->productArea = (uint8_t)productArea_id;
        pd->gameRegion = (uint8_t)gameRegion;
    }

    // MLC information
    MDReadInfo();	// TODO: Check for errors?
    for (unsigned int i = 0; i < 2; i++) {
        // We want the first MLC device.
        MDBlkDrv* drv = MDGetBlkDrv(i);
        if (!drv->registered || drv->params.device_type != SAL_DEVICE_TYPE_MLC) {
            continue;
        }

        pd->mlc.mid_prv = drv->params.mid_prv;
        pd->mlc.numBlocks = drv->params.numBlocks;
        pd->mlc.blockSize = drv->params.blockSize;
        memcpy(pd->mlc.name1, drv->params.name1, sizeof(pd->mlc.name1));
        break;
    }    

    // Hash the OTP ROM
    // TODO: Check for errors.
    uint8_t hash_ctx[IOSC_HASH_CONTEXT_SIZE];
    res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), NULL, 0, IOSC_HASH_FLAGS_INIT, NULL, 0);
    if (res == 0) {
        res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), otp, 0x400, IOSC_HASH_FLAGS_FINALIZE, pd->otp_sha256, sizeof(pd->otp_sha256));
    }

    // Hash the post data.
    // TODO: Check for errors.
    res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), NULL, 0, IOSC_HASH_FLAGS_INIT, NULL, 0);
    if (res == 0) {
        res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), (uint8_t*)pd, sizeof(*pd), IOSC_HASH_FLAGS_UPDATE, NULL, 0);
        if (res == 0) {
            // Add some more stuff to the hash.
            // NOTE: Reusing the OTP buffer here.
            memcpy(otp, desc_lines[0], 64);
            res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), otp, 64, IOSC_HASH_FLAGS_FINALIZE, pdh->post_sha256, sizeof(pdh->post_sha256));
        }
    }

    // Initialize the network.
    // TODO: Combine network init code with wupserver network init code.
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Submit System Data");
    index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Initializing netconf...");
    index += CHAR_SIZE_DRC_Y;

    res = netconf_init();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize netconf: %x", res);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
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

    index += CHAR_SIZE_DRC_Y;

    if (interface == 0xff) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(16, index, 0, "No network connection!");
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Connected using %s", (interface == NET_CFG_INTERFACE_TYPE_WIFI) ? "WIFI" : "ETHERNET");
    index += CHAR_SIZE_DRC_Y;

    uint8_t ip_address[4];
    res = netconf_get_assigned_address(interface, (uint32_t*) ip_address);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to get IP address: %x", res);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "IP address: %u.%u.%u.%u",
        ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    index += CHAR_SIZE_DRC_Y;

    // Initialize sockets.
    // NOTE: Not shutting down sockets later in case wupserver is active.
    res = socketInit();
    if (res <= 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to initialize socketlib: %x", res);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    int httpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpSocket < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "socket() failed: %x", httpSocket);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    // Connect to wiiu.gerbilsoft.com:80
    // FIXME: Need to add gethostbyname().
    // For now, we're hard-coding a Cloudflare IP address.
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 80;
    sockaddr.sin_addr.s_addr = 0xAC43C7F3;  // Cloudflare for wiiu.gerbilsoft.com [TODO: gethostbyname()]

    res = connect(httpSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "connect() failed: %x", res);
        closesocket(httpSocket);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        waitButtonInput();
        return;
    }

    gfx_printf(16, index, 0, "Connected, submitting data...");
    index += CHAR_SIZE_DRC_Y;

    // To reduce processing requirements here, we'll submit a simple HTTP/1.0 request
    // without encryption.
    static const char http_req[] = "POST /add-system.php HTTP/1.0\r\n"
        "Host: wiiu.gerbilsoft.com\r\n"
        "User-Agent: Wii U Recovery Menu/" VERSION_STRING "\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: 352\r\n\r\n";

    // Send the HTTP request.
    // TODO: Do we need a newline after the POST data?
    send(httpSocket, http_req, sizeof(http_req)-1, 0);
    send(httpSocket, pdh, sizeof(*pdh), 0);
    usleep(1000 * 1000);

    // TODO: Check for a return error code.
    closesocket(httpSocket);
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "System data submitted successfully.");
    index += CHAR_SIZE_DRC_Y;
    static const char link_prefix[] = "Check out the Wii U console database at:";
    gfx_print(16, index, 0, link_prefix);
    static const int xpos = 16 + CHAR_SIZE_DRC_X * sizeof(link_prefix);
    gfx_set_font_color(COLOR_LINK);
    // TODO: Underline attribute instead of double-printing?
    gfx_print(xpos, index, 0, "https://wiiu.gerbilsoft.com/");
    gfx_print(xpos, index, 0, "____________________________");

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    waitButtonInput();
}
