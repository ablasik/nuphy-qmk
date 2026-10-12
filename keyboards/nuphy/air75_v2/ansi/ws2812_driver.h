// Copyright 2025 adi4086 (https://github.com/adi4086)
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "color.h"

#define LEFT_SIDE    1
#define RIGHT_SIDE   2
#define SIDE_LINE    6
#define SIDE_LED_NUM 12

extern bool flush_rgb_leds;  // set to force the next key LED flush (e.g. after power-on)
extern bool flush_side_leds; // same for the side strips

bool is_side_ws2812_off(void);
void side_ws2812_set_color(int i, uint8_t r, uint8_t g, uint8_t b);
void side_ws2812_set_color_strip(uint8_t side, uint8_t r, uint8_t g, uint8_t b);
void side_ws2812_flush(void);
void side_ws2812_dim_strip(uint8_t side, uint8_t shift);
void ws2812_scale_range(uint8_t from, uint8_t to, uint8_t lv);
