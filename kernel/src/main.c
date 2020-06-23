/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "print.h"
#include "dlist.h"
#include "mm.h"
#include "thread.h"
#include "spinlock.h"
#include "gpio.h"

#include <stddef.h>
extern volatile u64 tick;

struct spinlock lock = {0};

volatile i32 counter = 0;

/// Testing can be done on pins PD28, PD27, PD18, PD19

static void test_thread(void* arg) {
	//gpio_set_function(GPIOD, 28, GPIO_FUNC_OFF);
	//gpio_set_direction(GPIOD, 28, GPIO_OUTPUT);

	for (u32 i = 0; i < 1000000; i++) {
		spinlock_aquire(&lock);
		counter++;
		spinlock_release(&lock);
	}

	printl("Plus thread: %d\n", counter);

	while (1) {
		//gpio_toggle(GPIOD, 28);
		//thread_sleep(10);
	}
}

static void test2_thread(void* arg) {
	//gpio_set_function(GPIOD, 27, GPIO_FUNC_OFF);
	//gpio_set_direction(GPIOD, 27, GPIO_OUTPUT);

	for (u32 i = 0; i < 1000000; i++) {
		spinlock_aquire(&lock);
		counter--;
		spinlock_release(&lock);
		asm volatile("nop");
	}

	printl("Minus thread: %d\n", counter);

	while (1) {
		//gpio_toggle(GPIOD, 27);
		//thread_sleep(10);
	}
}

static void test3_thread(void* arg) {
	gpio_set_function(GPIOD, 18, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOD, 18, GPIO_OUTPUT);
	while (1) {
		gpio_toggle(GPIOD, 18);
		thread_sleep(10);
	}
}

static void test4_thread(void* arg) {
	gpio_set_function(GPIOD, 19, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOD, 19, GPIO_OUTPUT);
	while (1) {
		gpio_toggle(GPIOD, 19);
		//printl("HELLO");
		thread_sleep(10);
	}
}

int main(void) {
	kernel_entry();

	struct thread_info test_info = {
		.name       = "Test",
		.stack_size = 100,
		.thread     = test_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info test2_info = {
		.name       = "Test2",
		.stack_size = 100,
		.thread     = test2_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info test3_info = {
		.name       = "Test3",
		.stack_size = 100,
		.thread     = test3_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info test4_info = {
		.name       = "Test4",
		.stack_size = 1000,
		.thread     = test4_thread,
		.class      = REAL_TIME,
		.arg        = "YO"
	};

	new_thread(&test_info);
	new_thread(&test2_info);
	new_thread(&test3_info);
	new_thread(&test4_info);

	scheduler_start();
}
