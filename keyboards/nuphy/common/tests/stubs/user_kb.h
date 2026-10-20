#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#ifndef MATRIX_ROWS
#define MATRIX_ROWS 6
#endif
#ifndef MATRIX_COLS
#define MATRIX_COLS 17
#endif
#define LINK_RF_24 0
#define LINK_BT_1 1
#define LINK_BT_2 2
#define LINK_BT_3 3
#define LINK_USB 4
#define RF_CONNECT 3
#define KC_LSFT 0xE1
#define KC_RALT 0xE6
#define KC_SPC 0x2C
typedef struct {uint8_t existing[28];uint8_t reserved[4];} user_config_t;
extern user_config_t user_config;
typedef struct {uint8_t link_mode,rf_state,rf_channel,ble_channel;} device_t;
extern device_t dev_info;
typedef struct {int unused;} host_driver_t;
extern bool f_wakeup_prepare,f_rf_sleep;
extern uint32_t dequeue_delay,no_act_time;
void eeconfig_read_kb_datablock(void *buf,uint32_t offset,uint32_t size);
void test_update_config(const void *buf,uint32_t offset,uint32_t size);
#define eeconfig_update_kb_datablock_field(config, field) test_update_config(&(config).field,offsetof(user_config_t,field),sizeof((config).field))
uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t at);
host_driver_t *host_get_driver(void);
uint8_t has_anymod(void);
uint8_t has_anykey(void);
uint8_t get_oneshot_mods(void);
uint8_t get_oneshot_locked_mods(void);
uint32_t matrix_get_row(uint8_t row);
void register_code(uint8_t key);
void unregister_code(uint8_t key);
void break_all_key(void);
