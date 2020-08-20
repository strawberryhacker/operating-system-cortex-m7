/* Copyright (C) StrawberryHacker */

#ifndef USB_HID_H
#define USB_HID_H

#include "types.h"
#include "usb.h"

struct usb_hid_device {
    u32 event;
    u32 x_cor;
    u32 y_cor;

    u8 is_enabled : 1;
};

void usb_hid_init(struct usb_driver* hid_driver);

#endif
