#pragma once
#include <stdbool.h>
typedef enum {USB_DEVICE_STATE_NO_INIT, USB_DEVICE_STATE_INIT, USB_DEVICE_STATE_CONFIGURED, USB_DEVICE_STATE_SUSPEND} usb_configure_state_t;
struct usb_device_state {usb_configure_state_t configure_state;};
void notify_usb_device_state_change_user(struct usb_device_state state);
usb_configure_state_t usb_device_state_get_configure_state(void);
