/* Copyright (C) StrawberryHacker */

#include "usb_phy.h"
#include "usbhs.h"
#include "nvic.h"
#include "clock.h"
#include "panic.h"
#include "print.h"
#include "usb.h"

#include <stddef.h>

#define PIPE_COUNT 1

struct usb_core usb_core;
struct usb_hardware usb_hw;
struct usb_pipe usb_pipes[PIPE_COUNT];

/*
 * Initializes the USB PHY including the interrupt and clock controller
 */
void usb_phy_init(void)
{
    /*
     * The reccomended start up sequence can be found in the 
     * datasheet at page 752
     */
    peripheral_clock_enable(34);

    usbhs_unfreeze_clock();
    usbhs_set_mode(USB_HOST);
    usbhs_enable();

    /* Enable the USB UPLL clock at 480 MHz */
    upll_init(UPLL_x40);

    /* Check if the USB clock is usable */
    if (usbhs_clock_usable() == 0) {
        panic("USB clock not usable");
    }

    usbhs_init(&usb_core, &usb_hw, usb_pipes, PIPE_COUNT);

    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_3);

    usb_init(&usb_core);
}
