#ifndef FLASH_H
#define FLASH_H

#include "types.h"
#include "cpu.h"

void flash_set_access_cycles(u8 access_cycles);

u8 flash_write_page(u32 page, const u8* data);

#endif