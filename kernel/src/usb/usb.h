/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"

enum usb_enum_state {
    USB_ENUM_IDLE,
    USB_ENUM_GET_DEV_DESC,
    USB_ENUM_SET_ADDRESS,
    USB_ENUM_GET_CONFIG_DESC
};

struct usb_core {
    /* Enumeration state */
    enum usb_enum_state enum_state;
};

void usb_init(struct usb_core* core, struct usbhc* hc);

#endif