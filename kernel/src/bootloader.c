/// Copyright (C) StrawberryHacker

#include "bootloader.h"
#include "sections.h"
#include "types.h"
#include "cpu.h"
#include "serial.h"
#include "print.h"
#include "memory.h"

struct image_info {
    // Version numer of the kernel
    u32 major_version;
    u32 minor_version;

    // Start address of the bootlaoder
    u32 bootloader_start;

    // The total size allocated to the bootlaoder. This includes the two special
    // structures 
    u32 bootloader_size;

    // Specifies where the bootloader info table is stored
    u32 bootloader_info;

    // Start address of the kernel
    u32 kernel_start;

    // The allocated size for the kernel
    u32 kernel_size;

    // Specifies where the kernel info is stored
    u32 kernel_info;
};

__bootsig__ u8 boot_signature[32];

__image_info__ struct image_info image_info = {
    .major_version    = 1,
    .minor_version    = 2,

    .bootloader_start = 0x00400000,
    .bootloader_size  = 0x00004000,
    .bootloader_info  = 0x00403E00,

    .kernel_start     = 0x00404000,
    .kernel_size      = 0x001FC000,
    .kernel_info      = 0x00404000
};

void bootloader_init(void) {
    serial_init();

    // Grab the bootlaoder info section
    const struct image_info* info = 
        (const struct image_info *)image_info.bootloader_info;

    // Print the bootloader and kernel version
    printl("\n\n\n\n\nUsing Vanilla bootloader" ANSI_GREEN " v%d.%d" ANSI_NORMAL,
        info->major_version, info->minor_version);

    printl("Using Vanilla kernel" ANSI_GREEN "     v%d.%d\n" ANSI_NORMAL,
        image_info.major_version, image_info.minor_version);
    
    print(ANSI_NORMAL);
}

void usart0_handler(void) {
	u8 rec_byte = serial_read();
	if (rec_byte == 0x00) {

		print_flush();
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		
		// Perform a soft reset
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}
