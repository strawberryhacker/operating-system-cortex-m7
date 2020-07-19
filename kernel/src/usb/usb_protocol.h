#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

#include "types.h"

/*
 * Data packet for the USB control trasfer
 */
struct usb_setup {
    u8 request_type;
    u8 request;
    u16 value;
    u16 index;
    u16 length;
};

#endif
