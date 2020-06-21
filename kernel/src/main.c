/// Copyright (C) StrawberryHacker

#include "types.h"
#include "gpio.h"
#include "kernel_entry.h"
#include "sd.h"
#include "debug.h"
#include "clock.h"
#include "nvic.h"
#include "thread.h"
#include "scheduler.h"
#include "list.h"
#include "mm.h"
#include "spinlock.h"

#include <stddef.h>

int main(void) {
	// Run the kernel boot sequence
	kernel_entry();

	peripheral_clock_enable(10);	
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);

	scheduler_start();
}
