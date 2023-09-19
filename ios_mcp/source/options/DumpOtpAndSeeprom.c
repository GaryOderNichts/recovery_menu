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
        printf_error(index, "Failed to read OTP: %x", res);
        return res;
    }

    res = EEPROM_Read(0, 0x100, seeprom);
    if (res < 0) {
        printf_error(index, "Failed to read EEPROM: %x", res);
        return res;
    }

    return 0;
}

/**
 * Write a file to SD.
 *
 * If an error occurs, a message will be displayed and the
 * user will be prompted to press a button.
 *
 * @param index     [in] Current Y position (will be updated)
 * @param fsaHandle [in] Open FSA handle
 * @param filename  [in] Filename, e.g. "otp.bin".
 * @param buf       [in] File data
 * @param size      [in] Size of buf
 * @return 0 on success; non-zero on error.
 */
static int write_file_to_sd(uint32_t *index, int fsaHandle, const char *filename, void *buf, size_t size)
{
    char path[128];
    snprintf(path, sizeof(path), "/vol/storage_recovsd/%s", filename);

    gfx_printf(16, *index, 0, "Creating %s...", filename);
    *index += CHAR_SIZE_DRC_Y + 4;

    int fileHandle;
    int res = FSA_OpenFile(fsaHandle, path, "w", &fileHandle);
    if (res < 0) {
        printf_error(*index, "Failed to create %s: %x", filename, res);
        *index += CHAR_SIZE_DRC_Y + 4;
        return res;
    }

    res = FSA_WriteFile(fsaHandle, buf, 1, size, fileHandle, 0);
    FSA_CloseFile(fsaHandle, fileHandle);
    if (res != size) {
        printf_error(*index, "Failed to write %s: %x", filename, res);
        *index += CHAR_SIZE_DRC_Y + 4;
        return res;
    }

    return 0;
}

void option_DumpOtpAndSeeprom(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Dumping OTP + SEEPROM...");
    setNotificationLED(NOTIF_LED_RED_BLINKING, 0);
    uint32_t index = 16 + 8 + 2 + 8;

    void* dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, 0x400, 0x40);
    if (!dataBuffer) {
        print_error(index, "Out of memory!");
        return;
    }

    /** OTP **/

    gfx_print(16, index, 0, "Reading OTP...");
    index += CHAR_SIZE_DRC_Y + 4;

    int res = IOS_ReadOTP(0, dataBuffer, 0x400);
    if (res < 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        printf_error(index, "Failed to read OTP: %x", res);
        return;
    }

    res = write_file_to_sd(&index, fsaHandle, "otp.bin", dataBuffer, 0x400);
    if (res != 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    /** SEEPROM **/

    gfx_print(16, index, 0, "Reading SEEPROM...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = EEPROM_Read(0, 0x100, (uint16_t*) dataBuffer);
    if (res < 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        printf_error(index, "Failed to read EEPROM: %x", res);
        return;
    }

    res = write_file_to_sd(&index, fsaHandle, "seeprom.bin", dataBuffer, 0x200);
    if (res != 0) {
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
        return;
    }

    /** All done! **/

    setNotificationLED(NOTIF_LED_PURPLE, 0);
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, 0, "Done!");
    waitButtonInput();

    IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
}
