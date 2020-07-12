/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "mm.h"
#include "sd_protocol.h"
#include "panic.h"
#include "fat32.h"
#include "hardware.h"
#include "gmac.h"
#include "ethernet.h"
#include "systick.h"
#include "cpu.h"

#include <stddef.h>

extern struct rq cpu_rq;


static void test_thread(void* arg) {
	while (1) {
		
		print("CPU\n");
		syscall_thread_sleep(1000);
	}
}

static void blink_thread(void* arg) {
	u32 count = 0;
	while (1) {
		for (u32 i = 0; i < 1000; i++) {
			syscall_thread_sleep(1);
			syscall_gpio_toggle(GPIOC, 8);
		}
		print("LED thread %d\n", count++);
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

	struct thread_info fat_info = {
		.name       = "FAT32 thread",
		.stack_size = 1024,
		.thread     = fat32_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info blink_info = {
		.name = "Blink",
		.stack_size = 32,
		.thread = blink_thread,
		.class = REAL_TIME,
		.arg = NULL
	};

	new_thread(&test_info);
	new_thread(&blink_info);
	new_thread(&fat_info);

	//fat32_thread(NULL);
	scheduler_start();
}
