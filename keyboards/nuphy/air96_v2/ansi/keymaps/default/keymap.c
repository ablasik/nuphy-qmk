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
[MAC_BASE] = LAYOUT_96_ansi(
   KC_ESC,    KC_BRID,   KC_BRIU,   TASK,        SEARCH,       MAC_VOICE, MAC_DND,   KC_MPRV,     KC_MPLY,   KC_MNXT,    KC_MUTE,    KC_VOLD,    KC_VOLU,     PRT_AREA,  KC_DEL,     KC_HOME,   KC_END,    KC_PGUP,   KC_PGDN,
   KC_GRV,    KC_1,      KC_2,      KC_3,        KC_4,         KC_5,      KC_6,      KC_7,        KC_8,      KC_9,       KC_0,       KC_MINS,    KC_EQL,      KC_BSPC,               KC_NUM,    KC_PSLS,   KC_PAST,   KC_PMNS,
   KC_TAB,    KC_Q,      KC_W,      KC_E,        KC_R,         KC_T,      KC_Y,      KC_U,        KC_I,      KC_O,       KC_P,       KC_LBRC,    KC_RBRC,     KC_BSLS,               KC_P7,     KC_P8,     KC_P9,     KC_PPLS,
   KC_CAPS,   KC_A,      KC_S,      KC_D,        KC_F,         KC_G,      KC_H,      KC_J,        KC_K,      KC_L,       KC_SCLN,    KC_QUOT,    KC_ENT,                             KC_P4,     KC_P5,     KC_P6,
   KC_LSFT,              KC_Z,      KC_X,        KC_C,         KC_V,      KC_B,      KC_N,        KC_M,      KC_COMM,    KC_DOT,     KC_SLSH,    KC_RSFT,                KC_UP,      KC_P1,     KC_P2,     KC_P3,     KC_PENT,
   KC_LCTL,   KC_LALT,   KC_LGUI,                              KC_SPC,                                                   KC_RGUI,    MO(1),      KC_RCTL,     KC_LEFT,   KC_DOWN,    KC_RGHT,   KC_P0,     KC_PDOT),
// layer 1 Mac Fn
[MAC_FN] = LAYOUT_96_ansi(
   LOCK_SCR,  KC_F1,     KC_F2,     KC_F3,       KC_F4,        KC_F5,     KC_F6,     KC_F7,       KC_F8,     KC_F9,      KC_F10,     KC_F11,     KC_F12,      PRT_SCR,   KC_INS,     _______,   _______,   _______,   _______,
   _______,   LNK_BLE1,  LNK_BLE2,  LNK_BLE3,    LNK_RF,       _______,   _______,   _______,     _______,   _______,    _______,    _______,    _______,     _______,               _______,   _______,   _______,   _______,
   LINK_PREV,  _______,   _______,   _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    DEV_RESET,  SLEEP_MODE,  BAT_SHOW,              _______,   _______,   _______,   _______,
   _______,   _______,   _______,   _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    _______,    _______,                            _______,   _______,   _______,
   MO(4),                _______,   _______,     RGB_TEST,     _______,   BAT_NUM,   _______,     MO(4),     RM_SPDD,    RM_SPDU,    _______,    MO(4),                  RM_VALU,    _______,   _______,   _______,   _______,
   _______,   _______,   _______,                              _______,                                                  _______,    _______,    _______,     RM_NEXT,   RM_VALD,    RM_HUEU,   _______,   _______),
// layer 2 win
[WIN_BASE] = LAYOUT_96_ansi(
   KC_ESC,    KC_F1,     KC_F2,     KC_F3,       KC_F4,        KC_F5,     KC_F6,     KC_F7,       KC_F8,     KC_F9,      KC_F10,     KC_F11,     KC_F12,      KC_PSCR,   KC_DEL,      KC_HOME,  KC_END,    KC_PGUP,   KC_PGDN,
   KC_GRV,    KC_1,      KC_2,      KC_3,        KC_4,         KC_5,      KC_6,      KC_7,        KC_8,      KC_9,       KC_0,       KC_MINS,    KC_EQL,      KC_BSPC,                KC_NUM,   KC_PSLS,   KC_PAST,   KC_PMNS,
   KC_TAB,    KC_Q,      KC_W,      KC_E,        KC_R,         KC_T,      KC_Y,      KC_U,        KC_I,      KC_O,       KC_P,       KC_LBRC,    KC_RBRC,     KC_BSLS,                KC_P7,    KC_P8,     KC_P9,     KC_PPLS,
   KC_CAPS,   KC_A,      KC_S,      KC_D,        KC_F,         KC_G,      KC_H,      KC_J,        KC_K,      KC_L,       KC_SCLN,    KC_QUOT,    KC_ENT,                              KC_P4,    KC_P5,     KC_P6,
   KC_LSFT,              KC_Z,      KC_X,        KC_C,         KC_V,      KC_B,      KC_N,        KC_M,      KC_COMM,    KC_DOT,     KC_SLSH,    KC_RSFT,                KC_UP,       KC_P1,    KC_P2,     KC_P3,     KC_PENT,
   KC_LCTL,   KC_LGUI,   KC_LALT,                              KC_SPC,                                                   KC_RALT,    MO(3),      KC_RCTL,     KC_LEFT,   KC_DOWN,     KC_RGHT,  KC_P0,     KC_PDOT),
// layer 3 win Fn
[WIN_FN] = LAYOUT_96_ansi(
   LOCK_SCR,  KC_BRID,   KC_BRIU,   KC_CALC,     SEARCH,       KC_F5,     KC_F6,     KC_MPRV,     KC_MPLY,   KC_MNXT,    KC_MUTE,    KC_VOLD,    KC_VOLU,     PRT_AREA,  KC_INS,       _______,  _______,   _______,   _______,
   _______,   LNK_BLE1,  LNK_BLE2,  LNK_BLE3,    LNK_RF,       _______,   _______,   _______,     _______,   _______,    _______,    _______,    _______,     _______,                _______,  _______,   _______,   _______,
   LINK_PREV,  _______,   _______,   _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    DEV_RESET,  SLEEP_MODE,  BAT_SHOW,               _______,  _______,   _______,   _______,
   _______,   _______,   _______,   _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    _______,    _______,                             _______,  _______,   _______,
   MO(4),                _______,   _______,     RGB_TEST,     _______,   BAT_NUM,   _______,     MO(4),     RM_SPDD,    RM_SPDU,    _______,    MO(4),                  RM_VALU,     _______,  _______,   _______,   _______,
   _______,   _______,   _______,                              _______,                                                  _______,    _______,    _______,     RM_NEXT,   RM_VALD,     RM_HUEU,  _______,   _______),
// layer 4 function
[M_LAYER] = LAYOUT_96_ansi(
   QK_REBOOT, DB_TOGG,   RF_DFU,    _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    DEBOUNCE_D,  DEBOUNCE_I, _______,   _______,     _______,   _______,   _______,   _______,
   _______,   _______,   _______,   _______,     _______,      _______,   _______,   _______,     _______,   _______,    _______,    SLEEP_D,    SLEEP_I,     _______,                _______,   _______,   _______,   _______,
   _______,   _______,   MACWIN_TG, _______,     _______,      _______,   _______,   _______,     _______,   SIDE_1,     _______,    _______,    SLEEP_NOW,   _______,                _______,   _______,   _______,   _______,
   CAPS_WORD, AWAKE_TG,   _______,   DEBOUNCE_T,  FROW_SWAP,    GAME_MODE, _______,   _______,     SOCD_TOG,  _______,    _______,    _______,    _______,                             _______,   _______,   _______,
   _______,              _______,   _______,     _______,      _______,   _______,   NUMLOCK_IND, _______,   SIDE_SPD,   SIDE_SPI,   _______,    _______,                SIDE_VAI,    _______,   _______,   _______,   _______,
   _______,   _______,   _______,                              _______,                                                  _______,    MO(4),      _______,     SIDE_MOD,  SIDE_VAD,    SIDE_HUI,  _______,   _______)
};
