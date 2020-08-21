/* Copyright (C) StrawberryHacker */

#include "usb_strings.h"

#define CLASS_CODES_MAX 0x12

const char* class_codes[] = {
    [0x00] = "DETERMINE FROM INTERFACE",
    [0x01] = "AUDIO",
    [0x02] = "CDC control",
    [0x03] = "HID",
    [0x04] = "RESERVED",
    [0x05] = "PHYSICAL",
    [0x06] = "IMAGE",
    [0x07] = "PRINTER",
    [0x08] = "MASS STORAGE",
    [0x09] = "HUB",
    [0x0A] = "CDC DATA",
    [0x0B] = "SMART CARD",
    [0x0C] = "RESERVED",
    [0x0D] = "CONTENT SECURITY",
    [0x0E] = "VIDEO",
    [0x0F] = "PERSONAL HEALTHCARE",
    [0x10] = "AUDIO/VIDEO DEVICE",
    [0x11] = "BILLBOARD",
    [0x12] = "USB-C BRIDGE"
};
