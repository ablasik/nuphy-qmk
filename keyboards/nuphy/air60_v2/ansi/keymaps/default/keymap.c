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

#include QMK_KEYBOARD_H
#include "layer_names.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

// layer 0 Mac
[MAC_BASE] = LAYOUT_60_ansi(
     KC_ESC,      KC_1,        KC_2,        KC_3,            KC_4,        KC_5,        KC_6,        KC_7,        KC_8,        KC_9,            KC_0,        KC_MINS,     KC_EQL,       KC_BSPC,
     KC_TAB,      KC_Q,        KC_W,        KC_E,            KC_R,        KC_T,        KC_Y,        KC_U,        KC_I,        KC_O,            KC_P,        KC_LBRC,     KC_RBRC,      KC_BSLS,
     KC_CAPS,     KC_A,        KC_S,        KC_D,            KC_F,        KC_G,        KC_H,        KC_J,        KC_K,        KC_L,            KC_SCLN,     KC_QUOT,                   KC_ENT,
     KC_LSFT,     KC_Z,        KC_X,        KC_C,            KC_V,        KC_B,        KC_N,        KC_M,        KC_COMM,     KC_DOT,          KC_SLSH,     KC_RSFT,     KC_UP,        KC_DEL,
     KC_LCTL,     KC_LALT,     KC_LGUI,                                                KC_SPC,                                KC_RGUI,         MO(1),       KC_LEFT,     KC_DOWN,      KC_RGHT),

// layer 1 Mac fn
[MAC_FN] = LAYOUT_60_ansi(
     KC_GRV,      KC_BRID,     KC_BRIU,     TASK,            SEARCH,       MAC_VOICE,  MAC_DND,     KC_MPRV,     KC_MPLY,     KC_MNXT,         KC_MUTE,     KC_VOLD,     KC_VOLU,      _______,
     LINK_PREV,     LNK_BLE1,    LNK_BLE2,    LNK_BLE3,        LNK_RF,       _______,    _______,     _______,     _______,     _______,         _______,     DEV_RESET,   SLEEP_MODE,   BAT_SHOW,
     _______,     _______,     _______,     _______,         _______,      _______,    _______,     _______,     _______,     _______,         _______,     _______,                   _______,
     MO(2),       _______,     _______,     _______,         _______,      _______,    _______,     MO(6),       RM_SPDD,     RM_SPDU,         _______,     MO(2),       RM_VALU,      _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     RM_NEXT,     RM_VALD,      RM_HUEU),

// layer 2 Mac Fn+shift
[MAC_FN_SFT] = LAYOUT_60_ansi(
     _______,     KC_F1,       KC_F2,       KC_F3,           KC_F4,        KC_F5,      KC_F6,       KC_F7,       KC_F8,       KC_F9,           KC_F10,      KC_F11,      KC_F12,       _______,
     _______,     _______,     _______,     _______,         _______,      _______,    _______,     _______,     _______,     _______,         _______,     _______,     _______,      _______,
     _______,     _______,     _______,     _______,         _______,      _______,    _______,     _______,     _______,     _______,         _______,     _______,                   _______,
     _______,     _______,     _______,     RGB_TEST,        _______,      _______,    _______,     _______,     _______,     _______,         _______,     _______,     _______,      _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     _______,     _______,      _______),

// layer 3 Win
[WIN_BASE] = LAYOUT_60_ansi(
     KC_ESC,      KC_1,        KC_2,        KC_3,            KC_4,        KC_5,        KC_6,        KC_7,        KC_8,        KC_9,            KC_0,        KC_MINS,     KC_EQL,       KC_BSPC,
     KC_TAB,      KC_Q,        KC_W,        KC_E,            KC_R,        KC_T,        KC_Y,        KC_U,        KC_I,        KC_O,            KC_P,        KC_LBRC,     KC_RBRC,      KC_BSLS,
     KC_CAPS,     KC_A,        KC_S,        KC_D,            KC_F,        KC_G,        KC_H,        KC_J,        KC_K,        KC_L,            KC_SCLN,     KC_QUOT,                   KC_ENT,
     KC_LSFT,     KC_Z,        KC_X,        KC_C,            KC_V,        KC_B,        KC_N,        KC_M,        KC_COMM,     KC_DOT,          KC_SLSH,     KC_RSFT,     KC_UP,        KC_DEL,
     KC_LCTL,     KC_LGUI,     KC_LALT,                                                KC_SPC,                                KC_RALT,         MO(4),       KC_LEFT,     KC_DOWN,      KC_RGHT),

// layer 4 win fn
[WIN_FN] = LAYOUT_60_ansi(
     KC_GRV,      KC_F1,       KC_F2,       KC_F3,           KC_F4,       KC_F5,       KC_F6,       KC_F7,       KC_F8,       KC_F9,           KC_F10,      KC_F11,      KC_F12,       _______,
     LINK_PREV,     LNK_BLE1,    LNK_BLE2,    LNK_BLE3,        LNK_RF,      _______,     _______,     _______,     _______,     _______,         _______,     DEV_RESET,   SLEEP_MODE,   BAT_SHOW,
     _______,     _______,     _______,     _______,         _______,     _______,     _______,     _______,     _______,     _______,         _______,     _______,                   _______,
     MO(5),       _______,     _______,     _______,         _______,     _______,     _______,     MO(6),       RM_SPDD,     RM_SPDU,         _______,     MO(5),       RM_VALU,      _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     RM_NEXT,     RM_VALD,      RM_HUEU),

// layer 5 win fn+shift
[WIN_FN_SFT] = LAYOUT_60_ansi(
     _______,     KC_BRID,     KC_BRIU,     _______,         _______,     _______,     _______,     KC_MPRV,     KC_MPLY,     KC_MNXT,         KC_MUTE,     KC_VOLD,     KC_VOLU,      _______,
     _______,     _______,     _______,     _______,         _______,     _______,     _______,     _______,     _______,     _______,         _______,     _______,     _______,      _______,
     _______,     _______,     _______,     _______,         _______,     _______,     _______,     _______,     _______,     _______,         _______,     _______,                   _______,
     _______,     _______,     _______,     RGB_TEST,        _______,     _______,     _______,     _______,     _______,     _______,         _______,     _______,     _______,      _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     _______,     _______,      _______),

// layer 6 function
[M_LAYER] = LAYOUT_60_ansi(
     QK_REBOOT,   DB_TOGG,     RF_DFU,      _______,         _______,     _______,     _______,     _______,     _______,     _______,         _______,     DEBOUNCE_D,  DEBOUNCE_I,   _______,
     _______,     _______,     MACWIN_TG,     _______,         _______,     _______,     _______,     _______,     _______,     SLEEP_D,         SLEEP_I,     _______,     SLEEP_NOW,    _______,
     CAPS_WORD,   _______,     _______,     DEBOUNCE_T,      _______,     GAME_MODE,   _______,     _______,     SOCD_TOG,    _______,         _______,     _______,                   _______,
     _______,     _______,     _______,     _______,         _______,     NUMLOCK_IND, _______,     _______,     SIDE_SPD,    SIDE_SPI,        _______,     _______,     SIDE_VAI,     _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     SIDE_MOD,    SIDE_VAD,     SIDE_HUI)
};

const is31fl3733_led_t PROGMEM g_is31fl3733_leds[RGB_MATRIX_LED_COUNT] = {
    {0, SW1_CS16,   SW2_CS16,   SW3_CS16},      // "Esc"
    {0, SW1_CS2,    SW2_CS2,    SW3_CS2},       // "!1"
    {0, SW1_CS3,    SW2_CS3,    SW3_CS3},       // "@2"
    {0, SW1_CS4,    SW2_CS4,    SW3_CS4},       // "#3"
    {0, SW1_CS5,    SW2_CS5,    SW3_CS5},       // "$4"
    {0, SW1_CS6,    SW2_CS6,    SW3_CS6},       // "%5"
    {0, SW1_CS7,    SW2_CS7,    SW3_CS7},       // "^6"
    {0, SW1_CS8,    SW2_CS8,    SW3_CS8},       // "&7"
    {0, SW1_CS9,    SW2_CS9,    SW3_CS9},       // "*8"
    {0, SW1_CS10,   SW2_CS10,   SW3_CS10},      // "(9"
    {0, SW1_CS11,   SW2_CS11,   SW3_CS11},      // ")0"
    {1, SW4_CS1,    SW5_CS1,    SW6_CS1},       // "_-"
    {1, SW4_CS2,    SW5_CS2,    SW6_CS2},       // "+="
    {1, SW4_CS3,    SW5_CS3,    SW6_CS3},       // "Backsp"
    {0, SW4_CS1,    SW5_CS1,    SW6_CS1},       // "Tab"
    {0, SW4_CS2,    SW5_CS2,    SW6_CS2},       // "Q"
    {0, SW4_CS3,    SW5_CS3,    SW6_CS3},       // "W"
    {0, SW4_CS4,    SW5_CS4,    SW6_CS4},       // "E"
    {0, SW4_CS5,    SW5_CS5,    SW6_CS5},       // "R"
    {0, SW4_CS6,    SW5_CS6,    SW6_CS6},       // "T"
    {0, SW4_CS7,    SW5_CS7,    SW6_CS7},       // "Y"
    {0, SW4_CS8,    SW5_CS8,    SW6_CS8},       // "U"
    {0, SW4_CS9,    SW5_CS9,    SW6_CS9},       // "I"
    {0, SW4_CS10,   SW5_CS10,   SW6_CS10},      // "O"
    {0, SW4_CS11,   SW5_CS11,   SW6_CS11},      // "P"
    {1, SW7_CS1,    SW8_CS1,    SW9_CS1},       // "{["
    {1, SW7_CS2,    SW8_CS2,    SW9_CS2},       // "}]"
    {1, SW7_CS3,    SW8_CS3,    SW9_CS3},       // "|\\"
    {0, SW7_CS1,    SW8_CS1,    SW9_CS1},       // "Caps"
    {0, SW7_CS2,    SW8_CS2,    SW9_CS2},       // "A"
    {0, SW7_CS3,    SW8_CS3,    SW9_CS3},       // "S"
    {0, SW7_CS4,    SW8_CS4,    SW9_CS4},       // "D"
    {0, SW7_CS5,    SW8_CS5,    SW9_CS5},       // "F"
    {0, SW7_CS6,    SW8_CS6,    SW9_CS6},       // "G"
    {0, SW7_CS7,    SW8_CS7,    SW9_CS7},       // "H"
    {0, SW7_CS8,    SW8_CS8,    SW9_CS8},       // "J"
    {0, SW7_CS9,    SW8_CS9,    SW9_CS9},       // "K"
    {0, SW7_CS10,   SW8_CS10,   SW9_CS10},      // "L"
    {0, SW7_CS11,   SW8_CS11,   SW9_CS11},      // ":"
    {1, SW7_CS16,   SW8_CS16,   SW9_CS16},      // "\""
    {1, SW7_CS14,   SW8_CS14,   SW9_CS14},      // "Enter"
    {0, SW10_CS1,    SW11_CS1,    SW12_CS1},       // "Shift"
    {0, SW10_CS3,    SW11_CS3,    SW12_CS3},       // "Z"
    {0, SW10_CS4,    SW11_CS4,    SW12_CS4},       // "X"
    {0, SW10_CS5,    SW11_CS5,    SW12_CS5},       // "C"
    {0, SW10_CS6,    SW11_CS6,    SW12_CS6},       // "V"
    {0, SW10_CS7,    SW11_CS7,    SW12_CS7},       // "B"
    {0, SW10_CS8,    SW11_CS8,    SW12_CS8},       // "N"
    {0, SW10_CS9,    SW11_CS9,    SW12_CS9},       // "M"
    {0, SW10_CS10,   SW11_CS10,   SW12_CS10},      // "<,"
    {0, SW10_CS11,   SW11_CS11,   SW12_CS11},      // ">."
    {1, SW10_CS1,    SW11_CS1,    SW12_CS1},       // "?/"
    {1, SW10_CS3,    SW11_CS3,    SW12_CS3},       // "Shift"
    {1, SW10_CS4,    SW11_CS4,    SW12_CS4},       // "↑"
    {1, SW7_CS4,    SW8_CS4,    SW9_CS4},       // "Del"
    {0, SW10_CS16,   SW11_CS16,   SW12_CS16},      // "Ctrl"
    {0, SW10_CS15,   SW11_CS15,   SW12_CS15},      // "Opt"
    {0, SW10_CS14,   SW11_CS14,   SW12_CS14},      // "Cmd"
    {0, SW10_CS13,   SW11_CS13,   SW12_CS13},      // "Space"
    {0, SW10_CS12,   SW11_CS12,   SW12_CS12},      // "Cmd"
    {1, SW10_CS16,   SW11_CS16,   SW12_CS16},      // "Fn"
    {1, SW10_CS13,   SW11_CS13,   SW12_CS13},      // "←"
    {1, SW10_CS12,   SW11_CS12,   SW12_CS12},      // "↓"
    {1, SW10_CS11,   SW11_CS11,   SW12_CS11},      // "→"

    {1, SW1_CS5,    SW2_CS5,    SW3_CS5},       // logo left
    {1, SW1_CS4,    SW2_CS4,    SW3_CS4},       //
    {1, SW1_CS3,    SW2_CS3,    SW3_CS3},       //
    {1, SW1_CS2,    SW2_CS2,    SW3_CS2},       //
    {1, SW1_CS1,    SW2_CS1,    SW3_CS1},       //

    {1, SW1_CS6,    SW2_CS6,    SW3_CS6},       // logo right
    {1, SW1_CS7,    SW2_CS7,    SW3_CS7},       //
    {1, SW1_CS8,    SW2_CS8,    SW3_CS8},       //
    {1, SW1_CS9,    SW2_CS9,    SW3_CS9},       //
    {1, SW1_CS10,   SW2_CS10,   SW3_CS10}       //
};
