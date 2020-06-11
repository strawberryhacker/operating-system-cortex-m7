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

static volatile u32 tick = 0;

__attribute__((section(".kernel_header"))) const u8 header[256] = {0};

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

	debug_print("Kernel started...\n");
	
	while (1) {
		if (tick >= 500) {
			tick = 0;
			debug_print("Hello from kernel\n");
			gpio_toggle(GPIOC, 8);
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