#include "is31fl3733_side_driver.h"

/**
 * @brief is_side_is31fl3733_off
 */

bool is_side_is31fl3733_off(void)
{
    is31fl3733_led_t led;
    for (int i = SIDE_INDEX; i < SIDE_INDEX + 10; i++) {
        memcpy_P(&led, (&g_is31fl3733_leds[i]), sizeof(led));
        if (driver_buffers[led.driver].pwm_buffer[led.r] != 0 || driver_buffers[led.driver].pwm_buffer[led.g] != 0 || driver_buffers[led.driver].pwm_buffer[led.b] != 0) {
            return false;
        }
    }
    return true;
}

void side_is31fl3733_set_color_strip(uint8_t side, uint8_t r, uint8_t g, uint8_t b) {
    // side = 1 => left
    // side = 2 => right
    // side = 3 => both
    uint8_t start = 0;
    uint8_t end = SIDE_LINE * 2;

    if (side == LEFT_SIDE)  { end = end - SIDE_LINE; }
    if (side == RIGHT_SIDE) { start = start + SIDE_LINE; }

    for (uint8_t i = start; i < end; i++) {
        rgb_matrix_set_color(SIDE_INDEX + i, r, g, b);
    }
}


/**
 * @brief  Dim what the side effect drew on one strip (value >> shift), so an
 *         indicator drawn on top stands out whatever the effect's colour.
 */
void side_is31fl3733_dim_strip(uint8_t side, uint8_t shift) {
    is31fl3733_led_t led;
    uint8_t start = side == RIGHT_SIDE ? SIDE_LINE : 0;
    for (uint8_t i = start; i < start + SIDE_LINE; i++) {
        memcpy_P(&led, (&g_is31fl3733_leds[SIDE_INDEX + i]), sizeof(led));
        uint8_t *pwm = driver_buffers[led.driver].pwm_buffer;
        rgb_matrix_set_color(SIDE_INDEX + i, pwm[led.r] >> shift, pwm[led.g] >> shift, pwm[led.b] >> shift);
    }
}
