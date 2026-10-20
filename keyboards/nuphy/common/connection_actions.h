// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef VIA_ENABLE
void connection_actions_init(void);
void connection_actions_task(void);
// Retry from the main task until true; keeps the OLD transport active meanwhile.
// Conditions match the source connection. Never call from a USB/UART callback.
bool connection_actions_before_switch(uint8_t target);
void connection_actions_cancel_switch(void);
// End a session for a different profile, a module reset, or confirmed loss.
// A command requesting pairing/sleep is not itself evidence of a new session.
void connection_actions_invalidate(void);
// Receive-path variant: cancels queued RF reports now, defers HID releases.
void connection_actions_observed_change(void);
void connection_actions_rf_status(uint8_t profile, bool connected);
// Physical input has priority over an automatic macro. Does not create a session.
void connection_actions_user_activity(void);
void connection_actions_key_event(bool pressed);
#else
static inline void connection_actions_init(void) {}
static inline void connection_actions_task(void) {}
static inline bool connection_actions_before_switch(uint8_t target) { (void)target; return true; }
static inline void connection_actions_cancel_switch(void) {}
static inline void connection_actions_invalidate(void) {}
static inline void connection_actions_observed_change(void) {}
static inline void connection_actions_rf_status(uint8_t profile, bool connected) { (void)profile; (void)connected; }
static inline void connection_actions_user_activity(void) {}
static inline void connection_actions_key_event(bool pressed) { (void)pressed; }
#endif
