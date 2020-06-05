/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"
#include "clock.h"
#include "watchdog.h"
#include "flash.h"
#include "serial.h"
#include "interrupt.h"

static volatile u32 tick = 0;
static u8 page_buffer[512];



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

	interrupt_i_enable();

	GPIOC->PER = (1 << 8);
	GPIOC->OER = (1 << 8);
	GPIOC->SODR = (1 << 8);

	// The processor runs at 12 MHz
	SYSTICK->CSR = 0b111;
	SYSTICK->RVR = 300000;

	
	for (u16 i = 0; i < 512; i++) {
		page_buffer[i] = 0xE8;
	}

	/*
	if (!flash_write_page(60, page_buffer)) {
		serial_write("Dammit.....!\n");
	}

	const u8* buffer_flash = (const u8 *)(0x00400000 + 60 * 512);
	for (u16 i = 0; i < 512; i++) {
		serial_write("%1h ", buffer_flash[i]);
	}
	*/
    while (1) {
		if (tick >= 500) {
			tick = 0;
			
			serial_write("Hello\n");

			// Toggle led
			if (GPIOC->ODSR & (1 << 8)) {
				GPIOC->CODR = (1 << 8);
			} else {
				GPIOC->SODR = (1 << 8);
			}
		}
	}
}

void systick_handler() {
	tick++;
}

void usart1_handler() {
	u8 tmp = serial_read();

	serial_write("%c", tmp);
}