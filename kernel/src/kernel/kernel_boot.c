#include "kernel_boot.h"
#include "watchdog.h"
#include "flash.h"
#include "clock.h"
#include "dram.h"
#include "bootloader.h"
#include "debug.h"
#include "cpu.h"
#include "gpio.h"
#include "systick.h"
#include "mm.h"
#include "sd.h"
#include "types.h"
#include "sd_protocol.h"

void kernel_boot(void) {
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
	
	debug_print("- - - - Vanilla kernel started - - - -\n");
	mm_init();

	sd_init();
	sd_protocol_init();
}
