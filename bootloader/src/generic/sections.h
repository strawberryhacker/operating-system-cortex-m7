/* Copyright (C) StrawberryHacker */

#ifndef SECTIONS_H
#define SECTIONS_H

/*
 * These sections are declared in the linker script. The `ramfunc` section 
 * must be used on all function which is modifying the flash. The rest is used
 * by the bootloader
 */
#define __ramfunc__ __attribute__((long_call, section(".ramfunc")))

#define __bootsig__ __attribute__((section(".boot_signature")))
#define __image_info__ __attribute__((section(".image_info")))
#define __hash_table__ __attribute__((section(".hash_table")))

#endif