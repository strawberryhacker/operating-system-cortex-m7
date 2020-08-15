/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "dlist.h"

void usb_start_enumeration(void) 
{
    /* Read the device descriptor */

    /* Host assigns a new address and set the device address */

    /* Configiurationm desc, interface descriptor, end point descriptor */
}

void root_hub_event(struct usbhc* hc, enum root_hub_event event)
{
    if (event == RH_EVENT_CONNECTION) {
        printl("USB core - connection");
        usbhc_send_reset(); // change this

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
        u8 setup[8] = {
            0x80,
            0x06,
            0x00,
            0x01,
            0x00,
            0x00,
            0x12,
            0x00
        };
        usbhc_send_setup_raw(&hc->pipe_base[0], setup);
    }
}

void usb_init(struct usbhc* hc)
{
    usbhc_add_root_hub_callback(hc, &root_hub_event);
}
