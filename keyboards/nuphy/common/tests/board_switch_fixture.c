// SPDX-License-Identifier: GPL-2.0-or-later
// The runner inserts the production board functions at PRODUCTION_ADAPTER.
#define main engine_test_main
#include "../../keyboards/nuphy/common/tests/test_connection_actions.c"
#undef main
#define RF_IDLE 0
#define HOST_USB_TYPE 0
#define HOST_RF_TYPE 2
#define CMD_SET_LINK 0xC0
#define CMD_NEW_ADV 0xC7
static uint8_t host_mode, f_send_channel, rf_blink_cnt, f_rf_sw_press, rf_sw_temp;
static unsigned rf_link_show_time, rf_linking_time, uart_calls, pairing_calls;
static bool f_rf_new_adv_ok = true;
static uint8_t dial_value;
static void host_set_driver(host_driver_t *d) { driver = d; }
static void uart_send_cmd(int cmd, int tries, int delay) {
    (void)tries; (void)delay; assert(!player.active && !pending);
    ++uart_calls; if (cmd == CMD_NEW_ADV) { ++pairing_calls; }
}
static void wait_ms(int ms) { now += ms; }
static void uart_receive_pro(void) {}
static uint8_t dial_read(void) { return dial_value; }
void switch_dev_link(uint8_t mode);
static void dial_set(uint8_t dial, bool show) {
    (void)show;
    uint8_t target = (dial & 1) ? LINK_USB : dev_info.rf_channel;
    if (target != dev_info.link_mode) { switch_dev_link(target); }
}
/* PRODUCTION_ADAPTER */
static void board_tick(void) {
    now += 20;
    if (dev_info.link_mode != LINK_USB && rf_confirmed) { connection_actions_rf_status(dev_info.link_mode, true); }
    dial_sw_scan(); requested_link_task(); connection_actions_task();
}
int main(void) {
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_WINDOWS, 1, 0);
    dev_info.rf_channel = LINK_BT_1; dev_info.ble_channel = LINK_BT_1;
    dial_value = 1;
    for (int i=0;i<20;++i) { board_tick(); }
    assert(downs == 0 && dev_info.link_mode == LINK_USB);
    dial_value = 0; // physical wired -> wireless
    for (int i=0;i<12;++i) { board_tick(); assert(dev_info.link_mode == LINK_USB && driver == &usb_driver); }
    for (int i=0;i<60;++i) { board_tick(); }
    assert(downs == 1 && !has_anykey());
    assert(dev_info.link_mode == LINK_BT_1 && driver == &rf_host_driver);

    // Physical wireless -> wired must also send on RF before changing driver.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 2, 0);
    dev_info.link_mode = LINK_BT_1; dev_info.rf_channel = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_invalidate();
    connection_actions_rf_status(LINK_BT_1, true);
    dial_value = 1;
    for (int i=0;i<12;++i) { board_tick(); assert(dev_info.link_mode == LINK_BT_1 && driver == &rf_host_driver); }
    for (int i=0;i<60;++i) { board_tick(); }
    assert(downs == 1 && !has_anykey());
    assert(dev_info.link_mode == LINK_USB && driver == &usb_driver);

    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 2, 0);
    dev_info.link_mode = LINK_BT_1; dev_info.rf_channel = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_invalidate();
    connection_actions_rf_status(LINK_BT_1, true);
    dial_value = 0;
    for (int i=0;i<20;++i) { board_tick(); }
    uart_calls = 0; physical = true;
    request_link_mode(LINK_BT_2, false); // Fn+2, source not changed yet
    assert(requested_link_pending && uart_calls == 0 && dev_info.rf_channel == LINK_BT_1);
    for(int i=0;i<20;++i) { board_tick(); assert(dev_info.link_mode == LINK_BT_1 && uart_calls == 0); }
    physical = false; connection_actions_key_event(false);
    for(int i=0;i<60;++i) { board_tick(); }
    assert(downs == 1 && uart_calls == 1 && !requested_link_pending);
    assert(dev_info.link_mode == LINK_BT_2 && dev_info.rf_channel == LINK_BT_2);

    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 2, 0);
    dev_info.link_mode = LINK_BT_1; dev_info.rf_channel = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_invalidate();
    connection_actions_rf_status(LINK_BT_1, true);
    uart_calls = pairing_calls = 0;
    request_link_mode(LINK_BT_3, true);
    assert(uart_calls == 0);
    for(int i=0;i<60;++i) { board_tick(); }
    assert(downs == 1 && pairing_calls == 1 && uart_calls == 2);
    assert(dev_info.link_mode == LINK_BT_3 && dev_info.rf_channel == LINK_BT_3);
    puts("PASS: production dial/profile/pairing adapters retain the source until playback drains");
    return 0;
}
