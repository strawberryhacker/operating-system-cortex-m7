#include "kernel_entry.h"
#include "watchdog.h"
#include "flash.h"
#include "clock.h"
#include "dram.h"
#include "debug.h"
#include "cpu.h"
#include "gpio.h"
#include "systick.h"
#include "mm.h"
#include "sd.h"
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
	debug_init();

	cpsie_i();
	
	// Configure the on-board LED
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
	
	debug_print("\n\n- - - - Vanilla kernel started - - - -\n");
	mm_init();

	// Work in progress...
	//sd_init();
	//sd_protocol_init();
}
