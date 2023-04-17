#pragma once

#include <stdint.h>

// some common colors used throughout the application
#define COLOR_BACKGROUND    0x000000ff
#define COLOR_PRIMARY       0xffffffff
#define COLOR_SECONDARY     0x3478e4ff
#define COLOR_SUCCESS       0x00ff00ff
#define COLOR_ERROR         0xff0000ff
#define COLOR_LINK          0x5555ffff

// visible screen sizes
#define SCREEN_WIDTH 854
#define SCREEN_HEIGHT 480

// All drawing is handled in terms of the DRC screen.
// The TV screen is scaled by 1.5 for 1280x720.
#define CHAR_SIZE_DRC_X 8
#define CHAR_SIZE_DRC_Y 16

int gfx_init_font(void);

void gfx_clear(uint32_t color);

void gfx_draw_pixel(uint32_t x, uint32_t y, uint32_t color);

void gfx_draw_rect_filled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t borderSize, uint32_t col);

void gfx_set_font_color(uint32_t col);

uint32_t gfx_get_text_width(const char* string);

typedef enum GfxPrintFlags {
    GfxPrintFlag_AlignRight     = (1U << 0),
    GfxPrintFlag_ClearBG        = (1U << 1),
    GfxPrintFlag_Underline      = (1U << 2),
    GfxPrintFlag_NewlinePlus4   = (1U << 3),    // Newlines should have +4
} GfxPrintFlags;

/**
 * Print text to the TV and DRC.
 *
 * Handles automatic newlines. ('\n' to go the next line)
 * NOTE: Automatic newlines might not work properly with AlignRight.
 *
 * @param x X position
 * @param y Y position
 * @param gfxPrintFlags GfxPrintFlags
 * @param str Text to print
 * @return New Y position. (If no '\n' characters were included, same as the input y value.)
 */
uint32_t gfx_print(uint32_t x, uint32_t y, uint32_t gfxPrintFlags, const char* str);

/**
 * Print text to the TV and DRC using printf()-style formatting.
 *
 * Handles automatic newlines. ('\n' to go the next line)
 * NOTE: Automatic newlines might not work properly with AlignRight.
 *
 * @param x X position
 * @param y Y position
 * @param gfxPrintFlags GfxPrintFlags
 * @param format printf()-style format
 * @param ... Format parameters
 * @return New Y position. (If no '\n' characters were included, same as the input y value.)
 */
__attribute__((format(printf, 4, 5)))
uint32_t gfx_printf(uint32_t x, uint32_t y, uint32_t gfxPrintFlags, const char* format, ...);
