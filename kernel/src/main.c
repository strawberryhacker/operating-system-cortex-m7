/// Copyright (C) StrawberryHacker

#include "types.h"
#include "gpio.h"
#include "kernel_boot.h"
#include "sd.h"

#include <stddef.h>

static volatile u32 tick = 0;

int main(void) {
	// Run the kernel boot sequence
	kernel_boot();

	tick = 499;
	while (1) {
		if (sd_is_connected()) {
			gpio_clear(GPIOC, 8);
		} else {
			gpio_set(GPIOC, 8);
		}
		if (tick >= 250) {
			tick = 0;
			//gpio_toggle(GPIOC, 8);
		}
	}
}

void systick_handler(void) {
	tick++;
}