/// Copyright (C) StrawberryHacker

#include "types.h"
#include "gpio.h"
#include "kernel_entry.h"
#include "sd.h"
#include "debug.h"
#include "clock.h"
#include "nvic.h"
#include "thread.h"
#include "sched.h"

#include <stddef.h>

static volatile u32 tick = 0;

void delay(i32 ms) {
	while (ms-- >= 0) {
		for (u32 i = 0; i < 200000; i++) {
			asm volatile ("nop");
		}
	}
}

void thread_1(void* arg) {
	volatile u32 i = 0;
	while (1) {
		if (i++ >= 500) {
			debug_print("Hello from Thread 1\n");
			i = 0;
		}
	}
}

void thread_2(void* arg) {
	volatile u32 i = 0;
	while (1) {
		if (i++ >= 500) {
			debug_print("Hello from Thread 2\n");
			i = 0;
		}
	}
}

struct tcb* threads[5];

int main(void) {
	// Run the kernel boot sequence
	kernel_entry();

	peripheral_clock_enable(10);	
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);
	
	threads[0] = thread_add("Thread 1", thread_1, 100, NULL);
	threads[1] = thread_add("Thread 2", thread_2, 100, NULL);

	for (u8 i = 0; i < 2; i++) {
		debug_print("Thread %d: %4h\n", i, (u32)threads[i]);
	}

	sched_start();
}
