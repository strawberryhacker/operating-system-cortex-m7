/* Copyright (C) StrawberryHacker */

#include "usb_hid.h"
#include "usb_debug.h"
#include "print.h"
#include "usb_protocol.h"

extern struct usb_core* usbc_private;

/* Static declarations */
static u8 usb_hid_connect(struct usb_iface* iface);
static u8 usb_hid_default(struct usb_iface* iface);

static u8 hid_buffer[128];
static struct usb_setup_desc setup;

struct urb* hid_urb;

#define HID_NUM_DEV_IDS 1

/*
 * Definition of the driver and the device IDs
 */
static const struct usb_dev_id hid_dev_id[HID_NUM_DEV_IDS] = {
    {
        .iface_class = USB_CLASS_HID,
        .flags = USB_DEV_ID_IFACE_CLASS_MASK
    }
};

static struct usb_driver hid_driver = {
    .name = "HID Class Driver",
    
    .connect = &usb_hid_connect,
    .disconnect = &usb_hid_default,
    .suspend = &usb_hid_default,
    .resume = &usb_hid_default,
    
    /* Inforation used in driver matching */
    .dev_ids = hid_dev_id,
    .num_dev_ids = HID_NUM_DEV_IDS
};

static u8 usb_hid_connect(struct usb_iface* iface)
{
    print("Loading the driver\n");
    /* This is where it assigns the driver */
    iface->driver = &hid_driver;

    /* Get descriptors */
    struct usb_dev* dev = iface->parent_dev;

    for (u32 i = 0; i < iface->num_eps; i++) {
        usb_print_ep_desc(&iface->eps[i].desc);
    }
    
    /* Find: do we need any more interfaces? */
    printl("Driver has been added");
    return 1;
}

static u8 usb_hid_default(struct usb_iface* iface)
{
    return 1;
}

void usb_hid_init(struct usb_core* usbc)
{
    usbc_add_driver(&hid_driver, usbc);
}
