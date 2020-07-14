/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fat32.h"
#include "panic.h"
#include "mm.h"
#include "sd.h"
#include "dynamic_linker.h"
#include "run.h"

#include <stddef.h>

extern struct rq cpu_rq;

static void blink_thread(void* arg) {

	char test_print[12] = "Hello World";
	while (1) {
		syscall_thread_sleep(1000);

		const char* ptr = (const char *)test_print;
		while (*ptr) {
			u8 status;
			do {
				status = syscall_print_get_status();
				syscall_thread_sleep(1);
			} while (!status);

			syscall_print_byte(*ptr);

			ptr++;
		}
		//print("Runtime:\n");
	}
}

/*
 * Used for testing the run binary
 */
static void test(void* arg) {
	syscall_thread_sleep(1000);

	run_binary("C:/bin/tictactoe.bin");

	while (1) {

	}
}

int main(void) {

	kernel_entry();

	struct thread_info fat32_info = {
		.name       = "FAT32 thread",
		.stack_size = 1024,
		.thread     = fat32_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info blink_info = {
		.name = "Blink",
		.stack_size = 256,
		.thread = blink_thread,
		.class = REAL_TIME,
		.arg = NULL
	};

	struct thread_info test_info = {
		.name = "Test",
		.stack_size = 1024,
		.thread = test,
		.class = REAL_TIME,
		.arg = NULL
	};

	//new_thread(&blink_info);
	new_thread(&fat32_info);
	new_thread(&test_info);
	
	scheduler_start();

}
