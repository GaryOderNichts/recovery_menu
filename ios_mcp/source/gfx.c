#include "gfx.h"
#include "imports.h"

#include <stdio.h>
#include <stdarg.h>

static uint32_t *const TV_FRAMEBUFFER = (uint32_t*)(0x14000000 + 0x3500000);
#define TV_HEIGHT 720
#define TV_STRIDE 1280

static uint32_t *const DRC_FRAMEBUFFER = (uint32_t*)(0x14000000 + 0x38c0000);
#define DRC_HEIGHT 480
#define DRC_STRIDE 896

#define CHAR_SIZE_TV_X 12
#define CHAR_SIZE_TV_Y 24

// Default font color is white.
static uint32_t font_color = 0xFFFFFFFF;

// Terminus fonts (8x16 for DRC, 12x24 for TV)
// NOTE: Allocated using IOS_HeapAllocAligned().
#include "font_bin.h"
#include "minilzo/minilzo.h"
static terminus_font *font = NULL;

extern int lzo_res;
int lzo_res;
extern unsigned int lzo_data_len;
unsigned int lzo_data_len;

int gfx_init_font(void)
{
    if (font)
        return 0;

    font = IOS_HeapAllocAligned(0xcaff, sizeof(*font), 0x40);
    if (!font) {
        printf("Memory allocation for the font buffer failed!\n");
        return -1;
    }

    lzo_uint data_len = sizeof(*font);
    int res = lzo1x_decompress_safe(terminus_lzo1x, sizeof(terminus_lzo1x),
        (lzo_bytep)font, &data_len, NULL);
    if (res != LZO_E_OK || data_len != sizeof(*font)) {
        // LZO decompression failed.
        printf("lzo1x_decompress() failed: res == %d, data_len == %lu\n", res, data_len);
        IOS_HeapFree(0xcaff, font);
        font = NULL;
        return -2;
    }

    lzo_res = res;
    lzo_data_len = data_len;
    // LZO decompression succeeded.
    return 0;
}

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
    for (i = 0; *string; i++, string++);
    return i * CHAR_SIZE_DRC_X;
}

static void gfx_draw_char(uint32_t x, uint32_t y, char c)
{
    // Skip anything outside of [32,128), since the font doesn't have it.
    if (c < 32 || c >= 128)
        return;
    c -= 32;

    // DRC blit: Terminus 8x16 bold
    const uint8_t *charDRC = font->ter_u16b[(unsigned char)c];
    uint32_t *p = DRC_FRAMEBUFFER + (y * DRC_STRIDE) + x;
    unsigned int stride_diff = DRC_STRIDE - CHAR_SIZE_DRC_X;

    for (uint32_t hcnt = CHAR_SIZE_DRC_Y; hcnt > 0; hcnt--) {
        uint8_t v = *charDRC++;
        for (uint32_t wcnt = CHAR_SIZE_DRC_X; wcnt > 0; wcnt--, v >>= 1) {
            if (v & 1) {
                *p = font_color;
            }
            p++;
        }
        p += stride_diff;
    }

    // TV blit: Terminus 12x24 bold
    const uint16_t *charTV = font->ter_u24b[(unsigned char)c];
    p = TV_FRAMEBUFFER + ((uint32_t)(y * 1.5) * TV_STRIDE) + (uint32_t)(x * 1.5);
    stride_diff = TV_STRIDE - CHAR_SIZE_TV_X;

    for (uint32_t hcnt = CHAR_SIZE_TV_Y; hcnt > 0; hcnt--) {
        uint16_t v = *charTV++;
        for (uint32_t wcnt = CHAR_SIZE_TV_X; wcnt > 0; wcnt--, v >>= 1) {
            if (v & 1) {
                *p = font_color;
            }
            p++;
        }
        p += stride_diff;
    }
}

void gfx_print(uint32_t x, uint32_t y, int alignRight, const char* string)
{
    if (alignRight) {
        x -= gfx_get_text_width(string);
    }

    for (; *string != '\0'; string++, x += CHAR_SIZE_DRC_X) {
        char chr = *string;
        if ((unsigned char)chr >= 32 && (unsigned char)chr <= 128) {
            gfx_draw_char(x, y, chr);
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
