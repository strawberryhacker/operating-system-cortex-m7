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
    .minor_version    = 0,

    .bootloader_start = 0x00400000,
    .bootloader_size  = 0x4000,
    .bootloader_info  = 0x00403E00,

    .kernel_start     = 0x00404000,
    .kernel_size      = 0x1FC000,
    .kernel_info      = 0x00404000
};

void bootloader_init(void) {
    serial_init();
}

void usart0_handler(void) {
	u8 rec_byte = serial_read();
	if (rec_byte == 0x00) {
		printl("Go to bootloader request");
		print_flush();
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		
		// Perform a soft reset
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}
