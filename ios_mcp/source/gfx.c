#include "gfx.h"
#include "imports.h"

#include <stdio.h>
#include <stdarg.h>

#include "font_bin.h"

static uint32_t* framebuffer;
static uint32_t width;
static uint32_t height;

#define CHAR_SIZE_X 8
#define CHAR_SIZE_Y 8

static uint32_t font_color = 0xffffffff;

void gfx_init(void* fb, uint32_t w, uint32_t h)
{
    framebuffer = (uint32_t*) fb;
    width = w;
    height = h;
}

void gfx_clear(uint32_t col)
{
    // both DC configurations use XRGB instead of RGBX
    col >>= 8;

    for (uint32_t i = 0; i < width * height; i++) {
        framebuffer[i] = col;
    }
}

void gfx_draw_pixel(uint32_t x, uint32_t y, uint32_t col)
{
    // both DC configurations use XRGB instead of RGBX
    col >>= 8;

    // scale and put pixel in the tv buffer
    for (uint32_t yy = (y * 1.5f); yy < ((y * 1.5f) + 1); yy++) {
        for (uint32_t xx = (x * 1.5f); xx < ((x * 1.5f) + 1); xx++) {
            uint32_t i = xx + yy * width;
            if (i < width * height) {
                framebuffer[i] = col;
            }
        }
    }
}

void gfx_draw_rect_filled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col)
{
    for (uint32_t yy = y; yy < y + h; yy++) {
        for (uint32_t xx = x; xx < x + w; xx++) {
            gfx_draw_pixel(xx, yy, col);
        }
    }
}

void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t borderSize, uint32_t col)
{
    gfx_draw_rect_filled(x, y, w, borderSize, col);
    gfx_draw_rect_filled(x, y + h - borderSize, w, borderSize, col);
    gfx_draw_rect_filled(x, y, borderSize, h, col);
    gfx_draw_rect_filled(x + w - borderSize, y, borderSize, h, col);
}

void gfx_set_font_color(uint32_t col)
{
    font_color = col;
}

uint32_t gfx_get_text_width(const char* string)
{
    uint32_t i;
    for (i = 0; string[i]; i++);
    return i * CHAR_SIZE_X;
}

static void gfx_draw_char(uint32_t x, uint32_t y, char c)
{
    if(c < 32) {
        return;
    }

    c -= 32;

    const uint8_t* charData = &font_bin[(CHAR_SIZE_X * CHAR_SIZE_Y * c) / 8];

    for (uint32_t i = 0; i < CHAR_SIZE_Y; i++) {
        uint8_t v = *(charData++);

        for (uint32_t j = 0; j < CHAR_SIZE_X; j++) {
            if(v & (1 << j)) {
                gfx_draw_pixel(x + j, y + i, font_color);
            }
        }
    }
}

void gfx_print(uint32_t x, uint32_t y, int alignRight, const char* string)
{
    if (alignRight) {
        x -= gfx_get_text_width(string);
    }

    for (uint32_t i = 0; string[i]; i++) {
        if(string[i] >= 32 && string[i] < 128) {
            gfx_draw_char(x + i * CHAR_SIZE_X, y, string[i]);
        }
    }
}

void gfx_printf(uint32_t x, uint32_t y, int alignRight, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[0x100];

    vsnprintf(buffer, sizeof(buffer), format, args);
    gfx_print(x, y, alignRight, buffer);

    va_end(args);
}
