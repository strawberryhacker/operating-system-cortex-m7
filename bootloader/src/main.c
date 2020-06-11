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

#define PACKET_START 0b10101010

struct packet_s {
	u8 cmd;
	u8 crc;
	u16 size;
	u8 payload[512];
};

enum state_e {
	STATE_IDLE,
	STATE_CMD,
	STATE_SIZE,
	STATE_PAYLOAD,
	STATE_CRC
};

enum error_s {
	NO_ERROR,
	CRC_ERROR,
	FLASH_ERROR,
	COMMAND_ERROR
};

void jump_to_image(u32 base_addr);

volatile struct packet_s packet = {0};
volatile enum state_e state = STATE_IDLE;
volatile u32 packet_index = 0;
volatile u8 packet_flag = 0;
static volatile u32 tick = 0;

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

	flash_erase_image(7000);

	debug_print("Bootloader started...\n");
	u32 page_counter = 0;
	
	while (1) {
		if (tick >= 500) {
			tick = 0;
			gpio_toggle(GPIOC, 8);
		}
		if (packet_flag) {
			packet_flag = 0;
			
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
	}
}

void systick_handler() {
	tick++;
}

/// The interrupt routine will receive the kernel image in packages of 512 bytes
/// Packet:      [ start byte ] [ command ] [ size ] [ payload ] [ CRC ]
///
/// start byte - indicates start of packet. Should be set to a non-ascii char
/// command    - one byte where 0x01 is reserved and mark the final package
/// size       - two bytes indicating the size of the package. First is LSByte
/// payload    - holds the data
/// CRC        - cyclic redundancy check
void usart1_handler() {
	u8 rec_byte = serial_read();

	switch (state) {
		case STATE_IDLE : {
			if (rec_byte == PACKET_START) {
				state = STATE_CMD;
			}
			break;
		}
		case STATE_CMD : {
			packet.cmd = rec_byte;
			state = STATE_SIZE;
			packet.size = 0;
			packet_index = 0;
			break;
		}
		case STATE_SIZE : {
			packet.size |= (rec_byte << (8 * packet_index));
			packet_index++;
			if (packet_index >= 2) {
				state = STATE_PAYLOAD;
				packet_index = 0;
			}
			break;
		}
		case STATE_PAYLOAD : {
			packet.payload[packet_index] = rec_byte;
			packet_index++;
			if (packet_index >= packet.size) {
				state = STATE_CRC;
			}
			break;
		}
		case STATE_CRC : {
			packet.crc = rec_byte;
			state = STATE_IDLE;
			packet_flag = 1;
			break;
		}
	}
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