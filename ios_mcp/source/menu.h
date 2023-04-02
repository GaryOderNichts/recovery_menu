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

#pragma once

#include <stddef.h>
#include <stdint.h>

#define VERSION_STRING "0.5"

// FSA handle
// Initialized by menuThread().
extern int fsaHandle;

int menuThread(void* arg);

/** Useful functions for submenus **/

typedef enum {
    MenuFlag_ShowTID        = (1U << 0),
    MenuFlag_NoClearScreen  = (1U << 1),
    MenuFlag_ShowGitHubLink = (1U << 2),
} MenuFlags;

typedef struct Menu {
    const char* name;
    union {
        void (*callback)(void);
        uint64_t tid;
    };
} Menu;

/**
 * Draw the top bar.
 * @param title Title
 */
void drawTopBar(const char* title);

/**
 * Draw a menu and wait for the user to select an option.
 * @param title Menu title
 * @param menu Array of menu entries
 * @param count Number of menu entries
 * @param selected Initial selected item index
 * @param flags
 * @param x
 * @param y
 * @return Selected menu entry index
 */
int drawMenu(const char* title, const Menu* menu, size_t count,
        int selected, uint32_t flags, uint32_t x, uint32_t y);

/**
 * Wait for the user to press a button.
 */
void waitButtonInput(void);

void print_error(int index, const char *msg);

void printf_error(int index, const char *format, ...);

/**
 * Initialize the network configuration.
 * @param index [in/out] Starting (and ending) Y position.
 * @return 0 on success; non-zero on error.
 */
int initNetconf(uint32_t* index);

/* Region code string table */
static const char region_tbl[7][4] = {
    "JPN", "USA", "EUR", "AUS",
    "CHN", "KOR", "TWN",
};

/**
 * Get region code information.
 * @param productArea_id Product area ID: 0-6
 * @param gameRegion Bitfield of game regions
 * @return 0 on success; negative on error.
 */
int getRegionInfo(int* productArea_id, int* gameRegion);

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
int read_otp_seeprom(void *buf, int index);
