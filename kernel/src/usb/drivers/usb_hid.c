/* Copyright (C) StrawberryHacker */

#include "usb_hid.h"
#include "usb_debug.h"
#include "print.h"
#include "usb_protocol.h"

extern struct usb_core* usbc_private;

/* Static declarations */
static u8 usb_hid_connect(struct usb_iface* iface);
static u8 usb_hid_default(struct usb_iface* iface);

#define HID_NUM_DEV_IDS 1

static u8 hid_buffer[128];
static struct usb_setup_desc setup;

struct urb* hid_urb;

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
    print("Number bytes: %d\n", urb->acctual_length);
}

static void hid_enable_config_callback(struct urb* urb)
{
    print("Received => %d\n", urb->acctual_length);
    struct usb_iface* iface = (struct usb_iface *)urb->context;
    static u8 i = 0;
    if (i == 0) {
        urb->setup_buffer[0] = 0x21;
        urb->setup_buffer[1] = 0x0A;
        urb->setup_buffer[2] = 0;
        urb->setup_buffer[3] = 0;
        urb->setup_buffer[4] = 0;
        urb->setup_buffer[5] = 0;
        urb->setup_buffer[6] = 0;
        urb->setup_buffer[7] = 0;

        usbhc_submit_urb(urb, iface->pipes[1]);
        i++;
    } else if (i == 1){
        setup.bmRequestType = USB_HOST_TO_DEVICE;
        setup.bRequest      = USB_REQ_SET_INTERFACE;
        setup.wValue        = 0;
        setup.wIndex        = 1;
        setup.wLength       = 2;
        usbhc_submit_urb(urb, iface->pipes[1]);
        i++;
    } else {
        printl("Callback");
        print("CFG: %32b\n", usbhw_pipe_get_configuration(iface->pipes[0]->num));
        print("EP => %d\n", iface->pipes[0]->config.endpoint);
        usbhc_submit_urb(hid_urb, iface->pipes[0]);
    }
}

static u8 hid_enable_config(struct usb_iface* iface)
{
    /* Ask the USB host controller for a pipe */
    struct usb_pipe* pipe = usbhc_request_pipe();
    print("Pipe number SETUP => %d\n", pipe->num);

    if (pipe == NULL) {
        return 0;
    }
    if (!usbc_iface_add_pipe(pipe, iface)) {
        return 0;
    }

    struct pipe_config pipe_cfg = {
        .endpoint = 0,
        .autoswitch = 0,
        .banks = 0,
        .device = iface->parent_dev->address,
        .frequency = 0,
        .size = PIPE_SIZE_64,
        .token = PIPE_TOKEN_SETUP,
        .type = PIPE_TYPE_CTRL
    };
    usbhc_alloc_pipe(pipe, &pipe_cfg);
    usbhc_set_address(pipe, iface->parent_dev->address);
    usbhc_set_ep_size(pipe, iface->parent_dev->ep0_size);

    /* MAke a URB */
    struct urb* urb = usbhc_alloc_urb();

    setup.bmRequestType = USB_HOST_TO_DEVICE;
    setup.bRequest      = USB_REQ_SET_CONFIGURATION;
    setup.wValue        = 1;
    setup.wIndex        = 0;
    setup.wLength       = 0;

    usbhc_fill_control_urb(urb, (u8 *)&setup, hid_buffer, &hid_enable_config_callback);

    for (u8 i = 0; i < 8; i++)
        print("%1h ", urb->setup_buffer[i]);

    urb->context = iface;
    usbhc_submit_urb(urb, pipe);
    return 1;
}

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
    struct usb_ep_desc* ep = &iface->eps[0].desc;

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
        .banks = 0,
        .device = iface->parent_dev->address,
        .frequency = 0xFF,
        .size = PIPE_SIZE_64,
        .token = PIPE_TOKEN_IN,
        .type = PIPE_TYPE_INT
    };
    usbhc_alloc_pipe(pipe, &pipe_cfg);
    usbhc_set_address(pipe, iface->parent_dev->address);
    usbhc_set_ep_size(pipe, ep->wMaxPacketSize);

    /* MAke a URB */
    hid_urb = usbhc_alloc_urb();
    hid_urb->flags = URB_FLAGS_INTERRUPT_IN;
    hid_urb->callback = &hid_callback;
    hid_urb->transfer_buffer = hid_buffer;

    print("Submitted URB\n");

    if (!hid_enable_config(iface)) {
        print("ERROR");
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
