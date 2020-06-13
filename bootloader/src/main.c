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
#include "sections.h"
#include "memory.h"
#include "hash.h"
#include "panic.h"

/// Defines the start byte in the transmission protocol
#define PACKET_START 0b10101010

/// The image info structure will be located in the last page of the bootloader
/// and in the fist page of the kernel
struct image_info {
	u32 major_version;
	u32 minor_version;
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_max_size;
};

struct hash {
	u8 digest[32];
	u32 size;
};

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

enum command_e {
	CMD_IMAGE_PACKET = 0,
	CMD_LAST_IMAGE_PACKET = 1,
	CMD_ERASE_FLASH
};

enum error_s {
	NO_ERROR,
	CRC_ERROR,
	FLASH_ERROR,
	COMMAND_ERROR
};

/// Static functions
void host_ack(enum error_s code);
void jump_to_image(u32 base_addr);

/// Incremented in the SysTick handler
static volatile u32 tick = 0;

/// Variables related to the host interface
volatile struct packet_s packet = {0};
volatile enum state_e state = STATE_IDLE;
volatile u32 packet_index = 0;
volatile u8 packet_flag = 0;

/// This descibes the mermory layout from the bootloaders perspective. It must
/// match with the kernel image info
__image_info__ const struct image_info boot_info = {
	.major_version    = 0,
	.minor_version    = 1,
	.boot_start       = 0x00400000,
	.boot_size        = 0x00004000,
	.kernel_start     = 0x00404000,
	.kernel_max_size  = 0x001FBE00
};

/// Temporarily location for the hash table
__attribute__((__aligned__(128))) struct hash kernel_hash;

/// Boot signature to determite if an attempted jump to the knrnel should be 
/// perfomed
__bootsig__ u8 boot_signature[32];

/// Used for performing an erase-write on a flash page < 32 
u8 flash_buffer[512];

int main(void) {
	// Disable the watchdog timer
	watchdog_disable();

	// The CPU will run at 300 Mhz so the flash access cycles has to be updated
	flash_set_access_cycles(10);

	// Set CPU frequency to 300 MHz and bus frequency to 150 MHz
	clock_source_enable(RC_OSCILLCATOR);
	rc_frequency_select(RC_12_MHz);
	main_clock_select(RC_OSCILLCATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

	// Enable the hash peripheral clock
	peripheral_clock_enable(32);
	debug_init();
	
	// This part determines if we have to stay in the bootloader
	if (memory_compare(boot_signature, "StayInBootloader", 16) == 0) {
		// We do not have to stay in the bootloader
		
		// This will make a image_info pointer to the first kernel page
		const struct image_info* kernel_info = 
			(const struct image_info *)0x00404000;
		
		// Compare the bootlaoder info against the kernel info
		if ((boot_info.boot_size == kernel_info->boot_size) && 
			(boot_info.boot_start == kernel_info->boot_start) &&
			(boot_info.kernel_start == kernel_info->kernel_start) &&
			(boot_info.kernel_max_size == kernel_info->kernel_max_size)) {

			// Check the knernel hash
			// Check the hash in the flash page
			memory_fill(kernel_hash.digest, 0, 32);
			hash256_generate((void *)0x00404000, kernel_hash.size,
						kernel_hash.digest);
		
			if (memory_compare((void *)0x00403C00, kernel_hash.digest, 32)) {
				
				// The image and header information is valid, and the CPU is 
				// not required to stay in the bootloader
				jump_to_image(KERNEL_IMAGE_ADDR);
			} else {
				debug_print("Kernel hash not right\n");
			}

		} else {
			debug_print("Kernel info does not match\n");
		}
	} else {
		memory_fill(boot_signature, 0, 32);
	}

	// Either a go-to-bootloader request or a non valid image condition has
	// occured. Start up the serial interfaces, enables interrupts and begin
	// processing packages from the host PC

	// Initialize serial communication
	serial_init();

	// Enable all interrupt with a configurable priority
	cpsie_i();

	// ACK the host so that it knows the bootloader is ready to receive data
	host_ack(NO_ERROR);

	u32 page_counter = 0;
	
	while (1) {
		if (packet_flag) {

			if (packet.cmd == CMD_ERASE_FLASH) {
				// The erase flash payload should be 4 bytes
				if (packet.size != 4) {
					while(1);
				}
				u32 erase_size = ((packet.payload[0] & 0xFF) |
								  (packet.payload[1] << 8) |
								  (packet.payload[2] << 16) |
								  (packet.payload[3] << 24));
				erase_size += 512;
				erase_size &= ~0b1111111;
				
				// Erase a number of 8k sectors from the flash
				flash_erase_image(erase_size);

				// Update the kernel size for the hashing algorithm
				kernel_hash.size = erase_size;

				// Sent the status code back to the programming tool, so the next
				// package can be sent
				host_ack(NO_ERROR);

			} else {
				// Padding non-complete pages
				if (packet.size != 512) {
					for (u32 i = packet.size; i < 512; i++) {
						packet.payload[i] = 0xFF;
					}
				}
				
				// Program the flash
				if (!(flash_write_image_page(page_counter++, (u8 *)packet.payload))) {
					panic("Flash write error");
				}
				
				// Sent the status code back to the programming tool, so the next
				// package can be sent
				host_ack(NO_ERROR);

				if (packet.cmd == 1) {
					// Generate the hash of the kernel region
					memory_fill(kernel_hash.digest, 0, 32);
					hash256_generate((void *)0x00404000, kernel_hash.size,
						kernel_hash.digest);
					
					u8* src = (u8 *)&kernel_hash;
					u8* dest = flash_buffer;
					for (u32 i = 0; i < 512; i++) {
						if (i < sizeof(struct hash)) {
							*dest++ = *src++;
						} else {
							*dest++ = 0xFF;
						}
					}

					

					// Write the hash and image size to the reserved hash 
					// memory section
					if (!(flash_erase_write(30, flash_buffer))) {
						panic("Flash error");
					}
					jump_to_image(KERNEL_IMAGE_ADDR);
				}
			}
			packet_flag = 0;
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

	debug_flush();
	
	// Deinitialize all usarts
	serial_deinit();
	debug_deinit();
	
	// Disable all interrupts withconfigurable priority
	cpsid_i();
	
	// Reset the clock tree and flash
	clock_tree_reset();
	flash_set_access_cycles(1);
	
	// Set vector table offset
	*((volatile u32 *)VECTOR_TABLE_BASE) = base_addr;
	
	// Insert a data memory barrier and a data synchronization barrier to ensure
	// that all instructions and data accesses are complete before proceeding
	dmb();
	dsb();
	
	// Flushing the instruction pipeline is mandatory before any self modifying 
	// code
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

void host_ack(enum error_s code) {
    serial_print("%c", (char)code);
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
			// If the bootloader
			if (rec_byte == 0) {
				serial_print("%c", (char)NO_ERROR);
			}
			if (rec_byte == PACKET_START) {
				if (packet_flag == 0) {
                    state = STATE_CMD;
                } else {
                    while (1);
                }
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