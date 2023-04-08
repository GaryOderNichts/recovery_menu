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

#include "DumpOtpAndSeeprom.h"

#include "fsa.h"
#include "gfx.h"
#include "menu.h"
#include "utils.h"

#include <stdint.h>

/**
 * Read OTP and SEEPROM.
 *
 * If an error occurs, a message will be displayed and the
 * user will be prompted to press a button.
 *
 * @param buf Buffer (must be 0x600 bytes)
 * @param index Row index for error messages
 * @return 0 on success; non-zero on error.
 */
int read_otp_seeprom(void *buf, int index)
{
    uint8_t* const otp = (uint8_t*)buf;
    uint16_t* const seeprom = (uint16_t*)buf + 0x200;

    int res = IOS_ReadOTP(0, otp, 0x400);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read OTP: %x", res);
        waitButtonInput();
        return res;
    }

    res = EEPROM_Read(0, 0x100, seeprom);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read EEPROM: %x", res);
        waitButtonInput();
        return res;
    }

    return 0;
}

void option_DumpOtpAndSeeprom(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Dumping OTP + SEEPROM...");

    uint32_t index = 16 + 8 + 2 + 8;
    gfx_print(16, index, 0, "Creating otp.bin...");
    index += CHAR_SIZE_DRC_Y + 4;

    void* dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x400, 0x40);
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

        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    gfx_print(16, index, 0, "Reading OTP...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = IOS_ReadOTP(0, dataBuffer, 0x400);
    if (res < 0) {
        gfx_set_font_color(COLOR_ERROR);
        gfx_printf(16, index, 0, "Failed to read OTP: %x", res);
        waitButtonInput();

        FSA_CloseFile(fsaHandle, otpHandle);
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
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
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
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

        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
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
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
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
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    FSA_CloseFile(fsaHandle, seepromHandle);
    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
}
