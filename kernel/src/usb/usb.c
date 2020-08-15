/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "list.h"
#include "usb_protocol.h"
#include "memory.h"
#include "bmalloc.h"
#include "thread.h"

static struct usb_setup setup;
static u8 transfer_buffer[512];

struct usb_core* private_core;

static void complete_callback(struct urb* urb);

void enumeration_callback(struct urb* urb) 
{
    /* Get info */

    /* Delete URB */

    switch(private_core->enum_state) {
        case USB_ENUM_GET_DEV_DESC:

            break;
    }
    /* Read the device descriptor */


    /* Host assigns a new address and set the device address */

    /* Configiurationm desc, interface descriptor, end point descriptor */
}

static void usb_get_dev_desc(struct usbhc* hc) 
{
    struct urb* urb = usbhc_alloc_urb();

    setup.bmRequestType = USB_REQ_TYPE_DEVICE_TO_HOST;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wIndex = 0;
    setup.wLength = 18;
    setup.wValue = 1; /* Device is number 1 */

    usbhc_fill_control_urb(urb, (u8 *)&setup, transfer_buffer, 512, 
        &complete_callback, "Hello");

    usbhc_submit_urb(urb, &hc->pipe_base[0]);
}

static void usc_enumerate(void* args)
{
    struct usbhc* hc = (struct usbhc *)args;
    while (1) {
        switch(private_core->enum_state) {
            case USB_ENUM_GET_DEV_DESC:
                usb_get_dev_desc(hc);
                break;
            case USB_ENUM_SET_ADDRESS:
                print("Setting address\n");
                while (1);
                break;
        }
        thread_sleep(10);
    }
}

static void complete_callback(struct urb* urb)
{
    printl("URB control tranfer complete");
    struct usb_setup* setup = (struct usb_setup *)urb->setup_buffer;

    print("Size: %d\n", setup->wLength);

    struct usb_dev_desc* dev = (struct usb_dev_desc *)urb->transfer_buffer;
    print("bLength: %d\n", dev->bLength);
    print("Max packet size: %d\n", dev->bMaxPacketSize);

    private_core->enum_state++;
}

void root_hub_event(struct usbhc* hc, enum root_hub_event event)
{
    if (event == RH_EVENT_CONNECTION) {
        printl("USB core - connection");
        usbhc_send_reset();

    } else if (event == RH_EVENT_DISCONNECTION) {
        printl("USB core - disconnection");

    } else if (event == RH_EVENT_RESET_SENT) {
        printl("USB core - reset sent");

        /* Set configuration for the control pipe */
        struct pipe_cfg cfg = {
            .banks = PIPE_BANKS_1,
            .autosw = 0,
            .device = 0,
            .frequency = 0,
            .pipe = 0,
            .size = PIPE_SIZE_64,
            .token = PIPE_TOKEN_SETUP,
            .type = PIPE_TYPE_CTRL
        };
        usbhc_alloc_pipe(&hc->pipe_base[0], &cfg);
        usbhc_set_address(&hc->pipe_base[0], 0);

        private_core->enum_state = USB_ENUM_GET_DEV_DESC;
    }
}

void usb_init(struct usb_core* core, struct usbhc* hc)
{
    private_core = core;
    private_core->enum_state = USB_ENUM_IDLE;

    usbhc_add_root_hub_callback(hc, &root_hub_event);

    /* Start the enumeration thread */
    struct thread_info enum_info = {
        .name = "USB enumerate",
        .class = REAL_TIME,
        .code_addr = 0,
        .stack_size = 512,
        .arg = hc,
        .thread = usc_enumerate
    };

    new_thread(&enum_info);
}
