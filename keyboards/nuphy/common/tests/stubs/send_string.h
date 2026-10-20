#pragma once
#include <stdint.h>
#define SS_QMK_PREFIX 1
#define SS_TAP_CODE 1
#define SS_DOWN_CODE 2
#define SS_UP_CODE 3
#define SS_DELAY_CODE 4
#define pgm_read_byte(p) (*(const uint8_t *)(p))
extern const uint8_t ascii_to_keycode_lut[128];
extern const uint8_t ascii_to_shift_lut[16],ascii_to_altgr_lut[16],ascii_to_dead_lut[16];
