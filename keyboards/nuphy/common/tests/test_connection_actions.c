// SPDX-License-Identifier: GPL-2.0-or-later
// Native tests of the production engine AND production EEPROM macro backend.
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "nvm_dynamic_keymap.h"
#include "../connection_actions.c"

static uint8_t memory[TOTAL_EEPROM_BYTE_COUNT];
static uint32_t now;
static int cut_after = -1;
static jmp_buf power_cut;
static unsigned downs, releases;
static uint8_t held[256];
static bool physical, receiving, queue_empty = true;
static os_variant_t detected;
static usb_configure_state_t configured;
user_config_t user_config;
device_t dev_info;
host_driver_t usb_driver, rf_host_driver;
host_driver_t *m_host_driver = &usb_driver;
static host_driver_t *driver;
bool f_dial_sw_init_ok, f_wakeup_prepare, f_rf_sleep;
uint32_t dequeue_delay, no_act_time;
static bool empty(void) { return queue_empty; }
rf_queue_t rf_queue = {empty};
const uint8_t ascii_to_keycode_lut[128] = {['a'] = 4, ['A'] = 4};
const uint8_t ascii_to_shift_lut[16] = {[8] = 2};
const uint8_t ascii_to_altgr_lut[16] = {0}, ascii_to_dead_lut[16] = {0};

uint8_t eeprom_read_byte(const uint8_t *addr) { assert((uintptr_t)addr < sizeof(memory)); return memory[(uintptr_t)addr]; }
void eeprom_read_block(void *dst, const void *addr, size_t size) { assert((uintptr_t)addr + size <= sizeof(memory)); memcpy(dst, memory + (uintptr_t)addr, size); }
void eeprom_update_byte(uint8_t *addr, uint8_t value) {
    assert((uintptr_t)addr < sizeof(memory));
    if (cut_after == 0) { longjmp(power_cut, 1); }
    if (cut_after > 0) { --cut_after; }
    memory[(uintptr_t)addr] = value;
}
void eeprom_update_block(const void *src, void *addr, size_t size) {
    for (size_t i = 0; i < size; ++i) { eeprom_update_byte((uint8_t *)addr + i, ((uint8_t *)src)[i]); }
}
// eeprom_core_t is 37 bytes. Use the real backend's default derived offsets.
void eeconfig_read_kb_datablock(void *buf, uint32_t offset, uint32_t size) { eeprom_read_block(buf, (void *)(uintptr_t)(37 + offset), size); }
void test_update_config(const void *buf, uint32_t offset, uint32_t size) { eeprom_update_block(buf, (void *)(uintptr_t)(37 + offset), size); }
uint8_t dynamic_keymap_macro_get_count(void) { return 16; }
uint16_t dynamic_keymap_macro_get_buffer_size(void) { return nvm_dynamic_keymap_macro_size(); }
void dynamic_keymap_macro_get_buffer(uint16_t offset, uint16_t size, uint8_t *data) { nvm_dynamic_keymap_macro_read_buffer(offset, size, data); }
uint32_t timer_read32(void) { return now; }
uint32_t timer_elapsed32(uint32_t at) { return now - at; }
host_driver_t *host_get_driver(void) { return driver; }
uint8_t has_anymod(void) { return 0; }
uint8_t has_anykey(void) { for (int i = 0; i < 256; ++i) { if (held[i]) return 1; } return 0; }
uint8_t get_oneshot_mods(void) { return 0; }
uint8_t get_oneshot_locked_mods(void) { return 0; }
uint32_t matrix_get_row(uint8_t row) { return row == 0 && physical; }
void register_code(uint8_t key) { ++downs; held[key] = 1; }
void unregister_code(uint8_t key) { held[key] = 0; }
void break_all_key(void) { assert(!receiving); memset(held, 0, sizeof(held)); ++releases; }
void clear_report_buffer_and_queue(void) { queue_empty = true; }
#ifndef CA_TEST_REAL_OS
os_variant_t detected_host_os(void) { return detected; }
void erase_wlength_data(void) { detected = OS_UNSURE; }
#endif
void notify_usb_device_state_change_user(struct usb_device_state state) { (void)state; }
usb_configure_state_t usb_device_state_get_configure_state(void) { return configured; }

enum { MACRO_START = 73 + 8 * MATRIX_ROWS * MATRIX_COLS * 2, OLD_SIZE = 4096 - MACRO_START };
static void reboot(void) {
    memset(&player, 0, sizeof(player));
    session = evaluated = false; pending = 0;
    seen_usb_epoch = usb_epoch = 0;
    eeconfig_read_kb_datablock(&user_config, 0, sizeof(user_config));
    connection_actions_init();
}
static void fresh(void) {
    memset(memory, 0, sizeof(memory));
    memset(held, 0, sizeof(held));
    cut_after = -1; now = downs = releases = 0;
    physical = false; queue_empty = true;
    f_dial_sw_init_ok = true; f_wakeup_prepare = f_rf_sleep = false;
    receiving = false;
    dequeue_delay = 0;
    dev_info = (device_t){.link_mode = LINK_USB, .rf_state = RF_CONNECT};
    driver = &usb_driver; configured = USB_DEVICE_STATE_CONFIGURED;
    detected = OS_WINDOWS; usb_state = USB_DEVICE_STATE_CONFIGURED;
    reboot();
}
static void advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 20) {
        now += 20;
        if (dev_info.link_mode != LINK_USB && rf_confirmed) { connection_actions_rf_status(dev_info.link_mode, true); }
        connection_actions_task();
    }
}
static void put_macro(uint8_t index, const uint8_t *bytes, size_t length) {
    connection_actions_user_activity();
    uint16_t at = index;
    assert(at + length + 16 < dynamic_keymap_macro_get_buffer_size());
    nvm_dynamic_keymap_macro_update_buffer(at, length, (uint8_t *)bytes);
}
static uint8_t set_field(uint8_t slot, uint8_t field, uint8_t value) {
    uint8_t command[4] = {id_custom_set_value, 0, 1 + slot * CA_FIELDS + field, value};
    via_custom_value_command_kb(command, sizeof(command));
    return command[0];
}
static void enable(uint8_t slot, uint8_t os, uint8_t link, uint8_t delay) {
    assert(set_field(slot, CA_OS, os) == id_custom_set_value);
    assert(set_field(slot, CA_CONNECTION, link) == id_custom_set_value);
    assert(set_field(slot, CA_DELAY, delay) == id_custom_set_value);
    assert(set_field(slot, CA_ENABLED, 1) == id_custom_set_value);
    connection_actions_invalidate(); // explicit new connection after editing in VIA
}
static void reconnect(void) {
    connection_actions_invalidate();
    if (dev_info.link_mode != LINK_USB) { connection_actions_rf_status(dev_info.link_mode, true); }
}
static const uint8_t tap_a[] = {1, 1, 4, 0};

static void persistence_tests(void) {
    fresh();
    assert(storage_status == CA_AVAILABLE);
    assert(dynamic_keymap_macro_get_buffer_size() == OLD_SIZE - 64);
    assert(memory[4095] == 0); // downgrade sentinel
    enable(3, CA_IOS, 4, 100);
    assert(set_field(3, CA_MACRO, 15) == id_custom_set_value);
    reboot();
    assert(rules[3][CA_ENABLED] && rules[3][CA_MACRO] == 15 && rules[3][CA_DELAY] == 100);
    assert(set_field(0, CA_MACRO, 16) == id_unhandled);
    assert(set_field(0, CA_OS, 255) == id_unhandled);
    assert(set_field(0, CA_DELAY, 101) == id_unhandled);
    assert(set_field(0, CA_CONNECTION, 7) == id_unhandled);
    assert(set_field(0, CA_TRIGGER, 2) == id_unhandled);
    assert(set_field(0, CA_ACTION, 1) == id_unhandled);
    // Read-only status controls still acknowledge SET with an exact echo.
    // In particular, USB Unknown must not change SET [7,0,29,1] to [...,0].
    detected = OS_UNSURE;
    for (uint8_t id = 0; id <= 29; id += 29) {
        for (uint8_t value = 0; value < 3; ++value) {
            uint8_t request[32] = {id_custom_set_value, 0, id, value};
            uint8_t expected[32]; memcpy(expected, request, sizeof(request));
            uint8_t saved_rules[sizeof(rules)]; memcpy(saved_rules, rules, sizeof(rules));
            via_custom_value_command_kb(request, sizeof(request));
            assert(!memcmp(request, expected, sizeof(request)));
            assert(!memcmp(saved_rules, rules, sizeof(rules)));
        }
    }
    uint8_t short_packet[4] = {id_custom_set_value, 0, 1, 1};
    for (uint8_t len = 0; len < 4; ++len) {
        short_packet[0] = id_custom_set_value;
        via_custom_value_command_kb(short_packet, len);
        assert(!rules[0][CA_ENABLED]);
    }
    uint8_t tail[64]; memcpy(tail, memory + CA_ADDR, 64);
    uint8_t attack[128]; memset(attack, 0xA5, sizeof(attack));
    nvm_dynamic_keymap_macro_update_buffer(OLD_SIZE - 64 - 1, 128, attack);
    assert(!memcmp(tail, memory + CA_ADDR, 64));
    nvm_dynamic_keymap_macro_reset();
    assert(!memcmp(tail, memory + CA_ADDR, 64));
    memory[CA_ADDR + 10] ^= 1;
    reboot();
    assert(storage_status == CA_BAD_CONFIG && !rules[3][CA_ENABLED]);
    assert(dynamic_keymap_macro_get_buffer_size() == OLD_SIZE - 64);

    // Occupied tail and an interrupted old VIA upload remain byte-for-byte intact.
    memset(memory, 0, sizeof(memory)); memory[4090] = 42;
    uint8_t before[4096]; memcpy(before, memory, sizeof(before));
    reboot(); assert(storage_status == CA_NO_SPACE);
    assert(dynamic_keymap_macro_get_buffer_size() == OLD_SIZE);
    assert(!memcmp(before, memory, sizeof(before)));
    memory[4090] = 0; memory[4095] = 255; memcpy(before, memory, sizeof(before));
    reboot(); assert(storage_status == CA_NO_SPACE && !memcmp(before, memory, sizeof(before)));
    memset(memory, 0, sizeof(memory));
    memset(memory + MACRO_START, 'a', OLD_SIZE - 1);
    memcpy(before, memory, sizeof(before));
    reboot(); assert(storage_status == CA_NO_SPACE && !memcmp(before, memory, sizeof(before)));

    // Preserve all non-CA bytes while migrating a real macro and a populated keymap.
    memset(memory, 0, sizeof(memory));
    memset(memory + 73, 0x67, MACRO_START - 73);
    memcpy(memory + MACRO_START, tap_a, sizeof(tap_a));
    memcpy(before, memory, sizeof(before));
    reboot();
    assert(storage_status == CA_AVAILABLE);
    assert(!memcmp(before + 73, memory + 73, CA_ADDR - 73));
    assert(!memcmp(before, memory, 37 + 28));

    // Every byte boundary of initial ownership + tail commit can lose power.
    for (int cut = 0; cut < 74; ++cut) {
        memset(memory, 0, sizeof(memory));
        memcpy(memory + MACRO_START, tap_a, sizeof(tap_a));
        cut_after = cut;
        if (!setjmp(power_cut)) { reboot(); }
        cut_after = -1; reboot();
        assert(!memcmp(memory + MACRO_START, tap_a, sizeof(tap_a)));
        for (int r = 0; r < 4; ++r) { assert(!rules[r][CA_ENABLED]); }
    }
    // Interrupted edits can retain the previous valid rule or become disabled,
    // but must never produce a different enabled action.
    fresh(); enable(0, CA_WINDOWS, 1, 0);
    memcpy(before, memory, sizeof(before));
    for (int cut = 0; cut < 67; ++cut) {
        memcpy(memory, before, sizeof(memory)); cut_after = -1; reboot();
        cut_after = cut;
        if (!setjmp(power_cut)) { set_field(0, CA_MACRO, 15); }
        cut_after = -1; reboot();
        assert(!rules[0][CA_ENABLED] || rules[0][CA_MACRO] == 0 || rules[0][CA_MACRO] == 15);
    }
}

static void engine_tests(void) {
    // Configuring a rule on an already connected keyboard does not run it.
    fresh(); put_macro(0, tap_a, sizeof(tap_a));
    assert(set_field(0, CA_ENABLED, 1) == id_custom_set_value);
    advance(1500); assert(downs == 0);
    reconnect(); advance(1500); assert(downs == 1);

    for (uint8_t os = CA_WINDOWS; os <= CA_UNKNOWN; ++os) {
        fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, os, 1, 0);
        const os_variant_t values[] = {OS_UNSURE, OS_WINDOWS, OS_MACOS, OS_LINUX, OS_IOS, OS_UNSURE};
        detected = values[os]; advance(1200); assert(downs == 1 && !has_anykey());
        advance(1000); assert(downs == 1);
        detected = OS_LINUX; advance(1000); assert(downs == 1); // no OS-update replay
        reconnect(); detected = values[os]; advance(1200); assert(downs == 2);
    }
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_WINDOWS, 1, 0);
    detected = OS_UNSURE; advance(1200); assert(downs == 0);
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_WINDOWS, 1, 0);
    m_host_driver = driver = NULL; advance(1200); assert(downs == 0);
    m_host_driver = driver = &rf_host_driver; advance(1200); assert(downs == 0);
    m_host_driver = &usb_driver;

    for (uint8_t profile = 0; profile < 4; ++profile) {
        fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_UNKNOWN, profile ? profile + 1 : 5, 0);
        dev_info.link_mode = profile; driver = &rf_host_driver;
        advance(1200); assert(downs == 0); // link_mode alone is never Ready
        connection_actions_rf_status((profile + 1) % 4, true);
        advance(1200); assert(downs == 0); // wrong profile ignored
        connection_actions_rf_status(profile, true);
        advance(1200); assert(downs == 1);
        for (int i = 0; i < 20; ++i) { connection_actions_rf_status(profile, true); advance(100); }
        assert(downs == 1);
        connection_actions_rf_status(profile, false);
        connection_actions_rf_status(profile, true);
        advance(1200); assert(downs == 2);
    }
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_WINDOWS, 0, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true); advance(1200); assert(downs == 0); // USB OS not RF OS

    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_ANY_OS, 0, 10);
    advance(700); connection_actions_invalidate();
    dev_info.link_mode = LINK_BT_2; driver = &rf_host_driver; advance(3000); assert(downs == 0);
    connection_actions_rf_status(LINK_BT_2, true); advance(2200); assert(downs == 1);

    // Win/Mac dial debounce must not create another connection session.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_ANY_OS, 0, 0);
    advance(1200); assert(downs == 1);
    connection_actions_user_activity(); f_dial_sw_init_ok = false; advance(200);
    f_dial_sw_init_ok = true; advance(1200); assert(downs == 1);

    // Local RF transitions/sleep and a missing status poll are not disconnects.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_ANY_OS, 0, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true); advance(1200); assert(downs == 1);
    dev_info.rf_state = 2; advance(1200);
    dev_info.rf_state = RF_CONNECT; advance(1200); assert(downs == 1);
    f_wakeup_prepare = true; advance(1200);
    f_wakeup_prepare = false; advance(1200); assert(downs == 1);
    now += 1000; connection_actions_task();
    connection_actions_rf_status(LINK_BT_1, true); advance(1200); assert(downs == 1);

    // USB is ready independently of an RF sleep flag. LED/protocol callbacks
    // don't retrigger it, but a bus reset does, even entirely between tasks.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_ANY_OS, 1, 0);
    f_rf_sleep = true; advance(1200); assert(downs == 1);
    notify_usb_device_state_change_kb((struct usb_device_state){USB_DEVICE_STATE_CONFIGURED});
    advance(1200); assert(downs == 1);
    notify_usb_device_state_change_kb((struct usb_device_state){USB_DEVICE_STATE_INIT});
    // The queued Reset notification must preserve already collected samples.
    assert(detected == OS_WINDOWS);
    notify_usb_device_state_change_kb((struct usb_device_state){USB_DEVICE_STATE_CONFIGURED});
    advance(1200); assert(downs == 2);

    // RF backpressure, physical input, and upload cancellation release a held modifier.
    const uint8_t delayed[] = {1, 2, 0xE0, 1, 4, '2', '0', '0', '0', '|', 1, 1, 4, 0};
    fresh(); put_macro(0, delayed, sizeof(delayed)); enable(0, CA_ANY_OS, 0, 0);
    advance(700); assert(held[0xE0]);
    connection_actions_user_activity(); advance(3000); assert(!has_anykey() && downs == 1);
    reconnect(); advance(700); assert(held[0xE0]);
    dynamic_keymap_macro_changed(); assert(!has_anykey());
    reconnect(); dev_info.link_mode = LINK_BT_3; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_3, true); queue_empty = false;
    unsigned old = downs; advance(1200); assert(downs == old);
    queue_empty = true; advance(100); assert(downs == old + 1);
    connection_actions_invalidate(); assert(!has_anykey());

    // RF loss callbacks run while the UART receive frame is still held. They
    // must not emit HID/UART recursively; the next main task releases it safely.
    fresh(); put_macro(0, delayed, sizeof(delayed)); enable(0, CA_ANY_OS, 0, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true); advance(700); assert(held[0xE0]);
    receiving = true; queue_empty = false;
    connection_actions_rf_status(LINK_BT_1, false);
    assert(queue_empty && held[0xE0]);
    connection_actions_rf_status(LINK_BT_1, true);
    receiving = false;
    connection_actions_task(); assert(!has_anykey());
    advance(700); assert(downs == 2); // new interval from the observed loss
    connection_actions_invalidate();

    fresh(); put_macro(0, tap_a, sizeof(tap_a));
    for (int r = 0; r < 4; ++r) { enable(r, CA_ANY_OS, 1, 0); }
    advance(2000); assert(downs == 4 && !has_anykey());
    // Malformed delay is rejected before any HID, including earlier down tokens.
    const uint8_t malformed[] = {1, 2, 0xE0, 1, 4, '9', 'x', 0};
    fresh(); put_macro(0, malformed, sizeof(malformed)); enable(0, CA_ANY_OS, 0, 0);
    advance(2000); assert(downs == 0 && !has_anykey());
    const uint8_t text[] = {'A','a',0};
    fresh(); put_macro(0, text, sizeof(text)); enable(0, CA_ANY_OS, 0, 0);
    advance(2000); assert(downs == 3 && !has_anykey());
    // Unsigned timer arithmetic survives wraparound.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); enable(0, CA_ANY_OS, 0, 1);
    now = UINT32_MAX - 200; advance(2000); assert(downs == 1);
}

static void before_rule(uint8_t slot, uint8_t os, uint8_t link, uint8_t delay) {
    enable(slot, os, link, delay);
    assert(set_field(slot, CA_TRIGGER, CA_BEFORE_SWITCH) == id_custom_set_value);
}

static void departure_tests(void) {
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_WINDOWS, 1, 0);
    advance(1200); assert(downs == 0); // before rules never run on arrival
    assert(connection_actions_before_switch(LINK_USB)); // same source, no event
    assert(!connection_actions_before_switch(LINK_BT_1));
    advance(80); assert(downs == 1 && driver == &usb_driver && dev_info.link_mode == LINK_USB);
    assert(!connection_actions_before_switch(LINK_BT_1)); // final release must drain
    advance(300); assert(connection_actions_before_switch(LINK_BT_1));
    assert(!has_anykey() && downs == 1);
    advance(500); assert(downs == 1); // retrying a completed latch never replays
    connection_actions_invalidate();
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true); advance(1500); assert(downs == 1);

    // Conditions refer to the SOURCE; wireless never borrows the USB OS guess.
    for (uint8_t source = LINK_RF_24; source <= LINK_USB; ++source) {
        fresh(); put_macro(0, tap_a, sizeof(tap_a));
        before_rule(0, CA_ANY_OS, 0, 0);
        dev_info.link_mode = source;
        driver = source == LINK_USB ? &usb_driver : &rf_host_driver;
        if (source != LINK_USB) { connection_actions_rf_status(source, true); }
        uint8_t target = source == LINK_USB ? LINK_BT_2 : LINK_USB;
        assert(!connection_actions_before_switch(target));
        advance(500); assert(downs == 1 && connection_actions_before_switch(target));
        assert(dev_info.link_mode == source && !has_anykey());
    }
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_WINDOWS, 1, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true);
    assert(connection_actions_before_switch(LINK_USB)); assert(downs == 0);

    // Fn/profile keys held at request: wait for release without cancelling.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 1, 1);
    physical = true; assert(!connection_actions_before_switch(LINK_BT_1));
    advance(300); assert(downs == 0);
    physical = false; connection_actions_key_event(false);
    advance(500); assert(downs == 1 && connection_actions_before_switch(LINK_BT_1));

    // Reversing the dial cancels the old request without sending to a new host.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 1, 10);
    assert(!connection_actions_before_switch(LINK_BT_1)); advance(300);
    assert(connection_actions_before_switch(LINK_USB)); advance(1500); assert(downs == 0);

    // A disconnected source cannot block or send the macro to the destination.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 1, 10);
    assert(!connection_actions_before_switch(LINK_BT_1));
    configured = USB_DEVICE_STATE_INIT; advance(100);
    assert(connection_actions_before_switch(LINK_BT_1) && downs == 0);

    // Cancellation releases unbalanced downs and still permits the requested switch.
    const uint8_t long_macro[] = {1,2,0xE0,1,4,'6','5','5','3','5','|',1,1,4,0};
    fresh(); put_macro(0, long_macro, sizeof(long_macro)); before_rule(0, CA_ANY_OS, 1, 0);
    assert(!connection_actions_before_switch(LINK_BT_1)); advance(100); assert(held[0xE0]);
    advance(15500); assert(connection_actions_before_switch(LINK_BT_1) && !has_anykey());
    assert(downs == 1);

    // RF release queue backpressure is drained before committing.
    fresh(); put_macro(0, tap_a, sizeof(tap_a)); before_rule(0, CA_ANY_OS, 2, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true);
    assert(!connection_actions_before_switch(LINK_BT_2));
    queue_empty = false; advance(300); assert(downs == 0);
    queue_empty = true; advance(100); assert(downs == 1);
    queue_empty = false; advance(500); assert(!connection_actions_before_switch(LINK_BT_2));
    queue_empty = true; advance(300); assert(connection_actions_before_switch(LINK_BT_2));

    // Lost UART status: no HID in receive callback, no replay after a newer Ready.
    fresh(); put_macro(0, long_macro, sizeof(long_macro)); before_rule(0, CA_ANY_OS, 2, 0);
    dev_info.link_mode = LINK_BT_1; driver = &rf_host_driver;
    connection_actions_rf_status(LINK_BT_1, true);
    assert(!connection_actions_before_switch(LINK_USB)); advance(100); assert(held[0xE0]);
    receiving = true; connection_actions_rf_status(LINK_BT_1, false);
    connection_actions_rf_status(LINK_BT_1, true); receiving = false;
    advance(100); assert(connection_actions_before_switch(LINK_USB) && !has_anykey());
    assert(downs == 1);

    // Schema stays compatible with stored after rules; new trigger survives reboot.
    fresh(); before_rule(3, CA_ANY_OS, 6, 100); reboot();
    assert(storage_status == CA_AVAILABLE && rules[3][CA_TRIGGER] == CA_BEFORE_SWITCH);
    puts("PASS: before-switch source routing, drain, cancellation, timeout, RF loss and persistence");
}

int main(void) {
    persistence_tests(); engine_tests(); departure_tests();
    puts("PASS: production engine + EEPROM backend, migration/power cuts, VIA validation, USB/RF sessions, cancellation and macro playback");
    return 0;
}
