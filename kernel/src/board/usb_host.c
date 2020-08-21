/* Copyright (C) StrawberryHacker */

#include "usb_host.h"
#include "usbhc.h"
#include "nvic.h"
#include "clock.h"
#include "panic.h"
#include "print.h"
#include "usb.h"
#include "usb_hid.h"
#include "usb_audio.h"

#include <stddef.h>

#define USB_PIPES 10

struct usb_pipe usb_pipes[USB_PIPES];
struct usbhc host_controller;
struct usb_core usb_core;

/* HID class support */
struct usb_driver hid_driver;
struct usb_driver audio_driver;

/*
 * Initializes the USB physical layer
 */
void usb_host_init(void)
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
    usbc_init(&usb_core, &host_controller);

    /* Add HID class driver */
    usb_hid_init(&usb_core);
    
    /* Enable NVIC */
    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_3);    
}