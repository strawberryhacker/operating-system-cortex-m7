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
extern u32 _end;

struct kernel_header {
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_size;
};

__attribute__((section(".kernel_header"))) struct kernel_header header = {
	.boot_start = 0x00400000,
	.boot_size = 0x4000,
	.kernel_start = 0x00404000,
	.kernel_size = 0x456456
};


__bootsig__ u8 boot_signature[32];

void memcpy(const void* src, void* dest, u32 size) {
	const u8* src_ptr = (const u8 *)src;
	u8* dest_ptr = (u8 *)dest;

	while (size--) {
		*dest_ptr++ = *src_ptr++;
	}
}

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

	debug_print("Kernel started yo...\n");
	
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
	if (rec_byte == 0) {
		memcpy("StayInHouse", boot_signature, 11);
		// Perform a soft reset
		asm volatile ("cpsid i" : : : "memory");
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}