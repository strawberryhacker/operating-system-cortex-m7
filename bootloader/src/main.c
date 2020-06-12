/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"
#include "clock.h"
#include "watchdog.h"
#include "flash.h"
#include "serial.h"
#include "nvic.h"
#include "debug.h"
#include "usart.h"
#include "gpio.h"
#include "systick.h"
#include "cpu.h"
#include "host_interface.h"


struct header_s {
	u32 major_version;
	u32 minor_version;
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_max_size;
};

struct hash_s {
	u8 hash[32];
	u32 start_addr;
	u32 size;
};

void jump_to_image(u32 base_addr);


static volatile u32 tick = 0;

extern volatile struct packet_s packet;
extern volatile u8 packet_flag;

// Special sections in memory
__bootsig__ u8 boot_signature[32];
__header__ struct header_s boot_header;
__hash__ struct hash_s image_hash;


int main(void) {
	// Disable the watchdog timer
	watchdog_disable();

	// The CPU will run at 300 Mhz so the flash access cycles has to be updated
	flash_set_access_cycles(7);

	// Set CPU frequency to 300 MHz and bus frequency to 150 MHz
	clock_source_enable(RC_OSCILLCATOR);
	rc_frequency_select(RC_12_MHz);
	main_clock_select(RC_OSCILLCATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

	// Initialize serial communication
	serial_init();
	debug_init();

	cpsie_i();
	
	// Configure the on-board LED
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);

	// Configure the systick
	systick_set_rvr(300000);
	systick_enable(1);
	

	debug_print("Bootloader started...\n");
	host_ack();
	u32 page_counter = 0;
	
	while (1) {
		if (tick >= 500) {
			tick = 0;
			gpio_toggle(GPIOC, 8);
		}
		if (packet_flag) {			
			if (packet.cmd == CMD_ERASE_FLASH) {
				if (packet.size != 4) {
					while(1);
				}
				u32 erase_size = ((packet.payload[0] & 0xFF) |
								  (packet.payload[1] << 8) |
								  (packet.payload[2] << 16) |
								  (packet.payload[3] << 24));
				// Erase the flash
				flash_erase_image(erase_size);

				// Sent the status code back to the programming tool, so the next
				// package can be sent
				debug_print("Flash erase: %d bytes\n", erase_size);
				print("%c", (char)NO_ERROR);

			} else {
				// Padding non-complete pages
				if (packet.size != 512) {
					for (u32 i = packet.size; i < 512; i++) {
						packet.payload[i] = 0xFF;
					}
				}
				
				// Program the flash
				if (!(flash_write_image_page(page_counter++, (u8 *)packet.payload))) {
					debug_print("Warning\n");
					asm volatile("cpsid f" : : : "memory");
					while (1);
				}
				
				// Sent the status code back to the programming tool, so the next
				// package can be sent
				print("%c", (char)NO_ERROR);

				if (packet.cmd == 1) {
					jump_to_image(KERNEL_IMAGE_ADDR);
				}
			}
			packet_flag = 0;
		}
	}
}

void systick_handler() {
	tick++;
}

void usart0_handler(void) {
	(void)usart_read(USART0);
}

void jump_to_image(u32 base_addr) {
	
	// The vector table base and thus the image base should be
	// aligned with 32 words
	if (base_addr & 0b1111111) {
		// Panic
	}
	debug_print("Firmware upgrade complete!\n");
	debug_flush();
	
	// Deinitialize all usarts
	serial_deinit();
	debug_deinit();
	
	// Disable systick
	systick_reset();
	
	// Disable all interrupts except NMI
	cpsid_i();
	
	// Reset the clock tree and flash
	clock_tree_reset();
	flash_set_access_cycles(1);
	
	// Clear any pending systick interrupts
	nvic_clear_pending(-1);
	
	// Set vector table offset
	*((volatile u32 *)VECTOR_TABLE_BASE) = base_addr;
	
	// Insert a data memory barrier and a data synchronization barrier to ensure that
	// all instructions and data accesses are complete before proceeding
	dmb();
	dsb();
	
	// Flushing the instruction pipeline is mandatory before any self modifying code
	isb();
	
	// Update stack pointer and program counter
	volatile u32* image_base = (volatile u32 *)base_addr;
	
	asm volatile (
	"mov r0, %0		\n\t"
	"ldr r1, [r0]	\n\t"
	"add r0, r0, #4	\n\t"
	"ldr r0, [r0]	\n\t"
	"orr r0, r0, #1	\n\t"
	"mov sp, r1		\n\t"
	"mov pc, r0		\n\t"
	: : "l" (image_base) : "r0", "r1", "memory"
	);
}