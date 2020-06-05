#include "flash.h"
#include "hardware.h"
#include "serial.h"

void flash_set_access_cycles(u8 access_cycles) {
    u32 reg = FLASH->FMR;
    reg &= ~(0xF << 8);
    reg |= (((access_cycles - 1) & 0xF) << 8);
    FLASH->FMR = reg;
}

u8 flash_write_page(u32 page, const u8* data) {
    // Wait for flash to be ready
    while (!(FLASH->FSR & (1 << 0)));
    for (u32 i = 0; i < 512; i++) {
         *((volatile u8 *)(0x00400000 + page * 512 + i)) = *data++;
    }

    FLASH->FCR = 0x5A000000 | 0x3 | ((page & 0xFFFF) << 8);

    // Wait for flash to be ready
    while (!(FLASH->FSR & (1 << 0)));

    // Check for errors
    if (FLASH->FSR & (1 << 2)) {
        // Locked
        return 0;
    }
    if (FLASH->FSR & (1 << 3)) {
        // Locked
        return 0;
    }
    if (FLASH->FSR & (1 << 1)) {
        // Error
        return 0;
    }
    return 1;
}