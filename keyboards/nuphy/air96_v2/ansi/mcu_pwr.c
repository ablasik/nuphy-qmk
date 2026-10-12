/* 
Copyright 2023 NuPhy, Persama (@Persama) & jincao1
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
#include "mcu_pwr.h"
#include "i2c_master.h"
#include "hal_usb.h"
#include "usb_main.h"
#include "hal_lld.h"

//------------------------------------------------

static bool rgb_led_on      = 0;
static uint16_t sleep_count = 0;

// Pin definitions
static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;



/**
 * @brief  Light sleep by powering off LEDs.
 * @note This is Nuphy's "open sourced" sleep logic. It's not deep sleep.
 */
void enter_light_sleep(void) {
    // uart_send_cmd(CMD_SET_CONFIG, 5, 5);
    if ((dev_info.link_mode == LINK_RF_24 && f_rf_sleep) || dev_info.rf_state != RF_CONNECT) {
        uart_send_cmd(CMD_SLEEP, 5, 5);
    } else {
        uart_send_cmd(CMD_SET_CONFIG, 5, 5);
    }

    led_pwr_sleep_handle();
    break_all_key();
    // clear_report_buffer_and_queue();
}

// Rows wake the MCU from STOP mode: in deep sleep the columns are driven high and
// the rows pulled down, so a key press is a rising edge and a release a falling one.
static void rows_wake_enable(void) {
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        palEnableLineEvent(row_pins[i], PAL_EVENT_MODE_BOTH_EDGES);
    }
}

static void rows_wake_disable(void) {
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        palDisableLineEvent(row_pins[i]);
    }
}

/**
 * @brief  Enter deep sleep
 * @note This is Nuphy's un-released logic with some cleanup/refactoring
 *       The MCU is put on a low power mode.
 */
void enter_deep_sleep(void) {
    //
    //------------------------ preventive restart
    sleep_count += 1;
    if (sleep_count > 100) { soft_reset_keyboard(); }

    //------------------------ RF to sleep
    enter_light_sleep();

    //------------------------ Configure WakeUp Key

#if !defined(DISABLE_MCU_SLEEP)
    // Close timer
    idle_timer_stop();
 
    // user_config.debounce_ms       = DEBOUNCE;
    // user_config.debounce_type     = 1;

    for (uint8_t i = 0; i < MATRIX_COLS; ++i) {
        gpio_set_pin_output_push_pull(col_pins[i]);
        gpio_write_pin_high(col_pins[i]);
    }
    for (uint8_t i = 0; i < MATRIX_ROWS; ++i) {
        gpio_set_pin_input_low(row_pins[i]);
    }

    rows_wake_enable();

    gpio_set_pin_output_push_pull(DEV_MODE_PIN);
    gpio_write_pin_low(DEV_MODE_PIN);

    gpio_set_pin_output_push_pull(SYS_MODE_PIN);
    gpio_write_pin_low(SYS_MODE_PIN);

    gpio_set_pin_output_push_pull(A11);
    gpio_write_pin_low(A11);
    gpio_set_pin_output_push_pull(A12);
    gpio_write_pin_low(A12);

    break_all_key();

    // Enter low power mode and wait for interrupt signal
    // STOP mode, regulator in low-power mode; any row edge wakes us
    PWR->CR = (PWR->CR & ~PWR_CR_PDDS) | PWR_CR_LPDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    //
#endif
}

/**
 * @brief  Power back up LEDs on exiting light sleep.
 * @note This is Nuphy's "open sourced" wake logic with some modifications. It's not deep sleep.
 */
void exit_light_sleep(bool stm32_init) {

    // Resume normal operations
    no_act_time = 0;
    f_rf_sleep = 0;
    f_wakeup_prepare = 0;

    // Power on LEDs
    led_pwr_wake_handle();

    // Reinitialize the system clock
    if (stm32_init) {
        stm32_clock_init(); 
        idle_timer_start();
    }

    // Tell the RF module to resume/reconnect right away (stock nuphy does this on every wake)
    uart_send_cmd(CMD_HAND, 0, 1);

    // USB remote wakeup (from nuphy 2.1.5). QMK's own suspend/wakeup loop is
    // compiled out by no_startup_check, so the keyboard has to do it here.
    if (dev_info.link_mode == LINK_USB && USB_DRIVER.state == USB_SUSPENDED && (USB_DRIVER.status & 2U /* remote wakeup enabled */)) {
        usb_lld_wakeup_host(&USB_DRIVER);
        wait_ms(50);
        uint8_t timeout = 10;
        while (USB_DRIVER.state == USB_SUSPENDED && timeout--) {
            usbWakeupHost(&USB_DRIVER);
            restart_usb_driver(&USB_DRIVER);
            wait_ms(50);
        }
        break_all_key();
    }
}


void matrix_scan_repeat(uint8_t repeat) {
    do {
        NOP_WAIT;
        matrix_scan();
    } while (repeat--);
}


/**
 * @brief Wake up from deep sleep
 * @note This is triggered by an interrupt event.
 *       This is mostly Nuphy's unreleased logic with cleanup/refactoring by me.
 */
void exit_deep_sleep(void) {
    rows_wake_disable();

 
    // Matrix initialization & Scan
    extern void matrix_init_custom(void);
    matrix_init_custom();
    clear_report_buffer_and_queue();
    matrix_scan_repeat(2);

    // m_uart_gpio_set_low_speed();

#if !defined(DISABLE_MCU_SLEEP)
    // Restore IO to working status
    gpio_set_pin_input_high(DEV_MODE_PIN); // PC0
    gpio_set_pin_input_high(SYS_MODE_PIN); // PC1

    gpio_set_pin_input_high(A11);
    gpio_set_pin_input_high(A12);


    /* Wake RF module */
    gpio_set_pin_output_push_pull(NRF_WAKEUP_PIN);
    gpio_write_pin_high(NRF_WAKEUP_PIN);

    /* Flag gor RF state */
    dev_info.rf_state = RF_LINKING;
    rf_disconnect_delay = UINT8_MAX;
    rf_linking_time     = 0;

    exit_light_sleep(true);
#endif
}

void led_pwr_sleep_handle(void) {
    pwr_led_off();
}

void led_pwr_wake_handle(void) {
    pwr_led_on();
}

void pwr_led_off(void) {
    if (!rgb_led_on) { return; }
    // LED power supply off
    gpio_set_pin_output_push_pull(DC_BOOST_PIN);
    gpio_write_pin_low(DC_BOOST_PIN);
    // drive SDB low (hardware shutdown) like stock nuphy; a floating input
    // may leave the IS31FL3733s out of shutdown and drawing current
    gpio_set_pin_output_push_pull(RGB_DRIVER_SDB1);
    gpio_write_pin_low(RGB_DRIVER_SDB1);
    gpio_set_pin_output_push_pull(RGB_DRIVER_SDB2);
    gpio_write_pin_low(RGB_DRIVER_SDB2);
    rgb_led_on = 0;
#if !defined(NO_DEBUG)
    dprint("RGB LED State: OFF\n");
#endif
}

void pwr_led_on(void) {
    if (rgb_led_on) { return; }
    // LED power supply on
    gpio_set_pin_output_push_pull(DC_BOOST_PIN);
    gpio_write_pin_high(DC_BOOST_PIN);
    gpio_set_pin_output_push_pull(RGB_DRIVER_SDB1);
    gpio_write_pin_high(RGB_DRIVER_SDB1);
    gpio_set_pin_output_push_pull(RGB_DRIVER_SDB2);
    gpio_write_pin_high(RGB_DRIVER_SDB2);
    rgb_matrix_set_color(RGB_MATRIX_LED_COUNT, 1, 1, 1);
    rgb_led_on = 1;
#if !defined(NO_DEBUG)
    dprint("RGB LED State: ON\n");
#endif
}

/*
 * Idle sleep: after 10 s without typing the main loop WFIs between scans, and a
 * 1 ms timer (TIM6 via ChibiOS GPT) wakes it for the next scan.
 */
static void idle_tick(GPTDriver *gptp) {} // only here to enable the update interrupt
static const GPTConfig idle_gpt_cfg = {.frequency = 1000000, .callback = idle_tick};

void idle_timer_init(void) {
    gptStart(&GPTD6, &idle_gpt_cfg);
    idle_timer_start();
}

void idle_timer_start(void) {
    if (GPTD6.state == GPT_READY) { gptStartContinuous(&GPTD6, 1000); }
}

void idle_timer_stop(void) {
    if (GPTD6.state == GPT_CONTINUOUS) { gptStopTimer(&GPTD6); }
}

void idle_enter_sleep(void) {
    if (no_act_time < 1000 || user_config.sleep_mode != 1) { return; }
    __WFI();
}
