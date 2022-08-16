#include "gfx.h"
#include "imports.h"

#include <stdio.h>
#include <stdarg.h>

#include "font_bin.h"

static uint32_t *const TV_FRAMEBUFFER = (uint32_t*)(0x14000000 + 0x3500000);
#define TV_HEIGHT 720
#define TV_STRIDE 1280

static uint32_t *const DRC_FRAMEBUFFER = (uint32_t*)(0x14000000 + 0x38c0000);
#define DRC_HEIGHT 480
#define DRC_STRIDE 896

#define CHAR_SIZE_X 8
#define CHAR_SIZE_Y 8

// Default font color is white.
static uint32_t font_color = 0xFFFFFFFF;

void gfx_clear(uint32_t col)
{
#ifdef DC_INIT
    // both DC configurations use XRGB instead of RGBX
    col >>= 8;
#endif

    for (uint32_t i = 0; i < TV_STRIDE * TV_HEIGHT; i++) {
        TV_FRAMEBUFFER[i] = col;
    }

    for (uint32_t i = 0; i < DRC_STRIDE * DRC_HEIGHT; i++) {
        DRC_FRAMEBUFFER[i] = col;
    }
}

void gfx_draw_pixel(uint32_t x, uint32_t y, uint32_t col)
{
#ifdef DC_INIT
    // both DC configurations use XRGB instead of RGBX
    col >>= 8;
#endif

    // put pixel in the drc buffer
    uint32_t i = x + y * DRC_STRIDE;
    if (i < DRC_STRIDE * DRC_HEIGHT) {
        DRC_FRAMEBUFFER[i] = col;
    }

    // scale and put pixel in the tv buffer
    for (uint32_t yy = (y * 1.5f); yy < ((y * 1.5f) + 1); yy++) {
        for (uint32_t xx = (x * 1.5f); xx < ((x * 1.5f) + 1); xx++) {
            uint32_t i = xx + yy * TV_STRIDE;
            if (i < TV_STRIDE * TV_HEIGHT) {
                TV_FRAMEBUFFER[i] = col;
            }
        }
    }
}

void gfx_draw_rect_filled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t col)
{
    // DRC fill: normal scale
    uint32_t *p = DRC_FRAMEBUFFER + (y * DRC_STRIDE) + x;
    uint32_t stride_diff = DRC_STRIDE - w;

    for (uint32_t hcnt = h; hcnt > 0; hcnt--) {
        for (uint32_t wcnt = w; wcnt > 0; wcnt--) {
            *p++ = col;
        }
        p += stride_diff;
    }

    // TV fill: 1.5x scale
    p = TV_FRAMEBUFFER + ((uint32_t)(y * 1.5) * TV_STRIDE) + (uint32_t)(x * 1.5);
    stride_diff = TV_STRIDE - (w * 1.5);

    for (uint32_t hcnt = h * 1.5; hcnt > 0; hcnt--) {
        for (uint32_t wcnt = w * 1.5; wcnt > 0; wcnt--) {
            *p++ = col;
        }
        p += stride_diff;
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

    // DRC blit: normal scale
    const uint8_t *charData = &font_bin[(CHAR_SIZE_X * CHAR_SIZE_Y * c) / 8];
    uint32_t *p = DRC_FRAMEBUFFER + (y * DRC_STRIDE) + x;
    unsigned int stride_diff = DRC_STRIDE - CHAR_SIZE_X;

    for (uint32_t hcnt = CHAR_SIZE_Y; hcnt > 0; hcnt--) {
        uint8_t v = *charData++;
        for (uint32_t wcnt = CHAR_SIZE_X; wcnt > 0; wcnt--, v >>= 1) {
            if (v & 1) {
                *p = font_color;
            }
            p++;
        }
        p += stride_diff;
    }

    // TV blit: 1.5x scale
    // TODO: Add a 12x24 font instead of this awful scaling method.
    charData = &font_bin[(CHAR_SIZE_X * CHAR_SIZE_Y * c) / 8];
    p = TV_FRAMEBUFFER + ((uint32_t)(y * 1.5) * TV_STRIDE) + (uint32_t)(x * 1.5);
    stride_diff = TV_STRIDE - (CHAR_SIZE_X * 1.5);

    for (uint32_t hcnt = CHAR_SIZE_Y * 1.5; hcnt > 0; hcnt--) {
        uint8_t v = *charData;
        for (uint32_t wcnt = CHAR_SIZE_X * 1.5; wcnt > 0; wcnt--) {
            if (v & 1) {
                *p = font_color;
            }
            if (!(wcnt & 1)) v >>= 1;   // HACK: Bad scaling method.
            p++;
        }
        if (!(hcnt & 1)) charData++;   // HACK: Bad scaling method.
        p += stride_diff;
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
