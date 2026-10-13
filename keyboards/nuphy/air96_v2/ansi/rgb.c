/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "user_kb.h"
#include "ansi.h"
#include "rgb_table.h"
#include "mcu_pwr.h"
#include "is31fl3733_side_driver.h"

/* side rgb mode */
enum {
    SIDE_WAVE = 0,
    SIDE_MIX,
    SIDE_STATIC,
    SIDE_BREATH,
    SIDE_OFF,
};

uint8_t side_play_point       = 0;

uint8_t rgb_color             = 0;
uint8_t rgb_start_led         = 0;
uint8_t rgb_end_led           = 0;
uint16_t rgb_show_time        = 0;
uint32_t rgb_indicator_timer  = 0;
uint8_t  low_bat_level        = 20;

uint8_t sys_light             = 3;

hsv_t hsv = { .h = 0, .s = 255, .v = 255};
rgb_t current_rgb = {.r = 0x00, .g = 0x00, .b = 0x00};

const uint8_t side_speed_table[5][5] = {
    [SIDE_WAVE]   = {14, 19, 25, 32, 40},
    [SIDE_MIX]    = {14, 19, 25, 32, 40},
    [SIDE_STATIC] = {50, 50, 50, 50, 50},
    [SIDE_BREATH] = {14, 19, 25, 32, 40},
    [SIDE_OFF]    = {50, 50, 50, 50, 50},
};

// Levels 0-4 as stock (4 = 96, the default). Stock topped out at 160; the
// IS31FL3733 is already at full current, so the top steps go up to full PWM.
const uint8_t side_light_table[7] = {
    0,
    16,
    32,
    64,
    96,
    160,
    255,
};

const uint8_t side_led_index_tab[SIDE_LINE][2] =
    {
        {SIDE_INDEX + 4, SIDE_INDEX + 5},
        {SIDE_INDEX + 3, SIDE_INDEX + 6},
        {SIDE_INDEX + 2, SIDE_INDEX + 7},
        {SIDE_INDEX + 1, SIDE_INDEX + 8},
        {SIDE_INDEX + 0, SIDE_INDEX + 9},
};

void set_sys_light(void) {
    sys_light = user_config.ee_side_light > 5 ? 1 : (3 - user_config.ee_side_light / 2);
    current_rgb.r = current_rgb.r / sys_light;
    current_rgb.g = current_rgb.g / sys_light;
    current_rgb.b = current_rgb.b / sys_light;
}

static void side_off_mode_show(void);
void rgb_matrix_update_pwm_buffers(void);

/*
 * Indicator language for the side strips (5 LEDs each, pos 0 = top).
 * Persistent = only states that change what the keys do; everything else is an
 * event shown briefly. Each indicator sits on the side of the key it belongs to.
 *   left strip  (Caps / Win side): Caps Lock / Caps Word, Win lock, game mode;
 *                briefly the whole strip for connecting / pairing / connected
 *   right strip (numpad side):     Num Lock; battery gauge on demand, low-battery
 *                pulse, and a 2s "toast" when a setting changes
 * Only the LEDs that carry information change; the rest keep the side effect.
 */
#ifndef SIDE_ROW0_IS_BOTTOM
// Row 0 is the top: stock battery bar lit rows 3-4 (LEDs 108/109) when low, filling from
// the bottom. If the gauge (Fn+\\) fills from the top on your board, set this to 1.
#    define SIDE_ROW0_IS_BOTTOM 0
#endif

static void side_pos(uint8_t side, uint8_t pos, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t row = SIDE_ROW0_IS_BOTTOM ? (SIDE_LINE - 1 - pos) : pos;
    rgb_matrix_set_color(side_led_index_tab[row][side == LEFT_SIDE ? 0 : 1], r, g, b);
}

// same as side_pos, scaled to indicator brightness (follows side light level)
static void side_pos_ind(uint8_t side, uint8_t pos, uint8_t r, uint8_t g, uint8_t b) {
    current_rgb.r = r, current_rgb.g = g, current_rgb.b = b;
    set_sys_light();
    side_pos(side, pos, current_rgb.r, current_rgb.g, current_rgb.b);
}

// side_pos_ind at a 0..255 level, for breathing/pulsing indicators
static void side_pos_lv(uint8_t side, uint8_t pos, uint8_t lv, uint8_t r, uint8_t g, uint8_t b) {
    side_pos_ind(side, pos, (r * lv) >> 8, (g * lv) >> 8, (b * lv) >> 8);
}

// 0..255 triangle wave with the given period, for breathing indicators
static uint8_t pulse(uint16_t period) {
    uint16_t t = timer_read32() % period;
    uint16_t half = period / 2;
    return t < half ? (t * 255) / half : ((period - t) * 255) / half;
}

static uint32_t toast_timer = 0;
static uint8_t  toast_n, toast_of;
static rgb_t    toast_rgb;

/**
 * @brief  Show a setting on the right strip for 2s: n of `of` LEDs lit from the
 *         bottom, the remaining choices dimmed so the scale is visible.
 */
void side_toast(uint8_t n, uint8_t of, uint8_t r, uint8_t g, uint8_t b) {
    toast_n     = n;
    toast_of    = of;
    toast_rgb   = (rgb_t){.r = r, .g = g, .b = b};
    uint32_t now = timer_read32();
    toast_timer  = now ? now : 1; // 0 = no toast. Not `now | 1`: that's 1 ms in the future half the time, which reads as long expired
}

static bool side_toast_show(void) {
    if (toast_timer == 0) { return false; }
    if (timer_elapsed32(toast_timer) > 2000) {
        toast_timer = 0;
        return false;
    }
    for (uint8_t i = 0; i < SIDE_LINE; i++) { // i counts from the bottom
        uint8_t div = i < toast_n ? 1 : (i < toast_of ? 8 : 0);
        if (div) {
            side_pos_ind(RIGHT_SIDE, SIDE_LINE - 1 - i, toast_rgb.r / div, toast_rgb.g / div, toast_rgb.b / div);
        } else {
            side_pos(RIGHT_SIDE, SIDE_LINE - 1 - i, RGB_OFF);
        }
    }
    return true;
}

void clear_rgb(void) {
    rgb_matrix_set_color_all(RGB_OFF);
    rgb_matrix_update_pwm_buffers();
}

void side_is31fl3733_refresh(void) {
    if (!is_side_is31fl3733_off() || (user_config.ee_side_light > 0 && user_config.ee_side_mode != SIDE_OFF)) {
        pwr_led_on(); // power on side LED before refresh
    }
}

/**
 * @brief  Adjusting the brightness of side lights.
 * @param  bright: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_light_control(uint8_t bright) {
    if (bright) {
        if (user_config.ee_side_light < SIDE_BRIGHT_MAX) { user_config.ee_side_light++; }
    } else {
        if (user_config.ee_side_light > 0) { user_config.ee_side_light--; }
    }
#ifndef NO_DEBUG
    dprintf("side matrix light_control [NOEEPROM]: %d\n", user_config.ee_side_light);
#endif
}

/**
 * @brief  Adjusting the speed of side lights.
 * @param  fast: 0 - decrease, 1 - increase.
 * @note  save to eeprom.
 */
void side_speed_control(uint8_t fast) {
    if (fast) {
        if (user_config.ee_side_speed > 0) { user_config.ee_side_speed--; }
    } else {
        if (user_config.ee_side_speed < SIDE_SPEED_MAX) { user_config.ee_side_speed++; }
    }
#ifndef NO_DEBUG
    dprintf("side matrix speed_control [NOEEPROM]: %d\n", user_config.ee_side_speed);
#endif
}

/**
 * @brief  Switch to the next color of side lights.
 * @param  color: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_colour_control(uint8_t color) {
    if (user_config.ee_side_mode != SIDE_WAVE) {
        if (user_config.ee_side_rgb) {
            user_config.ee_side_rgb    = 0;
            user_config.ee_side_colour = game_mode_enable;
        }
    }

    if (user_config.ee_side_rgb) {
        user_config.ee_side_rgb    = 0;
        user_config.ee_side_colour = color ? game_mode_enable : SIDE_COLOUR_MAX - 1;
    } else {
        if (color) { user_config.ee_side_colour++; }
        else { user_config.ee_side_colour--; }

        if (user_config.ee_side_colour >= SIDE_COLOUR_MAX) {
            user_config.ee_side_rgb    = 1;
            user_config.ee_side_colour = game_mode_enable;
        }
    }
#ifndef NO_DEBUG
    dprintf("side matrix colour_control [NOEEPROM]: %d rgb: %d\n", user_config.ee_side_colour, user_config.ee_side_rgb);
#endif
}

/**
 * @brief  Change the color mode of side lights.
 * @param  dir: 0 - prev, 1 - next.
 * @note  save to eeprom.
 */
void side_mode_control(uint8_t dir) {
    if (dir) {
        user_config.ee_side_mode = (user_config.ee_side_mode + 1) % (SIDE_OFF + 1);
    } else {
        if (user_config.ee_side_mode > 0) { user_config.ee_side_mode--; }
        else { user_config.ee_side_mode = SIDE_OFF; }
    }
    side_play_point          = 0;
#ifndef NO_DEBUG
    dprintf("side matrix mode_control [NOEEPROM]: %d\n", user_config.ee_side_mode);
#endif
}

void signal_sleep(void) {
    static const uint8_t link_rgb[2][3] = {{IND_BT}, {IND_24G}};
    const uint8_t *c = link_rgb[dev_info.link_mode == LINK_RF_24];
    uint8_t r = c[0], g = c[1], b = c[2];
    pwr_led_on();
    wait_ms(10);
    side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, r, g, b);
    rgb_matrix_update_pwm_buffers();
    // going to sleep: both strips drain from the top down
    for (uint8_t pos = 0; pos < SIDE_LINE; pos++) {
        wait_ms(60);
        side_pos(LEFT_SIDE, pos, RGB_OFF);
        side_pos(RIGHT_SIDE, pos, RGB_OFF);
        rgb_matrix_update_pwm_buffers();
    }
}

/**
 * @brief   system switch led show
 */
void sys_sw_led_show(void) {
    if (sys_show_timer == 0) { return; }
    sys_show_timer = 0;
    // Mac: Cmd key glows white. Win: Win key glows blue. (colour_lib: 9 white, 6 blue)
    signal_rgb_led(dev_info.sys_sw_state == SYS_SW_MAC ? 9 : 6, led_idx.KC_LGUI, UINT8_MAX, 3000);
}

/**
 * @brief  sleep_sw_led_show
 */
void sleep_sw_led_show(void) {
    if (sleep_show_timer == 0) { return; }
    sleep_show_timer = 0;
    // like every setting toast: 3 = full saving, 2 = lights only, 1 = never
    switch (user_config.sleep_mode) {
        case 1: side_toast(3, 3, IND_WHITE); break;
        case 2: side_toast(2, 3, IND_WHITE); break;
        default: side_toast(1, 3, IND_WHITE); break;
    }
}

/**
 * @brief  host system led indicate.
 */
void sys_led_show(void) {
    bool caps  = host_keyboard_led_state().caps_lock;
    bool cword = is_caps_word_on() && user_config.caps_word_enable;
    bool num   = user_config.numlock_state == 1 && host_keyboard_led_state().num_lock;

    // The side effect can be any colour (rainbow wave passes cyan, red, magenta...):
    // dim the rest of a strip to 1/4 while it carries an indicator, so it reads.
    if (caps || cword || keymap_config.no_gui || game_mode_enable) { side_is31fl3733_dim_strip(LEFT_SIDE, 2); }
    if (num) { side_is31fl3733_dim_strip(RIGHT_SIDE, 2); }

    // left: things that change what the left-hand keys do
    if (caps) {
        side_pos_ind(LEFT_SIDE, 0, IND_CAPS);                // Caps Lock
    } else if (cword) {
        side_pos_lv(LEFT_SIDE, 0, pulse(1200), IND_CAPS);    // Caps Word: breathing
    }
    if (keymap_config.no_gui) {
        side_pos_ind(LEFT_SIDE, 1, IND_LOCKED);              // Win/Cmd locked
    }
    if (game_mode_enable) {
        side_pos_ind(LEFT_SIDE, 2, IND_GAME);                // game mode
    }
    // right: next to the numpad
    if (num) {
        side_pos_ind(RIGHT_SIDE, 0, IND_WHITE);              // Num Lock
    }
}

static uint8_t breathe_data(uint8_t step) {
    uint8_t value     = 0;
    uint8_t adjust    = 8;
    uint8_t max_value = BREATHE_TAB_LEN - 1;
    if (step < max_value / 2) {
        value = ((step * adjust / 21) * step) / adjust + step;
    } else if (step  < BREATHE_TAB_LEN / 2  + 1) {
        value = 255;
    } else {
        value = ((((max_value - step) * adjust) / 21) * (max_value - step)) / adjust + (max_value - step);
    }
    return value;
}

/**
 * @brief  light_point_playing.
 * @param trend:
 * @param step:
 * @param len:
 * @param point:
 */
static void light_point_playing(uint8_t trend, uint8_t step, uint8_t len, uint8_t *point) {
    if (trend) {
        *point += step;
        if (*point > 254 && len == 0) { *point = 0; }
        else if (*point >= len) { *point -= len; }
    } else {
        *point -= step;
        if (*point < 1 && len == 0) { *point = 255; }
        else if (*point >= len) { *point = len - (255 - *point) - 1; }
    }
}

/**
 * @brief  count_rgb_light.
 * @param light_temp:
 */
static void count_rgb_light(uint8_t light_temp) {
    uint16_t temp;

    temp   = (light_temp)*current_rgb.r + current_rgb.r;
    current_rgb.r = temp >> 8;

    temp   = (light_temp)*current_rgb.g + current_rgb.g;
    current_rgb.g = temp >> 8;

    temp   = (light_temp)*current_rgb.b + current_rgb.b;
    current_rgb.b = temp >> 8;
}

/**
 * @brief  side_wave_mode_show.
 */
static void side_wave_mode_show(void) {
    uint8_t play_index;

    if (user_config.ee_side_rgb) {
        light_point_playing(0, 6, 0, &side_play_point);
    } else {
        light_point_playing(0, 2, BREATHE_TAB_LEN, &side_play_point);
    }

    play_index = side_play_point;
    for (uint8_t i = 0; i < SIDE_LINE; i++) {
        if (user_config.ee_side_rgb) {
            hsv.h = play_index;
            current_rgb = hsv_to_rgb_nocie(hsv);

            light_point_playing(1, 32, 0, &play_index);
        } else {
            current_rgb.r = colour_lib[user_config.ee_side_colour][0];
            current_rgb.g = colour_lib[user_config.ee_side_colour][1];
            current_rgb.b = colour_lib[user_config.ee_side_colour][2];
            light_point_playing(1,12,BREATHE_TAB_LEN,&play_index);
            count_rgb_light(breathe_data(play_index));
        }
        count_rgb_light(side_light_table[user_config.ee_side_light]);

        for (uint8_t j = 0; j < 2; j++) {
            rgb_matrix_set_color(side_led_index_tab[i][j], current_rgb.r, current_rgb.g, current_rgb.b);
        }
    }
}

/**
 * @brief  side_spectrum_mode_show.
 */
static void side_spectrum_mode_show(void) {
    light_point_playing(0, 2, 0, &side_play_point);
    hsv.h = side_play_point;
    current_rgb   = hsv_to_rgb_nocie(hsv);

    count_rgb_light(side_light_table[user_config.ee_side_light]);
    side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}


/**
 * @brief  side_breathe_mode_show.
 */
static void side_breathe_mode_show(void) {
    light_point_playing(1,1,BREATHE_TAB_LEN,&side_play_point);

    current_rgb.r = colour_lib[user_config.ee_side_colour][0];
    current_rgb.g = colour_lib[user_config.ee_side_colour][1];
    current_rgb.b = colour_lib[user_config.ee_side_colour][2];

    count_rgb_light(breathe_data(side_play_point));
    count_rgb_light(side_light_table[user_config.ee_side_light]);
    side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}

/**
 * @brief  side_static_mode_show.
 */
static void side_static_mode_show(void) {
    current_rgb.r = colour_lib[user_config.ee_side_colour][0];
    current_rgb.g = colour_lib[user_config.ee_side_colour][1];
    current_rgb.b = colour_lib[user_config.ee_side_colour][2];

    count_rgb_light(side_light_table[user_config.ee_side_light]);
    side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, current_rgb.r, current_rgb.g, current_rgb.b);
}

/**
 * @brief  side_off_mode_show.
 */
static void side_off_mode_show(void) {
    side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, RGB_OFF);
}

/**
 * @brief  side_one_control
 */
void side_one_control(void)
{
    user_config.ee_side_one = (user_config.ee_side_one + 1) % 4;
    side_one_timer = 0;
#ifndef NO_DEBUG
    dprintf("side matrix side_control [NOEEPROM]: %d\n", user_config.ee_side_one);
#endif
}

/**
 * @brief  side_one_show
 */
static void side_one_show(void)
{
    if (game_mode_enable || user_config.ee_side_one == 0 || user_config.ee_side_light == 0) { return; }
    static uint8_t my_side = RIGHT_SIDE;

    if (user_config.ee_side_one != LEFT_SIDE + RIGHT_SIDE) {
        side_is31fl3733_set_color_strip(user_config.ee_side_one, RGB_OFF);
    } else {
        if (side_one_timer < 50 ) {
            side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, RGB_OFF);
            return;
        } else if (side_one_timer > 6000 ) {
            side_one_timer = 50;
            my_side = my_side == RIGHT_SIDE ? LEFT_SIDE : RIGHT_SIDE;
        }
        side_is31fl3733_set_color_strip(my_side, RGB_OFF);
    }
}

/**
 * @brief  rf state indicate
 */
void rf_led_show(void) {
    static uint32_t rf_blink_timer = 0;

    if (!rf_blink_cnt && rf_link_show_time >= RF_LINK_SHOW_TIME) { return; }

    // colour = transport (the slot number lights up on keys 1-4)
    static const uint8_t link_rgb[3][3] = {{IND_BT}, {IND_24G}, {IND_USB}};
    const uint8_t *c = link_rgb[dev_info.link_mode == LINK_USB ? 2 : dev_info.link_mode == LINK_RF_24 ? 1 : 0];
    uint8_t r = c[0], g = c[1], b = c[2];

    uint8_t level = 255; // connected: solid
    if (rf_blink_cnt) {
        uint16_t period = dev_info.rf_state == RF_PAIRING ? RF_LED_PAIR_PERIOD : RF_LED_LINK_PERIOD * 2;
        if (dev_info.rf_state == RF_PAIRING) {
            level = timer_elapsed32(rf_blink_timer) < period / 2 ? 255 : 0; // pairing: fast blink
        } else {
            level = pulse(period); // connecting: breathe
        }
        if (timer_elapsed32(rf_blink_timer) >= period) {
            rf_blink_cnt--;
            rf_blink_timer = timer_read32();
        }
    }
    r = (r * level) >> 8, g = (g * level) >> 8, b = (b * level) >> 8;

    // a connection event briefly takes the whole left strip
    for (uint8_t i = 0; i < SIDE_LINE; i++) {
        side_pos_ind(LEFT_SIDE, i, r, g, b);
    }
    current_rgb.r = r, current_rgb.g = g, current_rgb.b = b;
    set_sys_light();

    // light up corresponding BT/RF key
    if (dev_info.link_mode <= LINK_BT_3) {
        uint8_t my_pos = dev_info.link_mode == LINK_RF_24 ? 4 : dev_info.link_mode;
        if (rf_link_show_time > RF_LINK_SHOW_TIME - 10) {
            rgb_matrix_set_color(led_idx.LINK[my_pos - 1], RGB_OFF);
        } else {
            rgb_required = 1;
            rgb_matrix_set_color(led_idx.LINK[my_pos - 1], current_rgb.r, current_rgb.g, current_rgb.b);
        }
    }
}

/**
 * @brief  bat_num_led.
 */
void bat_num_led(void) {
    if (!f_bat_num_show) { return; }
    uint8_t bat_percent = dev_info.rf_battery;
    uint8_t bat_pct = bat_percent / 10;
    uint8_t r, g, b;

    rgb_required = 1;

    // set color
    if (bat_percent < low_bat_level) {
        r = 0xff; g = 0x00; b = 0x00;
    }
    else if (bat_percent <= 50) {
        r = 0xff; g = 0x40; b = 0x00;
    }
    else if (bat_percent <= 80) {
        r = 0xff; g = 0xff; b = 0x00;
    } else {
        r = 0x00; g = 0xff; b = 0x00;
    }

    if (bat_percent % 10 == 0) { bat_pct--; }

    for(uint8_t i=0; i < bat_pct; i++) {
        rgb_matrix_set_color(led_idx.NUM[i], r, g, b);
    }
    // set percent

    if (bat_percent % 10 == 0) {
        r = 0x00; g = 0xff; b = 0x00;
    } else if (bat_percent % 10 <= 3) {
        r = 0xff; g = 0x00; b = 0x00;
    } else if (bat_percent % 10 <= 6) {
        r = 0xff; g = 0x40; b = 0x00;
    } else {
        r = 0x00; g = 0xff; b = 0x00;
    }

    rgb_matrix_set_color(led_idx.NUM[bat_pct], r, g, b);

}

/**
 * @brief  Battery level indicator
 */
void bat_percent_led(bool charging)
{
    uint8_t pct = dev_info.rf_battery;
    // 1-5 LEDs from the bottom, one per 20%
    uint8_t n   = 1 + (pct > 20) + (pct > 40) + (pct > 60) + (pct > 80);
    // colour bands as on NuPhy's Halo: green > 80%, yellow > 50%, orange > 20%, red
    static const uint8_t bands[4][3] = {{IND_BAT_HI}, {IND_BAT_MID}, {IND_BAT_LO}, {IND_LOCKED}};
    const uint8_t *c = bands[pct > 80 ? 0 : pct > 50 ? 1 : pct > 20 ? 2 : 3];

    for (uint8_t i = 0; i < SIDE_LINE; i++) { // i counts from the bottom
        uint8_t pos = SIDE_LINE - 1 - i;
        if (i >= n) {
            side_pos(RIGHT_SIDE, pos, RGB_OFF);
        } else if (charging && i == n - 1) {
            side_pos_lv(RIGHT_SIDE, pos, pulse(1500), c[0], c[1], c[2]); // charging: top segment breathes
        } else {
            side_pos_ind(RIGHT_SIDE, pos, c[0], c[1], c[2]);
        }
    }
}

/**
 * @brief  battery state indicate
 */
void bat_led_show(void)
{
    static bool bat_show_flag   = 1;
    static bool bat_show_breath = 0;
    static bool f_init          = 1;

    static uint32_t bat_show_time  = 0;

    static uint32_t bat_sts_debounce = 0;
    static uint8_t  charge_state     = 0;

    if (f_init) {
        f_init        = 0;
        bat_show_time = timer_read32();
        charge_state  = dev_info.rf_charge;
    }

    bool charging = dev_info.rf_charge == 0x03;
    bool low      = dev_info.rf_battery < low_bat_level && !charging;

    if (game_mode_enable) {
        if (low) { side_pos(RIGHT_SIDE, SIDE_LINE - 1, 0x40, 0x00, 0x00); }
        return;
    }

    if (charge_state != dev_info.rf_charge) {
        if (timer_elapsed32(bat_sts_debounce) > 1000) {
            if ((charge_state & 0x01) == 0 && (dev_info.rf_charge & 0x01) != 0) {
                bat_show_flag   = true;
                bat_show_breath = true;
                bat_show_time   = timer_read32();
            }
            charge_state = dev_info.rf_charge;
        }
    } else {
        bat_sts_debounce = timer_read32();
        if (timer_elapsed32(bat_show_time) > 5000) {
            bat_show_flag   = false;
            bat_show_breath = false;
        }
        if (charge_state == 0x03) {
            bat_show_breath = true;
        } else if (charge_state & 0x01) {
            dev_info.rf_battery = 100;
        }
    }

    if (f_bat_hold || bat_show_flag) {
        bat_percent_led(charging || bat_show_breath);
    } else if (low) {
        // low battery: only the bottom LED pulses red; the rest keeps the effect
        side_pos_lv(RIGHT_SIDE, SIDE_LINE - 1, pulse(2000), IND_LOCKED);
    }
}

/**
 * @brief  device_reset_show.
 */
void device_reset_show(void)
{
    pwr_led_on();
    for (uint8_t blink_cnt = 0; blink_cnt < 3; blink_cnt++) {
        rgb_matrix_set_color_all(RGB_WHITE);
        rgb_matrix_update_pwm_buffers();
        wait_ms(200);
        clear_rgb();
        wait_ms(200);
    }
}

/**
 * @brief  device_reset_init.
 */
void device_reset_init(void)
{
    side_play_point  = 0;
    game_mode_enable = 0;
    f_bat_hold       = false;

    rgb_matrix_enable_noeeprom();
    user_config_reset();
}

/**
 * @brief  rgb test
 */

void rgb_test_show(void) {
    // open power control
    pwr_led_on();

    for (uint8_t i = 0; i < SIDE_COLOUR_MAX; i++) {
        rgb_matrix_set_color_all(colour_lib[i][0], colour_lib[i][1], colour_lib[i][2]);
        side_is31fl3733_set_color_strip(LEFT_SIDE + RIGHT_SIDE, colour_lib[i][0], colour_lib[i][1], colour_lib[i][2]);
        rgb_matrix_update_pwm_buffers();
        wait_ms(500);
    }
    clear_rgb();
}


void signal_rgb_led(uint8_t selected_color, uint8_t start_led, uint8_t end_led, uint16_t show_time) {
    rgb_color           = selected_color;
    rgb_start_led       = start_led;
    rgb_end_led         = end_led > RGB_MATRIX_LED_COUNT ? start_led : end_led;
    rgb_show_time       = show_time;
    rgb_indicator_timer = timer_read32();
}

void rgb_led_indicator(void) {
    if (rgb_show_time ==  0) { return; }
    if (timer_elapsed32(rgb_indicator_timer) < rgb_show_time || rgb_show_time == UINT16_MAX) {
        current_rgb.r = colour_lib[rgb_color][0];
        current_rgb.g = colour_lib[rgb_color][1];
        current_rgb.b = colour_lib[rgb_color][2];

        rgb_required  = 2;
        for (uint8_t i = rgb_start_led; i <= rgb_end_led; i++) {
            rgb_matrix_set_color(i, current_rgb.r, current_rgb.g, current_rgb.b);
        }
    } else {
        for (uint8_t i = rgb_start_led; i <= rgb_end_led; i++) {
            rgb_matrix_set_color(i, RGB_OFF);
        }
        rgb_show_time       = 0;
        rgb_indicator_timer = 0;
    }
}

void caps_word_show(void) {
    static bool caps_word_rgb_on = 0;
    if (!is_caps_word_on() || game_mode_enable || !user_config.caps_word_enable) {
        if (caps_word_rgb_on) {
            caps_word_rgb_on = 0;
            rgb_matrix_set_color(led_idx.KC_CAPS, RGB_OFF);
        }
        return;
    } else {
        rgb_required     = 2;
        caps_word_rgb_on = 1;
        rgb_matrix_set_color(led_idx.KC_CAPS, RGB_CYAN);
    }
}


void numlock_rgb_show(void) {
    static bool num_lock_rgb_on = 0;
    if (!host_keyboard_led_state().num_lock || user_config.numlock_state != 2) {
        if (num_lock_rgb_on) {
            num_lock_rgb_on = 0;
            rgb_matrix_set_color(led_idx.KC_NUM, RGB_OFF);
        }
        return;
    } else {
        rgb_required    = 2;
        num_lock_rgb_on = 1;
        rgb_matrix_set_color(led_idx.KC_NUM, RGB_WHITE);
    }
}

// On battery the key backlight runs at half the brightness you set and fades out
// after a while without typing; on the cable (or charging) it's exactly as set.
#ifndef BAT_BACKLIGHT_LEVEL
#    define BAT_BACKLIGHT_LEVEL 128 // of 256
#endif
#ifndef IDLE_FADE_TICKS
#    define IDLE_FADE_TICKS 3000 // 30 s in 10 ms ticks
#endif
#define IDLE_FADE_LEN 200 // 2 s fade

bool key_light_idle_off = false; // fully faded: lets led_power_handle() cut LED power

void key_backlight_power(void) {
    key_light_idle_off = false;
    if (USB_ACTIVE || game_mode_enable) { return; }

    uint16_t lv = BAT_BACKLIGHT_LEVEL;
    if (no_act_time > IDLE_FADE_TICKS) {
        uint32_t t = no_act_time - IDLE_FADE_TICKS;
        lv         = t >= IDLE_FADE_LEN ? 0 : lv - (lv * t) / IDLE_FADE_LEN;
    }
    key_light_idle_off = lv == 0;
    is31fl3733_scale_range(0, SIDE_INDEX, lv);
}

/**
 * @brief  While Fn (or Fn+M) is held, light the keys that do something on that
 *         layer: mint for Fn, mustard for M (same colours as the Quick Guide).
 */
void layer_keys_show(void) {
    uint8_t layer = get_highest_layer(layer_state);
    bool    fn    = layer == MAC_FN || layer == WIN_FN;
    if (!fn && layer != M_LAYER) { return; }

    // full brightness whatever the backlight is set to: this is a lookup, and it
    // has to read with the backlight off
    uint8_t r = fn ? 0x30 : 0xFF, g = fn ? 0xFF : 0xB0, b = fn ? 0xC0 : 0x10;

    rgb_required = 1;
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t led = g_led_config.matrix_co[row][col];
            if (led == NO_LED) { continue; }
            uint16_t kc = keycode_at_keymap_location(layer, row, col);
            if (kc != KC_TRNS && kc != KC_NO) {
                rgb_matrix_set_color(led, r, g, b);
            } else {
                rgb_matrix_set_color(led, RGB_OFF);
            }
        }
    }
}

void rgb_matrix_step_game_mode(uint8_t step) {
    if (step) {
        user_config.game_rgb_mod++;
        if (user_config.game_rgb_mod > RGB_MATRIX_CUSTOM_GAME_KEYS) { user_config.game_rgb_mod = 1; }
        else if (user_config.game_rgb_mod > 3) { user_config.game_rgb_mod = RGB_MATRIX_CUSTOM_GAME_KEYS; }
    } else {
        user_config.game_rgb_mod--;
        if (user_config.game_rgb_mod > 3) { user_config.game_rgb_mod = 3; }
        else if (user_config.game_rgb_mod < 1) { user_config.game_rgb_mod = RGB_MATRIX_CUSTOM_GAME_KEYS; }
    }

    rgb_matrix_mode_noeeprom(user_config.game_rgb_mod);
}

/**
 * @brief  side_led_show.
 */
void normal_led_process(void) {
    static uint32_t side_update_time  = 0;

    uint32_t update_interval = game_mode_enable ? 500 : side_speed_table[user_config.ee_side_mode][user_config.ee_side_speed];

    if (timer_elapsed32(side_update_time) < update_interval) { return; }
    side_update_time = timer_read32();

    if (user_config.ee_side_light == 0) {
        side_off_mode_show();
    } else {
        switch (user_config.ee_side_mode) {
            case SIDE_WAVE:
                side_wave_mode_show();
                break;
            case SIDE_MIX:
                side_spectrum_mode_show();
                break;
            case SIDE_BREATH:
                side_breathe_mode_show();
                break;
            case SIDE_STATIC:
                side_static_mode_show();
                break;
            case SIDE_OFF:
                side_off_mode_show();
                break;
        }
    }


    side_one_show();
    sys_led_show(); // persistent indicators first: the battery gauge may cover them briefly
    bat_led_show();
    sleep_sw_led_show();
    side_toast_show();
    sys_sw_led_show();
}

void realtime_led_process(void) {
    rf_led_show();
    rgb_led_indicator();
    caps_word_show();
    numlock_rgb_show();
    bat_num_led();
}

void led_show(void) {
    if (f_wakeup_prepare) { return; }
    side_is31fl3733_refresh();
    normal_led_process();
    realtime_led_process();
}
