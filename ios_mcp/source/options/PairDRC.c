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

#include "PairDRC.h"

#include "ccr.h"
#include "gfx.h"
#include "menu.h"
#include "utils.h"

#include <stdint.h>
#include <unistd.h>

// Default to 30 seconds for the DRC pairing timeout.
#define DRC_PAIRING_TIMEOUT 30

void option_PairDRC(void)
{
    gfx_clear(COLOR_BACKGROUND);
    drawTopBar("Pairing Gamepad...");
    setNotificationLED(NOTIF_LED_RED_BLINKING, 0);

    uint32_t index = 16 + 8 + 2 + 8;

    int res = CCRCDCSetup();
    if (res < 0) {
        printf_error(index, "CCRCDCSetup() failed: %x", res);
        return;
    }

    gfx_print(16, index, 0, "Get pincode...");
    index += CHAR_SIZE_DRC_Y + 4;

    uint8_t pincode[4];
    res = CCRSysGetPincode(pincode);
    if (res < 0) {
        printf_error(index, "Failed to get pincode: %x", res);
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
    gfx_set_font_color(COLOR_PRIMARY);
    index += CHAR_SIZE_DRC_Y + 4;

    // DRC pairing timeout
    int timeout = DRC_PAIRING_TIMEOUT;

    // display a "this gamepad has already been paired" message, if a gamepad is alrady connected
    res = CCRCDCDevicePing(CCR_DEST_DRC0);
    if (res == 0) {
        gfx_print(16, index, 0, "Gamepad already connected, displaying message...");
        index += CHAR_SIZE_DRC_Y + 4;

        uint16_t args[2] = { 0, timeout + 5 };
        CCRCDCSysDrcDisplayMessage(CCR_DEST_DRC0, args);
    }

    usleep(1000 * 100);

    uint8_t mute = 0xff;
    CCRCDCPerSetLcdMute(CCR_DEST_DRC0, &mute);

    gfx_print(16, index, 0, "Starting pairing...");
    index += CHAR_SIZE_DRC_Y + 4;

    res = CCRSysStartPairing(CCR_DEST_DRC0, timeout, pincode);
    if (res < 0) {
        printf_error(index, "Failed to start pairing: %x", res);
        return;
    }

    uint32_t status;
    while (timeout--) {
        res = CCRCDCWpsStatus(&status);
        if (res < 0) {
            CCRCDCWpsStop();
            printf_error(index, "Failed to get status: %x", res);
            return;
        }

        if (status == 0) {
            // paired
            break;
        } else if (status == 2) {
            // pairing
        } else if (status == 1) {
            // searching
        } else {
            // error
            break;
        }

        gfx_printf(16, index, GfxPrintFlag_ClearBG, "Waiting for Gamepad... (%lx) (Timeout: %d) ", status, timeout);
        usleep(1000 * 1000);
    }

    if (status != 0 || timeout <= 0) {
        CCRCDCWpsStop();
        printf_error(index, "Failed to pair GamePad: %lx", res);
        return;
    }

    setNotificationLED(NOTIF_LED_PURPLE, 0);
    gfx_set_font_color(COLOR_SUCCESS);
    gfx_print(16, index, GfxPrintFlag_ClearBG, "Paired GamePad");
    waitButtonInput();
}
