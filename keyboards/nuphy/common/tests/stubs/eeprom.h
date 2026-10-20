#pragma once
#include <stdint.h>
#include <stddef.h>
#define TOTAL_EEPROM_BYTE_COUNT 4096
uint8_t eeprom_read_byte(const uint8_t *addr);
void eeprom_read_block(void *dst, const void *addr, size_t size);
void eeprom_update_byte(uint8_t *addr, uint8_t v);
void eeprom_update_block(const void *src, void *addr, size_t size);
