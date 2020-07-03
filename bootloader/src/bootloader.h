#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "types.h"
#include "frame.h"

u8 erase_kernel_image(const u8* data);

u8 write_bootloader_page(const u8* data, u32 size, u32 page);

u8 write_kernel_page(const u8* data, u32 size, u32 page);

void start_kernel(void);

#endif