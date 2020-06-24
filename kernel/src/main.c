/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"

#include <stddef.h>


static void test_thread(void* arg) {
	while (1) {
		syscall_thread_sleep(500);
		printl("Thread A says hello");
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

	new_thread(&test_info);

	scheduler_start();
}
