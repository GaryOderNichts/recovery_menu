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

#include "SystemInformation.h"

#include "fsa.h"
#include "gfx.h"
#include "mcp_misc.h"
#include "menu.h"
#include "mdinfo.h"

#include <stdint.h>
#include <string.h>

void option_SystemInformation(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("System Information");

    static const uint32_t y_pos_top = 16 + 8 + 2 + 8;
    static const uint32_t x_pos_left = 16;

    uint32_t y_pos = y_pos_top;
    uint32_t x_pos = x_pos_left;

    // parse OTP/SEEPROM for system information
    // 0x000-0x3FF: OTP
    // 0x400-0x5FF: SEEPROM
    // 0x600-0x7FF: misc for version.bin
    void *dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x800, 0x40);
    if (!dataBuffer) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_print(x_pos, y_pos, 0, "Out of memory!");
        waitButtonInput();
        return;
    }
    if (read_otp_seeprom(dataBuffer, y_pos) != 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }
    uint8_t* const otp = (uint8_t*)dataBuffer;
    uint16_t* const seeprom = (uint16_t*)dataBuffer + 0x200;

    /** Column 1: Model, serial number, mfg date, keyset, BSP revision **/

    // NOTE: gfx_printf() does not support precision specifiers.
    // We'll have to ensure the strings are terminated manually.
    char buf1[17];
    char buf2[17];

    memcpy(buf1, &seeprom[0xB8], 16);
    buf1[16] = '\0';
    gfx_printf(x_pos, y_pos, 0, "Model:    %s", buf1);
    y_pos += CHAR_SIZE_DRC_Y + 4;

    memcpy(buf1, &seeprom[0xAC], 8);
    buf1[8] = '\0';
    memcpy(buf2, &seeprom[0xB0], 16);
    buf2[16] = '\0';
    gfx_printf(x_pos, y_pos, 0, "Serial:   %s%s", buf1, buf2);
    y_pos += CHAR_SIZE_DRC_Y + 4;

    if (seeprom[0xC4] != 0) {
        gfx_printf(x_pos, y_pos, 0, "Mfg Date: %04x/%02x/%02x %02x:%02x",
            seeprom[0xC4], seeprom[0xC5] >> 8, seeprom[0xC5] & 0xFF,
            seeprom[0xC6] >> 8, seeprom[0xC6] & 0xFF);
        y_pos += CHAR_SIZE_DRC_Y + 4;
    }

    static const char keyset_tbl[4][8] = {"Factory", "Debug", "Retail", "Invalid"};
    gfx_printf(x_pos, y_pos, 0, "Keyset:   %s", keyset_tbl[(otp[0x080] & 0x18) >> 3]);
    y_pos += CHAR_SIZE_DRC_Y + 4;

    uint32_t bsp_rev;
    int res = bspGetHardwareVersion(&bsp_rev);
    if (res == 0) {
        gfx_printf(x_pos, y_pos, 0, "BSP rev:  0x%08lX", bsp_rev);
    } else {
        gfx_printf(x_pos, y_pos, 0, "BSP rev:  ERR=%d", res);
    }

    /** Column 2: boardType, sataDevice, consoleType **/
    y_pos = y_pos_top;
    x_pos += (CHAR_SIZE_DRC_X * 32);

    memcpy(buf1, &seeprom[0x21], 2);
    buf1[2] = '\0';
    gfx_printf(x_pos, y_pos, 0, "boardType:   %s", buf1);
    y_pos += CHAR_SIZE_DRC_Y + 4;

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
        gfx_printf(x_pos, y_pos, 0, "sataDevice:  %u (%s)", sataDevice_id, sataDevice);
    } else {
        gfx_printf(x_pos, y_pos, 0, "sataDevice:  %u", sataDevice_id);
    }
    y_pos += CHAR_SIZE_DRC_Y + 4;

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
        gfx_printf(x_pos, y_pos, 0, "consoleType: %u (%s)", consoleType_id, consoleType);
    } else {
        gfx_printf(x_pos, y_pos, 0, "consoleType: %u", consoleType_id);
    }

    /** Column 3: productArea, gameRegion, Wii U Menu Version **/
    y_pos = y_pos_top;
    x_pos += (CHAR_SIZE_DRC_X * 32);

    int productArea_id = 0; // 0-6, matches title ID
    int gameRegion;
    res = getRegionInfo(&productArea_id, &gameRegion);
    if (res >= 0) {
        // productArea is a single region.
        gfx_printf(x_pos, y_pos, 0, "productArea: %s", region_tbl[productArea_id]);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        // gameRegion is multiple regions.
        gfx_printf(x_pos, y_pos, 0, "gameRegion:  %s %s %s %s %s %s",
            (gameRegion & MCP_REGION_JAPAN)  ? region_tbl[0] : "---",
            (gameRegion & MCP_REGION_USA)    ? region_tbl[1] : "---",
            (gameRegion & MCP_REGION_EUROPE) ? region_tbl[2] : "---",
            (gameRegion & MCP_REGION_CHINA)  ? region_tbl[4] : "---",
            (gameRegion & MCP_REGION_KOREA)  ? region_tbl[5] : "---",
            (gameRegion & MCP_REGION_TAIWAN) ? region_tbl[6] : "---");
        y_pos += CHAR_SIZE_DRC_Y + 4;
    }

    // Wii U Menu version
    // NOTE: If productArea doesn't match the installed Wii U Menu,
    // the Wii U Menu version won't be displayed.
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
                gfx_printf(x_pos, y_pos, 0, "Wii U Menu version: %lu.%lu.%lu %c",
                    version_bin->major, version_bin->minor, version_bin->revision, version_bin->region);
                y_pos += CHAR_SIZE_DRC_Y + 4;
            }
        }
    }

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);

    /** Memory device info **/
    y_pos = y_pos_top + ((CHAR_SIZE_DRC_Y + 4) * 6);
    x_pos = x_pos_left;

    // Read info about IOS-FS' memory devices
    res = MDReadInfo();
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(x_pos, y_pos, 0, "Failed to read memory device info: %x", res);
        waitButtonInput();
        return;
    }

    for (unsigned int i = 0; i < 2; i++) {
        MDBlkDrv* drv = MDGetBlkDrv(i);
        // Ignore unregistered drivers and the SD Card
        if (!drv->registered || drv->params.device_type == SAL_DEVICE_TYPE_SD_CARD) {
            continue;
        }

        const char* deviceType = "Unknown";
        if (drv->params.device_type == SAL_DEVICE_TYPE_MLC) {
            deviceType = "MLC";
        }

        gfx_printf(x_pos, y_pos, 0, "Memory device %d (Type 0x%lx '%s'):", i, drv->params.device_type, deviceType);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        const uint16_t mid = drv->params.mid_prv >> 16;

        // Find the manufacturer based on the manufacturer ID
        // If you have a console with a manufacturer not listed here, please make a PR
        const char* manufacturer = "Unknown";
        if (mid == 0x11) {
            manufacturer = "Toshiba";
        } else if (mid == 0x15) {
            manufacturer = "Samsung";
        } else if (mid == 0x90) {
            manufacturer = "Hynix";
        }

        gfx_printf(x_pos, y_pos, 0, "  Manufacturer ID: 0x%02x (%s)", mid, manufacturer);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        uint16_t prv = drv->params.mid_prv & 0xff;

        gfx_printf(x_pos, y_pos, 0, "  Product revision: 0x%02x (%d.%d)", prv, prv >> 4, prv & 0xf);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        gfx_printf(x_pos, y_pos, 0, "  Product name: %s", drv->params.name1);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        uint32_t totalSizeMiB = (uint32_t) ((drv->params.numBlocks * drv->params.blockSize) / 1024ull / 1024ull);
        gfx_printf(x_pos, y_pos, 0, "  Size: %llu x %lu (%lu MiB)", drv->params.numBlocks, drv->params.blockSize, totalSizeMiB);
        y_pos += CHAR_SIZE_DRC_Y + 4;

        // Display the full CID and CSD registers
        uint32_t cid[4];
        res = MDGetCID(drv->deviceId, cid);
        if (res == 0) {
            gfx_printf(x_pos, y_pos, 0, "  CID: %08lx%08lx%08lx%08lx", cid[0], cid[1], cid[2], cid[3]);
            y_pos += CHAR_SIZE_DRC_Y + 4;
        }

        uint32_t csd[4];
        res = MDGetCSD(drv->deviceId, csd);
        if (res == 0) {
            gfx_printf(x_pos, y_pos, 0, "  CSD: %08lx%08lx%08lx%08lx", csd[0], csd[1], csd[2], csd[3]);
            y_pos += CHAR_SIZE_DRC_Y + 4;
        }
    }

    waitButtonInput();
}
