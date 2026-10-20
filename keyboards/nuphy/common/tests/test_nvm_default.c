// SPDX-License-Identifier: GPL-2.0-or-later
// Regression: keyboards that don't override the new hook keep their full buffer.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "eeprom.h"
#include "nvm_dynamic_keymap.h"

static uint8_t memory[4096];
uint8_t eeprom_read_byte(const uint8_t *addr) { assert((uintptr_t)addr < 4096); return memory[(uintptr_t)addr]; }
void eeprom_update_byte(uint8_t *addr, uint8_t v) { assert((uintptr_t)addr < 4096); memory[(uintptr_t)addr] = v; }
void eeprom_read_block(void *buf, const void *addr, size_t size) { memcpy(buf, memory + (uintptr_t)addr, size); }
void eeprom_update_block(const void *buf, void *addr, size_t size) { assert((uintptr_t)addr + size <= 4096); memcpy(memory + (uintptr_t)addr, buf, size); }

int main(void) {
    assert(nvm_dynamic_keymap_macro_size() == 4096 - 73 - 8 * 6 * 17 * 2);
    uint32_t size = nvm_dynamic_keymap_macro_size();
    uint8_t value = 0x42;
    nvm_dynamic_keymap_macro_update_buffer(size - 1, 1, &value);
    assert(memory[4095] == value);
    nvm_dynamic_keymap_macro_reset();
    assert(memory[4095] == 0);
    puts("PASS: default EEPROM hook preserves full VIA macro capacity");
}
