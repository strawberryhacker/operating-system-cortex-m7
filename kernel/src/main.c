/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"
#include "clock.h"
#include "watchdog.h"
#include "flash.h"
#include "nvic.h"
#include "debug.h"
#include "usart.h"
#include "gpio.h"
#include "cpu.h"
#include "systick.h"
#include "panic.h"
#include "dram.h"
#include "mm.h"
#include "bootloader.h"

#include <stddef.h>

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
	
	dram_init();

	// Enable the kernel to do firmware upgrades
	bootloader_init();

	// Initialize serial communication
	debug_init();

	cpsie_i();
	
	// Configure the on-board LED
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);

	// Configure the systick
	systick_set_rvr(300000);
	systick_enable(1);
	
	debug_print("Hello from kernel\n");
	mm_init();	

	tick = 499;
	while (1) {
		if (tick >= 250) {
			tick = 0;
			gpio_toggle(GPIOC, 8);
		}
	}
}

void systick_handler(void) {
	tick++;
}