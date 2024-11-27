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

#endif
