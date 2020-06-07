/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"
#include "clock.h"
#include "watchdog.h"
#include "flash.h"
#include "serial.h"
#include "interrupt.h"
#include "debug.h"
#include "usart.h"

struct packet_s {
	u8 cmd;
	u16 size;
	u8 payload[512];
};

enum state_e {
	STATE_IDLE,
	STATE_CMD,
	STATE_SIZE,
	STATE_PAYLOAD
};

#define PACKET_START 0b10101010

volatile struct packet_s packet = {0};
volatile enum state_e state = STATE_IDLE;
volatile u8 size_index = 0;
volatile u32 payload_index = 0;
static volatile u32 tick = 0;
volatile u8 new_packet = 0;

int main(void) {
	// Disable the watchdog timer
	watchdog_disable();
	flash_set_access_cycles(7);

	// Set CPU frequency to 300 MHz and bus frequency to 150 MHz
	clock_source_enable(RC_OSCILLCATOR);
	main_clock_select(RC_OSCILLCATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);
	
	// Set up serial communication
	serial_init();
	debug_init();

	interrupt_i_enable();

	GPIOC->PER = (1 << 8);
	GPIOC->OER = (1 << 8);
	GPIOC->SODR = (1 << 8);

	// The processor runs at 12 MHz
	SYSTICK->CSR = 0b111;
	SYSTICK->RVR = 300000;

	flash_erase_image(7000);

	u32 page_counter = 0;

	

	while (1) {
		if (tick >= 50) {
			tick = 0;
			// Toggle led
			if (GPIOC->ODSR & (1 << 8)) {
				GPIOC->CODR = (1 << 8);
			} else {
				GPIOC->SODR = (1 << 8);
			}
		}
		if (new_packet) {
			new_packet = 0;
			//debug_print("New packet: \n\tsize %d\n\tcmd: %1h\n\tPayload index: %d\n\tsize index: %d\n\tnew msg: %d\n", 
			//	packet.size, packet.cmd, payload_index, size_index, new_packet);
			
			if (packet.size != 512) {
				for (u32 i = packet.size; i < 512; i++) {
					packet.payload[i] = 0;
				}
			}
			// Program the flash
			if (!(flash_write_image_page(page_counter++, (u8 *)packet.payload))) {
				debug_print("Warning\n");
				asm volatile("cpsid f" : : : "memory");
				while (1);
			}
			
			print("%c", 'A');

			if (packet.cmd == 1) {

				debug_print("\n - - - - - - - - New firmware updated - - - - - - - - \n\n");

				for (u8 i = 0; i < 8; i++) {
					NVIC->ICER[i] = 0xFF;
				}

				USART1->CR = 0b10101100;
				USART0->CR = 0b10101100;

				// Vector table is at 0x00404000
				asm volatile ("dsb sy");
				asm volatile ("isb sy");
				*((volatile u32 *)0xE000ED08) = 0x00404000;
				asm volatile ("dsb sy");
				asm volatile ("isb sy");
				
				u32 stack_pointer = *((u32 * )0x00404000);
				u32 program_counter = (*((u32 * )(0x00404004))) | 1;
				
				asm volatile ("cpsid i" : : : "memory");
				
				asm volatile ("mov sp, %0\t\n" : : "l" (stack_pointer));
				asm volatile ("mov pc, %0\t\n" : : "l" (program_counter));
			}
		}
	}
}

void systick_handler() {
	tick++;
}

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
			size_index = 0;
			break;
		}
		case STATE_SIZE : {
			// Get the size
			packet.size |= (rec_byte << (8 * size_index));
			size_index++;
			if (size_index >= 2) {
				state = STATE_PAYLOAD;
				payload_index = 0;
			}
			break;
		}
		case STATE_PAYLOAD : {
			//debug_print("Ooh\n");
			packet.payload[payload_index] = rec_byte;
			payload_index++;
			if (payload_index >= packet.size) {
				state = STATE_IDLE;
				new_packet = 1;
			}
			break;
		}
	}
}

void usart0_handler(void) {
	(void)usart_read(USART0);
}