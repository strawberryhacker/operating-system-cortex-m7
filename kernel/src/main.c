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
#include "std.h"
#include "systick.h"
#include "sections.h"

static volatile u32 tick = 0;
extern u32 _end;

struct image_info {
	u32 major_version;
	u32 minor_version;
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_max_size;
};

__bootsig__ u8 boot_signature[32];

__image_info__ struct image_info header = {
	.major_version = 0,
	.minor_version = 1,
	.boot_start = 0x00400000,
	.boot_size = 0x4000,
	.kernel_start = 0x00404000,
	.kernel_max_size = 0x001FBE00
};

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
	tick = 499;
	while (1) {
		if (tick >= 100) {
			tick = 0;
			debug_print("Kernel\n");
			gpio_toggle(GPIOC, 8);
		}
	}
}

void systick_handler() {
	tick++;
}

void usart1_handler(void) {
	u8 rec_byte = usart_read(USART1);
	if (rec_byte == 0) {
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		// Perform a soft reset
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}