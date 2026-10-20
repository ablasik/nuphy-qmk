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

#pragma once

#define DEV_MODE_PIN                        C0
#define SYS_MODE_PIN                        C1
#define DC_BOOST_PIN                        C2
#define NRF_RESET_PIN                       B4
#define NRF_TEST_PIN                        B5
#define NRF_WAKEUP_PIN                      B8
#define DRIVER_LED_CS_PIN                   C6
#define DRIVER_SIDE_PIN                     C8
#define DRIVER_SIDE_CS_PIN                  C9

// RF module UART. QMK used to map SD1_* onto these; since #24855 only UART_* is read,
// and the defaults (A9/A10) are matrix columns: ghost keypresses and garbage RF frames.
#define UART_DRIVER                         SD1
#define UART_TX_PIN                         B6
#define UART_TX_PAL_MODE                    0
#define UART_RX_PIN                         B7
#define UART_RX_PAL_MODE                    0

#if !defined(NO_DEBUG) && !defined(CONSOLE_ENABLE)
#define NO_DEBUG
#endif // !NO_DEBUG

#if !defined(NO_PRINT) && !defined(CONSOLE_ENABLE)
#define NO_PRINT
#endif // !NO_PRINT
 
#if !defined(NO_DEBUG) && defined(CONSOLE_ENABLE)
#define DEBUG_MATRIX_SCAN_RATE
#endif // DEBUG_MATRIX_SCAN_RATE
 
#define DYNAMIC_KEYMAP_MACRO_DELAY          8

#define WAIT_US_TIMER                       GPTD14
// WS2812 timing (NuPhy's values), for QMK's driver and the side strip
#define WS2812_TIMING                       850
#define WS2812_T1H                          700
#define WS2812_T0H                          150
#define WS2812_LED_COUNT                    RGB_MATRIX_LED_COUNT // custom RGB driver wraps QMK's WS2812 one

// EEPROM: wear leveling on the last 8 kB of flash (4 kB logical)
#define WEAR_LEVELING_BACKING_SIZE          8192
#define WEAR_LEVELING_LOGICAL_SIZE          4096

#define EECONFIG_KB_DATA_SIZE               32
#define FROW_COL_F1                         1 // F1..F12 on matrix row 0
#define FROW_COL_F12                        12
#define GPIO_INPUT_PIN_DELAY                12
#define RF_QUEUE_SIZE                       64

#define OS_DETECTION_DEBOUNCE              250

// remove unused QMK functions to save space
/*
#define NO_ACTION_ONESHOT
#undef LAYER_LOCK_ENABLE
#undef LEADER_ENABLE
#undef GRAVE_ESC_ENABLE
#undef SPACE_CADET_ENABLE
*/

// Reset OS fingerprint at the bus event, before new descriptor requests.
#define OS_DETECTION_RESET_ON_USB_BUS_RESET
