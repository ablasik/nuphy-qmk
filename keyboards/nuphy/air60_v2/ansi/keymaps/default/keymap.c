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
     CAPS_WORD,   AWAKE_TG,     _______,     DEBOUNCE_T,      _______,     GAME_MODE,   _______,     _______,     SOCD_TOG,    _______,         _______,     _______,                   _______,
     _______,     _______,     _______,     _______,         _______,     NUMLOCK_IND, _______,     _______,     SIDE_SPD,    SIDE_SPI,        _______,     _______,     SIDE_VAI,     _______,
     _______,     _______,     _______,                                                _______,                               _______,         _______,     SIDE_MOD,    SIDE_VAD,     SIDE_HUI)
};
