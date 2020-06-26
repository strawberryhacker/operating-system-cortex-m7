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

#include <stddef.h>


static void test_thread(void* arg) {
	while (1) {
		thread_sleep(500);
		printl("Thread A says hello");
	}
}

volatile u8 buf[512];

int main(void) {
	kernel_entry();

	struct thread_info test_info = {
		.name       = "Test",
		.stack_size = 100,
		.thread     = test_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	new_thread(&test_info);

	//fat32_thread(NULL);
	sd_read(0, 1, buf);

	for (u32 i = 0; i < 512;) {
		print("%1h  ", buf[i]);
		if ((i++ % 10) == 0) {
			print("\n");
		}
	}

	while (1);

	scheduler_start();
}
