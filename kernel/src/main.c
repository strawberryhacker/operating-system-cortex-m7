/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"
#include "clock.h"
#include "watchdog.h"
#include "interrupt.h"
#include "debug.h"
#include "usart.h"
#include "flash.h"
#include "serial.h"

volatile u32 tick = 0;

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
	debug_init();
	serial_init();

	interrupt_i_enable();

	GPIOC->PER = (1 << 8);
	GPIOC->OER = (1 << 8);
	GPIOC->SODR = (1 << 8);

	// The processor runs at 12 MHz
	SYSTICK->CSR = 0b111;
	SYSTICK->RVR = 300000;

	while (1) {
		if (tick >= 500) {
			tick = 0;
			debug_print("Good\n");

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

void usart1_handler(void) {
	u8 rec_byte = usart_read(USART1);

	if (rec_byte == 'b') {
		// Perform a soft reset
		asm volatile ("cpsid i" : : : "memory");
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}