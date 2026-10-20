// SPDX-License-Identifier: GPL-2.0-or-later
// Native integration of actual ChibiOS ISR/queue + QMK OS detection + CA hook.
#define CA_TEST_REAL_OS
#define main engine_test_main
#include "../../keyboards/nuphy/common/tests/test_connection_actions.c"
#undef main
#include "../../quantum/os_detection.c"
fast_timer_t timer_read_fast(void) { return now; }
uint32_t timer_elapsed_fast(fast_timer_t at) { return now - at; }
#define OS_DETECTION_ENABLE
#define OS_DETECTION_RESET_ON_USB_BUS_RESET
#define USB_ENDPOINT_IN_COUNT 1
#define USB_ENDPOINT_OUT_COUNT 1
#define USB_PROTOCOL_REPORT 1
typedef enum {USB_EVENT_ADDRESS,USB_EVENT_CONFIGURED,USB_EVENT_SUSPEND,USB_EVENT_UNCONFIGURED,USB_EVENT_RESET,USB_EVENT_WAKEUP,USB_EVENT_STALLED} usbevent_t;
typedef struct {int configuration;} USBDriver;
static USBDriver USB_DRIVER = {1};
static int usb_endpoints_in[1],usb_endpoints_out[1];
static void osalSysLockFromISR(void) {}
static void osalSysUnlockFromISR(void) {}
static void chSysLockFromISR(void) {}
static void chSysUnlockFromISR(void) {}
static void usb_endpoint_in_configure_cb(int *p) {(void)p;}
static void usb_endpoint_out_configure_cb(int *p) {(void)p;}
static void usb_endpoint_in_suspend_cb(int *p) {(void)p;}
static void usb_endpoint_out_suspend_cb(int *p) {(void)p;}
static void usb_endpoint_in_wakeup_cb(int *p) {(void)p;}
static void usb_endpoint_out_wakeup_cb(int *p) {(void)p;}
static void suspend_wakeup_init(void) {}
static void state_notify(usb_configure_state_t value) {
    configured = value;
    struct usb_device_state state = {value};
    notify_usb_device_state_change_kb(state);
    os_detection_notify_usb_device_state_change(state);
}
static void usb_device_state_set_suspend(bool b,int n) {(void)b;(void)n;state_notify(USB_DEVICE_STATE_SUSPEND);}
static void usb_device_state_set_resume(bool b,int n) {(void)n;state_notify(b?USB_DEVICE_STATE_CONFIGURED:USB_DEVICE_STATE_INIT);}
static void usb_device_state_set_configuration(bool b,int n) {(void)n;state_notify(b?USB_DEVICE_STATE_CONFIGURED:USB_DEVICE_STATE_INIT);}
static void usb_device_state_set_reset(void) {state_notify(USB_DEVICE_STATE_INIT);}
static void usb_device_state_set_protocol(int p) {(void)p;state_notify(configured);}
/* PRODUCTION_USB_EVENTS */
static void samples(const uint16_t *values, unsigned count) {
    for(unsigned i=0;i<count;++i) {process_wlength(values[i]);now+=2;}
}
static void text_equals(uint8_t id,const char *expected) {
    uint8_t data[32]; memset(data,0xCC,sizeof(data));
    data[0]=id_custom_get_value;data[1]=0;data[2]=id;
    via_custom_value_command_kb(data,sizeof(data));
    assert(data[0]==id_custom_get_value && data[1]==0 && data[2]==id);
    assert(!strcmp((char *)data+3,expected));
    assert(data[31]==0);
}
int main(void) {
    const uint16_t win[]={0xFF,0xFF,4,0x24,4,0x24};
    const uint16_t mac[]={2,0x4E,2,0x1C,2,0x1A,0xFF,0xFF};
    const uint16_t linux[]={0xFF,0xFF,0xFF};
    const uint16_t ios[]={2,0x24,2,0x28};
    const uint16_t *seqs[]={win,mac,linux,ios};
    const unsigned sizes[]={6,8,3,4};
    const os_variant_t systems[]={OS_WINDOWS,OS_MACOS,OS_LINUX,OS_IOS};
    const char *names[]={"Windows (USB detected)","macOS (USB detected)","Linux (USB detected)","iOS/iPadOS (USB detected)"};
    fresh();erase_wlength_data();usb_event_queue_init();
    for(unsigned i=0;i<4;++i) {
        usb_event_cb(&USB_DRIVER,USB_EVENT_RESET);
        assert(detected_host_os()==OS_UNSURE); // old OS gone immediately
        // Descriptors can arrive BEFORE the queued Reset reaches the main task.
        samples(seqs[i],sizes[i]);
        assert(detected_host_os()==systems[i]);
        usb_event_queue_task(); // used to erase the newly detected OS here
        assert(detected_host_os()==systems[i]);
        usb_event_cb(&USB_DRIVER,USB_EVENT_CONFIGURED);
        usb_event_queue_task();now+=300;os_detection_task();
        assert(reported_os==systems[i]);
        text_equals(31,names[i]);
    }
    // A genuinely unrecognised enumeration remains Unknown, never Windows.
    usb_event_cb(&USB_DRIVER,USB_EVENT_RESET);usb_event_queue_task();
    assert(detected_host_os()==OS_UNSURE);text_equals(31,"Unknown (USB)");
    dev_info.link_mode=LINK_BT_1;text_equals(31,"Unknown (wireless)");
    dev_info.link_mode=LINK_USB;
    text_equals(30,"Ready");
    storage_status=CA_NO_SPACE;text_equals(30,"Unavailable: macro space");
    storage_status=CA_BAD_CONFIG;text_equals(30,"Invalid rules: edit to reset");
    // All packet lengths, including a one-byte text payload, remain bounded.
    for(unsigned length=0;length<=32;++length) {
        uint8_t guarded[34];memset(guarded,0xAA,sizeof(guarded));
        guarded[0]=id_custom_get_value;guarded[1]=0;guarded[2]=31;
        via_custom_value_command_kb(guarded,length);
        assert(guarded[32]==0xAA && guarded[33]==0xAA);
        if(length>=4) {assert(guarded[length-1]==0);}
    }
    // The real fingerprint now matches a Windows-specific before rule.
    fresh();erase_wlength_data();usb_event_queue_init();
    put_macro(0,tap_a,sizeof(tap_a));before_rule(0,CA_WINDOWS,1,0);
    usb_event_cb(&USB_DRIVER,USB_EVENT_RESET);samples(win,6);
    usb_event_queue_task();usb_event_cb(&USB_DRIVER,USB_EVENT_CONFIGURED);usb_event_queue_task();
    now+=300;os_detection_task();
    assert(!connection_actions_before_switch(LINK_BT_1));
    advance(500);assert(downs==1 && connection_actions_before_switch(LINK_BT_1));
    puts("PASS: real OS fingerprint survives queued USB Reset; Windows/macOS/Linux/iOS and UTF-8 status packets");
    return 0;
}
