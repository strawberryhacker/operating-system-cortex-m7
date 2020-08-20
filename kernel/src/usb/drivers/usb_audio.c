/* Copyright (C) StrawberryHacker */

#include "usb_audio.h"

const char audio_class_name[] = "USB audio class";

void usb_audio_init(struct usb_driver* driver)
{
    driver->name = audio_class_name;
}
