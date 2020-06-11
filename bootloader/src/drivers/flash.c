#include "flash.h"
#include "hardware.h"
#include "serial.h"

void flash_set_access_cycles(u8 access_cycles) {
    u32 reg = FLASH->FMR;
    reg &= ~(0xF << 8);
    reg |= (((access_cycles - 1) & 0xF) << 8);
    FLASH->FMR = reg;
}

/// Performs an erase-write operation on a page in the first 16 KiB of memory
__ramfunc__ u8 flash_erase_write(u32 page, const u8* buffer) {
	
    // This only works in the two first 8k sectors
    if (page >= 32) {
        return 0;
    }

    // Wait for the flash to be ready for a new command
    while (!(FLASH->FSR & 0b1));

	// Write to the internal latch buffer
    volatile u32* flash_dest = (volatile u32 *)(0x00400000 + 512 * page);
    u32* src_ptr = (u32 *)buffer;
	for (int i = 0; i < 512; i += 4) {
		*flash_dest++ = *src_ptr++;
	}

    // Memory barriers
	asm volatile ("dsb sy" : : : "memory");
    asm volatile ("dmb sy" : : : "memory");
	
	// Issue a erase then write command
	FLASH->FCR = (0x5A << 24) | ((page & 0xFFFF) << 8) | 0x3;
	
    // Wait for the command to go through
    u32 status;
    do {
        status = FLASH->FSR;
    } while (!(status & 0b1111));
	
	// Check for error
	if (status & 0b1110) {
		return 0;
	}
	return 1;
}

/// Erase `size` bytes of the flash from the start location of the kernel 
/// image. This is set to be 0x00404000. The minimum erase size is 8k
__ramfunc__ u8 flash_erase_image(u32 size) {
    u32 pages = size / 512;
    if (size % 512) {
        pages++;
    }

    u32 sect_8k = pages / 16;
    if (pages % 16) {
        sect_8k++;
    }

    // `sect_8k` now holds the number of 8k blocks to be erased from the the 
    // kernel image base address
    u32 sect_8k_base = 32;
    for (u32 i = 0; i < sect_8k; i++) {
        // Issue a 8k erase at `sect_8k_base`
        while (!(FLASH->FSR & 0b1));

        // For 8k erase or 16 sector erase, [1:0] should be 2 and [15:4] 
        // should hold the page divided by 16. 
        u16 arg = (u16)(sect_8k_base & ~0b1111) | 2;
        FLASH->FCR = 0x5A000000 | (arg << 8) | 0x07;

        // Wait for the command to be ready
        u32 status;
        do {
            status = FLASH->FSR;
        } while (!(status & 0b1));

        // Check for errors
        if (status & 0b1110) {
            return 0;
        }
        sect_8k_base += 16;
    }
    return 1;
}

/// Writes a sector to the flash at relativ offset `page` from the kernel image
/// base address
__ramfunc__ u8 flash_write_image_page(u32 page, const u8* buffer) {
	
    while (!(FLASH->FSR & 0b1));

    // Write page to the internal latchbuffer at relative offset
    volatile u32* flash_dest = (volatile u32 *)(0x00404000 + page * 512);
	u32* buffer_ptr = (u32 *)buffer;
    for (u32 i = 0; i < 512; i += 4) {
        *flash_dest++ = *buffer_ptr++;
    }

    asm volatile ("dmb sy" : : : "memory");
    asm volatile ("dsb sy" : : : "memory");
	asm volatile ("isb sy" : : : "memory");

    FLASH->FCR = 0x5A000000 | (((page + 32) & 0xFF) << 8) | 0x01;

    // Wait for the command to complete
    u32 status;
    do {
        status = FLASH->FSR;
    } while (!(status & 0b1));

    // Check for errors
    if (status & 0b1110) {
        return 0;
    }
    return 1;
}