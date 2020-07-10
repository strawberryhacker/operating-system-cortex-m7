/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "system_call.h"
#include "mm.h"
#include "sd_protocol.h"
#include "panic.h"
#include "fat32.h"
#include "hardware.h"
#include "gmac.h"
#include "ethernet.h"
#include "systick.h"

#include <stddef.h>


static void test_thread(void* arg) {
	while (1) {
		printl("Thread A says hello");
		thread_sleep(500);
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

	//new_thread(&test_info);
	//new_thread(&fat_info);
	fat32_thread(NULL);
	scheduler_start();
}
