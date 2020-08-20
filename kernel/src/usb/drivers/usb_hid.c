/* Copyright (C) StrawberryHacker */

#include "usb_hid.h"
#include "usb_debug.h"
#include "print.h"

const char hid_name[] = "USB HID class";

u8 usb_hid_probe(struct usb_iface* iface)
{
    usb_debug_print_iface_desc(iface);

    if (iface->desc.bInterfaceClass == 0x03) {
        return 1;
    } else {
        return 0;
    }
}

u8 usb_hid_start(struct usb_iface* iface)
{
    /* Ask for the HID spesific descriptors */

    /* Read the endpoint descriptors */
    
    /* Ask the USB host controller for a free pipe */

    /* Use the ep descriptors to configure the pipe */
}

void usb_hid_init(struct usb_driver* hid_driver)
{
    hid_driver->name = hid_name;
    hid_driver->probe = &usb_hid_probe;
}
