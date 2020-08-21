/* Copyright (C) StrawberryHacker */

#ifndef CONFIG_H
#define CONFIG_H

/* USB stuff */
#define URB_MAX_COUNT 256
#define URB_ALLOCATOR_BANK PMALLOC_BANK_2

#define USB_ENUM_BUFFER_SIZE 1024

/* Describes the maximum size of the USB product and manufaturer string */
#define USB_DEV_NAME_MAX_SIZE 64

#endif