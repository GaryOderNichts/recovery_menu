#pragma once

#include <stdint.h>

// visible screen sizes
#define SCREEN_WIDTH 854
#define SCREEN_HEIGHT 480

void gfx_clear(uint32_t color);

void gfx_draw_pixel(uint32_t x, uint32_t y, uint32_t color);

void gfx_draw_rect_filled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void gfx_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t borderSize, uint32_t col);

void gfx_set_font_color(uint32_t col);

uint32_t gfx_get_text_width(const char* string);

void gfx_print(uint32_t x, uint32_t y, int alignRight, const char* string);

void gfx_printf(uint32_t x, uint32_t y, int alignRight, const char* format, ...);
