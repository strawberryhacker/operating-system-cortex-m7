/* Copyright (C) StrawberryHacker */

#include "usb_hid.h"
#include "usb_debug.h"
#include "print.h"
#include "usb_protocol.h"

/* Static declarations */
static u8 usb_hid_connect(struct usb_iface* iface);
static u8 usb_hid_default(struct usb_iface* iface);

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

static void hid_callback(struct urb* urb)
{
    urb->acctual_length = 7;
}

static void usb_hid_start(struct urb* )

static u8 usb_hid_connect(struct usb_iface* iface)
{
    print("Loading the driver\n");
    /* THis is where is assigns the driver */
    iface->driver = &hid_driver;

    /* Get descriptors */
    struct usb_dev* dev = iface->parent_dev;

    for (u32 i = 0; i < iface->num_eps; i++) {
        usb_print_ep_desc(&iface->eps[i].desc);
    }

    struct usb_ep_desc* ep = &iface->eps[0];

    /* Ask the USB host controller for a pipe */
    struct usb_pipe* pipe = usbhc_request_pipe();
    print("Pipe number => %d\n", pipe->num);

    if (pipe == NULL) {
        return 0;
    }
    if (!usbc_iface_add_pipe(pipe, iface)) {
        return 0;
    }

    struct pipe_config pipe_cfg = {
        .endpoint = usb_get_ep_number(ep->bEndpointAddress),
        .autoswitch = 0,
        .banks = 1,
        .device = iface->parent_dev->address,
        .frequency = 0,
        .size = PIPE_SIZE_8,
        .token = PIPE_TOKEN_IN,
        .type = PIPE_TYPE_INT
    };

    usbhc_set_address(pipe, iface->parent_dev->address);
    usbhc_set_ep_size(pipe, ep->wMaxPacketSize);

    /* MAke a URB */
    struct urb* urb = usbhc_alloc_urb();

    usb_hid_start(urb);

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
