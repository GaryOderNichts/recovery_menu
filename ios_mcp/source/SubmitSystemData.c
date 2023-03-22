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
#include "utils.h"

#include <stdint.h>
#include <string.h>

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
        res = IOSC_GenerateHash(hash_ctx, sizeof(hash_ctx), (uint8_t*)pd, sizeof(*pd), IOSC_HASH_FLAGS_UPDATE, pdh->post_sha256, sizeof(pdh->post_sha256));
        if (res == 0) {
            // Mix in a 64-byte "secret".
        }
    }

    // Print a hexdump of the post data, plus final hash.
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Submit System Data");
    index = 16 + 8 + 2 + 8;
    gfx_printf(16, index, 0, "sizeof(*pdh) == %u", sizeof(*pdh));
    index += CHAR_SIZE_DRC_Y;
    for (unsigned int i = 0; i < sizeof(*pdh); i += 16) {
        const uint8_t *const x = ((const uint8_t*)pd) + i;
        gfx_printf(16, index, 0, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X",
            i, x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15]);
        index += CHAR_SIZE_DRC_Y;
    }

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    waitButtonInput();
}
