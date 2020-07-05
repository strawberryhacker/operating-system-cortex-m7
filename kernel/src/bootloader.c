/// Copyright (C) StrawberryHacker

#include "bootloader.h"
#include "sections.h"
#include "types.h"
#include "usart.h"
#include "cpu.h"
#include "serial.h"
#include "memory.h"

struct image_info {
	u32 major_version;
	u32 minor_version;
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_max_size;
};

__bootsig__ u8 boot_signature[32];

__image_info__ struct image_info header = {
	.major_version = 0,
	.minor_version = 1,
	.boot_start = 0x00400000,
	.boot_size = 0x4000,
	.kernel_start = 0x00404000,
	.kernel_max_size = 0x001FBE00
};

void bootloader_init(void) {
    serial_init();
}

void usart0_handler(void) {
	u8 rec_byte = usart_read(USART0);
	if (rec_byte == 0) {
		printl("Go to bootloader request");
		print_flush();
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		
		// Perform a soft reset
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}
