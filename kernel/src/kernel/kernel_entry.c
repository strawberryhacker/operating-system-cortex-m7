/// Copyright (C) StrawberryHacker

#include "kernel_entry.h"
#include "watchdog.h"
#include "flash.h"
#include "clock.h"
#include "dram.h"
#include "print.h"
#include "cpu.h"
#include "gpio.h"
#include "systick.h"
#include "mm.h"
#include "sd.h"
#include "nvic.h"
#include "types.h"
#include "sd_protocol.h"
#include "bootloader.h"

void kernel_entry(void) {
    // Disable the watchdog timer
	watchdog_disable();
	
	// Enable the coprocessor 10 and 11 for the FPU
	//*((u32 *)CPACR) = (0b11 << 20) | (0b11 << 22);
	//dsb();
	//isb();

	// Disable systick interrupt
	systick_disable();
	systick_clear_pending();

	// The CPU will run at 300 Mhz so the flash access cycles has to be updated
	flash_set_access_cycles(7);

	// Set CPU frequency to 300 MHz and bus frequency to 150 MHz
	clock_source_enable(RC_OSCILLCATOR);
	rc_frequency_select(RC_12_MHz);
	main_clock_select(RC_OSCILLCATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);
	
	dram_init();

	// Make the kernel listen for firmware upgrade
	bootloader_init();

	// Initialize serial communication
	print_init();
	print("\n\n- - - - Vanilla kernel started - - - -\n");

	cpsie_i();
	
	// Configure the on-board LED
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
	gpio_set(GPIOC, 8);

	// Configure the on board button
	peripheral_clock_enable(10);	
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);

	mm_init();

	// Work in progress...
	//sd_init();
	//sd_protocol_init();
}
