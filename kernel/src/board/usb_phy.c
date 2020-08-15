/* Copyright (C) StrawberryHacker */

#include "usb_phy.h"
#include "usbhc.h"
#include "nvic.h"
#include "clock.h"
#include "panic.h"
#include "print.h"
#include "usb.h"

#include <stddef.h>

#define USB_PIPES 10

struct usb_pipe usb_pipes[USB_PIPES];
struct usbhc host_controller;

/*
 * Initializes the USB physical layer
 */
void usb_phy_init(void)
{
    peripheral_clock_enable(34);
    usbhc_early_init();

    /* Initialize the USB main clock */
    upll_init(UPLL_x40);

    if (usbhc_clock_usable() == 0) {
        panic("USB clock not usable");
    }

    /* Setup the USB physical layer */
    usbhc_init(&host_controller, usb_pipes, USB_PIPES);

    /* Enable NVIC */
    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_3);

    usb_init(&host_controller);
}