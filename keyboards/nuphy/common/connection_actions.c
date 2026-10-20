// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
#include "connection_actions.h"

#ifdef VIA_ENABLE
#    include <string.h>
#    include "user_kb.h"
#    include "os_detection.h"
#    include "dynamic_keymap.h"
#    include "eeprom.h"
#    include "via.h"
#    include "rf_queue.h"
#    include "send_string.h"

extern host_driver_t *m_host_driver;
extern host_driver_t rf_host_driver;
extern rf_queue_t rf_queue;
extern bool f_dial_sw_init_ok;
void clear_report_buffer_and_queue(void);

#if DYNAMIC_KEYMAP_MACRO_COUNT != 16
#    error Update Connection Actions VIA JSON macro options when changing macro count
#endif

#define CA_RULES 4
#define CA_FIELDS 7
#define CA_TAIL_SIZE 64
#define CA_ADDR (TOTAL_EEPROM_BYTE_COUNT - CA_TAIL_SIZE)
#define CA_CHANNEL 0
#define CA_SETTLE_MS 500
#define CA_REPORT_MS 20
#define CA_SWITCH_TIMEOUT_MS 15000
#define CA_SWITCH_DRAIN_MS 100

// Stable VIA wire values, independent of QMK's OS enum and NuPhy's link enum.
enum { CA_ANY_OS, CA_WINDOWS, CA_MACOS, CA_LINUX, CA_IOS, CA_UNKNOWN };
enum { CA_ENABLED, CA_TRIGGER, CA_OS, CA_CONNECTION, CA_ACTION, CA_MACRO, CA_DELAY };
enum { CA_AFTER_READY, CA_BEFORE_SWITCH };
enum { CA_NO_SPACE, CA_AVAILABLE, CA_BAD_CONFIG };
static const uint8_t reservation[4] = {'N', 'C', 'A', '!'};
static const uint8_t signature[4]   = {'N', 'C', 'A', '1'};
static uint8_t rules[CA_RULES][CA_FIELDS];
static uint8_t storage_status;

// USB callbacks may run outside the main task. Only invalidate there; never send HID.
static volatile uint32_t usb_epoch;
static volatile usb_configure_state_t usb_state;
static uint32_t seen_usb_epoch;
static bool rf_confirmed;
static bool observed_loss;
static uint8_t rf_profile;
static uint32_t rf_seen_at;
static bool session;
static bool evaluated;
static uint8_t session_link;
static uint8_t pending;
static uint32_t ready_at;
static uint32_t due[CA_RULES];
static struct {
    bool active, done, draining;
    uint8_t source, target;
    uint32_t at, drain_at, epoch;
} departure;

// A non-blocking reader of the existing VIA/send_string encoding. One HID change
// per task, with backpressure on RF. Physical input always cancels playback.
static struct {
    bool active;
    uint16_t pos, end;
    uint32_t at, wait;
    uint8_t count, next;
    struct { uint8_t key; bool down; } events[8];
} player;

static bool reserved(void) {
    user_config_t stored;
    eeconfig_read_kb_datablock(&stored, 0, sizeof(stored));
    return memcmp(stored.reserved, reservation, sizeof(reservation)) == 0;
}

// EEPROM backend hook: keep the original size until a successful reservation.
// Never move the keymap, encoder map, or beginning of the macro buffer.
uint32_t dynamic_keymap_macro_eeprom_size(uint32_t available) {
    return reserved() && available > CA_TAIL_SIZE + 100 ? available - CA_TAIL_SIZE : available;
}

static uint16_t checksum(const uint8_t *data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    while (length--) {
        crc ^= (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; ++i) { crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1; }
    }
    return crc;
}

static bool valid_field(uint8_t field, uint8_t value) {
    switch (field) {
        case CA_ENABLED: return value <= 1;
        case CA_TRIGGER: return value <= CA_BEFORE_SWITCH;
        case CA_ACTION: return value == 0;
        case CA_OS: return value <= CA_UNKNOWN;
        case CA_CONNECTION: return value <= 6; // Any, USB, BT1/2/3, 2.4G, BT Any
        case CA_MACRO: return value < dynamic_keymap_macro_get_count();
        case CA_DELAY: return value <= 100; // units of 100 ms
    }
    return false;
}

static void save_rules(void) {
    uint8_t image[CA_TAIL_SIZE] = {0};
    image[4] = 1; // schema
    image[5] = CA_RULES;
    memcpy(image + 8, rules, sizeof(rules));
    memcpy(image, signature, sizeof(signature));
    uint16_t crc = checksum(image, CA_TAIL_SIZE - 4);
    image[CA_TAIL_SIZE - 4] = crc >> 8;
    image[CA_TAIL_SIZE - 3] = crc;
    // Invalid first, commit last. Interrupted saves load disabled defaults.
    eeprom_update_byte((uint8_t *)(uintptr_t)CA_ADDR, 0);
    eeprom_update_block(image + 4, (void *)(uintptr_t)(CA_ADDR + 4), CA_TAIL_SIZE - 4);
    eeprom_update_block(image, (void *)(uintptr_t)CA_ADDR, 4);
    storage_status = CA_AVAILABLE;
}

void dynamic_keymap_macro_changed(void) {
    connection_actions_user_activity();
}

static uint8_t macro_byte(uint16_t offset) {
    uint8_t value;
    dynamic_keymap_macro_get_buffer(offset, 1, &value);
    return value;
}

static bool can_reserve(void) {
    uint16_t size = dynamic_keymap_macro_get_buffer_size();
    if (size <= CA_TAIL_SIZE + 100 || macro_byte(size - 1) != 0) { return false; }
    // Conservative: require all M0..Mn terminators before the new validity flag,
    // and only zero padding thereafter, including every byte to be reserved.
    uint16_t pos = 0;
    uint8_t count = 0;
    while (pos < size - CA_TAIL_SIZE - 1 && count < dynamic_keymap_macro_get_count()) {
        if (macro_byte(pos++) == 0) { ++count; }
    }
    if (count != dynamic_keymap_macro_get_count()) { return false; }
    while (pos < size) {
        if (macro_byte(pos++) != 0) { return false; }
    }
    return true;
}

static void stop_player(void) {
    if (!player.active) { return; }
    player.active = false;
    // Playback starts only from idle and is cancelled BEFORE physical processing.
    // A final zero report also releases unbalanced user-authored macro downs.
    break_all_key();
}

void connection_actions_user_activity(void) {
    pending = 0;
    stop_player();
}

void connection_actions_key_event(bool pressed) {
    // Releasing the Fn/profile keys that requested a switch lets the source
    // become idle. A new press or input during playback still has priority.
    if (!pressed && departure.active && !player.active) { return; }
    connection_actions_user_activity();
}

void connection_actions_cancel_switch(void) {
    if (departure.active) { connection_actions_user_activity(); }
    memset(&departure, 0, sizeof(departure));
}

void connection_actions_invalidate(void) {
    connection_actions_cancel_switch();
    connection_actions_user_activity();
    session = false;
    rf_confirmed = false;
}

void connection_actions_observed_change(void) {
    // rf_protocol_receive() still owns rcv_start. Sending UART/HID here would
    // deadlock uart_send_bytes(), which waits for that receive frame to finish.
    observed_loss = true;
    rf_confirmed = false;
    if (player.active) { clear_report_buffer_and_queue(); }
}

void connection_actions_rf_status(uint8_t profile, bool connected) {
    if (profile != dev_info.link_mode || profile == LINK_USB) { return; }
    if (!connected) {
        if (rf_confirmed) { connection_actions_observed_change(); }
        return;
    }
    rf_seen_at = timer_read32();
    rf_profile = profile;
    rf_confirmed = true;
}

void notify_usb_device_state_change_kb(struct usb_device_state state) {
    // ChibiOS queues this notification. The fingerprint was already cleared
    // in the USB reset ISR; clearing here would erase new descriptor samples.
    if (state.configure_state != usb_state || state.configure_state == USB_DEVICE_STATE_INIT) {
        usb_state = state.configure_state;
        ++usb_epoch;
    }
    notify_usb_device_state_change_user(state);
}

void connection_actions_init(void) {
    connection_actions_invalidate();
    observed_loss = false;
    memset(rules, 0, sizeof(rules));
    storage_status = CA_NO_SPACE;
    if (!reserved()) {
        if (!can_reserve()) { return; }
        // Persist ownership BEFORE writing the tail. A power cut now leaves a
        // reserved but invalid block, never macro bytes mistaken for rules.
        memcpy(user_config.reserved, reservation, sizeof(reservation));
        eeconfig_update_kb_datablock_field(user_config, reserved);
        save_rules();
    }
    uint8_t image[CA_TAIL_SIZE];
    eeprom_read_block(image, (void *)(uintptr_t)CA_ADDR, sizeof(image));
    storage_status = CA_BAD_CONFIG;
    uint16_t crc = checksum(image, CA_TAIL_SIZE - 4);
    if (memcmp(image, signature, 4) || image[4] != 1 || image[5] != CA_RULES || image[60] != (crc >> 8) || image[61] != (uint8_t)crc || image[62] || image[63]) { return; }
    for (uint8_t r = 0; r < CA_RULES; ++r) {
        for (uint8_t f = 0; f < CA_FIELDS; ++f) {
            if (!valid_field(f, image[8 + r * CA_FIELDS + f])) { return; }
        }
    }
    memcpy(rules, image + 8, sizeof(rules));
    storage_status = CA_AVAILABLE;
}

static bool transport_ready(void) {
    if (dev_info.link_mode == LINK_USB) {
        return m_host_driver && m_host_driver != &rf_host_driver && usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED && host_get_driver() == m_host_driver;
    }
    return !f_wakeup_prepare && !f_rf_sleep && timer_elapsed32(rf_seen_at) < 650 && rf_confirmed && rf_profile == dev_info.link_mode && dev_info.rf_state == RF_CONNECT && host_get_driver() == &rf_host_driver;
}

static uint8_t host_os(void) {
    if (dev_info.link_mode != LINK_USB) { return CA_UNKNOWN; }
    switch (detected_host_os()) {
        case OS_WINDOWS: return CA_WINDOWS;
        case OS_MACOS: return CA_MACOS;
        case OS_LINUX: return CA_LINUX;
        case OS_IOS: return CA_IOS;
        default: return CA_UNKNOWN;
    }
}

static bool matches(uint8_t *rule, uint8_t os) {
    uint8_t connection = rule[CA_CONNECTION];
    // Existing link enum: 2.4G=0, BT1=1, BT2=2, BT3=3, USB=4.
    const uint8_t links[] = {0xFF, LINK_USB, LINK_BT_1, LINK_BT_2, LINK_BT_3, LINK_RF_24};
    bool link_match = connection == 0 || (connection < 6 && links[connection] == session_link) || (connection == 6 && session_link >= LINK_BT_1 && session_link <= LINK_BT_3);
    return rule[CA_ENABLED] && link_match && (rule[CA_OS] == CA_ANY_OS || rule[CA_OS] == os);
}

static bool idle(void) {
    if (has_anykey() || has_anymod() || get_oneshot_mods() || get_oneshot_locked_mods()) { return false; }
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        if (matrix_get_row(row)) { return false; }
    }
    return true;
}

// Validate the complete selected macro before emitting any report. Unlike the
// synchronous core sender, every read is bounded to the selected null terminator.
static bool start_macro(uint8_t id) {
    uint16_t size = dynamic_keymap_macro_get_buffer_size();
    if (id >= dynamic_keymap_macro_get_count() || !size || macro_byte(size - 1)) { return false; }
    uint16_t pos = 0;
    while (id && pos < size - 1) {
        if (macro_byte(pos++) == 0) { --id; }
    }
    if (id) { return false; }
    uint16_t start = pos;
    while (pos < size - 1) {
        uint8_t c = macro_byte(pos++);
        if (!c) {
            player.pos = start;
            player.end = pos - 1;
            player.next = player.count = 0;
            player.at = timer_read32();
            player.wait = 0;
            player.active = true;
            return true;
        }
        if (c >= 128) { return false; }
        if (c != SS_QMK_PREFIX) { continue; }
        if (pos >= size - 1) { return false; }
        uint8_t command = macro_byte(pos++);
        if (command >= SS_TAP_CODE && command <= SS_UP_CODE) {
            if (pos >= size - 1 || !macro_byte(pos++)) { return false; }
        } else if (command == SS_DELAY_CODE) {
            uint32_t delay = 0;
            bool digit = false;
            while (pos < size - 1 && (c = macro_byte(pos++)) >= '0' && c <= '9') {
                digit = true;
                delay = delay * 10 + c - '0';
                if (delay > 65535) { return false; }
            }
            if (!digit || c != '|') { return false; }
        } else { return false; }
    }
    return false;
}

static void event(uint8_t key, bool down) {
    player.events[player.count].key = key;
    player.events[player.count++].down = down;
}

static bool lut_bit(const uint8_t *lut, uint8_t c) {
    return (pgm_read_byte(lut + c / 8) >> (c % 8)) & 1;
}

static void player_task(void) {
    if (timer_elapsed32(player.at) < player.wait) { return; }
    if (dev_info.link_mode != LINK_USB && (!rf_queue.is_empty() || dequeue_delay)) { return; }
    if (player.next == player.count) {
        player.next = player.count = 0;
        if (player.pos == player.end) { stop_player(); return; }
        uint8_t c = macro_byte(player.pos++);
        if (c == SS_QMK_PREFIX) {
            uint8_t command = macro_byte(player.pos++);
            if (command == SS_DELAY_CODE) {
                uint32_t delay = 0;
                while ((c = macro_byte(player.pos++)) != '|') { delay = delay * 10 + c - '0'; }
                player.at = timer_read32();
                player.wait = delay;
                return;
            }
            uint8_t key = macro_byte(player.pos++);
            if (command != SS_UP_CODE) { event(key, true); }
            if (command != SS_DOWN_CODE) { event(key, false); }
        } else {
            uint8_t key = pgm_read_byte(ascii_to_keycode_lut + c);
            bool shift = lut_bit(ascii_to_shift_lut, c);
            bool altgr = lut_bit(ascii_to_altgr_lut, c);
            if (shift) { event(KC_LSFT, true); }
            if (altgr) { event(KC_RALT, true); }
            event(key, true);
            event(key, false);
            if (altgr) { event(KC_RALT, false); }
            if (shift) { event(KC_LSFT, false); }
            if (lut_bit(ascii_to_dead_lut, c)) { event(KC_SPC, true); event(KC_SPC, false); }
        }
    }
    uint8_t key = player.events[player.next].key;
    if (player.events[player.next++].down) { register_code(key); } else { unregister_code(key); }
    player.at = timer_read32();
    player.wait = CA_REPORT_MS;
    no_act_time = 0;
}

// A switch request is latched by the board and retried without changing link_mode,
// host driver, RF channel, or OS layer. Only the ordinary main task emits HID.
bool connection_actions_before_switch(uint8_t target) {
    if (target > LINK_USB || target == dev_info.link_mode) {
        connection_actions_cancel_switch();
        return true;
    }
    if (departure.active) {
        if (departure.target == target && departure.source == dev_info.link_mode) { return departure.done; }
        connection_actions_cancel_switch();
    }
    if (!f_dial_sw_init_ok || storage_status != CA_AVAILABLE || !transport_ready()) { return true; }
    connection_actions_user_activity();
    session_link = dev_info.link_mode;
    uint8_t os = host_os();
    for (uint8_t r = 0; r < CA_RULES; ++r) {
        if (rules[r][CA_TRIGGER] == CA_BEFORE_SWITCH && matches(rules[r], os)) {
            pending |= 1 << r;
            due[r] = (uint32_t)rules[r][CA_DELAY] * 100;
        }
    }
    if (!pending) { return true; }
    departure.active = true;
    departure.source = dev_info.link_mode;
    departure.target = target;
    departure.at = timer_read32();
    departure.epoch = usb_epoch;
    return false;
}

static void departure_task(void) {
    if (departure.done) { return; }
    // Loss must never redirect a partially played macro to the destination.
    if (departure.source != dev_info.link_mode || !transport_ready() ||
        (departure.source == LINK_USB && departure.epoch != usb_epoch) ||
        timer_elapsed32(departure.at) >= CA_SWITCH_TIMEOUT_MS) {
        connection_actions_user_activity();
        departure.done = true;
        return;
    }
    no_act_time = 0;
    if (player.active) { player_task(); return; }
    if (pending) {
        if (!idle()) { return; }
        for (uint8_t r = 0; r < CA_RULES; ++r) {
            if (!(pending & (1 << r))) { continue; }
            if (timer_elapsed32(departure.at) < due[r]) { return; }
            pending &= ~(1 << r);
            start_macro(rules[r][CA_MACRO]);
            return;
        }
    }
    // stop_player() sends a final release report. Let RF drain that report too,
    // then give the source host a short interval to process the shortcut.
    if (departure.source != LINK_USB && (!rf_queue.is_empty() || dequeue_delay)) { return; }
    if (!departure.draining) {
        departure.draining = true;
        departure.drain_at = timer_read32();
    }
    if (timer_elapsed32(departure.drain_at) >= CA_SWITCH_DRAIN_MS) { departure.done = true; }
}

void connection_actions_task(void) {
    if (observed_loss) {
        observed_loss = false;
        bool confirmed = rf_confirmed; // retain a newer Ready received in this batch
        if (departure.active) {
            connection_actions_user_activity();
            departure.done = true;
            session = false;
        } else {
            connection_actions_invalidate();
        }
        rf_confirmed = confirmed;
    }
    if (departure.active) { departure_task(); return; }
    if (!f_dial_sw_init_ok) { return; }
    if (seen_usb_epoch != usb_epoch) {
        seen_usb_epoch = usb_epoch;
        // USB enumeration may continue while an unrelated RF host is active.
        if (dev_info.link_mode == LINK_USB) { connection_actions_invalidate(); }
    }
    if (!transport_ready()) {
        if (session || player.active) {
            connection_actions_user_activity();
            // A local LINKING assignment, LED sleep, or missing poll is not proof
            // of a new RF connection. Retain deduplication until a real loss
            // report, profile change, or module reset invalidates it explicitly.
            if (dev_info.link_mode == LINK_USB || !rf_confirmed || session_link != dev_info.link_mode) {
                session = false;
            } else {
                evaluated = true;
            }
        }
        return;
    }
    if (session && session_link != dev_info.link_mode) { connection_actions_invalidate(); return; }
    if (!session) {
        session = true;
        evaluated = false;
        session_link = dev_info.link_mode;
        ready_at = timer_read32();
    }
    if (storage_status != CA_AVAILABLE) { pending = 0; evaluated = true; return; }
    if (!evaluated && timer_elapsed32(ready_at) >= CA_SETTLE_MS) {
        evaluated = true;
        uint8_t os = host_os();
        for (uint8_t r = 0; r < CA_RULES; ++r) {
            if (rules[r][CA_TRIGGER] == CA_AFTER_READY && matches(rules[r], os)) {
                pending |= 1 << r;
                due[r] = (uint32_t)rules[r][CA_DELAY] * 100;
            }
        }
    }
    if (player.active) { player_task(); return; }
    if (!idle()) { return; }
    for (uint8_t r = 0; r < CA_RULES; ++r) {
        if (!(pending & (1 << r))) { continue; }
        if (timer_elapsed32(ready_at) < CA_SETTLE_MS + due[r]) { continue; }
        pending &= ~(1 << r); // consume even empty/invalid macros; never retry in this session
        start_macro(rules[r][CA_MACRO]);
        return;
    }
}

static void label_reply(uint8_t *data, uint8_t length, const char *text) {
    // Keep the command/channel/value header intact and always terminate UTF-8.
    uint8_t available = length - 3;
    memset(data + 3, 0, available);
    uint8_t count = strlen(text);
    if (count >= available) { count = available - 1; }
    memcpy(data + 3, text, count);
}

static const char *os_label(void) {
    if (dev_info.link_mode != LINK_USB) { return "Unknown (wireless)"; }
    switch (host_os()) {
        case CA_WINDOWS: return "Windows (USB detected)";
        case CA_MACOS: return "macOS (USB detected)";
        case CA_LINUX: return "Linux (USB detected)";
        case CA_IOS: return "iOS/iPadOS (USB detected)";
        default: return "Unknown (USB)";
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    if (length < 3 || data[1] != CA_CHANNEL) { if (length) { data[0] = id_unhandled; } return; }
    // Text labels use separate IDs: retain numeric IDs 0/29 for compatibility.
    if (data[0] == id_custom_get_value && length >= 4 && (data[2] == 30 || data[2] == 31)) {
        const char *storage = storage_status == CA_AVAILABLE ? "Ready" : storage_status == CA_NO_SPACE ? "Unavailable: macro space" : "Invalid rules: edit to reset";
        label_reply(data, length, data[2] == 30 ? storage : os_label());
        return;
    }
    // Numeric status 0=no room, 1=ready, 2=invalid schema/CRC.
    if (data[0] == id_custom_get_value && data[2] == 0 && length >= 4) { data[3] = storage_status; return; }
    if (data[0] == id_custom_get_value && data[2] == 29 && length >= 4) { data[3] = dev_info.link_mode == LINK_USB && host_os() != CA_UNKNOWN; return; }
    // VIA matches the complete SET request against its echo. Informational
    // dropdowns ignore writes, but must not substitute their GET value here.
    if (data[0] == id_custom_set_value && (data[2] == 0 || data[2] == 29) && length >= 4) { return; }
    if (data[0] == id_custom_save && reserved()) { save_rules(); return; }
    if (length < 4 || data[2] < 1 || data[2] > CA_RULES * CA_FIELDS) { data[0] = id_unhandled; return; }
    uint8_t slot = (data[2] - 1) / CA_FIELDS;
    uint8_t field = (data[2] - 1) % CA_FIELDS;
    if (data[0] == id_custom_get_value) { data[3] = rules[slot][field]; return; }
    if (data[0] == id_custom_set_value && reserved() && valid_field(field, data[3])) {
        connection_actions_user_activity();
        if (transport_ready() && f_dial_sw_init_ok) {
            session = true;
            session_link = dev_info.link_mode;
            evaluated = true;
            seen_usb_epoch = usb_epoch;
        }
        rules[slot][field] = data[3];
        save_rules(); // also support VIA clients that do not issue custom_save
        return;
    }
    data[0] = id_unhandled;
}
#endif
