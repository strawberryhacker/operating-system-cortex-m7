/* Copyright (C) StrawberryHacker */

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
#include "cache.h"
#include "fpu.h"
#include "trand.h"
#include "prand.h"
#include "usb_host.h"
#include "usb_hid.h"
#include "led.h"
#include "button.h"

void kernel_entry(void) {
    /* Disable the watchdog timer */
	watchdog_disable();

	/* Disable systick interrupt */
	systick_disable();
	systick_clear_pending();
	
	/* Enable just fault, systick, pendsv and SVC interrupts */
	cpsie_f();
	cpsid_i();

	//fpu_enable();

	/*
	 * The CPU will run at 300 Mhz so the flash access cycles has to be
	 * updated
	 */
	flash_set_access_cycles(7);

	/* Set CPU frequency to 300 MHz and bus frequency to 150 MHz */
	clock_source_enable(CRYSTAL_OSCILLATOR, 0xFF);
	main_clock_select(CRYSTAL_OSCILLATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

	/* Initialize serial communication */
	print_init();
	
	/* Initilalize the DRAM interface */
	dram_init();

	/* Enable L1 I-cache and L1 D-cache */
	//dcache_enable();
	//icache_enable();

	/* Make the kernel listen for firmware upgrade */
	bootloader_init();
	
	/* On board button and LED */
	led_init();
	button_init();	

	/* Initialize the dynamic memory core */
	mm_init();

	/* Setup true random and psudo random */
	trand_init();
	prand_init();
	
	/* Reenable interrupts */
	cpsie_i();

	/* Start up the USB host core */
	usb_host_init();
}
