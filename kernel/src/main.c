/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fat32.h"
#include "programming.h"

#include <stddef.h>

static void blink_thread(void* arg) {
	
	while (1) {
		syscall_thread_sleep(95);
		syscall_gpio_toggle(GPIOC, 8);
		syscall_thread_sleep(5);
		syscall_gpio_toggle(GPIOC, 8);
	}
}

int main(void) {

	kernel_entry();

	struct thread_info fat32_info = {
		.name       = "FAT32 thread",
		.stack_size = 1024,
		.thread     = fat32_thread,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	struct thread_info blink_info = {
		.name       = "Blink",
		.stack_size = 256,
		.thread     = blink_thread,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	struct thread_info fpi_info = {
		.name       = "FPI",
		.stack_size = 256,
		.thread     = fpi,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	new_thread(&blink_info);	
	new_thread(&fat32_info);
	new_thread(&fpi_info);
	
	scheduler_start();
}
