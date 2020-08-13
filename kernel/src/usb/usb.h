/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"

struct usb_core {
    /* Some other thing in here */
};

void usb_init(struct usbhc* hc);

#endif