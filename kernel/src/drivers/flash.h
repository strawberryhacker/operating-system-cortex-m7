#ifndef FLASH_H
#define FLASH_H

#include "types.h"
#include "cpu.h"

void flash_set_access_cycles(u8 access_cycles);

__ramfunc__ u8 flash_erase_write(u32 page, const u8* buffer);

__ramfunc__ u8 flash_erase_image(u32 size);

__ramfunc__ u8 flash_write_image_page(u32 page, const u8* buffer);

#endif