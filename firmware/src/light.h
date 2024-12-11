/*
 * WS2812B Lights Control (Base + Left and Right Gimbals)
 * WHowe <github.com/whowechina>
 */

#ifndef LIGHT_H
#define LIGHT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

void light_init();
void light_update();

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v);
uint32_t load_color(const rgb_hsv_t *color);

void light_set_spinner(uint32_t color, bool hid);
void light_set_button(uint8_t index, uint32_t color, bool hid);
void light_set(uint8_t index, uint32_t color);

void light_set_cab(uint32_t color, bool hid);

#if RGB_ORDER == GRB
#define RGB(r, g, b) ((g << 16) | (r << 8) | b)
#elif RGB_ORDER == RGB
#define RGB(r, g, b) ((r << 16) | (g << 8) | b)
#endif

#define RED 0
#define GREEN 85
#define BLUE 170
#define YELLOW 42
#define CYAN 127
#define MAGENTA 212
#define ORANGE 21
#define PINK 234
#define PURPLE 191
#define LIME 64

#endif
