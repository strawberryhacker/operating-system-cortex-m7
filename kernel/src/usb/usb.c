/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "dlist.h"

struct usb_device test_device = {0};


u8 buffer[8] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
u8 req[18];

/*  
 * Control transfer callback. This is called when a control transfer
 * completes by the hardware layer.
*/
void usb_core_control_callback(struct usb_pipe* pipe)
{
    u8* ptr = pipe->x.control.data;
    print("Data: ");
    for (u8 i = 0; i < pipe->x.control.receive_size; i++) {
        print("%1h ", *ptr++);
    }
    print("\n");
}

/*
 * Root hub callback
 */
void usb_core_rh_callback(struct usb_core* core, enum root_hub_event event)
{
    if (event == RH_EVENT_RESET) {
        /* Reallocate pipe zero */
        u32 cfg = (3 << 4);
        printl("Reset sent");
        usbhc_pipe_allocate(core, cfg, 0, 1);
        struct usb_pipe* p = &core->hw->pipes[0];

        usbhc_add_pipe_callback(p, usb_core_control_callback);
        usbhc_control_transfer(core, p, req, buffer, 18);
    } else if (event == RH_EVENT_CONNECT) {
        usbhc_send_reset();
        printl("Connection");
    } else if (event == RH_EVENT_DISCONNECT) {
        printl("Disconnect");
    }
}

/*
 * SOF callback. This is called by the lower level USB hardware
 * driver to notify a SOF interrupt
 */
void usb_core_sof_callback(struct usb_core* core) {

}

void usb_init(struct usb_core* core)
{
    /* Assign the callbacks */
    core->rh_callback = usb_core_rh_callback;
    core->sof_callback = usb_core_sof_callback;
}
