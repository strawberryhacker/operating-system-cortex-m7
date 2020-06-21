/// Copyright (C) StrawberryHacker

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "print.h"
#include "dlist.h"
#include "mm.h"
#include "thread.h"
#include "spinlock.h"

#include <stddef.h>
extern volatile u64 tick;

struct spinlock lock = {0};

static void test_thread(void* arg) {
	while (1) {
		printl("HELLO");
		thread_sleep(500);
	}
}

static void test2_thread(void* arg) {
	while (1) {
		printl("\t\tHELLO %2e");
		thread_sleep(400);
	}
}

static void test3_thread(void* arg) {
	while (1) {
		printl("\t\t\t\tHELLO");
		thread_sleep(1000);
	}
}

static void test4_thread(void* arg) {
	while (1) {
		printl("\t\t\t\t\t\tHELLO");
		thread_sleep(50);
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
		.arg        = NULL
	};

	new_thread(&test_info);
	new_thread(&test2_info);
	new_thread(&test3_info);
	new_thread(&test4_info);

	scheduler_start();
}
