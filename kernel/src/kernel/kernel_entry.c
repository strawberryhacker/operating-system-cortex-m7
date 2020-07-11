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

	// Disable systick interrupt
	systick_disable();
	systick_clear_pending();
	
	// Enable just fault, systick, pendsv and SVC interrupts
	cpsie_f();
	cpsid_i();

	// Enable the coprocessor 10 and 11 for the FPU
	//*((u32 *)CPACR) = (0b11 << 20) | (0b11 << 22);
	//dsb();
	//isb();

	// The CPU will run at 300 Mhz so the flash access cycles has to be updated
	flash_set_access_cycles(7);

	// Set CPU frequency to 300 MHz and bus frequency to 150 MHz
	clock_source_enable(CRYSTAL_OSCILLATOR, 0xFF);
	main_clock_select(CRYSTAL_OSCILLATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);
	
	// Initilalize the DRAM interface
	dram_init();

	// Initialize serial communication
	print_init();

	// Make the kernel listen for firmware upgrade
	bootloader_init();
	
	// Configure the on-board LED
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
	gpio_clear(GPIOC, 8);

	// Configure the on board button
	peripheral_clock_enable(10);	
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);

	// Initialize the dynamic memory core
	mm_init();

	// Reenable interrupts
	cpsie_i();
}
