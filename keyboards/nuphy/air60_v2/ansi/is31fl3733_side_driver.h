// Copyright 2025 adi4086 (https://github.com/adi4086)
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "is31fl3733.h"

#define SIDE_LINE  5
#define SIDE_INDEX (RGB_MATRIX_LED_COUNT - 2 * SIDE_LINE) // side LEDs come last
#define LEFT_SIDE  1
#define RIGHT_SIDE 2

bool is_side_is31fl3733_off(void);
void side_is31fl3733_set_color_strip(uint8_t side, uint8_t r, uint8_t g, uint8_t b);
void side_is31fl3733_dim_strip(uint8_t side, uint8_t shift);
void is31fl3733_scale_range(uint8_t from, uint8_t to, uint8_t lv);
