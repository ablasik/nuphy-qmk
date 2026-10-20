#pragma once
#include <stdint.h>
#define DYNAMIC_KEYMAP_MACRO_COUNT 16
#define DYNAMIC_KEYMAP_LAYER_COUNT 8
uint8_t dynamic_keymap_macro_get_count(void);
uint16_t dynamic_keymap_macro_get_buffer_size(void);
void dynamic_keymap_macro_get_buffer(uint16_t offset, uint16_t size, uint8_t *data);
