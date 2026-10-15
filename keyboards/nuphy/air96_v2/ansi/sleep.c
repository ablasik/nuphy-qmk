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
#include "hal_usb.h"
#include "usb_main.h"
#include "mcu_pwr.h"
#include "rf_queue.h"

void deep_sleep_handle(void) {
    // Sync again before sleeping
    dev_sts_sync();

    // Setting Parameters
    f_goto_deepsleep   = 0;
    f_goto_sleep       = 0;
    f_wakeup_prepare   = 1;
    f_rf_sleep         = 1;

    // Signal sleep
    signal_sleep();

    enter_deep_sleep(); // puts the board in WFI mode and pauses the MCU
    exit_deep_sleep();  // This gets called when there is an interrupt (wake) event.
}

/**
 * @brief  Keep awake is on for the current link and its host isn't asleep.
 *         The keyboard doesn't sleep on that link either, or it couldn't send.
 *         Over the air it holds through link blinks: a host that's really gone
 *         still sends the keyboard to sleep via rf_linking_time / rf_disconnect_time.
 */
bool keep_awake(void) {
    if (!(user_config.link_awake & (1 << dev_info.link_mode))) { return false; }
    return dev_info.link_mode != LINK_USB || USB_DRIVER.state != USB_SUSPENDED;
}

/**
 * @brief  Keep awake: every KEEP_AWAKE_MS move the pointer 1 px and back, so the
 *         computer doesn't sleep or lock itself while idle.
 */
void keep_awake_task(void) {
    static uint32_t last = 0;
    if (!keep_awake() || timer_elapsed32(last) < KEEP_AWAKE_MS) { return; }
    if (dev_info.link_mode != LINK_USB && dev_info.rf_state != RF_CONNECT) { return; } // nowhere to send it
    if (no_act_time < 100) { return; } // not mid-typing: RF mouse reports clear the key report buffer

    last          = timer_read32();
    uint32_t idle = no_act_time; // RF sends reset it; the nudge isn't the user, so fade and sleep timers carry on
    report_mouse_t report = {.x = 1};
    host_mouse_send(&report);
    report.x = -1;
    host_mouse_send(&report);
    report.x = 0;
    host_mouse_send(&report);
    no_act_time = idle;
}

/**
 * @brief  Sleep Handle.
 */
void sleep_handle(void) {
    static uint8_t  usb_suspend_debounce = 0;
    static uint32_t rf_disconnect_time = 0;

    if (user_config.sleep_mode == 0 || USB_ACTIVE) { return; }
    if (delay_step_timer > 100) { no_act_time = 0; }

    if (delay_step_timer < 50) { return; }
    delay_step_timer = 0;

    // no MCU deep sleep on USB: STOP mode drives D+/D- low and the host sees a detach
    if (user_config.sleep_mode != 1 || f_rf_sleep || dev_info.link_mode == LINK_USB) {
        f_goto_deepsleep = 0;
    } else if (!keep_awake() && no_act_time >= (user_config.light_sleep + DEEP_SLEEP_TIME) * T_MIN) {
        f_goto_deepsleep = 1;
    }

    if (f_goto_deepsleep != 0) {
        deep_sleep_handle();
        return;
    }

    // sleep process
    if (f_goto_sleep) {
        f_goto_sleep     = 0;
        enter_light_sleep();
        f_wakeup_prepare = 1;
    }
        
    // sleep check
    if (f_goto_sleep || f_wakeup_prepare) {
        return;
    }

    if (dev_info.link_mode == LINK_USB) {
        if (USB_DRIVER.state == USB_SUSPENDED) {
            usb_suspend_debounce++;
            if (usb_suspend_debounce >= 10) {
                f_goto_sleep = 1;
            }
        } else {
            usb_suspend_debounce = 0;
        }
    } else if (!keep_awake() && no_act_time >= (user_config.light_sleep * T_MIN)) {
        f_goto_sleep     = 1;
    } else if (rf_linking_time >= (dev_info.link_mode == LINK_RF_24 ? (link_timeout / 4) : link_timeout)) {
        f_goto_deepsleep = 1;
        f_goto_sleep     = 1;
        rf_linking_time  = 0;
    } else if (dev_info.rf_state == RF_DISCONNECT) {
            rf_disconnect_time++;
        if (rf_disconnect_time > 10 * 4) {
            f_goto_deepsleep   = 1;
            f_goto_sleep       = 1;
            rf_disconnect_time = 0;
        }
    } else if (dev_info.rf_state == RF_CONNECT) {
            rf_disconnect_time = 0;
    }
}
