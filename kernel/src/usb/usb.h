/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhs.h"

/*
 * USB core device structure
 */
struct usb_device {
    u32 enum_delay;
    u8 enumerated;
};

/*
 * Core init
 */
void usb_init(struct usb_core* core);

#endif