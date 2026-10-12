// Copyright 2025 adi4086 (https://github.com/adi4086)
// SPDX-License-Identifier: GPL-2.0-or-later
#include "quantum.h"
#include "ws2812.h"
#include "ws2812_driver.h"

/*
 * Key LEDs: QMK's WS2812 bitbang driver behind a thin custom RGB matrix driver.
 * It only sends a frame when a colour changed (sending one takes ~2.5 ms with
 * interrupts off, long enough to drop bytes from the RF module's UART), and
 * keeps a copy of the colours for the battery-saver dimming.
 */
static rgb_t leds[RGB_MATRIX_LED_COUNT];
bool         flush_rgb_leds = false;

static void set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < 0 || index >= RGB_MATRIX_LED_COUNT) { return; }
    rgb_t *c = &leds[index];
    if (c->r != r || c->g != g || c->b != b) { flush_rgb_leds = true; }
    *c = (rgb_t){.r = r, .g = g, .b = b};
    ws2812_set_color(index, r, g, b);
}

static void set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        set_color(i, r, g, b);
    }
}

static void flush(void) {
    if (!flush_rgb_leds) { return; }
    ws2812_flush();
    flush_rgb_leds = false;
}

const rgb_matrix_driver_t rgb_matrix_driver = {
    .init          = ws2812_init,
    .flush         = flush,
    .set_color     = set_color,
    .set_color_all = set_color_all,
};

void ws2812_scale_range(uint8_t from, uint8_t to, uint8_t lv) {
    for (uint8_t i = from; i < to; i++) {
        set_color(i, (leds[i].r * lv) >> 8, (leds[i].g * lv) >> 8, (leds[i].b * lv) >> 8);
    }
}

/*
 * Side strips: a second WS2812 chain on DRIVER_SIDE_PIN, bit-banged with the
 * same timing as QMK's driver (which only handles one pin).
 */
#define NUMBER_NOPS    6
#define CYCLES_PER_SEC (CPU_CLOCK / NUMBER_NOPS * 0.4) // QMK's STM32F0 NOP fudge
#define NS_PER_CYCLE   (1000000000L / CYCLES_PER_SEC)
#define wait_ns(x)                                                   \
    do {                                                             \
        for (int i = 0; i < (x) / NS_PER_CYCLE; i++) {               \
            __asm__ volatile("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"); \
        }                                                            \
    } while (0)

static rgb_t side_leds[SIDE_LED_NUM];
bool         flush_side_leds = false;

static void side_send_byte(uint8_t byte) {
    for (uint8_t bit = 0; bit < 8; bit++) { // MSB first
        bool one = byte & (1 << (7 - bit));
        gpio_write_pin_high(DRIVER_SIDE_PIN);
        wait_ns(one ? WS2812_T1H : WS2812_T0H);
        gpio_write_pin_low(DRIVER_SIDE_PIN);
        wait_ns(one ? WS2812_T1L : WS2812_T0L);
    }
}

bool is_side_ws2812_off(void) {
    for (uint8_t i = 0; i < SIDE_LED_NUM; i++) {
        if (side_leds[i].r || side_leds[i].g || side_leds[i].b) { return false; }
    }
    return true;
}

void side_ws2812_set_color(int i, uint8_t r, uint8_t g, uint8_t b) {
    r >>= 2, g >>= 2, b >>= 2; // the side LEDs are much brighter than the keys
    rgb_t *c = &side_leds[i];
    if (c->r != r || c->g != g || c->b != b) { flush_side_leds = true; }
    *c = (rgb_t){.r = r, .g = g, .b = b};
}

void side_ws2812_set_color_strip(uint8_t side, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t start = side == RIGHT_SIDE ? SIDE_LINE : 0;
    uint8_t end   = side == LEFT_SIDE ? SIDE_LINE : SIDE_LED_NUM;
    for (uint8_t i = start; i < end; i++) {
        side_ws2812_set_color(i, r, g, b);
    }
}

void side_ws2812_dim_strip(uint8_t side, uint8_t shift) {
    uint8_t start = side == RIGHT_SIDE ? SIDE_LINE : 0;
    for (uint8_t i = start; i < start + SIDE_LINE; i++) {
        rgb_t *c = &side_leds[i];
        if (c->r | c->g | c->b) { flush_side_leds = true; }
        c->r >>= shift, c->g >>= shift, c->b >>= shift;
    }
}

void side_ws2812_flush(void) {
    if (!flush_side_leds) { return; }
    chSysLock(); // timing critical
    for (uint8_t i = 0; i < SIDE_LED_NUM; i++) { // GRB order
        side_send_byte(side_leds[i].g);
        side_send_byte(side_leds[i].r);
        side_send_byte(side_leds[i].b);
    }
    wait_ns(1000 * WS2812_TRST_US);
    chSysUnlock();
    flush_side_leds = false;
}
