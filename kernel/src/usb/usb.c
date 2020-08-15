/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "list.h"
#include "memory.h"

static u8 setup_buffer[64];
static u8 transfer_buffer[512];

void usb_start_enumeration(void) 
{
    /* Read the device descriptor */

    /* Host assigns a new address and set the device address */

    /* Configiurationm desc, interface descriptor, end point descriptor */
}

static void complete_callback(struct urb* urb)
{
    printl("URB callback");
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
        /* Start enumeration */        
        struct urb* urb = usbhc_alloc_urb();
        setup_buffer[0] = 0x80;
        setup_buffer[1] = 0x06;
        setup_buffer[2] = 0x00;
        setup_buffer[3] = 0x01;
        setup_buffer[4] = 0x00;
        setup_buffer[5] = 0x00;
        setup_buffer[6] = 0x12;
        setup_buffer[7] = 0x00;
        usbhc_fill_control_urb(urb, setup_buffer, transfer_buffer, 512, 
            complete_callback, "Notte");
        usbhc_submit_urb(urb, &hc->pipe_base[0]);
    }
}

void usb_init(struct usbhc* hc)
{
    usbhc_add_root_hub_callback(hc, &root_hub_event);
}
