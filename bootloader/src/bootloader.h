/* Copyright (C) StrawberryHacker */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "types.h"
#include "frame.h"

struct image_info {
    /* Version number for the bootlaoder */
    u32 major_version;
    u32 minor_version;

    /* Start address of the bootlaoder */
    u32 bootloader_start;

    /* 
     * The total size allocated to the bootlaoder. This includes the two special
     * structures
     */ 
    u32 bootloader_size;

    /* Specifies where the bootloader info table is stored */
    u32 bootloader_info;

    /* Start address of the kernel */
    u32 kernel_start;

    /* The allocated size for the kernel */
    u32 kernel_size;

    /* Specifies where the kernel info is stored */
    u32 kernel_info;
};

u8 check_boot_signature(void);

u8 check_info_match(void);

void clear_boot_signature(void);

u8 erase_kernel_image(const u8* data);

u8 write_bootloader_page(const u8* data, u32 size, u32 page);

u8 write_kernel_page(const u8* data, u32 size, u32 page);

u8 store_kernel_hash(void);

u8 verify_kernel_hash(void);

void set_flash_write_offset(const u8* data);

void start_kernel(void);

#endif
